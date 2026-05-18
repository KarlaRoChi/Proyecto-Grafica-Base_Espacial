/*
* PROYECTO INDIVIDUAL: Base Espacial Sustentable
* Alumna: Rojas Chimal Karla Beatriz
*
*/

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <exception>
#include <float.h>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader_m.h>
#include <camera.h>
#include <model.h>
#include <animatedmodel.h>
#include <light.h>
#include <cubemap.h>
#include <text_renderer.h>

// ============ FORWARD DECLARATIONS ============
bool Start();
bool Update();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void SetLightUniforms(Shader* shader);
void SetMaterial(Shader* shader, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, int alpha);
Model* CargarModelo(const std::string& ruta);

glm::mat4 ComputeView();
glm::vec3 ComputeEyePos();
void UpdateCharacter(float dt);
void DrawCharacter(const glm::mat4& projection, const glm::mat4& view);
bool CheckCollision(glm::vec3 pos, float radius, int zone);
void ChangeZone(int z);
void GenerarColisionesDeModelo(Model* modelo, int zoneIndex, glm::mat4 transformacionBase);
void InitDoors();
void CheckDoorTriggers();
void RefreshDoorState();

// HUD / Menu
void InitHud();
void DrawHud();
void DrawPauseMenu();
void DrawControlsOverlay();
void DrawMinimap();
void DrawQuad2D(float x, float y, float w, float h, glm::vec4 color);
void DrawTextCentered(const std::string& text, float cx, float y, float scale, glm::vec3 color);

// Minijuegos
void InitInvernaderoGame();
void UpdateInvernaderoGame(float dt);
void DrawInvernaderoGame(const glm::mat4& projection, const glm::mat4& view);
void InvernaderoHit(int lane);

// Espectrografia del meteorito (segundo minijuego de la zona invernadero)
void InitEspectroGame();
void UpdateEspectroGame(float dt);
void DrawEspectroGame(const glm::mat4& projection, const glm::mat4& view);
void EspectroSetColor(int spectralIdx);
// Posicion del meteorito y chase cam que lo sigue por su orbita. Se
// declaran aqui porque ComputeView (definida antes) los usa cuando el
// minijuego de espectrografia esta activo.
glm::vec3 EspectroMeteorPos(float t);
void EspectroChaseCam(glm::vec3& eye, glm::vec3& target);

void InitEnergiaGame();
void UpdateEnergiaGame(float dt);
void DrawEnergiaGame(const glm::mat4& projection, const glm::mat4& view);
void EnergiaPress(int button);

void InitReciclajeGame();
void DrawReciclajeGame(const glm::mat4& projection, const glm::mat4& view);

void InitCapsulasGame();
void UpdateCapsulasGame(float dt);
void DrawCapsulasGame(const glm::mat4& projection, const glm::mat4& view);
void CapsulasCheckNearest();
// SetCurrentZoneDifficulty se declara despues del enum Difficulty (mas abajo)

// ============ TIPOS ============
enum CameraMode { CAM_FREE = 0, CAM_FIRST = 1, CAM_SECOND = 2, CAM_THIRD = 3 };

struct Player {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    float yaw = 0.0f;
    float pitch = -8.0f;     // inclinacion de la camara (mirar arriba/abajo)
    float walkSpeed = 2.2f;
    float sprintMult = 1.9f;
    float turnSpeed = 110.0f;
    float vy = 0.0f;         // velocidad vertical para el salto
    bool  isWalking = false;
    bool  isSprinting = false;
    bool  grounded = true;
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

struct Room {
    std::string name;
    glm::vec3 spawnPos;
    float     spawnYaw;
    glm::vec2 boundsMin;
    glm::vec2 boundsMax;
    glm::vec3 freeCamPos;
};

struct InteractiveBox {
    glm::vec3 position;
    glm::vec3 startPos;
    float rotationY = 0.0f;
    float scale = 1.0f;
};

// Puerta de transito entre zonas. Cuando el jugador entra a la hitbox circular
// (centro + radius), se le teletransporta a fromZone -> toZone. fromZone == -1
// indica que la puerta se activa en cualquier zona, distinto de toZone.
struct ZoneDoor {
    glm::vec3 center;
    float     radius;
    int       fromZone;
    int       toZone;
};

struct TrailParticle {
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec4 color;
    float life;
    float maxLife;
    float size;
};

// ============ PLANTA FRACTAL (L-SYSTEM HELECHO) ============
struct TurtleState {
    glm::vec3 pos;
    glm::vec3 dir;
};

class FractalPlant {
public:
    unsigned int VAO, VBO;
    int vertexCount = 0;
    bool initialized = false;

    void Generate(int iterations) {
        if (initialized) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            initialized = false;
            vertexCount = 0;
        }

        if (iterations <= 0) return;

        std::string current = "X";
        for (int i = 0; i < iterations; i++) {
            std::string next = "";
            for (char c : current) {
                if (c == 'X') next += "F-[[X]+X]+F[+FX]-X";
                else if (c == 'F') next += "FF";
                else next += c;
            }
            current = next;
        }

        std::vector<float> vertices;
        std::vector<TurtleState> stack;
        TurtleState state;
        state.pos = glm::vec3(0.0f);
        state.dir = glm::vec3(0.0f, 1.0f, 0.0f);

        float length = 0.25f / pow(1.5f, iterations - 1);
        float angle = glm::radians(25.0f);

        for (char c : current) {
            if (c == 'F') {
                glm::vec3 nextPos = state.pos + state.dir * length;
                vertices.push_back(state.pos.x); vertices.push_back(state.pos.y); vertices.push_back(state.pos.z);
                vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
                vertices.push_back(0.0f); vertices.push_back(0.0f);
                vertices.push_back(nextPos.x); vertices.push_back(nextPos.y); vertices.push_back(nextPos.z);
                vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
                vertices.push_back(1.0f); vertices.push_back(1.0f);
                state.pos = nextPos;
            }
            else if (c == '+') {
                glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
                rot = glm::rotate(rot, angle * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
                state.dir = glm::normalize(glm::vec3(rot * glm::vec4(state.dir, 0.0f)));
            }
            else if (c == '-') {
                glm::mat4 rot = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0.0f, 0.0f, 1.0f));
                rot = glm::rotate(rot, -angle * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
                state.dir = glm::normalize(glm::vec3(rot * glm::vec4(state.dir, 0.0f)));
            }
            else if (c == '[') {
                stack.push_back(state);
            }
            else if (c == ']') {
                state = stack.back();
                stack.pop_back();
            }
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        vertexCount = vertices.size() / 8;
        initialized = true;
    }

    void Draw(Shader& shader, glm::mat4 modelMatrix) {
        if (!initialized || vertexCount == 0) return;
        shader.use();
        shader.setMat4("model", modelMatrix);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, vertexCount);
        glBindVertexArray(0);
    }
};

// ============ GLOBALES ============
GLFWwindow* window;
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
int currentZone = 0;
// Arrancamos en tercera persona para que WASD mueva al personaje desde el
// primer frame (CAM_FREE bloqueaba el movimiento del avatar hasta F1/F2/F3).
CameraMode cameraMode = CAM_THIRD;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float gEscala = 1.0f;
bool  gWireframe = false;

// Shaders
Shader* phongShader = nullptr;
Shader* cubemapShader = nullptr;
Shader* dynamicShader = nullptr;
Shader* fresnelShader = nullptr;
Shader* proceduralShader = nullptr;
Shader* wavesShader = nullptr;
Shader* hudShader = nullptr;

// ============ HUD / MENU ============
TextRenderer* gText = nullptr;
unsigned int hudVAO = 0, hudVBO = 0;
glm::mat4 hudProjection = glm::mat4(1.0f);

enum MenuState { MENU_NONE = 0, MENU_PAUSE = 1, MENU_CONTROLS = 2, MENU_MAP = 3 };
MenuState menuState = MENU_NONE;
int menuSelection = 0;
bool showMinimap = true;

// Banner grande que aparece unos segundos al cambiar de zona ("INVERNADERO")
std::string gZoneBanner;
float gZoneBannerTimer = 0.0f;
const float ZONE_BANNER_DURATION = 2.5f;

// === NARRATIVA GUIADA ===
// Cola de mensajes contextuales (estilo "Bienvenido a la estacion..."). Cada
// entrada se muestra durante NARRATIVE_DURATION y luego pasa al siguiente.
// Se acumulan al entrar a una zona o al iniciar un minijuego.
std::vector<std::string> gNarrativeQueue;
float gNarrativeTimer = 0.0f;
const float NARRATIVE_DURATION = 4.5f;
bool gIntroShown = false;

void PushNarrative(const std::string& msg) {
    gNarrativeQueue.push_back(msg);
    if (gNarrativeTimer <= 0.0f) gNarrativeTimer = NARRATIVE_DURATION;
}

// Vacia la cola de narrativa por completo. Se usa al cambiar de zona para
// que los mensajes contextuales del modulo anterior no sigan reproduciendose
// despues del teleport (con cada salto se empujan 2 mensajes de ~4.5 s, asi
// que al saltar rapido entre 3-4 zonas se acumulaban 10+ s de texto stale).
void ClearNarrative() {
    gNarrativeQueue.clear();
    gNarrativeTimer = 0.0f;
}

// === MODO DE JUEGO (HISTORIA/RECORRIDO vs LIBRE) ===
// El modo HISTORIA guia al jugador por las 5 zonas en orden (HUB, INV, REC,
// ENE, CAP), empujando la narrativa de cada modulo y explicando la tarea.
// Se avanza manualmente con N para que cada jugador respete su propio ritmo
// (puede quedarse a probar el minijuego antes de seguir). El modo LIBRE es
// exploracion sin guion, identico al comportamiento previo del proyecto.
// El jugador alterna entre modos desde el menu de pausa.
enum GameMode { MODE_LIBRE = 0, MODE_RECORRIDO = 1 };
GameMode gGameMode = MODE_LIBRE;
int gRecorridoStep = 0; // 0..4 -> indice dentro de RECORRIDO_ORDER
const int RECORRIDO_ORDER[5] = { 0, 1, 2, 3, 4 }; // HUB, INV, REC, ENE, CAP

void StartRecorrido();
void AdvanceRecorridoStep();
void PauseRecorrido();
void ResumeRecorrido();

// === BLOQUEO DE CAMARA PARA MINIJUEGOS ===
// Cuando un minijuego pide "vista fija" (top-down invernadero, panel Simon)
// guardamos el modo de camara previo y bloqueamos los controles de mouse.
// TAB durante el minijuego sirve de "interrumpir" para liberar la camara.
bool gMinigameCamLocked = false;
CameraMode gCamModeBeforeLock = CAM_THIRD;

// Modelos
Model* HUB_OPACO = nullptr;
Model* INVERNADERO_OPACO = nullptr;
Model* INVERNADERO_TRASLUCIDO = nullptr;
Model* RECICLAJE_OPACO = nullptr;
Model* RECICLAJE_METAL = nullptr;
Model* ENERGIA_OPACO = nullptr;
Model* ENERGIA_METALICO = nullptr;
Model* CAPSULAS_OPACO = nullptr;
AnimatedModel* PERSONAJE = nullptr;
Model* CAJA_MODELO = nullptr;
Model* MOON2 = nullptr;
Model* BEBEDERO_BASE = nullptr;
Model* PUERTA_MODELO = nullptr;
Model* BEBEDERO_AGUA = nullptr;
// Primitivos genericos para los minijuegos: cubo y plano basicos. Si por
// algun motivo no estan en disco, las rutas que los usan se quedan con
// CAJA_MODELO para no romper el render (graceful fallback).
Model* CUBO_MODELO = nullptr;
Model* PLANO_MODELO = nullptr;

CubeMap* mainCubeMap = nullptr;
std::vector<Light> gLights;

glm::vec4 globalAmbient = glm::vec4(0.30f, 0.30f, 0.30f, 1.0f);

Player player;
const float PLAYER_RADIUS = 0.4f;
const float PLAYER_HEIGHT = 1.7f;

Room rooms[5];
std::vector<AABB> autoColliders[5];
InteractiveBox interactiveBoxes[5];

// Puertas: 4 desde el HUB (N/S/E/O) + 4 de retorno desde cada zona al HUB.
ZoneDoor zoneDoors[8];
// El jugador puede aparecer ya dentro de una hitbox de puerta tras un teleport.
// Marcamos por puerta si el jugador estaba dentro el frame anterior y solo
// disparamos en el flanco de entrada (fuera -> dentro).
bool doorPlayerInside[8] = { false };

bool keyFLast = false;
bool keyF1Last = false, keyF2Last = false, keyF3Last = false, keyF4Last = false, keyF8Last = false;
bool keyZoneLast[5] = { false, false, false, false, false };

// ============ MINIJUEGOS ============
enum Difficulty { DIFF_EASY = 0, DIFF_MEDIUM = 1, DIFF_EXPERT = 2 };
void SetCurrentZoneDifficulty(Difficulty d);

// --- Invernadero (Ritmo / OSU) ---
struct OsuNote {
    int lane;        // 0..3
    float hitTime;   // segundos desde inicio del juego
    bool hit = false;
    bool missed = false;
};

struct WaterDrop {
    glm::vec3 pos;
    float vy;
    float life;
    int lane;
};

struct InvernaderoGame {
    Difficulty difficulty = DIFF_EASY;
    bool active = false;
    bool topDown = false;
    float gameTime = 0.0f;
    std::vector<OsuNote> notes;
    std::vector<WaterDrop> drops;     // gotas que caen sobre la maceta al acertar
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int hits = 0;
    int misses = 0;
    int activeLanes = 4;              // 2 para FACIL, 4 para MEDIO/EXPERTO
    float waterEffect[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // animacion al regar
};

// --- Energia (Simon) ---
struct SimonGame {
    Difficulty difficulty = DIFF_EASY;
    bool active = false;
    std::vector<int> pattern;
    int playerIndex = 0;
    int phase = 0;       // 0=esperar inicio, 1=mostrar patron, 2=input jugador, 3=fallo
    float phaseTimer = 0.0f;
    int showIndex = -1;
    int level = 1;
    int score = 0;
    float buttonGlow[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

// --- Reciclaje ---
enum MaterialType { MAT_METAL = 0, MAT_VIDRIO = 1, MAT_CARTON = 2 };
struct RecycleItem {
    glm::vec3 position;
    MaterialType type;
    bool sorted = false;
};

struct ReciclajeGame {
    Difficulty difficulty = DIFF_EASY;
    bool active = false;
    std::vector<RecycleItem> items;
    int selectedItem = -1;     // item bajo selección (uso futuro)
    int selectedBin = -1;      // bote bajo selección (uso futuro)
    int score = 0;
    int selectedBox = 0;       // 0..4 caja interactiva activa para mover en X/Y/Z
};

// --- Capsulas (Buscar al intruso) ---
struct CapsulasGame {
    Difficulty difficulty = DIFF_EASY;
    bool active = false;
    int totalCapsules = 4;
    int intruderIdx = 0;
    float timeLeft = 60.0f;
    float moveInterval = 0.0f;     // 0 = no se mueve
    float moveTimer = 0.0f;
    int   lastChecked = -1;
    float lastCheckedTimer = 0.0f; // animacion de flash al revisar
    bool  win = false;
    bool  lose = false;
};

// --- Espectrografia del meteorito (segundo minijuego del invernadero) ---
// El meteorito recorre una orbita circular sobre el domo. Cada cierto
// intervalo aparece una "linea espectral objetivo" (H-alpha, H-beta,
// H-gamma o Na/Sun) y el jugador debe cambiar el color del meteorito con
// la tecla correspondiente antes de que se cumpla el beat.
//   - H-alpha  (656 nm) -> tecla 1 -> rojo
//   - H-beta   (486 nm) -> tecla 2 -> cyan
//   - H-gamma  (434 nm) -> tecla 3 -> violeta
//   - Na (sun) (589 nm) -> tecla 4 -> amarillo
struct EspectroGame {
    Difficulty difficulty = DIFF_EASY;
    bool active = false;
    int currentColor = 0;       // color actual del meteorito (0..3)
    std::vector<int> pattern;   // lineas objetivo en orden
    int patternIdx = 0;         // siguiente objetivo
    float beatInterval = 1.6f;  // tiempo entre beats
    float beatTimer = 0.0f;     // cuenta regresiva al siguiente check
    int score = 0;
    int hits = 0;
    int misses = 0;
    int combo = 0;
    int maxCombo = 0;
    float lastFeedbackTimer = 0.0f; // glow en pantalla al acertar/fallar
    bool lastHit = false;
};

InvernaderoGame gInvernadero;
SimonGame gEnergia;
ReciclajeGame gReciclaje;
CapsulasGame gCapsulas;
EspectroGame gEspectro;

// Lineas espectrales del minijuego del meteorito. Las definimos aqui para que
// SpawnMoonTrail (mas abajo) pueda usarlas directamente sin forward-extern.
const glm::vec4 ESPECTRO_COLORS[4] = {
    glm::vec4(1.00f, 0.10f, 0.18f, 1.0f), // H-alpha rojo  (656 nm)
    glm::vec4(0.15f, 0.95f, 1.00f, 1.0f), // H-beta cyan   (486 nm)
    glm::vec4(0.55f, 0.20f, 1.00f, 1.0f), // H-gamma violeta (434 nm)
    glm::vec4(1.00f, 0.85f, 0.18f, 1.0f)  // Na/Sun amarillo (589 nm)
};
const char* ESPECTRO_NAMES[4] = { "H-alpha", "H-beta", "H-gamma", "Na/Sun" };

const glm::vec3 CAPSULA_SLOTS[8] = {
    glm::vec3( -7.5f, 0.9f, -1.5f),
    glm::vec3( -7.5f, 0.9f,  1.5f),
    glm::vec3( -9.5f, 0.9f, -1.5f),
    glm::vec3( -9.5f, 0.9f,  1.5f),
    glm::vec3(-11.5f, 0.9f, -1.5f),
    glm::vec3(-11.5f, 0.9f,  1.5f),
    glm::vec3(-13.5f, 0.9f, -1.5f),
    glm::vec3(-13.5f, 0.9f,  1.5f)
};

// Macetas distribuidas en dos filas (izquierda y derecha del invernadero),
// con dos macetas cada una a distinto z. Las notas no caen de arriba: viajan
// HORIZONTALMENTE desde el exterior del muro hacia su maceta:
//   - lanes 0,1 (fila IZQUIERDA): notas vienen de -x y scrollean a +x
//   - lanes 2,3 (fila DERECHA):  notas vienen de +x y scrollean a -x
const glm::vec3 INV_PLANT_POSITIONS[4] = {
    glm::vec3(-3.5f, 0.0f,  -8.0f), // 0 = fila izq, atras
    glm::vec3(-3.5f, 0.0f, -14.0f), // 1 = fila izq, adelante
    glm::vec3( 3.5f, 0.0f,  -8.0f), // 2 = fila der, atras
    glm::vec3( 3.5f, 0.0f, -14.0f)  // 3 = fila der, adelante
};
// +1 si la nota viaja hacia +x, -1 si hacia -x. Es decir, las notas de la
// fila izquierda (x negativo) vienen de la pared izquierda y van hacia el
// centro (+x), y viceversa.
const float INV_SCROLL_DIR[4] = { +1.0f, +1.0f, -1.0f, -1.0f };

// La zona ENERGIA tiene 6 pilas (L_1..L_3 y R_1..R_3). El minijuego de
// luces usa SOLO una pila para concentrar la atencion del jugador: la pila
// central derecha (R_2) que queda visible de frente al spawn. Los 4 botones
// del Simon estan distribuidos en una columna sobre la cara frontal de esa
// pila para que se puedan clickear sin perdersepor el modelo.
const glm::vec3 SIMON_BUTTON_POSITIONS[4] = {
    glm::vec3(2.8f, 0.9f, 11.5f),   // boton inferior
    glm::vec3(2.8f, 1.5f, 11.5f),
    glm::vec3(2.8f, 2.1f, 11.5f),
    glm::vec3(2.8f, 2.7f, 11.5f)    // boton superior
};

const glm::vec3 RECYCLE_BIN_POSITIONS[3] = {
    glm::vec3(11.5f, 0.5f, -2.5f),  // METAL
    glm::vec3(11.5f, 0.5f,  0.0f),  // VIDRIO
    glm::vec3(11.5f, 0.5f,  2.5f)   // CARTON
};

bool keyZLast = false, keyXLast = false, keyCLast = false, keyVLast = false;
bool keyGLast = false, keyBLast = false, keyELast = false;
bool keyTabLast = false;
bool keyDiff7Last = false, keyDiff8Last = false, keyDiff9Last = false;

glm::mat4 baseModelMat(1.0f);
FractalPlant* miPlantaFractal = nullptr;
int fractalIteration = 0;

bool keyPLast = false;
bool keyOLast = false;

std::vector<TrailParticle> moonTrail;
Shader* trailShader = nullptr;
unsigned int trailVAO = 0;
unsigned int trailVBO = 0;
int moonTrailMode = 0; // 0 = cyan, 1 = magenta, 2 = yellow
float trailEmitTimer = 0.0f;

// ============ MAIN ============
int main() {
    if (!Start()) return -1;
    while (!glfwWindowShouldClose(window)) {
        if (!Update()) break;
    }
    glfwTerminate();
    return 0;
}

// ============ HELPERS ============
Model* CargarModelo(const std::string& ruta) {
    std::cout << "[Modelo] Cargando: " << ruta << " ... ";
    Model* m = nullptr;
    try { m = new Model(ruta); }
    catch (...) { std::cout << "FALLO" << std::endl; return nullptr; }
    if (!m || m->meshes.empty()) { std::cout << "FALLO" << std::endl; if (m) delete m; return nullptr; }
    std::cout << "OK (" << m->meshes.size() << " meshes)" << std::endl;
    return m;
}

void SetMaterial(Shader* shader, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, int alpha) {
    shader->setVec4("MaterialAmbientColor", ambient);
    shader->setVec4("MaterialDiffuseColor", diffuse);
    shader->setVec4("MaterialSpecularColor", specular);
    shader->setInt("alphaIndex", alpha);
}


// Construye AABBs en coordenadas de mundo a partir de cada mesh del FBX.
// Solo nos quedamos con "props" claros (mesas, atril, macetas, pilas, capsulas).
// Lo que parezca geometria de la sala (paredes, pisos, techos) se descarta: el
// rect del Room ya cubre esos limites y si lo metieramos como collider podria
// engulir la posicion del jugador y dejarlo trabado al spawnear.
void GenerarColisionesDeModelo(Model* modelo, int zoneIndex, glm::mat4 transformacionBase) {
    if (!modelo) return;
    for (size_t i = 0; i < modelo->meshes.size(); i++) {
        if (modelo->meshes[i].vertices.empty()) continue;
        glm::vec3 minAABB = glm::vec3(FLT_MAX);
        glm::vec3 maxAABB = glm::vec3(-FLT_MAX);
        for (size_t j = 0; j < modelo->meshes[i].vertices.size(); j++) {
            glm::vec4 posMundo = transformacionBase * glm::vec4(modelo->meshes[i].vertices[j].Position, 1.0f);
            minAABB = glm::min(minAABB, glm::vec3(posMundo));
            maxAABB = glm::max(maxAABB, glm::vec3(posMundo));
        }
        float anchoX = maxAABB.x - minAABB.x;
        float anchoY = maxAABB.y - minAABB.y;
        float anchoZ = maxAABB.z - minAABB.z;

        // Filtros tipo prop:
        //   - footprint pequenio (no es pared ni piso entero)
        //   - debe sobresalir del piso al menos 30 cm
        //   - no debe abarcar TODA la sala (evita atrapar al jugador)
        if (anchoX > 5.0f || anchoZ > 5.0f) continue;
        if (anchoY > 4.0f) continue;
        if (maxAABB.y < 0.30f) continue;        // decal / LED en el piso
        if (minAABB.y > 2.5f) continue;          // colgado del techo
        if (anchoX < 0.05f && anchoZ < 0.05f) continue; // mesh degenerado

        autoColliders[zoneIndex].push_back({ minAABB, maxAABB });
    }
}

glm::vec4 GetMoonTrailBaseColor(int mode) {
    switch (mode) {
    case 0:
        return glm::vec4(0.35f, 0.70f, 0.95f, 1.0f); // CYAN suave

    case 1:
        return glm::vec4(0.90f, 0.40f, 0.85f, 1.0f); // MAGENTA suave

    case 2:
        return glm::vec4(0.95f, 0.85f, 0.30f, 1.0f); // YELLOW suave
    }

    return glm::vec4(0.35f, 0.70f, 0.95f, 1.0f);
}
void SpawnMoonTrail(const glm::vec3& pos, const glm::vec3& dir, float t) {

    // Si el minijuego de espectrografia esta activo, el color base de la
    // estela usa la linea espectral seleccionada por el jugador en ese
    // momento. Asi el visual coincide con la mecanica del juego.
    glm::vec4 baseColor;
    if (gEspectro.active) {
        baseColor = ESPECTRO_COLORS[gEspectro.currentColor];
    } else {
        baseColor = GetMoonTrailBaseColor(moonTrailMode);
    }

    glm::vec3 forward = glm::normalize(dir);
    if (glm::length(forward) < 0.0001f)
        forward = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 side = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(side) < 0.0001f)
        side = glm::cross(forward, glm::vec3(1.0f, 0.0f, 0.0f));

    side = glm::normalize(side);
    glm::vec3 up = glm::normalize(glm::cross(side, forward));

    // M�s part�culas, m�s compactas, sin efecto resorte
    for (int i = 0; i < 70; i++) {

        float r = (float)rand() / RAND_MAX;
        float r2 = (float)rand() / RAND_MAX;

        // dispersi�n org�nica
        float spread = 0.18f + r * 0.75f;

        float offsetSide = ((float)rand() / RAND_MAX - 0.5f) * spread;
        float offsetUp = ((float)rand() / RAND_MAX - 0.5f) * spread;

        // ligera elongaci�n horizontal
        offsetSide *= 1.35f;

        TrailParticle p;  // <-- FALTABA ESTA L�NEA

        p.pos = pos + side * offsetSide + up * offsetUp;

        float speed = 0.75f + r * 0.85f;

        p.vel = -forward * speed
            + side * ((r - 0.5f) * 0.22f)
            + up * ((r2 - 0.5f) * 0.18f);

        // Variaciones de color EXTRA-amplias: ahora cada particula puede
        // saltar a cualquier tono del arcoiris, con destellos blancos y
        // ocasionales tintes "espectrales" tipo H-alpha/H-beta/Na para
        // recordar la temati ca de espectrografia astronomica del minijuego
        // del meteorito.
        float r3 = (float)rand() / RAND_MAX;
        float r4 = (float)rand() / RAND_MAX;
        float r5 = (float)rand() / RAND_MAX;
        float r6 = (float)rand() / RAND_MAX;

        // Brillo: amplitud aun mayor (0.30..1.60)
        float colorVariation = 0.30f + r * 1.30f;
        glm::vec3 variedColor = glm::vec3(baseColor) * colorVariation;

        // Mezcla con un color aleatorio brillante (hasta 55% de tinte)
        glm::vec3 randomTint(r3, r4, r5);
        float mixT = 0.55f * r5;
        variedColor = glm::mix(variedColor, randomTint, mixT);

        // 20% de las particulas reciben un "tinte espectral" especifico:
        //   H-alpha rojo, H-beta cyan, H-gamma violeta, Na amarillo.
        if (r6 < 0.20f) {
            glm::vec3 spectral;
            int kind = (int)(r6 * 100.0f) % 4;
            switch (kind) {
            case 0: spectral = glm::vec3(1.0f, 0.10f, 0.18f); break; // H-alpha
            case 1: spectral = glm::vec3(0.15f, 0.95f, 1.0f);  break; // H-beta
            case 2: spectral = glm::vec3(0.55f, 0.20f, 1.0f);  break; // H-gamma
            default: spectral = glm::vec3(1.0f, 0.85f, 0.18f); break; // Na/sun
            }
            variedColor = glm::mix(variedColor, spectral, 0.7f);
        }

        // Jitter por canal aun mas fuerte (0.45 vs 0.30 anterior)
        variedColor.r += (r  - 0.5f) * 0.45f;
        variedColor.g += (r2 - 0.5f) * 0.45f;
        variedColor.b += (r3 - 0.5f) * 0.45f;

        // Destellos casi blancos mas frecuentes (10%) + intensos
        if (r4 > 0.90f) variedColor += glm::vec3(0.75f, 0.75f, 0.75f);

        variedColor = glm::clamp(variedColor, glm::vec3(0.0f), glm::vec3(1.0f));
        p.color = glm::vec4(variedColor, 1.0f);

        p.life = 0.75f + r * 0.45f;
        p.maxLife = p.life;

        p.size = 6.0f + r * 6.0f;

        moonTrail.push_back(p);
    }
}
void UpdateMoonTrail(float dt) {
    for (auto& p : moonTrail) {
        p.pos += p.vel * dt;
        p.vel *= 0.996f;
        p.life -= dt * 0.55f;
        p.size *= 0.997f;
    }

    moonTrail.erase(
        std::remove_if(
            moonTrail.begin(),
            moonTrail.end(),
            [](const TrailParticle& p) { return p.life <= 0.0f; }
        ),
        moonTrail.end()
    );

    if (moonTrail.size() > 9000) {
        moonTrail.erase(moonTrail.begin(), moonTrail.begin() + 1200);
    }
}

// ============ HABITACIONES ============
void InitRooms() {
    rooms[0].name = "HUB";
    rooms[0].spawnPos = glm::vec3(0.0f, 0.0f, 2.0f);
    rooms[0].spawnYaw = 180.0f;
    rooms[0].boundsMin = glm::vec2(-5.0f, -5.0f);
    rooms[0].boundsMax = glm::vec2(5.0f, 5.0f);

    rooms[1].name = "INVERNADERO";
    rooms[1].spawnPos = glm::vec3(0.0f, 0.0f, -6.0f);
    rooms[1].spawnYaw = 180.0f;
    rooms[1].boundsMin = glm::vec2(-7.0f, -20.0f);
    rooms[1].boundsMax = glm::vec2(7.0f, -5.0f);

    rooms[2].name = "RECICLAJE";
    rooms[2].spawnPos = glm::vec3(7.0f, 0.0f, 0.0f);
    rooms[2].spawnYaw = 90.0f;
    rooms[2].boundsMin = glm::vec2(6.0f, -3.5f);
    rooms[2].boundsMax = glm::vec2(12.5f, 3.5f);

    rooms[3].name = "ENERGIA";
    rooms[3].spawnPos = glm::vec3(0.0f, 0.0f, 7.5f);
    rooms[3].spawnYaw = 0.0f;
    rooms[3].boundsMin = glm::vec2(-5.5f, 6.5f);
    rooms[3].boundsMax = glm::vec2(5.5f, 13.5f);

    rooms[4].name = "CAPSULAS";
    rooms[4].spawnPos = glm::vec3(-7.0f, 0.0f, 0.0f);
    rooms[4].spawnYaw = 270.0f;
    rooms[4].boundsMin = glm::vec2(-15.0f, -2.5f);
    rooms[4].boundsMax = glm::vec2(-6.5f, 2.5f);

    for (int i = 0; i < 5; ++i) rooms[i].freeCamPos = glm::vec3(0.0f, 5.0f, 12.0f);

    // Ubicaciones iniciales de las cajas
    for (int i = 0; i < 5; ++i) {
        interactiveBoxes[i].startPos = glm::vec3(6.5f + (i * 0.5f), 0.1f, (i % 2 == 0) ? -1.5f : 1.5f);
        interactiveBoxes[i].position = interactiveBoxes[i].startPos;
        interactiveBoxes[i].rotationY = 0.0f;
        interactiveBoxes[i].scale = 1.0f;
    }

    InitDoors();
}

// ============ PUERTAS ENTRE ZONAS ============
// HUB es zona 0 con bounds (-5,-5)..(5,5). Cada puerta es una hitbox circular
// pegada al borde correspondiente. Las puertas de retorno se ubican en la
// frontera de cada zona adyacente al HUB.
void InitDoors() {
    // ---- 4 puertas del HUB hacia cada habitacion ----
    // Sur -> INVERNADERO
    zoneDoors[0] = { glm::vec3( 0.0f, 0.0f, -4.5f), 0.9f, 0, 1 };
    // Este -> RECICLAJE
    zoneDoors[1] = { glm::vec3( 4.5f, 0.0f,  0.0f), 0.9f, 0, 2 };
    // Norte -> ENERGIA
    zoneDoors[2] = { glm::vec3( 0.0f, 0.0f,  4.5f), 0.9f, 0, 3 };
    // Oeste -> CAPSULAS
    zoneDoors[3] = { glm::vec3(-4.5f, 0.0f,  0.0f), 0.9f, 0, 4 };

    // ---- 4 puertas de retorno al HUB desde cada zona ----
    zoneDoors[4] = { glm::vec3( 0.0f, 0.0f, -5.5f), 0.9f, 1, 0 }; // INVERNADERO -> HUB
    zoneDoors[5] = { glm::vec3( 6.5f, 0.0f,  0.0f), 0.9f, 2, 0 }; // RECICLAJE -> HUB
    zoneDoors[6] = { glm::vec3( 0.0f, 0.0f,  7.0f), 0.9f, 3, 0 }; // ENERGIA -> HUB
    zoneDoors[7] = { glm::vec3(-7.0f, 0.0f,  0.0f), 0.9f, 4, 0 }; // CAPSULAS -> HUB
}

static bool PlayerInsideDoor(const ZoneDoor& d) {
    float dx = player.position.x - d.center.x;
    float dz = player.position.z - d.center.z;
    return dx * dx + dz * dz <= d.radius * d.radius;
}

void CheckDoorTriggers() {
    for (int i = 0; i < 8; ++i) {
        const ZoneDoor& d = zoneDoors[i];
        if (d.fromZone != currentZone) {
            doorPlayerInside[i] = false;
            continue;
        }
        bool now = PlayerInsideDoor(d);
        if (now && !doorPlayerInside[i]) {
            doorPlayerInside[i] = true;
            ChangeZone(d.toZone);
            return;
        }
        doorPlayerInside[i] = now;
    }
}

// Tras un teleport el jugador puede aparecer dentro de una hitbox de puerta
// (p.ej. una puerta de retorno cerca del spawn de la zona). Marcamos esas
// puertas como "ya dentro" para que solo se disparen cuando salga y vuelva.
void RefreshDoorState() {
    for (int i = 0; i < 8; ++i) {
        const ZoneDoor& d = zoneDoors[i];
        doorPlayerInside[i] = (d.fromZone == currentZone) && PlayerInsideDoor(d);
    }
}

bool CheckCollision(glm::vec3 pos, float radius, int zone) {
    if (zone < 0 || zone >= 5) return false;
    Room& r = rooms[zone];

    if (pos.x - radius < r.boundsMin.x) return true;
    if (pos.x + radius > r.boundsMax.x) return true;
    if (pos.z - radius < r.boundsMin.y) return true;
    if (pos.z + radius > r.boundsMax.y) return true;

    return false;
}

void ChangeZone(int z) {
    if (z < 0 || z >= 5) return;
    currentZone = z;
    player.position = rooms[z].spawnPos;
    player.position.y = 0.0f;
    player.yaw = rooms[z].spawnYaw;

    glm::vec3 fwd(sinf(glm::radians(player.yaw)), 0.0f, cosf(glm::radians(player.yaw)));
    camera.Position = player.position - fwd * 1.0f + glm::vec3(0.0f, 1.7f, 0.0f);

    moonTrail.clear();
    trailEmitTimer = 0.0f;

    // El spawn de varias zonas cae dentro de la hitbox de su puerta de retorno;
    // marcamos ese estado para que la puerta solo se dispare cuando el jugador
    // salga y vuelva a entrar (detección de flanco).
    RefreshDoorState();

    // Banner grande con el nombre de la zona durante ZONE_BANNER_DURATION s
    gZoneBanner = rooms[z].name;
    gZoneBannerTimer = ZONE_BANNER_DURATION;

    // Al cambiar de zona apagamos cualquier minijuego pendiente y soltamos la
    // camara. Los minijuegos solo arrancan al pulsar B (o V para espectro)
    // en cada sala.
    gInvernadero.active = false;
    gInvernadero.topDown = false;
    gReciclaje.active = false;
    gEnergia.active = false;
    gCapsulas.active = false;
    gEspectro.active = false;
    gMinigameCamLocked = false;

    // Si el jugador venia con mensajes pendientes (intro o de la zona
    // anterior), los soltamos: al cambiar de modulo lo unico relevante es
    // la narrativa del modulo nuevo. Esto evita el "lag" de mensajes
    // viejos que se reproducian segundos despues del teleport cuando se
    // saltaba rapido entre zonas.
    ClearNarrative();

    // Narrativa contextual por zona (estilo recorrido guiado).
    switch (z) {
    case 0:
        PushNarrative("HUB CENTRAL: cuatro puertas doradas conectan los modulos de la estacion.");
        PushNarrative("Atraviesa una puerta o pulsa 1-4 para teletransportarte.");
        break;
    case 1:
        PushNarrative("INVERNADERO: aqui crecen los cultivos de la estacion.");
        PushNarrative("Tarea: riega las plantas siguiendo el ritmo. Pulsa B para empezar.");
        break;
    case 2:
        PushNarrative("RECICLAJE: clasifica los desechos por tipo de material.");
        PushNarrative("Tarea: ordena los desechos en las cajas correctas. Pulsa B para empezar.");
        break;
    case 3:
        PushNarrative("ENERGIA: el reactor necesita un patron de encendido.");
        PushNarrative("Tarea: pulsa los botones de las pilas en el orden indicado. Pulsa B para empezar.");
        break;
    case 4:
        PushNarrative("CAPSULAS: un intruso se infiltro en una de las criocamaras.");
        PushNarrative("Tarea: revisa cada capsula con F. Pulsa B para empezar.");
        break;
    }
}

// ============ RECORRIDO GUIADO ============
// El recorrido es un guion lineal de 5 pasos (HUB -> INV -> REC -> ENE ->
// CAP). Cada paso teletransporta al jugador a la zona y empuja un mensaje
// recordatorio para que sepa que el recorrido sigue activo y como avanzar.
// El jugador puede pausar en cualquier momento (vuelve a modo LIBRE
// conservando el paso actual) y reanudar despues; tambien puede mover por
// el mundo con WASD durante el recorrido, N solo es para saltar al siguiente
// modulo de la secuencia.

void StartRecorrido() {
    gGameMode = MODE_RECORRIDO;
    gRecorridoStep = 0;
    // ChangeZone limpia narrativa y empuja los 2 mensajes del HUB; los de
    // recorrido quedan justo despues en la cola.
    ChangeZone(RECORRIDO_ORDER[gRecorridoStep]);
    PushNarrative("RECORRIDO: te guiare por los 5 modulos en orden.");
    PushNarrative("Pulsa N para pasar al siguiente modulo o ESC para pausar.");
}

void AdvanceRecorridoStep() {
    if (gGameMode != MODE_RECORRIDO) return;
    gRecorridoStep++;
    if (gRecorridoStep >= 5) {
        // Recorrido completo: regresamos al HUB y bajamos a modo libre.
        gRecorridoStep = 0;
        gGameMode = MODE_LIBRE;
        ChangeZone(RECORRIDO_ORDER[0]);
        PushNarrative("Recorrido completo. Ahora exploras en modo libre.");
        PushNarrative("Reactivalo desde el menu (ESC) cuando quieras.");
        return;
    }
    ChangeZone(RECORRIDO_ORDER[gRecorridoStep]);
    char buf[160];
    snprintf(buf, sizeof(buf),
             "RECORRIDO %d/5: %s. Prueba el minijuego o pulsa N para continuar.",
             gRecorridoStep + 1, rooms[RECORRIDO_ORDER[gRecorridoStep]].name.c_str());
    PushNarrative(buf);
}

void PauseRecorrido() {
    if (gGameMode != MODE_RECORRIDO) return;
    gGameMode = MODE_LIBRE;
    PushNarrative("Recorrido pausado. Tienes el control libre.");
    PushNarrative("Reanudalo desde el menu (ESC) cuando quieras seguir.");
}

void ResumeRecorrido() {
    if (gGameMode != MODE_LIBRE) return;
    gGameMode = MODE_RECORRIDO;
    // Teletransportamos al jugador al modulo donde se quedo el recorrido
    // (mas intuitivo que dejarlo donde estaba paseando). ChangeZone limpia
    // narrativa y empuja la guia de la zona; el mensaje de "reanudado"
    // queda detras en la cola.
    ChangeZone(RECORRIDO_ORDER[gRecorridoStep]);
    char buf[160];
    snprintf(buf, sizeof(buf),
             "RECORRIDO %d/5 reanudado en %s. Pulsa N para continuar.",
             gRecorridoStep + 1, rooms[RECORRIDO_ORDER[gRecorridoStep]].name.c_str());
    PushNarrative(buf);
}

// ============ START ============
bool Start() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Base Espacial - Karla Rojas", NULL, NULL);
    if (!window) return false;
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return false;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    phongShader = new Shader("shaders/11_PhongShaderMultLightsv2.vs", "shaders/11_PhongShaderMultLightsv2.fs");
    cubemapShader = new Shader("shaders/10_vertex_cubemap.vs", "shaders/10_fragment_cubemap.fs");
    dynamicShader = new Shader("shaders/10_vertex_skinning-IT.vs", "shaders/10_fragment_skinning-IT.fs");
    dynamicShader->setBonesIDs(MAX_RIGGING_BONES);

    fresnelShader = new Shader("shaders/11_Fresnel.vs", "shaders/11_Fresnel.fs");
    proceduralShader = new Shader("shaders/12_ProceduralAnimation.vs", "shaders/12_ProceduralAnimation.fs");
    wavesShader = new Shader("shaders/13_wavesAnimation.vs", "shaders/13_wavesAnimation.fs");
    trailShader = new Shader("shaders/trail.vs", "shaders/trail.fs");

    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    baseModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    baseModelMat = glm::scale(baseModelMat, glm::vec3(gEscala));

    HUB_OPACO = CargarModelo("models/HUB_OPACO.fbx");
    INVERNADERO_OPACO = CargarModelo("models/INVERNADERO_OPACO.fbx");
    INVERNADERO_TRASLUCIDO = CargarModelo("models/INVERNADERO_TRASLUCIDO.fbx");
    RECICLAJE_OPACO = CargarModelo("models/RECICLAJE_OPACO.fbx");
    RECICLAJE_METAL = CargarModelo("models/RECICLAJE_METAL.fbx");
    ENERGIA_OPACO = CargarModelo("models/ENERGIA_OPACO.fbx");
    ENERGIA_METALICO = CargarModelo("models/ENERGIA_METALICO.fbx");
    CAPSULAS_OPACO = CargarModelo("models/CAPSULAS_OPACO.fbx");
    CAJA_MODELO = CargarModelo("models/CAJA_INTERACTIVA.fbx");
    MOON2 = CargarModelo("models/moon2.fbx");
    BEBEDERO_BASE = CargarModelo("models/bebedero.fbx");
    PUERTA_MODELO = CargarModelo("models/puerta.fbx");
    BEBEDERO_AGUA = CargarModelo("models/bebedero_agua.fbx");
    CUBO_MODELO   = CargarModelo("models/cubo.fbx");
    PLANO_MODELO  = CargarModelo("models/plano.fbx");

    GenerarColisionesDeModelo(HUB_OPACO, 0, baseModelMat);
    GenerarColisionesDeModelo(INVERNADERO_OPACO, 1, baseModelMat);
    GenerarColisionesDeModelo(RECICLAJE_OPACO, 2, baseModelMat);
    GenerarColisionesDeModelo(RECICLAJE_METAL, 2, baseModelMat);
    GenerarColisionesDeModelo(ENERGIA_OPACO, 3, baseModelMat);
    GenerarColisionesDeModelo(ENERGIA_METALICO, 3, baseModelMat);
    GenerarColisionesDeModelo(CAPSULAS_OPACO, 4, baseModelMat);

    try { PERSONAJE = new AnimatedModel("models/personaje.fbx", 0); }
    catch (...) { PERSONAJE = nullptr; }

    InitRooms();
    ChangeZone(0);

    std::vector<std::string> faces{
        "textures/cubemap/01/posx.jpg", "textures/cubemap/01/negx.jpg",
        "textures/cubemap/01/posy.jpg", "textures/cubemap/01/negy.jpg",
        "textures/cubemap/01/posz.jpg", "textures/cubemap/01/negz.jpg"
    };
    mainCubeMap = new CubeMap();
    mainCubeMap->loadCubemap(faces);

    // === LUCES ===
    gLights.clear();

    // Layout de gLights con slots fijos (cada zona escribe siempre en el
    // mismo indice y las que no aplican se setean Power=0 cada frame):
    //   [0]    luz principal de la habitacion (color/intensidad por zona)
    //   [1]    "linterna" cercana al jugador
    //   [2]    nucleo del reactor (ENERGIA, intermitente)
    //   [3,4]  focos extra de CAPSULAS (highlight del intruso)
    //   [5..8] botones Simon de ENERGIA (rojo, verde, azul, amarillo)

    // [0] Luz principal de la habitacion
    Light roomLight;
    roomLight.type = 0;
    roomLight.Position = glm::vec3(0.0f, 4.0f, 0.0f);
    roomLight.Color = glm::vec4(1.0f, 0.98f, 0.92f, 1.0f);
    roomLight.Power = glm::vec4(5.0f);
    roomLight.alphaIndex = 16;
    roomLight.distance = 12.0f;
    gLights.push_back(roomLight);

    // [1] Linterna del jugador
    Light playerLight;
    playerLight.type = 0;
    playerLight.Position = glm::vec3(0.0f, 2.0f, 0.0f);
    playerLight.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    playerLight.Power = glm::vec4(0.3f);
    playerLight.alphaIndex = 64;
    playerLight.distance = 2.0f;
    gLights.push_back(playerLight);

    // [2] Luz INTERMITENTE del nucleo del reactor
    Light reactorLight;
    reactorLight.type = 0;
    reactorLight.Position = glm::vec3(0.0f, 2.25f, 10.0f);
    reactorLight.Color = glm::vec4(0.35f, 0.85f, 1.0f, 1.0f);
    reactorLight.Power = glm::vec4(0.0f);
    reactorLight.alphaIndex = 32;
    reactorLight.distance = 9.0f;
    gLights.push_back(reactorLight);

    // [3] Luz Extra 1 (Cpsulas)
    Light capsulaLight1;
    capsulaLight1.type = 0;
    capsulaLight1.Position = glm::vec3(0.0f);
    capsulaLight1.Color = glm::vec4(0.92f, 0.95f, 1.0f, 1.0f);
    capsulaLight1.Power = glm::vec4(0.0f);
    capsulaLight1.alphaIndex = 16;
    capsulaLight1.distance = 8.0f;
    gLights.push_back(capsulaLight1);

    // [4] Luz Extra 2 (Cpsulas)
    Light capsulaLight2 = capsulaLight1;
    gLights.push_back(capsulaLight2);

    // [5..8] Botones Simon de la zona ENERGIA. Apagadas por defecto; los
    // colores se actualizan cada frame en Update segun el estado del minijuego.
    glm::vec3 simonColors[4] = {
        glm::vec3(1.0f, 0.25f, 0.25f),
        glm::vec3(0.25f, 1.0f, 0.30f),
        glm::vec3(0.30f, 0.55f, 1.0f),
        glm::vec3(1.0f, 0.95f, 0.25f)
    };
    for (int i = 0; i < 4; ++i) {
        Light sb;
        sb.type = 0;
        sb.Position = SIMON_BUTTON_POSITIONS[i];
        sb.Color = glm::vec4(simonColors[i], 1.0f);
        sb.Power = glm::vec4(0.0f);
        sb.alphaIndex = 8;
        sb.distance = 4.0f;
        gLights.push_back(sb);
    }

    miPlantaFractal = new FractalPlant();
    miPlantaFractal->Generate(0);

    InitHud();

    return true;
}

// ============ CAMARA ============
// Cuando un minijuego pide vista fija (top-down invernadero, panel Simon)
// devolvemos un par eye/target propios y bloqueamos el mouse en ese frame.
glm::mat4 ComputeView() {
    if (currentZone == 1 && gInvernadero.active && gInvernadero.topDown) {
        // Centro ajustado al medio de las dos filas de macetas (z = -11).
        glm::vec3 center(0.0f, 0.0f, -11.0f);
        glm::vec3 eye(0.0f, 16.0f, -10.99f);
        return glm::lookAt(eye, center, glm::vec3(0.0f, 0.0f, -1.0f));
    }
    if (currentZone == 1 && gEspectro.active && gMinigameCamLocked) {
        // Chase cam real: la camara viaja con el meteorito en su propia
        // orbita, anclada detras del cometa (sobre su tangente de avance)
        // con una pequena elevacion. El target se proyecta ligeramente
        // adelante para que el jugador anticipe el beat y la composicion
        // tenga sensacion de movimiento. La math vive en EspectroChaseCam
        // para que ComputeEyePos use el mismo eye y la iluminacion sea
        // consistente con la vista.
        glm::vec3 eye, target;
        EspectroChaseCam(eye, target);
        return glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (currentZone == 3 && gEnergia.active && gMinigameCamLocked) {
        // Vista fija frente a la pila R_2 (centro entre los 4 botones de esa pila)
        glm::vec3 center(2.8f, 1.8f, 11.5f);
        glm::vec3 eye  (2.8f, 2.0f,  8.5f);
        return glm::lookAt(eye, center, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (cameraMode == CAM_FREE) return camera.GetViewMatrix();
    // forward incorpora yaw (horizontal) y pitch (vertical) para que el mouse
    // pueda inclinar la mirada hacia arriba/abajo en 1ra/3ra/2da persona.
    float yawR   = glm::radians(player.yaw);
    float pitchR = glm::radians(player.pitch);
    glm::vec3 fwd = glm::vec3(
        sinf(yawR) * cosf(pitchR),
        sinf(pitchR),
        cosf(yawR) * cosf(pitchR)
    );
    // fwdFlat: forward sin pitch (para colocar las camaras de seguimiento)
    glm::vec3 fwdFlat = glm::vec3(sinf(yawR), 0.0f, cosf(yawR));
    glm::vec3 eye, target;
    const float headHeight = 1.7f, distBack = 4.5f, distFront = 4.5f, camHeight = 2.8f;
    switch (cameraMode) {
    case CAM_FIRST:
        eye = player.position + glm::vec3(0.0f, headHeight, 0.0f);
        target = eye + fwd;
        break;
    case CAM_THIRD: {
        // La orbita en 3ra persona usa el pitch para subir/bajar la altura
        glm::vec3 back = -fwdFlat * distBack;
        eye = player.position + back + glm::vec3(0.0f, camHeight + player.pitch * 0.04f, 0.0f);
        target = player.position + glm::vec3(0.0f, 1.2f + player.pitch * 0.015f, 0.0f);
        break;
    }
    case CAM_SECOND: {
        glm::vec3 front = fwdFlat * distFront;
        eye = player.position + front + glm::vec3(0.0f, camHeight + player.pitch * 0.04f, 0.0f);
        target = player.position + glm::vec3(0.0f, 1.2f + player.pitch * 0.015f, 0.0f);
        break;
    }
    default: return camera.GetViewMatrix();
    }
    // Clamp del eye a los bounds de la habitacion para que la camara nunca
    // salga del modulo (atravesando paredes/techo). Dejamos un pequenio
    // margen para que la lente no quede pegada a la pared.
    if (currentZone >= 0 && currentZone < 5) {
        const Room& r = rooms[currentZone];
        const float margin = 0.4f;
        eye.x = glm::clamp(eye.x, r.boundsMin.x + margin, r.boundsMax.x - margin);
        eye.z = glm::clamp(eye.z, r.boundsMin.y + margin, r.boundsMax.y - margin);
        // techo aproximado en 6 m, suelo en 0.4 m
        eye.y = glm::clamp(eye.y, 0.6f, 5.5f);
    }
    return glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 ComputeEyePos() {
    if (currentZone == 1 && gInvernadero.active && gInvernadero.topDown) {
        return glm::vec3(0.0f, 14.0f, -9.99f);
    }
    if (currentZone == 1 && gEspectro.active && gMinigameCamLocked) {
        glm::vec3 eye, target;
        EspectroChaseCam(eye, target);
        return eye;
    }
    if (currentZone == 3 && gEnergia.active && gMinigameCamLocked) {
        return glm::vec3(2.8f, 2.0f, 8.5f);
    }
    if (cameraMode == CAM_FREE) return camera.Position;
    glm::vec3 fwd = glm::vec3(sinf(glm::radians(player.yaw)), 0.0f, cosf(glm::radians(player.yaw)));
    glm::vec3 eye;
    if      (cameraMode == CAM_FIRST)  eye = player.position + glm::vec3(0.0f, 1.7f, 0.0f);
    else if (cameraMode == CAM_THIRD)  eye = player.position - fwd * 4.5f + glm::vec3(0.0f, 2.8f + player.pitch * 0.04f, 0.0f);
    else if (cameraMode == CAM_SECOND) eye = player.position + fwd * 4.5f + glm::vec3(0.0f, 2.8f + player.pitch * 0.04f, 0.0f);
    else return camera.Position;
    if (currentZone >= 0 && currentZone < 5) {
        const Room& r = rooms[currentZone];
        const float margin = 0.4f;
        eye.x = glm::clamp(eye.x, r.boundsMin.x + margin, r.boundsMax.x - margin);
        eye.z = glm::clamp(eye.z, r.boundsMin.y + margin, r.boundsMax.y - margin);
        eye.y = glm::clamp(eye.y, 0.6f, 5.5f);
    }
    return eye;
}

// ============ UPDATE PERSONAJE ============
void UpdateCharacter(float dt) {
    if (!PERSONAJE) return;
    // Si hay un menu abierto, el avatar no responde al input pero seguimos
    // dejando que la animacion idle corra para que se vea natural al volver.
    if (menuState != MENU_NONE) {
        PERSONAJE->UpdateAnimation(dt);
        return;
    }

    player.isWalking = false;
    player.isSprinting = false;
    glm::vec3 fwd = glm::vec3(sinf(glm::radians(player.yaw)), 0.0f, cosf(glm::radians(player.yaw)));
    glm::vec3 movement(0.0f);

    // El avatar solo se mueve cuando NO estamos en camara libre y cuando no
    // estamos dentro de un minijuego con camara bloqueada (en ese caso WASD
    // queda inactivo para que el jugador se concentre en la tarea).
    if (cameraMode != CAM_FREE && !gMinigameCamLocked) {
        bool sprint = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS) ||
                      (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        float speed = player.walkSpeed * (sprint ? player.sprintMult : 1.0f);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { movement += fwd * speed * dt; player.isWalking = true; }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { movement -= fwd * speed * dt; player.isWalking = true; }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { player.yaw += player.turnSpeed * dt; }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { player.yaw -= player.turnSpeed * dt; }

        player.isSprinting = sprint && player.isWalking;

        // Salto: ESPACIO desde el piso da un impulso vertical
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.grounded) {
            player.vy = 4.5f;
            player.grounded = false;
        }
    }

    glm::vec3 newPos = player.position + movement;
    glm::vec3 testX(newPos.x, 0.0f, player.position.z);
    if (!CheckCollision(testX, PLAYER_RADIUS, currentZone)) player.position.x = newPos.x;

    glm::vec3 testZ(player.position.x, 0.0f, newPos.z);
    if (!CheckCollision(testZ, PLAYER_RADIUS, currentZone)) player.position.z = newPos.z;

    // Integracion vertical para el salto
    if (!player.grounded) {
        player.vy -= 12.0f * dt;
        player.position.y += player.vy * dt;
        if (player.position.y <= 0.0f) {
            player.position.y = 0.0f;
            player.vy = 0.0f;
            player.grounded = true;
        }
    } else {
        player.position.y = 0.0f;
    }

    CheckDoorTriggers();

    PERSONAJE->UpdateAnimation(dt);
}

void DrawCharacter(const glm::mat4& projection, const glm::mat4& view) {
    if (!PERSONAJE) return;
    bool topDown = (currentZone == 1 && gInvernadero.active && gInvernadero.topDown);
    if (cameraMode == CAM_FIRST && !topDown) return;

    dynamicShader->use();
    dynamicShader->setMat4("projection", projection);
    dynamicShader->setMat4("view", view);
    dynamicShader->setVec3("EyeDirection_cameraspace", ComputeEyePos());

    glm::mat4 m(1.0f);
    m = glm::translate(m, player.position);
    m = glm::rotate(m, glm::radians(player.yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::scale(m, glm::vec3(0.01f));
    dynamicShader->setMat4("model", m);

    for (unsigned int i = 0; i < PERSONAJE->m_NumBones; i++) {
        dynamicShader->SetBoneTransform(i, PERSONAJE->gBones[i]);
    }
    PERSONAJE->Draw(*dynamicShader);
}

// ============ UPDATE PRINCIPAL ============
bool Update() {
    float currentFrame = (float)glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    float t = currentFrame;

    processInput(window);
    UpdateCharacter(deltaTime);

    // Cursor visible cuando el menu esta abierto o cuando jugamos Simon (clic).
    // Solo llamamos glfwSetInputMode cuando el estado cambia para que GLFW no
    // teletransporte el cursor cada frame; y reseteamos firstMouse en cada
    // transicion para que el siguiente xoffset/yoffset no produzca un salto
    // gigantesco en el yaw del jugador.
    static bool prevWantCursor = false;
    bool wantCursor = (menuState != MENU_NONE) ||
                      (currentZone == 3 && gEnergia.active && gMinigameCamLocked);
    if (wantCursor != prevWantCursor) {
        glfwSetInputMode(window, GLFW_CURSOR,
                         wantCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        firstMouse = true;
        prevWantCursor = wantCursor;
    }

    if (menuState == MENU_NONE && gZoneBannerTimer > 0.0f) {
        gZoneBannerTimer -= deltaTime;
    }

    // Mostrar la intro una sola vez al inicio del programa
    if (!gIntroShown) {
        PushNarrative("Bienvenido a la estacion espacial sustentable.");
        PushNarrative("Explora los 4 modulos y completa la tarea de cada uno.");
        PushNarrative("Muevete con WASD, mira con el mouse. Pulsa ESC para el menu.");
        gIntroShown = true;
    }

    // Procesa la cola de narrativa: cada mensaje vive NARRATIVE_DURATION s.
    if (menuState == MENU_NONE && !gNarrativeQueue.empty()) {
        gNarrativeTimer -= deltaTime;
        if (gNarrativeTimer <= 0.0f) {
            gNarrativeQueue.erase(gNarrativeQueue.begin());
            gNarrativeTimer = gNarrativeQueue.empty() ? 0.0f : NARRATIVE_DURATION;
        }
    }

    // --- Logica de minijuegos --- (pausada cuando el menu esta abierto)
    if (menuState == MENU_NONE) {
        if (currentZone == 1 && gInvernadero.active) UpdateInvernaderoGame(deltaTime);
        if (currentZone == 1 && gEspectro.active)    UpdateEspectroGame(deltaTime);
        if (currentZone == 3 && gEnergia.active)     UpdateEnergiaGame(deltaTime);
        if (currentZone == 4 && gCapsulas.active)    UpdateCapsulasGame(deltaTime);
    }

    // Resetear luces extras por defecto
    if (gLights.size() > 2) gLights[2].Power = glm::vec4(0.0f);
    if (gLights.size() > 3) gLights[3].Power = glm::vec4(0.0f);
    if (gLights.size() > 4) gLights[4].Power = glm::vec4(0.0f);
    // Botones Simon de ENERGIA (slots 5..8) apagados salvo cuando estemos
    // dentro de la zona correspondiente y se reactiven mas abajo.
    for (size_t i = 5; i < gLights.size() && i < 9; ++i) {
        gLights[i].Power = glm::vec4(0.0f);
    }

    if (gLights.size() > 0) {
        switch (currentZone) {
        case 0: // HUB
            gLights[0].Position = glm::vec3(0.0f, 2.7f, 0.0f);
            gLights[0].Color = glm::vec4(1.0f, 0.98f, 0.92f, 1.0f);
            gLights[0].Power = glm::vec4(3.0f);
            gLights[0].distance = 15.0f;
            break;

        case 1: // INVERNADERO
            gLights[0].Position = glm::vec3(0.0f, 7.5f, -12.5f);
            gLights[0].Color = glm::vec4(0.75f, 1.0f, 0.75f, 1.0f);
            gLights[0].Power = glm::vec4(11.0f);
            gLights[0].distance = 40.0f;
            break;

        case 2: // RECICLAJE
            gLights[0].Position = glm::vec3(9.3f, 4.0f, 0.0f);
            gLights[0].Color = glm::vec4(0.65f, 0.8f, 1.0f, 1.0f);
            gLights[0].Power = glm::vec4(6.0f);
            gLights[0].distance = 15.0f;
            break;

        case 3: // ENERGIA
            gLights[0].Position = glm::vec3(0.0f, 4.0f, 10.0f);
            gLights[0].Color = glm::vec4(1.0f, 0.55f, 0.25f, 1.0f);
            gLights[0].Power = glm::vec4(7.0f);
            gLights[0].distance = 16.0f;

            if (gLights.size() > 2) {
                float fastBlink = (sinf(t * 11.0f) > 0.0f) ? 1.0f : 0.15f;
                float slowPulse = 0.6f + 0.4f * sinf(t * 2.3f);
                float power = 14.0f * fastBlink * slowPulse;

                float warm = 0.5f + 0.5f * sinf(t * 1.7f);
                glm::vec4 col = glm::mix(
                    glm::vec4(0.30f, 0.85f, 1.00f, 1.0f),
                    glm::vec4(0.85f, 0.95f, 1.00f, 1.0f),
                    warm
                );

                gLights[2].Position = glm::vec3(0.0f, 2.25f, 10.0f);
                gLights[2].Color = col;
                gLights[2].Power = glm::vec4(power);
                gLights[2].distance = 9.0f;
            }

            // Botones Simon: brillo proporcional a buttonGlow + halo base
            // suave para que siempre se vean encendidos cuando estes en la
            // zona (el minijuego los pulsa cuando suenan en la secuencia o
            // cuando el jugador los presiona).
            for (int i = 0; i < 4 && (size_t)(5 + i) < gLights.size(); ++i) {
                float glow = gEnergia.buttonGlow[i];
                float ambient = 0.5f + 0.15f * sinf(t * 1.5f + i);
                float power = ambient + glow * 5.0f;
                gLights[5 + i].Position = SIMON_BUTTON_POSITIONS[i];
                gLights[5 + i].Power = glm::vec4(power);
            }
            break;

        case 4: // CAPSULAS (3 Luces en el pasillo)
            // Luz Central
            gLights[0].Position = glm::vec3(-10.75f, 4.5f, 0.0f);
            gLights[0].Color = glm::vec4(0.92f, 0.95f, 1.0f, 1.0f);
            gLights[0].Power = glm::vec4(4.0f);
            gLights[0].distance = 10.0f;

            // Luz Este (Entrada)
            if (gLights.size() > 3) {
                gLights[3].Position = glm::vec3(-7.5f, 4.5f, 0.0f);
                gLights[3].Color = glm::vec4(0.92f, 0.95f, 1.0f, 1.0f);
                gLights[3].Power = glm::vec4(4.0f);
                gLights[3].distance = 10.0f;
            }

            // Luz Oeste (Fondo)
            if (gLights.size() > 4) {
                gLights[4].Position = glm::vec3(-14.0f, 4.5f, 0.0f);
                gLights[4].Color = glm::vec4(0.92f, 0.95f, 1.0f, 1.0f);
                gLights[4].Power = glm::vec4(4.0f);
                gLights[4].distance = 10.0f;
            }
            break;
        }
    }

    if (gLights.size() > 1) gLights[1].Position = player.position + glm::vec3(0.0f, 2.2f, 0.0f);

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, gWireframe ? GL_LINE : GL_FILL);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 view = ComputeView();
    glm::vec3 eyePos = ComputeEyePos();

    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);
    phongShader->setVec3("eye", eyePos);
    phongShader->setVec4("globalAmbient", globalAmbient);
    SetLightUniforms(phongShader);

    phongShader->setMat4("model", baseModelMat);
    phongShader->setFloat("transparency", 1.0f);

    glm::vec4 ambOpaque = glm::vec4(0.20f, 0.20f, 0.20f, 1.0f);
    glm::vec4 diffOpaque = glm::vec4(1.00f, 1.00f, 1.00f, 1.0f);
    glm::vec4 specOpaque = glm::vec4(0.40f, 0.40f, 0.40f, 1.0f);
    int alphaOpaque = 32;


    // Material metalico mas pulido: specular muy alto y un tono base mas claro
    // para que se note el brillo tipo cromo y el highlight a contraluz.
    glm::vec4 ambMetal = glm::vec4(0.06f, 0.06f, 0.07f, 1.0f);
    glm::vec4 diffMetal = glm::vec4(0.65f, 0.66f, 0.70f, 1.0f);
    glm::vec4 specMetal = glm::vec4(20.0f, 20.0f, 22.0f, 1.0f);
    int alphaMetal = 4096;

    // Pase fresnel real con cubemap del skybox para los modelos metalicos.
    // Se dibuja con blend aditivo encima del pase phong base, asi el color
    // del FBX sigue debajo y arriba se anade el reflejo del entorno (chrome).
    // Bias alto + Scale moderado mantiene reflejo incluso al ver el metal
    // de frente; en angulos rasantes la reflexion llega a 1 (rim brillante).
    auto DrawMetalFresnelOverlay = [&](Model* m, const glm::mat4& modelMat) {
        if (!m || !fresnelShader || !mainCubeMap) return;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        // El pase phong base ya escribio depth para este modelo; con GL_LESS
        // por defecto el segundo pase no pasaria el depth test (los z son
        // iguales). Cambiamos a GL_LEQUAL para que el overlay quede visible.
        glDepthFunc(GL_LEQUAL);
        fresnelShader->use();
        fresnelShader->setMat4("projection", projection);
        fresnelShader->setMat4("view", view);
        fresnelShader->setMat4("model", modelMat);
        fresnelShader->setVec3("cameraPosition", eyePos);
        fresnelShader->setFloat("_Bias",  0.20f);
        fresnelShader->setFloat("_Scale", 0.90f);
        fresnelShader->setFloat("_Power", 3.0f);
        fresnelShader->setFloat("mRefractionRatio", 0.98f);
        fresnelShader->setFloat("fresnelAlpha", 0.55f); // intensidad del overlay
        fresnelShader->setFloat("diffuseMix",  0.0f);    // solo cubemap, no diffuse
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mainCubeMap->textureID);
        fresnelShader->setInt("cubetex", 5);
        glActiveTexture(GL_TEXTURE0);
        m->Draw(*fresnelShader);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
    };


    // Dibuja la puerta dorada PUERTA_MODELO en el centro de la hitbox dada,
    // con un pulso de brillo ambiental + boost cuando el jugador se acerca.
    // Se usa tanto para las 4 puertas del HUB como para las puertas de
    // retorno de cada zona habitable (asi todas las zonas tienen un
    // marcador visible donde se activa el teletransporte).
    auto DrawZoneDoor = [&](const ZoneDoor& d, float doorYaw) {
        if (!PUERTA_MODELO) return;
        float pulseGlobal = 0.85f + 0.15f * sinf(t * 2.2f);
        float dx = player.position.x - d.center.x;
        float dz = player.position.z - d.center.z;
        float dist2 = dx * dx + dz * dz;
        // Brillo cercano: hasta 1.7x cuando el jugador esta encima.
        // (`near`/`far` son macros de windef.h, no usar como variables)
        float proximity = glm::clamp(1.0f - dist2 / 9.0f, 0.0f, 1.0f);
        float pulse = pulseGlobal * (1.0f + proximity * 0.7f);

        glm::vec4 doorAmb  = glm::vec4(0.25f, 0.20f, 0.08f, 1.0f) * pulse;
        glm::vec4 doorDiff = glm::vec4(0.95f, 0.78f, 0.30f, 1.0f) * pulse;
        glm::vec4 doorSpec = glm::vec4(0.60f, 0.55f, 0.30f, 1.0f);
        phongShader->use();
        SetMaterial(phongShader, doorAmb, doorDiff, doorSpec, 64);

        glm::mat4 m(1.0f);
        m = glm::translate(m, d.center);
        m = glm::rotate(m, glm::radians(doorYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.6f));
        phongShader->setMat4("model", m);
        PUERTA_MODELO->Draw(*phongShader);
    };

    // Yaw para cada puerta de retorno (idx 4..7 -> INV, REC, ENE, CAP).
    // La cara de la puerta encara hacia el HUB (jugador entrante).
    //   INV (sur del HUB) -> mira al norte: yaw 0
    //   REC (este)        -> mira al oeste: yaw 90
    //   ENE (norte)       -> mira al sur:   yaw 180
    //   CAP (oeste)       -> mira al este:  yaw -90
    const float RETURN_DOOR_YAW[4] = { 0.0f, 90.0f, 180.0f, -90.0f };

    switch (currentZone) {
    case 0: // HUB
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (HUB_OPACO) HUB_OPACO->Draw(*phongShader);

        // Puertas N/S/E/O del HUB. Yaw para que la cara mire hacia adentro.
        // 0=S, 1=E, 2=N, 3=O.
        {
            const float doorYaws[4] = { 180.0f, -90.0f, 0.0f, 90.0f };
            for (int i = 0; i < 4; ++i) {
                if (zoneDoors[i].fromZone != 0) continue;
                DrawZoneDoor(zoneDoors[i], doorYaws[i]);
            }
            // Restablecer material para no afectar dibujos posteriores
            SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        }
        break;

    case 1: // INVERNADERO
        mainCubeMap->drawCubeMap(*cubemapShader, projection, view);

        phongShader->use();
        phongShader->setMat4("projection", projection);
        phongShader->setMat4("view", view);
        phongShader->setMat4("model", baseModelMat);
        phongShader->setVec3("eye", eyePos);
        SetLightUniforms(phongShader);

        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (INVERNADERO_OPACO) INVERNADERO_OPACO->Draw(*phongShader);

        // Domo translucido: fresnel real con cubemap del entorno. El alpha
        // baja a 0.18 para que se vea como vidrio (antes 0.35 daba un look
        // mas reflejante que transparente). Bias bajo + Scale alto concentra
        // el reflejo solo en los bordes, dejando el centro casi transparente
        // (clasico fresnel de vidrio curvado).
        if (INVERNADERO_TRASLUCIDO && fresnelShader && mainCubeMap) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            fresnelShader->use();
            fresnelShader->setMat4("projection", projection);
            fresnelShader->setMat4("view", view);
            fresnelShader->setMat4("model", baseModelMat);
            fresnelShader->setVec3("cameraPosition", eyePos);
            fresnelShader->setFloat("_Bias",  0.05f);
            fresnelShader->setFloat("_Scale", 1.20f);
            fresnelShader->setFloat("_Power", 2.5f);
            fresnelShader->setFloat("mRefractionRatio", 1.02f);
            fresnelShader->setFloat("fresnelAlpha", 0.18f);
            fresnelShader->setFloat("diffuseMix",  0.0f);
            // Cubemap del skybox en la unidad 5: las unidades 0..4 las usa
            // Mesh::Draw para texture_diffuseN; la 5 queda libre para nuestro
            // samplerCube. Restauramos la unidad activa al terminar.
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, mainCubeMap->textureID);
            fresnelShader->setInt("cubetex", 5);
            glActiveTexture(GL_TEXTURE0);
            INVERNADERO_TRASLUCIDO->Draw(*fresnelShader);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        // ---- ESTELA DEL COMETA (m�s visible) ----
        if (MOON2 && proceduralShader) {
            proceduralShader->use();
            proceduralShader->setMat4("projection", projection);
            proceduralShader->setMat4("view", view);

            glm::mat4 moonMat(1.0f);
            moonMat = glm::translate(moonMat, glm::vec3(0.0f, 11.0f, -12.5f));
            moonMat = glm::rotate(moonMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            moonMat = glm::scale(moonMat, glm::vec3(0.5f));
            proceduralShader->setMat4("model", moonMat);

            float moonTime = t * 0.3f;
            float moonRadius = 15.0f;
            float moonHeight = 0.0f;

            proceduralShader->setFloat("time", moonTime);
            proceduralShader->setFloat("radius", moonRadius);
            proceduralShader->setFloat("height", moonHeight);

            MOON2->Draw(*proceduralShader);

            // Posici�n del cometa en el mundo
            glm::vec3 moonLocalPos(
                moonRadius * cosf(moonTime),
                moonRadius * sinf(moonTime),
                moonHeight
            );
            glm::vec3 moonWorldPos = glm::vec3(moonMat * glm::vec4(moonLocalPos, 1.0f));

            glm::vec3 moonLocalDir(
                -moonRadius * sinf(moonTime),
                moonRadius * cosf(moonTime),
                0.0f
            );
            glm::vec3 moonWorldDir = glm::vec3(moonMat * glm::vec4(moonLocalDir, 0.0f));
            if (glm::length(moonWorldDir) > 0.0001f) moonWorldDir = glm::normalize(moonWorldDir);
            else moonWorldDir = glm::vec3(0.0f, 1.0f, 0.0f);

            trailEmitTimer += deltaTime;

            if (trailEmitTimer >= 0.002f){
                trailEmitTimer = 0.0f;
                SpawnMoonTrail(moonWorldPos, moonWorldDir, t);
            }

            UpdateMoonTrail(deltaTime);

            if (trailShader && !moonTrail.empty()) {
                trailShader->use();
                trailShader->setMat4("projection", projection);
                trailShader->setMat4("view", view);

                glDisable(GL_DEPTH_TEST);      // clave para que no se oculte
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                glBindVertexArray(trailVAO);

                for (const auto& p : moonTrail) {
                    float k = glm::clamp(p.life / p.maxLife, 0.0f, 1.0f);

                    glm::vec4 c = p.color;
                    c.a = k * 1.0f;

                    trailShader->setVec4("particleColor", c);
                    trailShader->setFloat(
                        "particleSize",
                        p.size * (0.55f + 0.35f * k)
                    );

                    float pos[3] = { p.pos.x, p.pos.y, p.pos.z };
                    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pos), pos);

                    glDrawArrays(GL_POINTS, 0, 1);
                }

                glBindVertexArray(0);

                glEnable(GL_DEPTH_TEST);

                glDepthMask(GL_TRUE);

                glDisable(GL_BLEND);

                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
        }

        phongShader->use();
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        glm::mat4 bebederoMat(1.0f);
        bebederoMat = glm::translate(bebederoMat, glm::vec3(5.0f, 0.2f, -10.1f));
        bebederoMat = glm::rotate(bebederoMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        bebederoMat = glm::scale(bebederoMat, glm::vec3(gEscala));
        phongShader->setMat4("model", bebederoMat);
        if (BEBEDERO_BASE) BEBEDERO_BASE->Draw(*phongShader);

        if (BEBEDERO_AGUA && wavesShader) {
            wavesShader->use();
            wavesShader->setMat4("projection", projection);
            wavesShader->setMat4("view", view);
            wavesShader->setMat4("model", bebederoMat);
            wavesShader->setFloat("time", t);
            BEBEDERO_AGUA->Draw(*wavesShader);
        }

        // ====== MINIJUEGO OSU (Invernadero) ======
        if (gInvernadero.active) DrawInvernaderoGame(projection, view);
        if (gEspectro.active)    DrawEspectroGame(projection, view);

        // ====== PLANTA FRACTAL (HELECHO) ======
        if (fractalIteration > 0 && miPlantaFractal) {
            phongShader->use();

            glm::vec4 ambPlanta = glm::vec4(0.05f, 0.4f, 0.05f, 1.0f);
            glm::vec4 diffPlanta = glm::vec4(0.1f, 0.8f, 0.1f, 1.0f);
            glm::vec4 specPlanta = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
            SetMaterial(phongShader, ambPlanta, diffPlanta, specPlanta, alphaOpaque);

            glm::mat4 plantMat(1.0f);
            plantMat = glm::translate(plantMat, glm::vec3(2.1f, 0.5f, -10.1f));
            plantMat = glm::scale(plantMat, glm::vec3(0.4f));

            glLineWidth(4.0f);
            miPlantaFractal->Draw(*phongShader, plantMat);
            glLineWidth(1.0f);
        }

        // Puerta de retorno al HUB (zoneDoors[4], borde norte del invernadero)
        DrawZoneDoor(zoneDoors[4], RETURN_DOOR_YAW[0]);
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        break;

    case 2: // RECICLAJE
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (RECICLAJE_OPACO) RECICLAJE_OPACO->Draw(*phongShader);
        // Pase metalico base + overlay fresnel/rim para que se vea brilloso
        phongShader->setMat4("model", baseModelMat);
        SetMaterial(phongShader, ambMetal, diffMetal, specMetal, alphaMetal);
        if (RECICLAJE_METAL) {
            RECICLAJE_METAL->Draw(*phongShader);
            DrawMetalFresnelOverlay(RECICLAJE_METAL, baseModelMat);
        }

        // ====== CAJAS INTERACTIVAS CON RESTRICCIONES MANUALES ======
        if (CAJA_MODELO) {
            SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
            for (int i = 0; i < 5; i++) {
                glm::mat4 m(1.0f);
                m = glm::translate(m, interactiveBoxes[i].position);
                m = glm::rotate(m, interactiveBoxes[i].rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
                m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                // Escala base ajustada a 0.5f, multiplicada por su escala interactiva
                m = glm::scale(m, glm::vec3(0.5f * interactiveBoxes[i].scale));

                phongShader->setMat4("model", m);
                CAJA_MODELO->Draw(*phongShader);
            }
        }

        // ====== MINIJUEGO RECICLAJE (items + botes) ======
        if (gReciclaje.active) DrawReciclajeGame(projection, view);

        // Puerta de retorno al HUB (zoneDoors[5], borde oeste del modulo)
        DrawZoneDoor(zoneDoors[5], RETURN_DOOR_YAW[1]);
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        break;

    case 3: // ENERGIA
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (ENERGIA_OPACO) ENERGIA_OPACO->Draw(*phongShader);
        // Pase metalico base + overlay fresnel/rim
        phongShader->setMat4("model", baseModelMat);
        SetMaterial(phongShader, ambMetal, diffMetal, specMetal, alphaMetal);
        if (ENERGIA_METALICO) {
            ENERGIA_METALICO->Draw(*phongShader);
            DrawMetalFresnelOverlay(ENERGIA_METALICO, baseModelMat);
        }

        // ====== MINIJUEGO SIMON (Energia) ======
        if (gEnergia.active) DrawEnergiaGame(projection, view);

        // Puerta de retorno al HUB (zoneDoors[6], borde sur del modulo)
        DrawZoneDoor(zoneDoors[6], RETURN_DOOR_YAW[2]);
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        break;

    case 4: // CAPSULAS
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (CAPSULAS_OPACO) CAPSULAS_OPACO->Draw(*phongShader);

        // ====== MINIJUEGO BUSCAR AL INTRUSO (Capsulas) ======
        if (gCapsulas.active) DrawCapsulasGame(projection, view);

        // Puerta de retorno al HUB (zoneDoors[7], borde este del modulo)
        DrawZoneDoor(zoneDoors[7], RETURN_DOOR_YAW[3]);
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        break;
    }

    DrawCharacter(projection, view);

    DrawHud();

    glfwSwapBuffers(window);
    glfwPollEvents();
    return true;
}

// ============ HUD / MENU ============
void InitHud() {
    hudShader = new Shader("shaders/hud.vs", "shaders/hud.fs");
    hudProjection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);

    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    gText = new TextRenderer();
    std::vector<std::string> fontTries = {
        "fonts/JetBrainsMono-Bold.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/consola.ttf"
    };
    // Antes 32 px: el texto se veia diminuto. 48 px da letras claramente
    // legibles en pantallas 1024x768 sin necesidad de subir mucho el scale.
    gText->Init(fontTries, 48, "shaders/text.vs", "shaders/text.fs",
                (int)SCR_WIDTH, (int)SCR_HEIGHT);
}

void DrawQuad2D(float x, float y, float w, float h, glm::vec4 color) {
    if (!hudShader) return;
    float verts[12] = {
        x,     y,
        x + w, y,
        x + w, y + h,
        x,     y,
        x + w, y + h,
        x,     y + h
    };
    hudShader->use();
    hudShader->setMat4("projection", hudProjection);
    hudShader->setVec4("uColor", color);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DrawTextCentered(const std::string& text, float cx, float y, float scale, glm::vec3 color) {
    if (!gText || !gText->ready) return;
    float w = gText->MeasureWidth(text, scale);
    gText->RenderText(text, cx - w * 0.5f, y, scale, color);
}

static const char* kZoneNames[5] = { "HUB", "INVERNADERO", "RECICLAJE", "ENERGIA", "CAPSULAS" };

// Etiqueta del primer item del menu (toggle del recorrido). Depende del modo
// actual y del paso: si nunca empezo el recorrido decimos "Iniciar", si esta
// activo "Pausar", y si esta pausado a mitad "Reanudar (X/5)".
static std::string RecorridoMenuLabel() {
    if (gGameMode == MODE_RECORRIDO) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Pausar recorrido (%d/5)", gRecorridoStep + 1);
        return buf;
    }
    if (gRecorridoStep == 0) return "Iniciar recorrido guiado";
    char buf[64];
    snprintf(buf, sizeof(buf), "Reanudar recorrido (%d/5)", gRecorridoStep + 1);
    return buf;
}

void DrawPauseMenu() {
    // Fondo oscuro semitransparente
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    DrawQuad2D(0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, glm::vec4(0.0f, 0.0f, 0.05f, 0.65f));

    // Panel central: subi la altura porque ahora hay 5 opciones en lugar de 4.
    float panelW = 460.0f, panelH = 440.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f;
    float panelY = (SCR_HEIGHT - panelH) * 0.5f;
    DrawQuad2D(panelX, panelY, panelW, panelH, glm::vec4(0.05f, 0.10f, 0.18f, 0.92f));
    // Borde superior tipo Genshin (linea dorada)
    DrawQuad2D(panelX, panelY + panelH - 4.0f, panelW, 4.0f, glm::vec4(0.92f, 0.78f, 0.35f, 1.0f));
    DrawQuad2D(panelX, panelY, panelW, 4.0f, glm::vec4(0.92f, 0.78f, 0.35f, 1.0f));

    float cx = SCR_WIDTH * 0.5f;
    DrawTextCentered("PAUSA", cx, panelY + panelH - 70.0f, 1.0f, glm::vec3(0.95f, 0.82f, 0.35f));
    DrawTextCentered(std::string("Zona: ") + kZoneNames[currentZone], cx,
                     panelY + panelH - 110.0f, 0.55f, glm::vec3(0.85f, 0.88f, 0.95f));
    DrawTextCentered(std::string("Modo: ") + (gGameMode == MODE_RECORRIDO ? "HISTORIA" : "LIBRE"),
                     cx, panelY + panelH - 138.0f, 0.50f,
                     gGameMode == MODE_RECORRIDO
                         ? glm::vec3(0.40f, 0.90f, 0.65f)
                         : glm::vec3(0.70f, 0.78f, 0.95f));

    std::string recLabel = RecorridoMenuLabel();
    // "Continuar" se mantiene como la primera opcion para preservar el habito
    // de cerrar el menu con Enter; el toggle del recorrido entra justo debajo
    // donde sigue siendo el primer destino al pulsar abajo o pasar el mouse.
    const std::string options[5] = {
        "Continuar", recLabel, "Controles", "Mapa", "Salir"
    };
    float yBase = panelY + panelH - 180.0f;
    float lineH = 46.0f;
    for (int i = 0; i < 5; ++i) {
        float y = yBase - i * lineH;
        if (i == menuSelection) {
            DrawQuad2D(panelX + 30.0f, y - 10.0f, panelW - 60.0f, 38.0f,
                       glm::vec4(0.92f, 0.78f, 0.35f, 0.28f));
        }
        glm::vec3 col = (i == menuSelection)
            ? glm::vec3(1.0f, 0.95f, 0.55f)
            : glm::vec3(0.80f, 0.85f, 0.92f);
        DrawTextCentered(options[i], cx, y, 0.68f, col);
    }
    DrawTextCentered("Flechas para navegar, Enter para seleccionar, ESC para volver",
                     cx, panelY + 22.0f, 0.40f, glm::vec3(0.55f, 0.60f, 0.70f));

    glEnable(GL_DEPTH_TEST);
}

void DrawControlsOverlay() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    DrawQuad2D(0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, glm::vec4(0.0f, 0.0f, 0.05f, 0.75f));

    float panelW = 620.0f, panelH = 540.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f;
    float panelY = (SCR_HEIGHT - panelH) * 0.5f;
    DrawQuad2D(panelX, panelY, panelW, panelH, glm::vec4(0.05f, 0.10f, 0.18f, 0.94f));
    DrawQuad2D(panelX, panelY + panelH - 4.0f, panelW, 4.0f, glm::vec4(0.92f, 0.78f, 0.35f, 1.0f));
    DrawQuad2D(panelX, panelY, panelW, 4.0f, glm::vec4(0.92f, 0.78f, 0.35f, 1.0f));

    float cx = SCR_WIDTH * 0.5f;
    DrawTextCentered("CONTROLES", cx, panelY + panelH - 60.0f, 0.95f, glm::vec3(0.95f, 0.82f, 0.35f));

    // Filas: descripcion + tecla
    struct Row { const char* label; const char* key; };
    Row rows[] = {
        { "Avanzar / Retroceder",     "W / S" },
        { "Girar izquierda / derecha","A / D" },
        { "Camara",                   "Mouse" },
        { "Correr (sprint)",          "Shift" },
        { "Saltar",                   "Espacio" },
        { "Interactuar",              "F" },
        { "Camara 1ra / 2da / 3ra",   "F1 / F2 / F3" },
        { "Camara libre",             "F8" },
        { "Menu pausa",               "ESC" },
        { "Mapa rapido",              "M" },
        { "Ir a zona (HUB..Capsulas)","0 1 2 3 4" },
        { "Dificultad zona",          "5 / 6 / 7" },
        { "Recorrido: siguiente paso","N" },
        { "Salir / interrumpir minijuego", "TAB" }
    };
    int n = sizeof(rows) / sizeof(rows[0]);

    float yTop = panelY + panelH - 110.0f;
    float lineH = 32.0f;
    float labelX = panelX + 40.0f;
    float keyX   = panelX + panelW - 240.0f;

    for (int i = 0; i < n; ++i) {
        float y = yTop - i * lineH;
        if (i % 2 == 0) {
            DrawQuad2D(panelX + 20.0f, y - 6.0f, panelW - 40.0f, lineH - 2.0f,
                       glm::vec4(1.0f, 1.0f, 1.0f, 0.04f));
        }
        if (gText && gText->ready) {
            gText->RenderText(rows[i].label, labelX, y, 0.45f, glm::vec3(0.85f, 0.88f, 0.95f));
            gText->RenderText(rows[i].key,   keyX,   y, 0.45f, glm::vec3(1.0f,  0.95f, 0.55f));
        }
    }

    DrawTextCentered("ESC para volver", cx, panelY + 22.0f, 0.40f,
                     glm::vec3(0.55f, 0.60f, 0.70f));

    glEnable(GL_DEPTH_TEST);
}

// Coordenadas del mundo a coordenadas del minimapa. Cada habitacion se
// pinta como un rect aproximado a sus bounds, y la posicion del jugador
// se mapea con la misma transformacion para que coincida.
static void WorldToMap(float wx, float wz, float mapX, float mapY,
                       float mapW, float mapH, float wMin, float wMax,
                       float& outX, float& outY) {
    float u = (wx - wMin) / (wMax - wMin);
    float v = (wz - wMin) / (wMax - wMin);
    // En el mundo, +z es "sur" del HUB y -z es "norte". En el mapa
    // queremos que el norte (-z) este arriba, asi que invertimos v.
    outX = mapX + u * mapW;
    outY = mapY + (1.0f - v) * mapH;
}

void DrawMinimap() {
    if (!showMinimap) return;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float size = 200.0f;
    float pad  = 16.0f;
    float mapX = (float)SCR_WIDTH - size - pad;
    float mapY = (float)SCR_HEIGHT - size - pad;

    // Fondo y borde
    DrawQuad2D(mapX - 3.0f, mapY - 3.0f, size + 6.0f, size + 6.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));
    DrawQuad2D(mapX, mapY, size, size, glm::vec4(0.05f, 0.10f, 0.18f, 0.85f));

    // Rango del mundo cubierto por el mapa. Cubrimos -16..16 en X y Z para
    // que entren las 5 zonas comodamente.
    const float W_MIN = -16.0f, W_MAX = 16.0f;

    // Pintamos cada habitacion como un rect transparente
    glm::vec4 zoneColors[5] = {
        glm::vec4(0.85f, 0.85f, 0.85f, 0.55f), // HUB neutro
        glm::vec4(0.40f, 0.85f, 0.45f, 0.55f), // INVERNADERO verde
        glm::vec4(0.90f, 0.65f, 0.30f, 0.55f), // RECICLAJE naranja
        glm::vec4(0.50f, 0.70f, 1.00f, 0.55f), // ENERGIA azul
        glm::vec4(0.80f, 0.45f, 0.95f, 0.55f)  // CAPSULAS morado
    };
    for (int i = 0; i < 5; ++i) {
        float x1, y1, x2, y2;
        WorldToMap(rooms[i].boundsMin.x, rooms[i].boundsMin.y, mapX, mapY, size, size,
                   W_MIN, W_MAX, x1, y1);
        WorldToMap(rooms[i].boundsMax.x, rooms[i].boundsMax.y, mapX, mapY, size, size,
                   W_MIN, W_MAX, x2, y2);
        float rx = (x1 < x2) ? x1 : x2;
        float ry = (y1 < y2) ? y1 : y2;
        float rw = std::fabs(x2 - x1);
        float rh = std::fabs(y2 - y1);
        glm::vec4 c = zoneColors[i];
        if (i == currentZone) c.a = 0.95f; // zona actual se ilumina
        DrawQuad2D(rx, ry, rw, rh, c);
    }

    // Puerta como pequeño marcador amarillo
    for (int i = 0; i < 8; ++i) {
        if (zoneDoors[i].fromZone != currentZone) continue;
        float dx, dy;
        WorldToMap(zoneDoors[i].center.x, zoneDoors[i].center.z, mapX, mapY,
                   size, size, W_MIN, W_MAX, dx, dy);
        DrawQuad2D(dx - 3.0f, dy - 3.0f, 6.0f, 6.0f,
                   glm::vec4(1.0f, 0.95f, 0.35f, 1.0f));
    }

    // Posicion del jugador (punto rojo)
    float px, py;
    WorldToMap(player.position.x, player.position.z, mapX, mapY, size, size,
               W_MIN, W_MAX, px, py);
    DrawQuad2D(px - 4.0f, py - 4.0f, 8.0f, 8.0f, glm::vec4(1.0f, 0.25f, 0.25f, 1.0f));
    DrawQuad2D(px - 2.0f, py - 2.0f, 4.0f, 4.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (gText && gText->ready) {
        gText->RenderText(kZoneNames[currentZone], mapX + 6.0f, mapY + size - 22.0f,
                          0.40f, glm::vec3(1.0f, 0.95f, 0.55f));
    }

    glEnable(GL_DEPTH_TEST);
}

// Banner grande con el nombre de la zona al entrar; fade-in 0.3s, sostiene
// el resto del tiempo y fade-out durante el ultimo medio segundo.
void DrawZoneBanner() {
    if (gZoneBannerTimer <= 0.0f || gZoneBanner.empty()) return;
    if (!gText || !gText->ready) return;

    float t = gZoneBannerTimer;
    float fade;
    if (t > ZONE_BANNER_DURATION - 0.3f) {
        // fade-in al entrar
        fade = (ZONE_BANNER_DURATION - t) / 0.3f;
    } else if (t < 0.5f) {
        // fade-out al final
        fade = t / 0.5f;
    } else {
        fade = 1.0f;
    }
    fade = glm::clamp(fade, 0.0f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float cx = SCR_WIDTH * 0.5f;
    float baseY = SCR_HEIGHT * 0.72f;
    // Sombra detras del texto
    DrawQuad2D(0.0f, baseY - 24.0f, (float)SCR_WIDTH, 110.0f,
               glm::vec4(0.0f, 0.0f, 0.08f, 0.45f * fade));
    // Linea dorada
    DrawQuad2D(0.0f, baseY - 24.0f, (float)SCR_WIDTH, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, fade));
    DrawQuad2D(0.0f, baseY + 86.0f, (float)SCR_WIDTH, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, fade));

    DrawTextCentered(gZoneBanner, cx, baseY + 18.0f, 1.6f,
                     glm::vec3(0.95f, 0.85f, 0.40f) * fade);

    glEnable(GL_DEPTH_TEST);
}

// Devuelve la dificultad efectiva para mostrar instrucciones. Las zonas sin
// minijuego (HUB y reciclaje aun en construccion) heredan EASY para mostrar
// la guia completa al jugador.
static Difficulty CurrentHelpDifficulty() {
    switch (currentZone) {
    case 1: return gInvernadero.difficulty;
    case 2: return gReciclaje.difficulty;
    case 3: return gEnergia.difficulty;
    case 4: return gCapsulas.difficulty;
    default: return DIFF_EASY;
    }
}

// Texto de ayuda contextual segun zona + dificultad. A menor dificultad mas
// pistas; en EXPERT solo recordamos las teclas clave del minijuego.
static std::vector<std::string> BuildHelpLines() {
    std::vector<std::string> out;
    Difficulty d = CurrentHelpDifficulty();

    // Linea de modo: si el recorrido esta activo se ve siempre en cualquier
    // dificultad para que el jugador no se olvide del progreso. En modo
    // libre la omitimos para no quitar lugar a las pistas de cada zona.
    if (gGameMode == MODE_RECORRIDO) {
        char buf[140];
        snprintf(buf, sizeof(buf),
                 "RECORRIDO %d/5: %s  (N=siguiente modulo, ESC=pausar)",
                 gRecorridoStep + 1, kZoneNames[RECORRIDO_ORDER[gRecorridoStep]]);
        out.push_back(buf);
    }

    // Recordatorio prioritario: cuando estas dentro de un minijuego con
    // camara fija (OSU, Espectro, Simon) la unica forma rapida de soltar
    // la camara y volver a moverte es TAB. Siempre se muestra arriba del
    // bloque de pistas para que el jugador no se sienta "atrapado".
    bool tabExitsMinigame = gInvernadero.active || gEspectro.active || gEnergia.active;
    if (tabExitsMinigame) {
        out.push_back("Pulsa TAB para salir del minijuego");
    }

    if (d <= DIFF_EASY) {
        out.push_back("WASD para moverte  |  Shift correr  |  Espacio saltar");
        out.push_back("ESC abre el menu  |  TAB oculta el mapa");
        out.push_back("Camina a una puerta dorada para teletransportarte");
    }

    switch (currentZone) {
    case 0: // HUB
        if (d <= DIFF_EASY) {
            out.push_back("Las 4 puertas (N/S/E/O) llevan a cada zona");
            out.push_back("Atajos rapidos: 0=HUB 1=Invernadero 2=Reciclaje 3=Energia 4=Capsulas");
        }
        break;
    case 1: // INVERNADERO
        out.push_back("B = ritmo macetas (H J K L)  |  V = espectrografia meteorito (1 2 3 4)");
        if (d <= DIFF_MEDIUM) out.push_back("G alterna vista cenital  |  P/O hace crecer/resetea el helecho");
        if (d <= DIFF_EASY)   out.push_back("Espectro: 1 H-a (rojo)  2 H-b (cyan)  3 H-g (violeta)  4 Na (amarillo)");
        if (d <= DIFF_EASY)   out.push_back("Solo se usan 2 macetas en FACIL; MEDIO y EXPERTO usan las 4");
        break;
    case 2: // RECICLAJE
        out.push_back("O/L rota | U/J Y/H I/K mueven cajas | T/G escala | R reset");
        if (d <= DIFF_EASY) out.push_back("Cada caja tiene un control distinto; experimenta con cada una");
        break;
    case 3: // ENERGIA
        out.push_back("B = iniciar partida Simon  |  Z X C V = botones (en orden del patron)");
        if (d <= DIFF_EASY) out.push_back("Memoriza la secuencia de colores y repitela");
        break;
    case 4: // CAPSULAS
        out.push_back("B = nueva ronda  |  F = revisar la capsula mas cercana");
        if (d <= DIFF_EASY) out.push_back("Busca al intruso antes de que se acabe el tiempo");
        break;
    }

    // Recordatorio de dificultad
    const char* difLabel[3] = { "FACIL", "MEDIO", "EXPERTO" };
    char buf[96];
    snprintf(buf, sizeof(buf), "Dificultad: %s  (7/8/9 para cambiar)", difLabel[(int)d]);
    out.push_back(buf);
    return out;
}

void DrawInstructionsPanel() {
    if (!gText || !gText->ready) return;
    std::vector<std::string> lines = BuildHelpLines();
    if (lines.empty()) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float pad = 16.0f;
    float lineH = 22.0f;
    float panelH = lines.size() * lineH + 20.0f;
    float panelW = 500.0f;
    float x = pad;
    float y = pad; // esquina inferior izquierda

    DrawQuad2D(x, y, panelW, panelH, glm::vec4(0.05f, 0.10f, 0.18f, 0.78f));
    DrawQuad2D(x, y + panelH - 3.0f, panelW, 3.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));

    for (size_t i = 0; i < lines.size(); ++i) {
        float lineY = y + panelH - 20.0f - (float)i * lineH;
        gText->RenderText(lines[i], x + 14.0f, lineY, 0.40f,
                          glm::vec3(0.85f, 0.88f, 0.95f));
    }
    glEnable(GL_DEPTH_TEST);
}

// Panel inferior central con el mensaje de narrativa actual. Tiene fade-in y
// fade-out segun gNarrativeTimer para que se sienta como un dialogo guiado.
void DrawNarrativePanel() {
    if (gNarrativeQueue.empty()) return;
    if (!gText || !gText->ready) return;

    float t = gNarrativeTimer;
    float fade;
    if (t > NARRATIVE_DURATION - 0.4f) fade = (NARRATIVE_DURATION - t) / 0.4f;
    else if (t < 0.6f)                 fade = t / 0.6f;
    else                                fade = 1.0f;
    fade = glm::clamp(fade, 0.0f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const std::string& msg = gNarrativeQueue.front();
    float scale = 0.65f;
    float w = gText->MeasureWidth(msg, scale);
    float panelW = w + 70.0f;
    if (panelW > (float)SCR_WIDTH - 40.0f) panelW = (float)SCR_WIDTH - 40.0f;
    float panelH = 70.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f;
    float panelY = SCR_HEIGHT * 0.18f;

    DrawQuad2D(panelX, panelY, panelW, panelH,
               glm::vec4(0.03f, 0.06f, 0.10f, 0.82f * fade));
    DrawQuad2D(panelX, panelY + panelH - 3.0f, panelW, 3.0f,
               glm::vec4(0.35f, 0.85f, 1.0f, fade));
    DrawQuad2D(panelX, panelY, panelW, 3.0f,
               glm::vec4(0.35f, 0.85f, 1.0f, fade));

    DrawTextCentered(msg, (float)SCR_WIDTH * 0.5f, panelY + 22.0f, scale,
                     glm::vec3(0.92f, 0.95f, 1.0f) * fade);

    glEnable(GL_DEPTH_TEST);
}

void DrawHud() {
    if (!hudShader) return;
    // El HUD usa proyeccion ortografica que el callback de resize mantiene
    // sincronizada con el viewport, por eso aqui solo orquestamos quien
    // dibuja segun el estado del menu.
    if (menuState == MENU_CONTROLS) {
        DrawControlsOverlay();
        return;
    }
    if (menuState == MENU_PAUSE) {
        DrawPauseMenu();
        return;
    }

    // HUD en juego (menu cerrado)
    DrawMinimap();
    DrawInstructionsPanel();
    DrawZoneBanner();
    DrawNarrativePanel();
}

// ============ INPUT & CONTROLES ============
void processInput(GLFWwindow* window) {
    // --- ESC abre / cierra el menu (deteccion de flanco) ---
    static bool keyEscLast = false;
    bool escNow = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escNow && !keyEscLast) {
        if (menuState == MENU_NONE) {
            menuState = MENU_PAUSE;
            menuSelection = 0;
        } else if (menuState == MENU_PAUSE) {
            menuState = MENU_NONE;
        } else {
            menuState = MENU_PAUSE; // desde un sub-menu, vuelve a pausa
            menuSelection = 0;
        }
    }
    keyEscLast = escNow;

    // --- Si el menu esta abierto, solo navegacion del menu ---
    if (menuState != MENU_NONE) {
        static bool kUpLast = false, kDnLast = false, kEnterLast = false;
        static bool kMouseLast = false;
        bool up = (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) ||
                  (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
        bool dn = (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) ||
                  (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
        bool enter = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) ||
                     (glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) ||
                     (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

        if (menuState == MENU_PAUSE) {
            if (up && !kUpLast) menuSelection = (menuSelection + 4) % 5;
            if (dn && !kDnLast) menuSelection = (menuSelection + 1) % 5;

            // Hover y clic con el mouse sobre los 5 botones del menu de pausa.
            // Replicamos la misma geometria que dibuja DrawPauseMenu para que
            // las hitboxes coincidan exactamente con lo que ve el jugador.
            double mxd, myd;
            glfwGetCursorPos(window, &mxd, &myd);
            float mx = (float)mxd;
            // En GLFW (0,0) es esquina superior izquierda; la proyeccion del
            // HUD pone Y=0 en la parte inferior, asi que invertimos.
            float my = (float)SCR_HEIGHT - (float)myd;

            float panelW = 460.0f, panelH = 440.0f;
            float panelX = (SCR_WIDTH  - panelW) * 0.5f;
            float panelY = (SCR_HEIGHT - panelH) * 0.5f;
            float yBase = panelY + panelH - 180.0f;
            float lineH = 46.0f;
            int hover = -1;
            for (int i = 0; i < 5; ++i) {
                float y = yBase - i * lineH - 10.0f;
                float h = 38.0f;
                float x1 = panelX + 30.0f;
                float x2 = panelX + panelW - 30.0f;
                if (mx >= x1 && mx <= x2 && my >= y && my <= y + h) {
                    hover = i; break;
                }
            }
            if (hover >= 0) menuSelection = hover;

            bool mouseNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            bool clicked = (mouseNow && !kMouseLast && hover >= 0);
            kMouseLast = mouseNow;

            if ((enter && !kEnterLast) || clicked) {
                switch (menuSelection) {
                case 0: menuState = MENU_NONE; break;                   // Continuar
                case 1: // Toggle recorrido: iniciar / pausar / reanudar segun estado
                    if (gGameMode == MODE_RECORRIDO)       PauseRecorrido();
                    else if (gRecorridoStep == 0)          StartRecorrido();
                    else                                    ResumeRecorrido();
                    menuState = MENU_NONE; break;
                case 2: menuState = MENU_CONTROLS; break;               // Controles
                case 3: showMinimap = !showMinimap;                     // Mapa (toggle)
                        menuState = MENU_NONE; break;
                case 4: glfwSetWindowShouldClose(window, true); break;  // Salir
                }
            }
        } else if (menuState == MENU_CONTROLS) {
            // En el overlay de controles, cualquier clic vuelve al menu pausa
            bool mouseNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            if (mouseNow && !kMouseLast) menuState = MENU_PAUSE;
            kMouseLast = mouseNow;
        }
        kUpLast = up;
        kDnLast = dn;
        kEnterLast = enter;
        return; // congela el resto del input mientras el menu este abierto
    }

    // --- TAB: alterna el minimapa rapidamente sin pasar por el menu ---
    static bool keyTabLast = false;
    bool tabNow = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabNow && !keyTabLast) showMinimap = !showMinimap;
    keyTabLast = tabNow;

    // 1. Camara libre: se controla con las FLECHAS (antes WASD).
    if (cameraMode == CAM_FREE) {
        if (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS) camera.ProcessKeyboard(FORWARD,  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) camera.ProcessKeyboard(LEFT,     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT,    deltaTime);
    }

    // 2. Cambio de camaras: F1=1ra, F2=2da, F3=3ra, F8=libre
    bool f1 = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
    bool f2 = glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS;
    bool f3 = glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS;
    bool f8 = glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS;

    if (f1 && !keyF1Last) cameraMode = CAM_FIRST;
    if (f2 && !keyF2Last) cameraMode = CAM_SECOND;
    if (f3 && !keyF3Last) cameraMode = CAM_THIRD;
    if (f8 && !keyF8Last) cameraMode = CAM_FREE;

    keyF1Last = f1;
    keyF2Last = f2;
    keyF3Last = f3;
    keyF8Last = f8;

    // 3. Controles universales: Teletransporte a Zonas. Durante el minijuego
    // de espectrografia las teclas 1..4 sirven como teclas de color, asi que
    // dejamos solo el 0 (regreso al HUB) habilitado.
    int zoneKeys[5] = { GLFW_KEY_0,    GLFW_KEY_1,    GLFW_KEY_2,    GLFW_KEY_3,    GLFW_KEY_4 };
    int zoneKeysKP[5] = { GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4 };

    for (int i = 0; i < 5; ++i) {
        bool now = (glfwGetKey(window, zoneKeys[i]) == GLFW_PRESS) ||
            (glfwGetKey(window, zoneKeysKP[i]) == GLFW_PRESS);

        bool allowZone = !(gEspectro.active && i >= 1 && i <= 4);
        if (now && !keyZoneLast[i] && allowZone) ChangeZone(i);
        keyZoneLast[i] = now;
    }

    // Dificultad de minijuego de la zona actual: 7=Facil, 8=Medio, 9=Experto
    bool d7 = glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS;
    bool d8 = glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS;
    bool d9 = glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS;
    if (d7 && !keyDiff7Last) SetCurrentZoneDifficulty(DIFF_EASY);
    if (d8 && !keyDiff8Last) SetCurrentZoneDifficulty(DIFF_MEDIUM);
    if (d9 && !keyDiff9Last) SetCurrentZoneDifficulty(DIFF_EXPERT);
    keyDiff7Last = d7; keyDiff8Last = d8; keyDiff9Last = d9;

    // N = avanzar al siguiente modulo del RECORRIDO GUIADO (solo si esta
    // activo). En modo libre la N se reusa para el color cyan del cometa
    // dentro del invernadero, asi que aqui solo procesamos el flanco
    // global cuando el recorrido manda.
    static bool keyNGlobalLast = false;
    bool nGlobal = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    if (nGlobal && !keyNGlobalLast && gGameMode == MODE_RECORRIDO) {
        AdvanceRecorridoStep();
    }
    keyNGlobalLast = nGlobal;

    // ==========================================
    // CONTROLES ESTRICTOS POR ZONA
    // ==========================================

    // ZONA 1: INVERNADERO
    // Minijuego de ritmo (OSU) + controles del helecho y cometa
    if (currentZone == 1) {
        bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pPressed && !keyPLast) {
            if (fractalIteration < 7) {
                fractalIteration++;
                miPlantaFractal->Generate(fractalIteration);
                std::cout << "Iteracion de helecho: " << fractalIteration << std::endl;
            }
        }
        keyPLast = pPressed;

        bool oPressed = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
        if (oPressed && !keyOLast) {
            fractalIteration = 0;
            miPlantaFractal->Generate(fractalIteration);
            std::cout << "Helecho reseteado a iteracion 0" << std::endl;
        }
        keyOLast = oPressed;

        // Colores del cometa SOLO en invernadero (N=cyan, M=magenta, Y=yellow;
        // la tecla C se reserva al minijuego Simon en otra zona). Cuando el
        // RECORRIDO esta activo, N se reusa como "siguiente paso" arriba,
        // asi que aqui evitamos el doble disparo (N en recorrido no debe
        // tambien cambiar el color del cometa).
        static bool nLast = false;
        static bool mLast = false;
        static bool yLast = false;
        bool nKey = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
        bool mKey = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
        bool yKey = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
        if (nKey && !nLast && gGameMode != MODE_RECORRIDO) {
            moonTrailMode = 0; std::cout << "CYAN" << std::endl;
        }
        if (mKey && !mLast) { moonTrailMode = 1; std::cout << "MAGENTA" << std::endl; }
        if (yKey && !yLast) { moonTrailMode = 2; std::cout << "YELLOW"  << std::endl; }
        nLast = nKey;
        mLast = mKey;
        yLast = yKey;

        // --- Minijuego OSU ---
        // B = iniciar/reiniciar partida (activa vista cenital y bloqueo de
        //     camara automaticamente). TAB durante el juego = "interrumpir".
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            InitInvernaderoGame();
            gInvernadero.active = true;
            gInvernadero.topDown = true;
            if (!gMinigameCamLocked) {
                gCamModeBeforeLock = cameraMode;
                gMinigameCamLocked = true;
            }
            PushNarrative("INVERNADERO: pulsa H J K L cuando la nota llegue a su maceta.");
            std::cout << "[Invernadero] Nueva partida (dificultad " << gInvernadero.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        // TAB interrumpe el minijuego (sale de la vista cenital y suelta camara)
        if (gMinigameCamLocked && gInvernadero.active) {
            static bool tabIntLast = false;
            bool tabInt = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
            if (tabInt && !tabIntLast) {
                gInvernadero.active = false;
                gInvernadero.topDown = false;
                gMinigameCamLocked = false;
                cameraMode = gCamModeBeforeLock;
                PushNarrative("Minijuego de invernadero interrumpido.");
            }
            tabIntLast = tabInt;
        }

        bool gKey = glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS;
        if (gKey && !keyGLast) {
            gInvernadero.topDown = !gInvernadero.topDown;
        }
        keyGLast = gKey;

        // Carriles: H J K L (cuatro plantas, izquierda a derecha)
        bool hKey = glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS;
        bool jKey = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
        bool kKey = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
        bool lKey = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
        static bool hLast=false, jLast=false, kLast=false, lLast=false;
        if (hKey && !hLast) InvernaderoHit(0);
        if (jKey && !jLast) InvernaderoHit(1);
        if (kKey && !kLast) InvernaderoHit(2);
        if (lKey && !lLast) InvernaderoHit(3);
        hLast = hKey; jLast = jKey; kLast = kKey; lLast = lKey;

        // --- Minijuego ESPECTROGRAFIA ---
        // V = iniciar/reiniciar el desafio de espectro. La camara se bloquea
        //     siguiendo la trayectoria del meteorito y el jugador cambia el
        //     color con 1=H-alpha 2=H-beta 3=H-gamma 4=Na/Sun.
        // TAB = interrumpir.
        static bool vLast = false;
        bool vKey = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
        if (vKey && !vLast) {
            InitEspectroGame();
            gEspectro.active = true;
            // Apagamos el ritmo de plantas para no superponer dos vistas.
            gInvernadero.active = false;
            gInvernadero.topDown = false;
            if (!gMinigameCamLocked) {
                gCamModeBeforeLock = cameraMode;
                gMinigameCamLocked = true;
            }
            PushNarrative("ESPECTROGRAFIA: pulsa 1 (H-a) 2 (H-b) 3 (H-g) 4 (Na) al ritmo del meteorito.");
        }
        vLast = vKey;

        if (gEspectro.active) {
            // Cambio de color por teclas 1..4. Reusamos las teclas numericas
            // porque las de zona (1..4) tienen flanco propio y solo se
            // disparan cuando esta zona ya esta activa, no hay conflicto.
            static bool e1Last=false, e2Last=false, e3Last=false, e4Last=false;
            bool e1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
            bool e2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
            bool e3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
            bool e4 = glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS;
            if (e1 && !e1Last) EspectroSetColor(0);
            if (e2 && !e2Last) EspectroSetColor(1);
            if (e3 && !e3Last) EspectroSetColor(2);
            if (e4 && !e4Last) EspectroSetColor(3);
            e1Last = e1; e2Last = e2; e3Last = e3; e4Last = e4;

            // TAB interrumpe el espectro
            static bool tabIntEsp = false;
            bool tabInt = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
            if (tabInt && !tabIntEsp) {
                gEspectro.active = false;
                gMinigameCamLocked = false;
                cameraMode = gCamModeBeforeLock;
                PushNarrative("Minijuego de espectrografia interrumpido.");
            }
            tabIntEsp = tabInt;
        }
    }
    else {
        keyPLast = false;
        keyOLast = false;
    }

    // ZONA 2: RECICLAJE
    // - Seleccion de caja activa con TAB (cicla 0..4) o teclas 5..9 directas
    // - Caja seleccionada se mueve en X/Y/Z con IJKL y UM
    // - Rotacion con O/L (mientras esten libres no choquen con el OSU de invernadero)
    // - Reset con R
    // La logica de separar items (metal/vidrio/carton) se deja COMENTADA y lista.
    if (currentZone == 2) {
        float boxSpeed = 2.5f * deltaTime;

        // B = iniciar el modulo de reciclaje (activa items y narrativa)
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            InitReciclajeGame();
            gReciclaje.active = true;
            PushNarrative("RECICLAJE: ordena los desechos segun el tipo de material.");
            PushNarrative("Pulsa TAB para cambiar de caja e I J K L para moverla.");
            std::cout << "[Reciclaje] Nueva partida (dificultad " << gReciclaje.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        bool tabKey = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabKey && !keyTabLast) {
            gReciclaje.selectedBox = (gReciclaje.selectedBox + 1) % 5;
            std::cout << "[Reciclaje] Caja activa: " << gReciclaje.selectedBox << std::endl;
        }
        keyTabLast = tabKey;

        int s = gReciclaje.selectedBox;
        InteractiveBox& box = interactiveBoxes[s];

        // Movimiento en los 3 ejes para la caja seleccionada
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) box.position.z -= boxSpeed; // adelante
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) box.position.z += boxSpeed; // atras
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) box.position.x -= boxSpeed; // izquierda
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) box.position.x += boxSpeed; // derecha
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) box.position.y += boxSpeed; // sube
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) box.position.y -= boxSpeed; // baja
        if (box.position.y < 0.0f) box.position.y = 0.0f;
        if (box.position.y > 3.0f) box.position.y = 3.0f;

        // Rotacion y escala de la caja seleccionada
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) box.rotationY -= boxSpeed;
        if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) box.rotationY += boxSpeed;
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) box.scale -= 1.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) box.scale += 1.0f * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            for (int i = 0; i < 5; ++i) {
                interactiveBoxes[i].position = interactiveBoxes[i].startPos;
                interactiveBoxes[i].rotationY = 0.0f;
                interactiveBoxes[i].scale = 1.0f;
            }
        }

        // ---- INTERACCION SEPARAR BASURA (DEJADA LISTA, COMENTADA) ----
        // Idea: el usuario deberá tomar (raycast/clic) cada item y depositarlo en el
        // bote correspondiente. Por ahora dejamos el esqueleto preparado:
        //
        // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        //     // 1) Hacer ray-pick contra gReciclaje.items y los botes
        //     //    - si el item esta cerca del jugador -> gReciclaje.selectedItem = idx;
        //     //    - si ya hay item seleccionado y se clickea un bote ->
        //     //      MaterialType bin = gReciclaje.items[selectedItem].type
        //     //                         == bin  -> score++  (acierto)
        //     //                         != bin  -> score--  (fallo)
        //     //      gReciclaje.items[selectedItem].sorted = true;
        //     //      gReciclaje.selectedItem = -1;
        // }
    }

    // ZONA 3: ENERGIA - Minijuego Simon (memoria con botones de bateria)
    if (currentZone == 3) {
        // B = iniciar/reiniciar partida (bloquea la camara frente al reactor)
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            InitEnergiaGame();
            gEnergia.active = true;
            if (!gMinigameCamLocked) {
                gCamModeBeforeLock = cameraMode;
                gMinigameCamLocked = true;
            }
            PushNarrative("ENERGIA: memoriza el orden de las luces. Pulsa Z X C V o haz clic en el boton.");
            std::cout << "[Energia] Nueva partida (dificultad " << gEnergia.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        // TAB = interrumpir el minijuego
        if (gMinigameCamLocked && gEnergia.active) {
            static bool tabIntEn = false;
            bool tabInt = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
            if (tabInt && !tabIntEn) {
                gEnergia.active = false;
                gMinigameCamLocked = false;
                cameraMode = gCamModeBeforeLock;
                PushNarrative("Minijuego de energia interrumpido.");
            }
            tabIntEn = tabInt;
        }

        // Botones: Z X C V (las 4 baterias). Se mantiene para teclado.
        bool zKey = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        bool xKey = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
        bool cKey = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        bool vKey = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
        if (zKey && !keyZLast) EnergiaPress(0);
        if (xKey && !keyXLast) EnergiaPress(1);
        if (cKey && !keyCLast) EnergiaPress(2);
        if (vKey && !keyVLast) EnergiaPress(3);
        keyZLast = zKey; keyXLast = xKey; keyCLast = cKey; keyVLast = vKey;

        // CLIC DEL MOUSE: cada boton Simon se proyecta a pantalla y comparamos
        // con la posicion del cursor. Solo aceptamos clics si el juego esta
        // en fase 2 (turno del jugador).
        if (gEnergia.active) {
            static bool mouseLast = false;
            bool mouseNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            if (mouseNow && !mouseLast && gEnergia.phase == 2) {
                double mx, my;
                glfwGetCursorPos(window, &mx, &my);
                glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom),
                    (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
                glm::mat4 vw = ComputeView();
                int bestIdx = -1;
                float bestDist = 70.0f; // radio en px
                for (int i = 0; i < 4; ++i) {
                    glm::vec4 clip = proj * vw * glm::vec4(SIMON_BUTTON_POSITIONS[i], 1.0f);
                    if (clip.w <= 0.0f) continue;
                    glm::vec3 ndc = glm::vec3(clip) / clip.w;
                    float sx = (ndc.x * 0.5f + 0.5f) * SCR_WIDTH;
                    float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * SCR_HEIGHT;
                    float dx = sx - (float)mx;
                    float dy = sy - (float)my;
                    float d = sqrtf(dx * dx + dy * dy);
                    if (d < bestDist) { bestDist = d; bestIdx = i; }
                }
                if (bestIdx >= 0) EnergiaPress(bestIdx);
            }
            mouseLast = mouseNow;
        }
    }

    // ZONA 4: CAPSULAS - Buscar al intruso
    // E = revisar la capsula mas cercana al avatar
    // B = nueva partida con la dificultad actual
    if (currentZone == 4) {
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            InitCapsulasGame();
            gCapsulas.active = true;
            PushNarrative("CAPSULAS: pulsa F junto a una capsula para revisarla antes de que se acabe el tiempo.");
            std::cout << "[Capsulas] Nueva partida (dificultad " << gCapsulas.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        // F (interactuar/recoger) revisa la capsula mas cercana al jugador.
        // El nombre coincide con la convencion Genshin documentada en el HUD
        // y en el README.
        bool fKey = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        if (fKey && !keyELast) {
            CapsulasCheckNearest();
        }
        keyELast = fKey;
    }
}

void SetLightUniforms(Shader* shader) {
    shader->setInt("numLights", (int)gLights.size());
    for (size_t i = 0; i < gLights.size(); ++i) {
        std::ostringstream ss; ss << "allLights[" << i << "].";
        shader->setVec3((ss.str() + "Position").c_str(), gLights[i].Position);
        shader->setVec3((ss.str() + "Direction").c_str(), gLights[i].Direction);
        shader->setVec4((ss.str() + "Color").c_str(), gLights[i].Color);
        shader->setVec4((ss.str() + "Power").c_str(), gLights[i].Power);
        shader->setInt((ss.str() + "alphaIndex").c_str(), gLights[i].alphaIndex);
        float d = (gLights[i].distance > 0.0001f) ? gLights[i].distance : 1.0f;
        shader->setFloat((ss.str() + "distance").c_str(), d);
        shader->setInt((ss.str() + "type").c_str(), gLights[i].type);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    hudProjection = glm::ortho(0.0f, (float)width, 0.0f, (float)height);
    if (gText) gText->SetProjection(width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoffset = (float)xpos - lastX; float yoffset = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;
    if (menuState != MENU_NONE) return;
    if (gMinigameCamLocked) return; // camara fija durante minijuego
    if (cameraMode == CAM_FREE) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    } else {
        // En primera/segunda/tercera persona el mouse rota el yaw del jugador
        // y el pitch controla la inclinacion vertical (mirar arriba/abajo).
        const float SENS = 0.18f;
        player.yaw   -= xoffset * SENS;
        player.pitch += yoffset * SENS;
        if (player.pitch >  85.0f) player.pitch =  85.0f;
        if (player.pitch < -85.0f) player.pitch = -85.0f;
    }
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (cameraMode == CAM_FREE) camera.ProcessMouseScroll((float)yoffset);
}

// =============================================================================
// ============         IMPLEMENTACION DE LOS MINIJUEGOS            ============
// =============================================================================

// Cambia la dificultad de la zona actual pero NO inicia el minijuego: el
// jugador debe pulsar B para arrancar. Asi el ajuste 5/6/7 es solo un slider.
void SetCurrentZoneDifficulty(Difficulty d) {
    const char* difLbl[3] = { "FACIL", "MEDIO", "EXPERTO" };
    switch (currentZone) {
    case 1: gInvernadero.difficulty = d; break;
    case 2: gReciclaje.difficulty   = d; break;
    case 3: gEnergia.difficulty     = d; break;
    case 4: gCapsulas.difficulty    = d; break;
    default: return;
    }
    std::cout << "[Zona " << currentZone << "] Dificultad: " << difLbl[(int)d] << std::endl;
}

// ============ INVERNADERO (OSU) ============

void InitInvernaderoGame() {
    gInvernadero.notes.clear();
    gInvernadero.gameTime = 0.0f;
    gInvernadero.score = 0;
    gInvernadero.combo = 0;
    gInvernadero.maxCombo = 0;
    gInvernadero.hits = 0;
    gInvernadero.misses = 0;
    for (int i = 0; i < 4; i++) gInvernadero.waterEffect[i] = 0.0f;

    // Parametros segun dificultad. En FACIL usamos solo 2 macetas (carriles
    // 0 y 1), en MEDIO y EXPERTO los 4 carriles disponibles.
    int    totalNotes;
    float  spacing;          // separacion media entre notas (s)
    int    numLanes;
    switch (gInvernadero.difficulty) {
    case DIFF_EASY:   totalNotes = 16; spacing = 1.4f; numLanes = 2; break;
    case DIFF_MEDIUM: totalNotes = 30; spacing = 0.9f; numLanes = 4; break;
    case DIFF_EXPERT: totalNotes = 45; spacing = 0.55f; numLanes = 4; break;
    default:          totalNotes = 16; spacing = 1.4f; numLanes = 2; break;
    }
    gInvernadero.activeLanes = numLanes;

    float t = 2.5f; // primer nota tras 2.5s de margen
    int lastLane = -1;
    srand(12345); // patron reproducible
    for (int i = 0; i < totalNotes; i++) {
        OsuNote n;
        int lane;
        do { lane = rand() % numLanes; } while (lane == lastLane && (rand() % 3 != 0));
        n.lane = lane;
        n.hitTime = t;
        gInvernadero.notes.push_back(n);
        lastLane = lane;

        // pequeñas variaciones rítmicas
        float jitter = ((rand() % 100) / 100.0f - 0.5f) * spacing * 0.4f;
        t += spacing + jitter;
    }
}

void InvernaderoHit(int lane) {
    if (!gInvernadero.active) return;
    // Ventana de acierto por dificultad
    float window;
    switch (gInvernadero.difficulty) {
    case DIFF_EASY:   window = 0.30f; break;
    case DIFF_MEDIUM: window = 0.20f; break;
    case DIFF_EXPERT: window = 0.10f; break;
    default:          window = 0.30f; break;
    }

    // Buscar la nota mas cercana en ese carril aun no resuelta
    int bestIdx = -1;
    float bestDt = 9999.0f;
    for (size_t i = 0; i < gInvernadero.notes.size(); i++) {
        OsuNote& n = gInvernadero.notes[i];
        if (n.lane != lane || n.hit || n.missed) continue;
        float dt = fabsf(n.hitTime - gInvernadero.gameTime);
        if (dt < bestDt) { bestDt = dt; bestIdx = (int)i; }
    }

    gInvernadero.waterEffect[lane] = 1.0f;

    if (bestIdx >= 0 && bestDt <= window) {
        gInvernadero.notes[bestIdx].hit = true;
        gInvernadero.hits++;
        gInvernadero.combo++;
        if (gInvernadero.combo > gInvernadero.maxCombo) gInvernadero.maxCombo = gInvernadero.combo;
        int bonus = 100 + gInvernadero.combo * 5;
        gInvernadero.score += bonus;

        // Spawn de gotas de agua sobre la maceta acertada (animacion de riego)
        glm::vec3 base = INV_PLANT_POSITIONS[lane];
        for (int k = 0; k < 14; ++k) {
            WaterDrop d;
            float rx = ((float)rand() / RAND_MAX - 0.5f) * 0.7f;
            float rz = ((float)rand() / RAND_MAX - 0.5f) * 0.7f;
            d.pos = base + glm::vec3(rx, 2.5f + ((float)rand() / RAND_MAX) * 0.5f, rz);
            d.vy = -2.0f - ((float)rand() / RAND_MAX) * 1.5f;
            d.life = 1.4f;
            d.lane = lane;
            gInvernadero.drops.push_back(d);
        }

        // Feedback al jugador
        if (gInvernadero.combo % 5 == 0 && gInvernadero.combo > 0) {
            char buf[80];
            snprintf(buf, sizeof(buf), "Combo x%d! Sigue regando.", gInvernadero.combo);
            PushNarrative(buf);
        }
        std::cout << "[OSU] HIT  carril " << lane << "  combo=" << gInvernadero.combo
                  << "  score=" << gInvernadero.score << std::endl;
    }
    else {
        gInvernadero.combo = 0;
        gInvernadero.misses++;
        PushNarrative("Fallaste el ritmo. Atinale a la maceta cuando llegue la nota.");
        std::cout << "[OSU] MISS carril " << lane << std::endl;
    }
}

void UpdateInvernaderoGame(float dt) {
    gInvernadero.gameTime += dt;
    for (int i = 0; i < 4; i++) {
        gInvernadero.waterEffect[i] -= dt * 2.0f;
        if (gInvernadero.waterEffect[i] < 0.0f) gInvernadero.waterEffect[i] = 0.0f;
    }
    // Integrar gotas de agua que cayeron al regar
    for (auto& d : gInvernadero.drops) {
        d.vy -= 9.0f * dt;
        d.pos.y += d.vy * dt;
        d.life -= dt;
        if (d.pos.y < 0.1f) { d.pos.y = 0.1f; d.life = std::min(d.life, 0.05f); }
    }
    gInvernadero.drops.erase(
        std::remove_if(gInvernadero.drops.begin(), gInvernadero.drops.end(),
                       [](const WaterDrop& d) { return d.life <= 0.0f; }),
        gInvernadero.drops.end());
    // Marcar como missed cualquier nota cuyo tiempo ya paso fuera de ventana
    float graceMiss = 0.35f;
    bool allDone = true;
    for (auto& n : gInvernadero.notes) {
        if (!n.hit && !n.missed) {
            if (gInvernadero.gameTime - n.hitTime > graceMiss) {
                n.missed = true;
                gInvernadero.combo = 0;
                gInvernadero.misses++;
                std::cout << "[OSU] MISS auto carril " << n.lane << std::endl;
            }
            else {
                allDone = false;
            }
        }
    }
    if (allDone && !gInvernadero.notes.empty()) {
        static bool reported = false;
        if (!reported) {
            std::cout << "[OSU] FIN. Score=" << gInvernadero.score
                      << "  Aciertos=" << gInvernadero.hits
                      << "  Fallos="   << gInvernadero.misses
                      << "  MaxCombo=" << gInvernadero.maxCombo << std::endl;
            reported = true;
        }
    }
}

void DrawInvernaderoGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!CAJA_MODELO) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    // Colores por carril: las dos macetas de la fila IZQ comparten tonos
    // verdes, las dos de la fila DER comparten tonos azules para que se
    // distingan visualmente las dos "filas de ritmo".
    glm::vec4 laneColors[4] = {
        glm::vec4(0.55f, 0.95f, 0.40f, 1.0f), // izq atras  verde claro
        glm::vec4(0.20f, 0.80f, 0.30f, 1.0f), // izq adelante verde oscuro
        glm::vec4(0.55f, 0.80f, 1.00f, 1.0f), // der atras  celeste
        glm::vec4(0.20f, 0.55f, 1.00f, 1.0f)  // der adelante azul
    };

    int lanes = gInvernadero.activeLanes;
    if (lanes < 2) lanes = 2;
    if (lanes > 4) lanes = 4;

    // 1) Marcador plano sobre cada maceta ACTIVA para indicar donde caera la
    //    nota. Usa PLANO_MODELO (mas natural que un cubo aplanado); si por
    //    algun motivo no carga, hace fallback a CAJA_MODELO escalado fino.
    //    Se respetan las macetas originales del modelo INVERNADERO_OPACO;
    //    el marcador apenas roza el piso (y = 0.02) y "florece" cuando
    //    recibimos un acierto (waterEffect).
    Model* floorMarker = PLANO_MODELO ? PLANO_MODELO : CAJA_MODELO;
    bool floorIsPlano  = (PLANO_MODELO != nullptr);
    for (int i = 0; i < lanes; i++) {
        glm::vec3 base = INV_PLANT_POSITIONS[i];
        base.y = 0.02f;
        float pulse = 0.55f + 0.15f * sinf(gInvernadero.gameTime * 3.0f + i);
        float waterBoost = gInvernadero.waterEffect[i];
        glm::vec4 c = laneColors[i] * (pulse + waterBoost * 1.5f);
        SetMaterial(phongShader, c * 0.4f, c, glm::vec4(0.4f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, base);
        // Para el plano usamos escala uniforme en XZ (ya esta horizontal en
        // el FBX). Para el cubo de fallback aplanamos el eje Z.
        if (floorIsPlano) {
            float s = 0.85f + waterBoost * 0.5f;
            m = glm::scale(m, glm::vec3(s, 1.0f, s));
        } else {
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            m = glm::scale(m, glm::vec3(0.85f + waterBoost * 0.5f,
                                        0.85f + waterBoost * 0.5f,
                                        0.05f));
        }
        phongShader->setMat4("model", m);
        floorMarker->Draw(*phongShader);
    }

    // 1b) Capa de transparencia de color encima de las macetas activas: un
    //     cilindro semi-translucido que envuelve la maceta original sin
    //     ocultarla. Asi el jugador identifica facil cuales son los carriles.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < lanes; i++) {
        glm::vec3 base = INV_PLANT_POSITIONS[i];
        base.y = 0.4f;
        float pulse = 0.50f + 0.25f * sinf(gInvernadero.gameTime * 1.8f + i);
        glm::vec4 c = laneColors[i];
        c.a = 0.35f + 0.15f * pulse + gInvernadero.waterEffect[i] * 0.30f;
        SetMaterial(phongShader, c * 0.5f, c, glm::vec4(0.2f), 16);
        phongShader->setFloat("transparency", c.a);

        glm::mat4 m(1.0f);
        m = glm::translate(m, base);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(1.1f, 1.1f, 0.9f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }
    phongShader->setFloat("transparency", 1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // 2) Notas: se acercan horizontalmente desde el muro exterior de su
    //    fila. Aparecen lookAhead segundos antes y desaparecen 0.4 s
    //    despues del hitTime (ventana de fallo).
    float lookAhead = 2.0f;
    float scrollDist = 5.5f; // unidades que recorre la nota antes de llegar
    for (auto& n : gInvernadero.notes) {
        if (n.hit || n.missed) continue;
        if (n.lane >= lanes) continue;
        float tToHit = n.hitTime - gInvernadero.gameTime;
        if (tToHit > lookAhead) continue;
        if (tToHit < -0.4f) continue;

        glm::vec3 target = INV_PLANT_POSITIONS[n.lane];
        glm::vec3 start  = target;
        start.x -= INV_SCROLL_DIR[n.lane] * scrollDist; // viene desde el muro
        float a = glm::clamp(1.0f - (tToHit / lookAhead), 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(start, target, a);
        pos.y = 0.35f;

        glm::vec4 c = laneColors[n.lane];
        SetMaterial(phongShader, c * 0.3f, c, glm::vec4(0.5f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.28f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }

    // 3) Gotas de agua: pequenios cubos translucidos azules que caen hacia la
    //    maceta justo despues de acertar. Da la sensacion de regar.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (const WaterDrop& d : gInvernadero.drops) {
        glm::vec4 c(0.45f, 0.75f, 1.0f, glm::clamp(d.life, 0.0f, 1.0f));
        SetMaterial(phongShader, c * 0.4f, c, glm::vec4(0.9f, 0.9f, 1.0f, 1.0f), 64);
        phongShader->setFloat("transparency", c.a);

        glm::mat4 m(1.0f);
        m = glm::translate(m, d.pos);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.12f, 0.12f, 0.18f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }
    phongShader->setFloat("transparency", 1.0f);
    glDisable(GL_BLEND);
}

// ============ ENERGIA (Simon) ============

void InitEnergiaGame() {
    gEnergia.pattern.clear();
    gEnergia.playerIndex = 0;
    gEnergia.phase = 1;
    gEnergia.phaseTimer = 0.0f;
    gEnergia.showIndex = -1;
    gEnergia.score = 0;
    for (int i = 0; i < 4; i++) gEnergia.buttonGlow[i] = 0.0f;

    int startLen;
    switch (gEnergia.difficulty) {
    case DIFF_EASY:   startLen = 3; gEnergia.level = 1; break;
    case DIFF_MEDIUM: startLen = 4; gEnergia.level = 2; break;
    case DIFF_EXPERT: startLen = 6; gEnergia.level = 4; break;
    default:          startLen = 3; gEnergia.level = 1; break;
    }
    srand((unsigned)(glfwGetTime() * 1000.0));
    for (int i = 0; i < startLen; i++) gEnergia.pattern.push_back(rand() % 4);
}

void EnergiaPress(int button) {
    if (!gEnergia.active) return;
    if (gEnergia.phase != 2) return; // solo se aceptan inputs en fase de jugador
    gEnergia.buttonGlow[button] = 1.0f;

    int expected = gEnergia.pattern[gEnergia.playerIndex];
    if (button == expected) {
        gEnergia.playerIndex++;
        gEnergia.score += 50;
        std::cout << "[Simon] OK boton " << button << "  (" << gEnergia.playerIndex
                  << "/" << gEnergia.pattern.size() << ")" << std::endl;
        if (gEnergia.playerIndex >= (int)gEnergia.pattern.size()) {
            gEnergia.level++;
            gEnergia.pattern.push_back(rand() % 4);
            gEnergia.playerIndex = 0;
            gEnergia.phase = 1;            // volver a mostrar patron
            gEnergia.phaseTimer = 0.6f;     // pausa antes de mostrar
            gEnergia.showIndex = -1;
            char buf[80];
            snprintf(buf, sizeof(buf), "Patron correcto! Nivel %d. Memoriza el nuevo orden.", gEnergia.level);
            PushNarrative(buf);
            std::cout << "[Simon] NIVEL " << gEnergia.level
                      << "  Score=" << gEnergia.score << std::endl;
        } else {
            PushNarrative("Vas bien, sigue la secuencia.");
        }
    }
    else {
        char buf[100];
        snprintf(buf, sizeof(buf), "Fallaste. Esperaba el boton %d. Score final %d.",
                 expected, gEnergia.score);
        PushNarrative(buf);
        std::cout << "[Simon] FALLO. Esperaba " << expected
                  << " y se presiono " << button
                  << ".  Score final=" << gEnergia.score << std::endl;
        gEnergia.phase = 3;
        gEnergia.phaseTimer = 1.5f;
    }
}

void UpdateEnergiaGame(float dt) {
    for (int i = 0; i < 4; i++) {
        gEnergia.buttonGlow[i] -= dt * 2.5f;
        if (gEnergia.buttonGlow[i] < 0.0f) gEnergia.buttonGlow[i] = 0.0f;
    }

    if (gEnergia.phase == 1) {
        // Fase de mostrar patron: ilumina uno por uno
        // velocidad de showing segun dificultad
        float onTime, offTime;
        switch (gEnergia.difficulty) {
        case DIFF_EASY:   onTime = 0.55f; offTime = 0.30f; break;
        case DIFF_MEDIUM: onTime = 0.40f; offTime = 0.20f; break;
        case DIFF_EXPERT: onTime = 0.25f; offTime = 0.12f; break;
        default:          onTime = 0.55f; offTime = 0.30f; break;
        }

        gEnergia.phaseTimer -= dt;
        if (gEnergia.phaseTimer <= 0.0f) {
            gEnergia.showIndex++;
            if (gEnergia.showIndex >= (int)gEnergia.pattern.size()) {
                // termino de mostrar -> turno del jugador
                gEnergia.phase = 2;
                gEnergia.showIndex = -1;
                gEnergia.playerIndex = 0;
                std::cout << "[Simon] Repite el patron (Z X C V)" << std::endl;
            }
            else {
                int idx = gEnergia.pattern[gEnergia.showIndex];
                gEnergia.buttonGlow[idx] = 1.0f;
                gEnergia.phaseTimer = onTime + offTime;
            }
        }
    }
    else if (gEnergia.phase == 3) {
        gEnergia.phaseTimer -= dt;
        if (gEnergia.phaseTimer <= 0.0f) {
            // Reiniciar partida tras fallo
            InitEnergiaGame();
        }
    }
}

void DrawEnergiaGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!CAJA_MODELO) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    // Las pilas reales del modelo ya tienen botones; encima dibujamos un cubo
    // de color para que el jugador identifique claramente los 4 botones del
    // Simon y pueda hacer clic con el mouse. Cada cubo brilla con buttonGlow.
    glm::vec4 simonColors[4] = {
        glm::vec4(1.0f, 0.25f, 0.25f, 1.0f), // rojo
        glm::vec4(0.25f, 1.0f, 0.30f, 1.0f), // verde
        glm::vec4(0.30f, 0.55f, 1.0f, 1.0f), // azul
        glm::vec4(1.0f, 0.95f, 0.25f, 1.0f)  // amarillo
    };

    float t = (float)glfwGetTime();
    // Cubos de los botones del Simon: ahora usamos el primitivo CUBO_MODELO
    // (mas limpio que CAJA_MODELO, que viene tinteada del FBX). Fallback al
    // anterior si CUBO_MODELO no esta presente para no romper el render.
    Model* buttonModel = CUBO_MODELO ? CUBO_MODELO : CAJA_MODELO;
    for (int i = 0; i < 4; ++i) {
        float glow = gEnergia.buttonGlow[i];
        float pulse = 0.55f + 0.20f * sinf(t * 2.5f + i);
        glm::vec4 c = simonColors[i] * (pulse + glow * 1.8f);
        SetMaterial(phongShader, c * 0.4f, c, glm::vec4(0.7f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, SIMON_BUTTON_POSITIONS[i]);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.55f + glow * 0.20f));
        phongShader->setMat4("model", m);
        buttonModel->Draw(*phongShader);
    }
}

// ============ RECICLAJE ============

void InitReciclajeGame() {
    gReciclaje.items.clear();
    gReciclaje.score = 0;
    gReciclaje.selectedItem = -1;
    gReciclaje.selectedBin = -1;

    int total;
    switch (gReciclaje.difficulty) {
    case DIFF_EASY:   total = 6;  break;
    case DIFF_MEDIUM: total = 9;  break;
    case DIFF_EXPERT: total = 12; break;
    default:          total = 6;  break;
    }

    srand(54321);
    for (int i = 0; i < total; i++) {
        RecycleItem it;
        // Posiciones dispersas en el piso de reciclaje (X 7..10, Z -3..3)
        float fx = 7.5f + (rand() % 100) / 100.0f * 2.5f;
        float fz = -3.0f + (rand() % 100) / 100.0f * 6.0f;
        it.position = glm::vec3(fx, 0.3f, fz);
        it.type = (MaterialType)(rand() % 3);
        it.sorted = false;
        gReciclaje.items.push_back(it);
    }
    std::cout << "[Reciclaje] " << total << " items generados" << std::endl;
}

void DrawReciclajeGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!CAJA_MODELO) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    glm::vec4 matColors[3] = {
        glm::vec4(0.6f, 0.6f, 0.7f, 1.0f),  // metal: gris azulado
        glm::vec4(0.3f, 0.8f, 0.85f,1.0f),  // vidrio: turquesa
        glm::vec4(0.75f,0.55f,0.30f,1.0f)   // carton: ocre
    };

    // Items en el piso: cubos de colores por tipo de material. Usamos
    // CUBO_MODELO porque queremos un cubo geometrico simple (no tinta del FBX).
    Model* itemModel = CUBO_MODELO ? CUBO_MODELO : CAJA_MODELO;
    for (auto& it : gReciclaje.items) {
        if (it.sorted) continue;
        glm::vec4 c = matColors[(int)it.type];
        SetMaterial(phongShader, c * 0.3f, c, glm::vec4(0.4f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, it.position);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.28f));
        phongShader->setMat4("model", m);
        itemModel->Draw(*phongShader);
    }

    // Botes (3): metal, vidrio, carton
    for (int b = 0; b < 3; b++) {
        glm::vec4 c = matColors[b] * 1.2f;
        SetMaterial(phongShader, c * 0.35f, c, glm::vec4(0.5f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, RECYCLE_BIN_POSITIONS[b]);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.8f, 0.8f, 1.4f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }

    // Marcador visual sobre la caja interactiva seleccionada
    if (gReciclaje.selectedBox >= 0 && gReciclaje.selectedBox < 5) {
        glm::vec4 c(1.0f, 1.0f, 0.2f, 1.0f);
        SetMaterial(phongShader, c * 0.4f, c, glm::vec4(0.8f), 32);
        glm::mat4 m(1.0f);
        glm::vec3 p = interactiveBoxes[gReciclaje.selectedBox].position;
        p.y += 1.6f * interactiveBoxes[gReciclaje.selectedBox].scale;
        float pulse = 0.85f + 0.15f * sinf((float)glfwGetTime() * 6.0f);
        m = glm::translate(m, p);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.18f * pulse));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }
}

// ============ CAPSULAS (Buscar al intruso) ============

void InitCapsulasGame() {
    switch (gCapsulas.difficulty) {
    case DIFF_EASY:
        gCapsulas.totalCapsules = 4;
        gCapsulas.timeLeft = 60.0f;
        gCapsulas.moveInterval = 0.0f;       // intruso fijo
        break;
    case DIFF_MEDIUM:
        gCapsulas.totalCapsules = 6;
        gCapsulas.timeLeft = 45.0f;
        gCapsulas.moveInterval = 20.0f;
        break;
    case DIFF_EXPERT:
        gCapsulas.totalCapsules = 8;
        gCapsulas.timeLeft = 30.0f;
        gCapsulas.moveInterval = 10.0f;
        break;
    default:
        gCapsulas.totalCapsules = 4;
        gCapsulas.timeLeft = 60.0f;
        gCapsulas.moveInterval = 0.0f;
        break;
    }
    if (gCapsulas.totalCapsules > 8) gCapsulas.totalCapsules = 8;
    srand((unsigned)(glfwGetTime() * 1000.0) + 7);
    gCapsulas.intruderIdx = rand() % gCapsulas.totalCapsules;
    gCapsulas.moveTimer = gCapsulas.moveInterval;
    gCapsulas.lastChecked = -1;
    gCapsulas.lastCheckedTimer = 0.0f;
    gCapsulas.win = false;
    gCapsulas.lose = false;
    std::cout << "[Capsulas] " << gCapsulas.totalCapsules << " capsulas, "
              << gCapsulas.timeLeft << "s, intervalo de movimiento="
              << gCapsulas.moveInterval << "s" << std::endl;
}

void CapsulasCheckNearest() {
    if (!gCapsulas.active) return;
    if (gCapsulas.win || gCapsulas.lose) return;

    // Cápsula más cercana al avatar dentro de un radio de "asomarse"
    int best = -1;
    float bestDist = 2.0f; // metros
    for (int i = 0; i < gCapsulas.totalCapsules; i++) {
        glm::vec3 d = CAPSULA_SLOTS[i] - player.position;
        d.y = 0.0f;
        float dist = glm::length(d);
        if (dist < bestDist) { bestDist = dist; best = i; }
    }

    if (best < 0) {
        PushNarrative("Acercate a una capsula para revisarla.");
        std::cout << "[Capsulas] Acercate a una capsula para revisarla." << std::endl;
        return;
    }

    gCapsulas.lastChecked = best;
    gCapsulas.lastCheckedTimer = 0.6f;

    if (best == gCapsulas.intruderIdx) {
        gCapsulas.win = true;
        char buf[80];
        snprintf(buf, sizeof(buf), "Intruso atrapado en la capsula %d! Te sobraron %.1fs.",
                 best, gCapsulas.timeLeft);
        PushNarrative(buf);
        std::cout << "[Capsulas] INTRUSO ATRAPADO en capsula " << best
                  << "  Tiempo restante=" << gCapsulas.timeLeft << "s" << std::endl;
    }
    else {
        char buf[80];
        snprintf(buf, sizeof(buf), "Capsula %d vacia. Sigue buscando.", best);
        PushNarrative(buf);
        std::cout << "[Capsulas] Capsula " << best << " vacia." << std::endl;
    }
}

void UpdateCapsulasGame(float dt) {
    if (gCapsulas.win || gCapsulas.lose) return;

    gCapsulas.timeLeft -= dt;
    if (gCapsulas.lastCheckedTimer > 0.0f) gCapsulas.lastCheckedTimer -= dt;

    if (gCapsulas.timeLeft <= 0.0f) {
        gCapsulas.timeLeft = 0.0f;
        gCapsulas.lose = true;
        std::cout << "[Capsulas] TIEMPO AGOTADO. El intruso estaba en capsula "
                  << gCapsulas.intruderIdx << "." << std::endl;
        return;
    }

    if (gCapsulas.moveInterval > 0.0f) {
        gCapsulas.moveTimer -= dt;
        if (gCapsulas.moveTimer <= 0.0f) {
            int prev = gCapsulas.intruderIdx;
            do {
                gCapsulas.intruderIdx = rand() % gCapsulas.totalCapsules;
            } while (gCapsulas.intruderIdx == prev && gCapsulas.totalCapsules > 1);
            gCapsulas.moveTimer = gCapsulas.moveInterval;
            std::cout << "[Capsulas] El intruso se movio..." << std::endl;
        }
    }
}

void DrawCapsulasGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!CAJA_MODELO) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    float t = (float)glfwGetTime();

    for (int i = 0; i < gCapsulas.totalCapsules; i++) {
        // Color base: cyan suave para todas las capsulas (interrogante neutro)
        glm::vec4 c(0.40f, 0.85f, 1.0f, 1.0f);

        // Si recien la revisamos, flashear verde (acierto) o rojo (vacia)
        if (gCapsulas.lastChecked == i && gCapsulas.lastCheckedTimer > 0.0f) {
            float k = glm::clamp(gCapsulas.lastCheckedTimer / 0.6f, 0.0f, 1.0f);
            if (i == gCapsulas.intruderIdx) {
                c = glm::mix(c, glm::vec4(0.1f, 1.0f, 0.2f, 1.0f), k);
            }
            else {
                c = glm::mix(c, glm::vec4(1.0f, 0.2f, 0.2f, 1.0f), k);
            }
        }

        // Si la partida termino, revelar al intruso parpadeando
        if ((gCapsulas.win || gCapsulas.lose) && i == gCapsulas.intruderIdx) {
            float pulse = 0.5f + 0.5f * sinf(t * 6.0f);
            c = glm::mix(c, glm::vec4(1.0f, 0.7f, 0.1f, 1.0f), pulse);
        }

        // Resaltar la capsula a la que se esta acercando el avatar
        glm::vec3 toCaps = CAPSULA_SLOTS[i] - player.position; toCaps.y = 0.0f;
        bool isNear = glm::length(toCaps) < 2.0f;
        float nearBoost = isNear ? 0.45f : 0.0f;

        float pulseAll = 0.85f + 0.15f * sinf(t * 2.0f + i);
        glm::vec4 finalC = c * (pulseAll + nearBoost);
        SetMaterial(phongShader, finalC * 0.35f, finalC, glm::vec4(0.5f), 32);

        // Cubo flotante de senalizacion encima de la cápsula
        glm::mat4 m(1.0f);
        m = glm::translate(m, CAPSULA_SLOTS[i] + glm::vec3(0.0f, 0.8f + 0.15f*sinf(t*1.5f + i), 0.0f));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, t * 0.8f + i, glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, glm::vec3(0.30f + nearBoost * 0.10f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }
}

// ============ ESPECTROGRAFIA DEL METEORITO ============
// El minijuego "espectro" se desarrolla en la zona invernadero a la par del
// rytmo de plantas: sirve para que el jugador identifique las cuatro lineas
// espectrales mas comunes en astronomia. ESPECTRO_COLORS/NAMES estan
// declarados arriba para que SpawnMoonTrail pueda usarlos.

void InitEspectroGame() {
    gEspectro.pattern.clear();
    gEspectro.patternIdx = 0;
    gEspectro.currentColor = 0;
    gEspectro.score = 0;
    gEspectro.hits = 0;
    gEspectro.misses = 0;
    gEspectro.combo = 0;
    gEspectro.maxCombo = 0;
    gEspectro.lastFeedbackTimer = 0.0f;

    int totalBeats;
    switch (gEspectro.difficulty) {
    case DIFF_EASY:   totalBeats = 10; gEspectro.beatInterval = 2.0f; break;
    case DIFF_MEDIUM: totalBeats = 15; gEspectro.beatInterval = 1.3f; break;
    case DIFF_EXPERT: totalBeats = 22; gEspectro.beatInterval = 0.85f; break;
    default:          totalBeats = 10; gEspectro.beatInterval = 2.0f; break;
    }
    gEspectro.beatTimer = gEspectro.beatInterval + 1.0f; // pequeno preview

    srand(98765);
    int last = -1;
    for (int i = 0; i < totalBeats; ++i) {
        int v;
        do { v = rand() % 4; } while (v == last);
        gEspectro.pattern.push_back(v);
        last = v;
    }
}

void EspectroSetColor(int idx) {
    if (!gEspectro.active) return;
    if (idx < 0 || idx > 3) return;
    gEspectro.currentColor = idx;
}

void UpdateEspectroGame(float dt) {
    if (gEspectro.lastFeedbackTimer > 0.0f) gEspectro.lastFeedbackTimer -= dt;
    if (gEspectro.patternIdx >= (int)gEspectro.pattern.size()) return;

    gEspectro.beatTimer -= dt;
    if (gEspectro.beatTimer > 0.0f) return;

    int target = gEspectro.pattern[gEspectro.patternIdx];
    bool hit = (gEspectro.currentColor == target);
    gEspectro.lastFeedbackTimer = 0.8f;
    gEspectro.lastHit = hit;
    if (hit) {
        gEspectro.hits++;
        gEspectro.combo++;
        if (gEspectro.combo > gEspectro.maxCombo) gEspectro.maxCombo = gEspectro.combo;
        gEspectro.score += 100 + gEspectro.combo * 10;
        if (gEspectro.combo % 4 == 0) {
            char buf[100];
            snprintf(buf, sizeof(buf), "Espectro alineado x%d! Score %d.",
                     gEspectro.combo, gEspectro.score);
            PushNarrative(buf);
        }
    } else {
        gEspectro.combo = 0;
        gEspectro.misses++;
        char buf[120];
        snprintf(buf, sizeof(buf), "Linea esperada: %s. Pulsaste %s.",
                 ESPECTRO_NAMES[target], ESPECTRO_NAMES[gEspectro.currentColor]);
        PushNarrative(buf);
    }
    gEspectro.patternIdx++;
    gEspectro.beatTimer = gEspectro.beatInterval;

    if (gEspectro.patternIdx >= (int)gEspectro.pattern.size()) {
        char buf[120];
        snprintf(buf, sizeof(buf), "Espectro completo. Aciertos %d/%d. Score %d.",
                 gEspectro.hits, (int)gEspectro.pattern.size(), gEspectro.score);
        PushNarrative(buf);
    }
}

// Posicion del meteorito en el mundo en el instante t. La orbita es la misma
// que usa la zona invernadero al renderizar MOON2 (radio 15 a 11 m de altura
// sobre y=11, plano XZ rotado -90 grados en torno al eje X). Devolvemos la
// posicion ya transformada al espacio mundo para que la camara la siga.
glm::vec3 EspectroMeteorPos(float t) {
    float mt = t * 0.3f;
    float radius = 15.0f;
    glm::mat4 moonMat(1.0f);
    moonMat = glm::translate(moonMat, glm::vec3(0.0f, 11.0f, -12.5f));
    moonMat = glm::rotate(moonMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    moonMat = glm::scale(moonMat, glm::vec3(0.5f));
    glm::vec3 local(radius * cosf(mt), radius * sinf(mt), 0.0f);
    return glm::vec3(moonMat * glm::vec4(local, 1.0f));
}

// Chase cam del meteorito: muestreamos la orbita un instante antes para
// estimar la tangente y posicionar la camara detras del cometa, con una
// pequena elevacion sobre el plano orbital. El target se proyecta un poco
// adelante para que el jugador vea hacia donde se dirige el meteoro y
// pueda anticipar el beat del minijuego de espectrografia.
void EspectroChaseCam(glm::vec3& eye, glm::vec3& target) {
    float t = (float)glfwGetTime();
    glm::vec3 meteor = EspectroMeteorPos(t);
    glm::vec3 prev   = EspectroMeteorPos(t - 0.10f);
    glm::vec3 fwd    = meteor - prev;
    if (glm::length(fwd) > 1e-4f) fwd = glm::normalize(fwd);
    else                          fwd = glm::vec3(0.0f, 0.0f, -1.0f);
    // 4 m atras y 1.6 m arriba: distancia suficiente para que los 4 halos
    // espectrales (radio 1.8 m alrededor del meteoro) entren en cuadro,
    // y el target adelante da sensacion de avance.
    eye    = meteor - fwd * 4.0f + glm::vec3(0.0f, 1.6f, 0.0f);
    target = meteor + fwd * 1.5f;
}

void DrawEspectroGame(const glm::mat4& projection, const glm::mat4& view) {
    // El meteorito se dibuja en case 1 (INVERNADERO) usando proceduralShader.
    // Aqui solo dibujamos los 4 indicadores espectrales orbitando junto al
    // meteorito como "objetivos" para que el jugador identifique el target.
    if (!CAJA_MODELO) return;
    float t = (float)glfwGetTime();
    glm::vec3 mpos = EspectroMeteorPos(t);

    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    // El siguiente target del patron determina el "halo" que se muestra
    int targetIdx = (gEspectro.patternIdx < (int)gEspectro.pattern.size())
                    ? gEspectro.pattern[gEspectro.patternIdx] : -1;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (int i = 0; i < 4; ++i) {
        float ang = t * 0.8f + i * glm::radians(90.0f);
        glm::vec3 pos = mpos + glm::vec3(cosf(ang) * 1.8f, sinf(ang) * 0.6f,
                                          sinf(ang) * 1.8f);
        glm::vec4 c = ESPECTRO_COLORS[i];
        float boost = (i == targetIdx) ? 2.0f : 0.6f;
        c.a = (i == targetIdx) ? 0.95f : 0.45f;
        SetMaterial(phongShader, c * 0.5f, c * boost, glm::vec4(0.8f), 16);
        phongShader->setFloat("transparency", c.a);

        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, ang, glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, glm::vec3(0.45f + (i == targetIdx ? 0.20f : 0.0f)));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }
    phongShader->setFloat("transparency", 1.0f);
    glDisable(GL_BLEND);
}