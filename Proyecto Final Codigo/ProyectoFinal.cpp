/*
* PROYECTO INDIVIDUAL: Base Espacial Sustentable
* Alumna: Rojas Chimal Karla Beatriz
*
*/

#include <iostream>
#include <stdlib.h>
#include <ctime>
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
void UpdateDoorAnim(float dt);
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
void DrawEspectroHUD();
void EspectroSetColor(int spectralIdx);
static void EspectroStopBar();
static void EspectroNextRound();
static float EspectroLinePos(int idx);
// Forward de meteorito + chase cam (usado por ComputeView).
glm::vec3 EspectroMeteorPos(float t);
void EspectroChaseCam(glm::vec3& eye, glm::vec3& target);

void InitEnergiaGame();
void UpdateEnergiaGame(float dt);
void DrawEnergiaGame(const glm::mat4& projection, const glm::mat4& view);
void DrawEnergiaHUD();
void EnergiaPress(int button);

void InitReciclajeGame();
void UpdateReciclajeGame(float dt);
void DrawReciclajeGame(const glm::mat4& projection, const glm::mat4& view);
void DrawReciclajeHUD();
void ReciclajeAction();

// Hamburguesa (zona CAPSULAS): armado por capas con teclas T Y U I O P.
void InitHamburguesaGame();
void UpdateHamburguesaGame(float dt);
void DrawHamburguesaGame(const glm::mat4& projection, const glm::mat4& view);
void DrawHamburguesaHUD();
void HamburguesaPress(int ingredient);

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
// Tamano de la ventana. NO es const para que framebuffer_size_callback los
// actualice en cada resize / paso a pantalla completa: el HUD lee estos
// valores cada frame, asi el minimapa, las barras de estatus, el panel de
// instrucciones, el manual, etc. se reposicionan solos sin codigo extra.
unsigned int SCR_WIDTH  = 1024;
unsigned int SCR_HEIGHT = 768;

Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
int currentZone = 0;
// Arrancamos en tercera persona para que WASD mueva al personaje desde el
// primer frame (CAM_FREE bloqueaba el movimiento del avatar hasta F1/F2/F3).
// Arrancamos en primera persona para que el jugador entre inmediatamente en
// la perspectiva del astronauta. F1/F2/F3 alternan entre 1ra/2da/3ra y F8
// libera la camara orbital.
CameraMode cameraMode = CAM_FIRST;

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

enum MenuState { MENU_NONE = 0, MENU_PAUSE = 1, MENU_CONTROLS = 2, MENU_MAP = 3, MENU_QUESTS = 4 };
MenuState menuState = MENU_NONE;
int menuSelection = 0;
bool showMinimap = true;

// ============ VIDA DEL ASTRONAUTA: barras de estatus + flags ============
// 5 barras (agua/comida/oxigeno/energia/tareas) + flags de minijuegos
// completados. gStats.tareas se recalcula en DrawQuestsWidget como
// doneCount/N_QUESTS, asi que la barra siempre cuadra con el manual.
struct PlayerStats {
    float agua    = 0.20f;
    float comida  = 0.20f;
    float oxigeno = 1.00f;
    float energia = 0.20f;
    float tareas  = 0.20f;

    // Flags de minijuegos completados con exito (para palomear el manual y
    // para gatear acciones encadenadas como "preparar comida").
    bool plantsWatered    = false;  // OSU del invernadero
    bool cometDone        = false;  // espectro del cometa
    bool recycleDone      = false;  // clasificar todos los items
    bool simonDone        = false;  // memoria del reactor
    bool foodPrepared     = false;  // preparar comida en capsulas
    bool plantPlanted     = false;  // helecho fractal crecido en invernadero
};
PlayerStats gStats;

// Nota: la version anterior de la zona CAPSULAS solo tenia un timer de
// cocinado de 3 s (gCookingActive / gCookingTimer / COOKING_DURATION).
// Lo sustituimos por el minijuego de armado de hamburguesa por capas
// (struct HamburguesaGame, mas abajo), asi que esas globales ya no son
// necesarias.

// ALT libera el mouse (lo descaptura) sin abrir ningun menu. Util para
// mover la ventana, alt-tabear o hacer una captura de pantalla. Cuando esta
// liberado mouse_callback no rota la camara y el cursor se ve.
bool gCursorReleased = false;

// Panel de instrucciones: por defecto se muestra "compacto" (solo 2-3 lineas
// basicas) y la tecla H lo expande con la lista completa.
bool gHelpExpanded = false;

// Input direccional del jugador en el frame actual (WASD). Lo lee el HUD
// para dibujar el joystick virtual que refleja por donde se esta moviendo.
// Componentes en [-1,1]: x = lateral (A negativo / D positivo aunque A/D
// rotan, lo usamos como giro visual), y = forward (S negativo / W positivo).
glm::vec2 gMoveInput = glm::vec2(0.0f);
bool      gIsSprinting = false;

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
const float NARRATIVE_DURATION = 2.6f;
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

// Ingredientes de la hamburguesa (zona CAPSULAS). Cada uno es un fbx
// independiente que se apila visualmente sobre la mesa de practicas cuando
// el jugador acierta la tecla correspondiente en el minijuego.
Model* HAMB_PLATO        = nullptr;
Model* HAMB_PAN_INFERIOR = nullptr;
Model* HAMB_CARNE        = nullptr;
Model* HAMB_QUESO        = nullptr;
Model* HAMB_JITOMATE     = nullptr;
Model* HAMB_LECHUGA      = nullptr;
Model* HAMB_PAN_SUPERIOR = nullptr;

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

// --- Animacion de apertura ---
// Cuando el jugador entra a una puerta, la puerta sube doorOpenAmount metros
// y, al llegar al tope, dispara el teleport. Asi se ve la accion "se abre y
// el personaje pasa" en vez de un teletransporte instantaneo. Una sola
// puerta puede estar abriendose a la vez (gDoorAnimating).
float doorOpenAmount[8] = { 0.0f };          // 0=cerrada, 1=arriba (en unidades de DOOR_SLIDE_HEIGHT)
int   gDoorAnimating    = -1;                // idx de la puerta en proceso, -1=ninguna
const float DOOR_OPEN_SPEED   = 1.6f;        // 1/seg: tarda ~0.6 s en abrir
const float DOOR_SLIDE_HEIGHT = 3.2f;        // metros que sube en world space

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
    glm::vec3 vel;     // velocidad 3D (gravedad solo en y)
    float life;        // segundos restantes hasta desaparecer
    float maxLife;     // vida inicial, para fade
    int lane;
    bool landed = false;   // true cuando toco la tierra de la maceta
    bool splash = false;   // true si nacio como salpicadura al impactar
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
    // Metricas del minijuego (mismo set que OSU/Espectro/Reciclaje para que
    // el HUD pueda mostrarlas de forma consistente con los otros modulos).
    int hits = 0;        // botones presionados correctamente en TODA la partida
    int misses = 0;      // fallos (cada fallo es 1 miss y reinicia patron)
    int combo = 0;       // racha de aciertos seguidos sin fallar
    int maxCombo = 0;    // mejor racha de la partida actual
    int bestLevel = 1;   // mejor nivel alcanzado (sobrevive a fallos hasta cerrar)
    // Hasta 6 botones (PRINCIPIANTE usa 4, MEDIO y EXPERTO usan 6). Reservamos
    // 6 slots siempre; SimonActiveButtons() decide cuantos se usan en runtime.
    float buttonGlow[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
};

// --- Reciclaje ---
// Minijuego "Compactador": hay varias cajas (modelo CAJA_INTERACTIVA)
// dispersas por el piso de la sala. El jugador las recoge con E (la
// mas cercana) y las apila en un cuadro delimitado del suelo. Si suelta
// fuera del cuadro, la caja queda en el piso para volver a recogerla.
// Objetivo: dejar TODAS las cajas apiladas dentro del cuadro (sin
// "desorden") antes de que se acabe el reloj (en MEDIO/EXPERTO).
enum MaterialType { MAT_METAL = 0, MAT_VIDRIO = 1, MAT_CARTON = 2, MAT_COUNT = 3 };

struct ReciclajeBox {
    glm::vec3 pos;          // posicion en el mundo (cambia si la sueltan)
    int  color    = 0;      // 0..2, puramente visual
    bool placed   = false;  // ya esta en la torre
    int  stackIdx = -1;     // si placed, posicion dentro de la torre
};

struct ReciclajeGame {
    Difficulty difficulty = DIFF_EASY;
    bool active           = false;
    std::vector<ReciclajeBox> boxes;
    int  placedCount      = 0;
    int  carrying         = -1;         // indice de caja cargada, -1 = ninguna
    int  score            = 0;
    int  drops            = 0;          // soltadas fuera del cuadro
    int  bestStack        = 0;
    float layerAnim       = 0.0f;       // animacion de la ultima caja apilada
    float feedbackTimer   = 0.0f;
    int   feedbackKind    = 0;          // 0 ninguno, 1 ok, 2 fuera de zona, 3 sin caja cerca
    float timeLimit       = 0.0f;       // 0 = sin reloj
    float timeLeft        = 0.0f;
    bool  finished        = false;
    bool  failedByTime    = false;
};

// ============ Espectrografia (segundo minijuego del invernadero) ============
// Barra del espectro visible con 4 lineas de Balmer del hidrogeno. SPACE
// detiene el indicador; cae dentro del sweet spot -> hit.
struct EspectroGame {
    Difficulty difficulty = DIFF_EASY;
    bool active = false;
    int currentColor = 0;            // color (linea espectral) seleccionada

    // Estado del indicador y del sweet spot
    float barPos       = 0.0f;        // posicion del indicador en 0..1
    float barSpeed     = 0.55f;       // velocidad del barrido (1/s)
    int   barDir       = +1;          // direccion del ping-pong (+1 / -1)
    float sweetCenter  = 0.5f;        // centro del sweet spot en 0..1
    float sweetHalf    = 0.06f;       // semiancho del sweet spot
    int   targetLine   = 0;           // linea espectral objetivo (0..3)
    bool  stopped      = false;       // indicador detenido tras pulsar SPACE
    float feedbackTimer = 0.0f;       // tiempo de pausa con resultado en pantalla
    bool  lastHit      = false;       // el ultimo intento entro al sweet spot

    int rounds   = 0;                 // total de rondas a jugar
    int roundIdx = 0;                 // ronda actual (0-based)
    int hits     = 0;
    int misses   = 0;
    int score    = 0;
    int combo    = 0;
    int maxCombo = 0;
};

InvernaderoGame gInvernadero;
SimonGame gEnergia;
ReciclajeGame gReciclaje;
EspectroGame gEspectro;

// --- Hamburguesa (Zona CAPSULAS, mesa de practicas) ---
// Minijuego de armado por capas: la receta marca el orden bottom-up
// (PAN INFERIOR -> CARNE -> QUESO -> JITOMATE -> LECHUGA -> PAN SUPERIOR),
// el plato esta siempre sobre la mesa. Cada acierto apila el ingrediente
// real (modelo fbx) y suma score; un error rompe el combo pero deja la
// torre intacta. Al colocar la ultima capa la tarea "preparar comida"
// se da por completa y se llenan las barras de agua/comida.
enum HamburgIng {
    HAMB_ING_PAN_INFERIOR = 0,
    HAMB_ING_CARNE        = 1,
    HAMB_ING_QUESO        = 2,
    HAMB_ING_JITOMATE     = 3,
    HAMB_ING_LECHUGA      = 4,
    HAMB_ING_PAN_SUPERIOR = 5,
    HAMB_ING_COUNT        = 6
};

struct HamburguesaGame {
    Difficulty difficulty = DIFF_EASY;
    bool active           = false;
    int  step             = 0;       // 0..HAMB_ING_COUNT (cuando llega a 6 termino)
    int  score            = 0;
    int  hits             = 0;
    int  misses           = 0;
    int  combo            = 0;
    int  maxCombo         = 0;
    int  bestStep         = 0;       // mejor numero de capas en la partida actual
    float layerAnim[HAMB_ING_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }; // 0..1 al apilar
    float feedbackTimer   = 0.0f;    // tiempo de flash rojo tras error
    bool  lastWrong       = false;   // ultimo intento fallido
    int   lastWrongIng    = -1;
    float timeLimit       = 0.0f;    // segundos (0 = sin reloj)
    float timeLeft        = 0.0f;    // cuenta regresiva en MEDIO/EXPERTO
    bool  finished        = false;   // todas las capas colocadas
    bool  failedByTime    = false;
};

HamburguesaGame gHamburguesa;

// Datos por ingrediente del burger. ALTURA local de cada capa (en
// unidades de mundo) y la TECLA asociada. La altura se suma en
// cascada para apilar las capas sobre el plato. Las teclas se eligen
// fuera del rango de zonas (0..4) y dificultad (7,8,9), y procurando
// no chocar con los demas minijuegos de otras zonas.
const char* HAMB_NAMES[HAMB_ING_COUNT] = {
    "PAN INFERIOR", "CARNE", "QUESO", "JITOMATE", "LECHUGA", "PAN SUPERIOR"
};
const char* HAMB_KEY_HINT[HAMB_ING_COUNT] = {
    "T", "Y", "U", "I", "O", "P"
};
const int HAMB_KEYS[HAMB_ING_COUNT] = {
    GLFW_KEY_T, GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I, GLFW_KEY_O, GLFW_KEY_P
};
// Altura visual aproximada por capa (mundo). Cada capa se apila sumando
// esta cantidad sobre la anterior. Valores conservadores para que las
// capas no se traslapen ni floten demasiado lejos entre si.
const float HAMB_LAYER_H[HAMB_ING_COUNT] = {
    0.08f,  // pan inferior
    0.06f,  // carne
    0.025f, // queso
    0.04f,  // jitomate
    0.05f,  // lechuga
    0.10f   // pan superior
};
// Color del chip / icono en el HUD por ingrediente (estilo Simon).
const glm::vec4 HAMB_CHIP_COLORS[HAMB_ING_COUNT] = {
    glm::vec4(0.88f, 0.68f, 0.32f, 1.0f), // pan inferior - dorado
    glm::vec4(0.62f, 0.26f, 0.18f, 1.0f), // carne - cafe rojizo
    glm::vec4(0.98f, 0.82f, 0.20f, 1.0f), // queso - amarillo
    glm::vec4(0.92f, 0.22f, 0.20f, 1.0f), // jitomate - rojo
    glm::vec4(0.32f, 0.78f, 0.30f, 1.0f), // lechuga - verde
    glm::vec4(0.94f, 0.78f, 0.40f, 1.0f)  // pan superior - dorado claro
};

// Tapa de la mesa de practicas (zona CAPSULAS). Las capas se apilan aqui.
const glm::vec3 HAMB_TABLE_TOP = glm::vec3(-14.5f, 0.73f, 0.0f);
const float    HAMB_PLATE_LIFT = 0.005f; // evita z-fight con el mantel
// Escala de los FBX del burger. 0.15 -> hamburguesa de ~15 cm.
const float    HAMB_MODEL_SCALE = 0.15f;

// Lineas de Balmer del hidrogeno (objetivos del espectro).
const glm::vec4 ESPECTRO_COLORS[4] = {
    glm::vec4(1.00f, 0.10f, 0.18f, 1.0f), // H-alpha  (656 nm) rojo
    glm::vec4(0.15f, 0.95f, 1.00f, 1.0f), // H-beta   (486 nm) cyan
    glm::vec4(0.55f, 0.20f, 1.00f, 1.0f), // H-gamma  (434 nm) violeta
    glm::vec4(0.35f, 0.10f, 1.00f, 1.0f)  // H-delta  (410 nm) violeta profundo
};
const char* ESPECTRO_NAMES[4] = { "H-alpha", "H-beta", "H-gamma", "H-delta" };
// Longitudes de onda (nm). La barra del espectro va de 400 a 700 nm.
const float ESPECTRO_NM[4] = { 656.3f, 486.1f, 434.0f, 410.2f };

// Macetas reales del .obj del invernadero (filas F1 y F2, maceteros 2/3).
// Notas del OSU scrollean horizontalmente desde el muro hacia la maceta.
const glm::vec3 INV_PLANT_POSITIONS[4] = {
    glm::vec3(-2.189f, -1.0f, -13.6f), // 0 = F1_2 (izq, maceta 2)
    glm::vec3(-2.189f, -1.0f, -15.85f), // 1 = F1_3 (izq, maceta 3)
    glm::vec3( 2.200f, -1.0f, -13.6f), // 2 = F2_2 (der, maceta 2)
    glm::vec3( 2.200f, -1.0f, -15.85f)  // 3 = F2_3 (der, maceta 3)
};
// FACIL usa los carriles 0 y 1 directamente (F1_2 y F1_3), asi se queda
// pegado a la misma fila de macetas reales sin necesidad de un arreglo
// alterno.
static glm::vec3 InvLanePos(int lane) {
    return INV_PLANT_POSITIONS[lane];
}
// +1 si la nota viaja hacia +x, -1 si hacia -x. Es decir, las notas de la
// fila izquierda (x negativo) vienen de la pared izquierda y van hacia el
// centro (+x), y viceversa.
const float INV_SCROLL_DIR[4] = { +1.0f, +1.0f, -1.0f, -1.0f };

// ============ Layout de botones Simon (zona ENERGIA) ============
// Botones reales de la pila R_1 leidos del .obj. x=5.076 = cara visible.
// Modo PRINCIPIANTE: 4 botones (2x2). Modo MEDIO/EXPERTO: 6 (2x3).
// Z shifted 6 cm a -Z para que el cubo quede mas a la izquierda en cam.
static const glm::vec3 SIMON_POS_4[4] = {
    glm::vec3(5.076f, 1.50f, 12.84f),  // 0 ROJO     - top-right (R_1_20)
    glm::vec3(5.076f, 1.50f, 12.54f),  // 1 VERDE    - top-left  (R_1_21)
    glm::vec3(5.076f, 1.10f, 12.84f),  // 2 AZUL     - bot-right (R_1_10)
    glm::vec3(5.076f, 1.10f, 12.54f)   // 3 AMARILLO - bot-left  (R_1_11)
};
static const glm::vec3 SIMON_POS_6[6] = {
    glm::vec3(5.076f, 1.50f, 12.84f),  // 0 ROJO     - top-right
    glm::vec3(5.076f, 1.50f, 12.54f),  // 1 VERDE    - top-mid
    glm::vec3(5.076f, 1.50f, 12.24f),  // 2 AZUL     - top-left
    glm::vec3(5.076f, 1.10f, 12.84f),  // 3 AMARILLO - bot-right
    glm::vec3(5.076f, 1.10f, 12.54f),  // 4 NARANJA  - bot-mid
    glm::vec3(5.076f, 1.10f, 12.24f)   // 5 MAGENTA  - bot-left
};

// Separacion en X de la membrana respecto a la cara del boton. Negativo
// = hacia el jugador (entre la camara y el boton). 4 cm para que el
// plano flote claramente DELANTE del cubo sin clipear con el modelo.
const float SIMON_MEMBRANE_OFFSET = -0.040f;

// Hasta 6 colores: los primeros 4 son el Simon clasico (rojo / verde /
// azul / amarillo). Los dos extra (naranja vibrante y magenta brillante)
// se eligen para que sean distinguibles de los anteriores incluso con la
// iluminacion calida del cuarto de energia.
static const glm::vec4 SIMON_COLORS[6] = {
    glm::vec4(1.00f, 0.20f, 0.22f, 1.0f), // 0 ROJO
    glm::vec4(0.20f, 1.00f, 0.30f, 1.0f), // 1 VERDE
    glm::vec4(0.25f, 0.55f, 1.00f, 1.0f), // 2 AZUL
    glm::vec4(1.00f, 0.92f, 0.20f, 1.0f), // 3 AMARILLO
    glm::vec4(1.00f, 0.48f, 0.05f, 1.0f), // 4 NARANJA
    glm::vec4(0.95f, 0.30f, 0.95f, 1.0f)  // 5 MAGENTA
};
static const char* SIMON_NAMES[6]    = { "ROJO", "VERDE", "AZUL", "AMARILLO", "NARANJA", "MAGENTA" };
static const char* SIMON_KEY_HINT[6] = { "Z", "X", "C", "V", "N", "M" };

// Cuantos botones estan en uso ahora (depende de la dificultad). 4 en
// PRINCIPIANTE, 6 en MEDIO/EXPERTO. Centralizado aqui para que el draw,
// el HUD, el click y la generacion del patron pidan siempre la misma
// cuenta y se sincronicen sin desfases entre frames.
static inline int SimonActiveButtons() {
    return (gEnergia.difficulty == DIFF_EASY) ? 4 : 6;
}
static inline glm::vec3 SimonButtonPos(int i) {
    int n = SimonActiveButtons();
    if (i < 0 || i >= n) return glm::vec3(0.0f);
    return (n == 4) ? SIMON_POS_4[i] : SIMON_POS_6[i];
}
// Centro del cluster (para enfocar la camara). Con 4 botones cae entre
// las columnas 12.6/12.9; con 6 cae sobre la columna central 12.6.
static inline glm::vec3 SimonClusterCenter() {
    return (SimonActiveButtons() == 4)
        ? glm::vec3(5.076f, 1.30f, 12.75f)
        : glm::vec3(5.076f, 1.30f, 12.60f);
}

const glm::vec3 RECYCLE_BIN_POSITIONS[3] = {
    glm::vec3(11.5f, 0.5f, -2.5f),  // METAL
    glm::vec3(11.5f, 0.5f,  0.0f),  // VIDRIO
    glm::vec3(11.5f, 0.5f,  2.5f)   // CARTON
};

// ============ Configuracion del minijuego de RECICLAJE ============
// Cuadro delimitado para apilar (centro y semianchos). El jugador debe
// estar dentro de este rectangulo para soltar la caja con E y apilarla.
// Fuera del cuadro, soltar deja la caja en el piso (lo que se considera
// "desorden" y suma a la metrica drops).
const glm::vec3 RECICLAJE_STACK_CENTER = glm::vec3(8.5f, 0.30f, 0.0f);
const float     RECICLAJE_STACK_HALF_X = 0.70f;  // semiancho X del cuadro
const float     RECICLAJE_STACK_HALF_Z = 0.70f;  // semiancho Z del cuadro
const float     RECICLAJE_PICKUP_RANGE = 1.4f;   // alcance para recoger caja
const float     RECICLAJE_LAYER_H      = 0.34f;  // altura por capa (m)
const float     RECICLAJE_BOX_SCALE    = 0.18f;  // escala visual

const char* RECICLAJE_NAMES[3] = { "METAL", "VIDRIO", "CARTON" };
// Color por tipo: solo visual (no afecta el orden de apilado).
const glm::vec4 RECICLAJE_COLORS[3] = {
    glm::vec4(0.32f, 0.55f, 0.95f, 1.0f), // METAL  - azul
    glm::vec4(0.30f, 0.85f, 0.55f, 1.0f), // VIDRIO - verde
    glm::vec4(0.78f, 0.55f, 0.30f, 1.0f)  // CARTON - cafe
};

bool keyZLast = false, keyXLast = false, keyCLast = false, keyVLast = false;
bool keyGLast = false, keyBLast = false;
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
    // Clavadas exactamente sobre la pared interna de cada modulo: el centro
    // del marco esta en el limite del cuarto, no dentro. Limites de las
    // habitaciones: INV boundsMax.y=-5, REC boundsMin.x=6, ENE boundsMin.y=6.5,
    // CAP boundsMax.x=-6.5. El hitbox de radio 0.9 sigue activando el
    // teleport: la pared frena al jugador a ~0.4 m del centro, bien dentro
    // del radio del trigger.
    zoneDoors[4] = { glm::vec3( 0.0f, 0.0f, -5.00f), 0.9f, 1, 0 }; // INVERNADERO -> HUB
    zoneDoors[5] = { glm::vec3( 6.00f, 0.0f, 0.0f),  0.9f, 2, 0 }; // RECICLAJE -> HUB
    zoneDoors[6] = { glm::vec3( 0.0f, 0.0f,  6.50f), 0.9f, 3, 0 }; // ENERGIA -> HUB
    zoneDoors[7] = { glm::vec3(-6.50f, 0.0f, 0.0f),  0.9f, 4, 0 }; // CAPSULAS -> HUB
}

static bool PlayerInsideDoor(const ZoneDoor& d) {
    float dx = player.position.x - d.center.x;
    float dz = player.position.z - d.center.z;
    return dx * dx + dz * dz <= d.radius * d.radius;
}

void CheckDoorTriggers() {
    // Si ya hay una puerta abriendose, no permitimos disparar otra hasta
    // que termine la animacion (evita dobles teleports si el jugador se
    // mueve rapido entre hitboxes).
    if (gDoorAnimating >= 0) return;

    for (int i = 0; i < 8; ++i) {
        const ZoneDoor& d = zoneDoors[i];
        if (d.fromZone != currentZone) {
            doorPlayerInside[i] = false;
            continue;
        }
        bool now = PlayerInsideDoor(d);
        if (now && !doorPlayerInside[i]) {
            doorPlayerInside[i] = true;
            // En vez de teletransportar al instante, arrancamos la animacion
            // de la puerta. Al terminar, UpdateDoorAnim hace ChangeZone.
            gDoorAnimating = i;
            doorOpenAmount[i] = 0.0f;
            return;
        }
        doorPlayerInside[i] = now;
    }
}

void UpdateDoorAnim(float dt) {
    if (gDoorAnimating < 0) return;
    int i = gDoorAnimating;
    doorOpenAmount[i] += dt * DOOR_OPEN_SPEED;
    if (doorOpenAmount[i] >= 1.0f) {
        doorOpenAmount[i] = 1.0f;
        int toZone = zoneDoors[i].toZone;
        gDoorAnimating = -1;
        // Las demas puertas (incluida esta, pero en la zona destino) deben
        // arrancar cerradas; ChangeZone resetea su estado de "jugador dentro".
        for (int j = 0; j < 8; ++j) doorOpenAmount[j] = 0.0f;
        ChangeZone(toZone);
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

    // Muebles/props: circulo (jugador) vs AABB en XZ. Si la base de la caja
    // queda por encima de la cabeza (~1.8 m) el jugador puede pasar por debajo.
    // Tambien se ignora cualquier caja que tape el hueco de una puerta de la
    // zona actual (los muros vienen partidos en pedazos alrededor del marco).
    const float PLAYER_HEIGHT = 1.8f;
    const float DOOR_PAD = 0.3f;
    for (const AABB& b : autoColliders[zone]) {
        if (b.min.y > PLAYER_HEIGHT) continue;

        bool overlapsDoor = false;
        for (int i = 0; i < 8; ++i) {
            const ZoneDoor& d = zoneDoors[i];
            if (d.fromZone != zone) continue;
            float dx = std::max(b.min.x - DOOR_PAD, std::min(d.center.x, b.max.x + DOOR_PAD)) - d.center.x;
            float dz = std::max(b.min.z - DOOR_PAD, std::min(d.center.z, b.max.z + DOOR_PAD)) - d.center.z;
            if (dx * dx + dz * dz < d.radius * d.radius) { overlapsDoor = true; break; }
        }
        if (overlapsDoor) continue;

        float cx = std::max(b.min.x, std::min(pos.x, b.max.x));
        float cz = std::max(b.min.z, std::min(pos.z, b.max.z));
        float dx = pos.x - cx;
        float dz = pos.z - cz;
        if (dx * dx + dz * dz < radius * radius) return true;
    }

    return false;
}

// ============ Stop de todos los minijuegos en curso ============
// Llamado antes de arrancar uno nuevo (B/V) para que no se encimen
// vistas / HUDs / camaras bloqueadas. No resetea sus stats, solo
// apaga el flag .active y suelta la camara fija.
static void StopActiveMinigames() {
    gInvernadero.active = false;
    gInvernadero.topDown = false;
    gEspectro.active    = false;
    gEnergia.active     = false;
    gReciclaje.active   = false;
    gHamburguesa.active = false;
    gMinigameCamLocked  = false;
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
    gEspectro.active = false;
    // El minijuego de la hamburguesa tambien se cierra al cambiar de
    // zona: antes el flag se quedaba activo y el HUD seguia visible
    // sobre el HUB / otras salas aunque la partida estuviera lista.
    gHamburguesa.active = false;
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
        PushNarrative("HUB CENTRAL: cuatro puertas luminosas conectan los modulos de la estacion.");
        PushNarrative("Atraviesa una puerta o pulsa 1-4 para teletransportarte.");
        break;
    case 1:
        PushNarrative("INVERNADERO: aqui crecen los cultivos de la estacion.");
        PushNarrative("1) Plantar: pulsa P para crecer el helecho.");
        PushNarrative("2) Regar: pulsa B y atina al ritmo con H J K L.");
        PushNarrative("3) Cometa: pulsa V para iniciar la espectrografia.");
        break;
    case 2:
        PushNarrative("RECICLAJE: hay cajas tiradas por toda la sala. Hay que apilarlas en el cuadro dorado.");
        PushNarrative("Tarea: pulsa B para iniciar. Camina a una caja y E para recogerla; vuelve al cuadro y E para soltarla.");
        break;
    case 3:
        PushNarrative("ENERGIA: el reactor necesita un patron de encendido.");
        PushNarrative("Tarea: pulsa los botones de las pilas en el orden indicado. Pulsa B para empezar.");
        break;
    case 4:
        PushNarrative("CAPSULAS: Lugar donde la tripulacion duerme.");
        PushNarrative("Tarea: Haz una hamburguesa. Pulsa B para empezar.");
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

    // Ingredientes del minijuego de hamburguesa (zona CAPSULAS).
    HAMB_PLATO        = CargarModelo("models/hamburguesa_plato.fbx");
    HAMB_PAN_INFERIOR = CargarModelo("models/hamburguesa_paninferior.fbx");
    HAMB_CARNE        = CargarModelo("models/hamburguesa_carne.fbx");
    HAMB_QUESO        = CargarModelo("models/hamburguesa_queso.fbx");
    HAMB_JITOMATE     = CargarModelo("models/hamburguesa_jitomate.fbx");
    HAMB_LECHUGA      = CargarModelo("models/hamburguesa_lechuga.fbx");
    HAMB_PAN_SUPERIOR = CargarModelo("models/hamburguesa_pansuperior.fbx");

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

    // [5..10] Botones Simon de la zona ENERGIA. Reservamos 6 slots (los 4
    // del modo PRINCIPIANTE + los 2 extra de MEDIO/EXPERTO). En cada frame
    // la zona ENERGIA actualiza posicion y power segun SimonActiveButtons();
    // los slots que sobran (4 y 5 en PRINCIPIANTE) se quedan apagados.
    for (int i = 0; i < 6; ++i) {
        Light sb;
        sb.type = 0;
        sb.Position = SIMON_POS_6[i]; // placeholder, se actualiza en Update
        sb.Color = SIMON_COLORS[i];
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
        // Cam top-down sobre las macetas. center/eye.z = -12.1 (acumulado
        // +2 m en +Z para subir el contenido 2 m en pantalla). eye.y = 9
        // (camara 1 m mas cerca del suelo para acercar la vista).
        glm::vec3 center(0.0f, 0.0f, -14.10f);
        glm::vec3 eye(0.0f, 8.5f, -14.09f);
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
        // Vista fija frente a la pila R_1 (rincon +X / +Z). La camara se
        // pone a la altura del cluster de botones y se separa sobre -X
        // para que los botones queden centrados y bien grandes en pantalla.
        // En layout 4 (2x2) la separacion es 2.4 m; en layout 6 (2x3) la
        // pila se ensancha 0.3 m mas en Z, asi que retrocedemos a 2.8 m
        // para que los 6 sigan cabiendo comodamente en el frame.
        glm::vec3 center = SimonClusterCenter();
        float backDist = (SimonActiveButtons() == 4) ? 2.4f : 2.8f;
        glm::vec3 eye    = center + glm::vec3(-backDist, 0.05f, 0.0f);
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
        return glm::vec3(0.0f, 13.0f, -12.09f);
    }
    if (currentZone == 1 && gEspectro.active && gMinigameCamLocked) {
        glm::vec3 eye, target;
        EspectroChaseCam(eye, target);
        return eye;
    }
    if (currentZone == 3 && gEnergia.active && gMinigameCamLocked) {
        float backDist = (SimonActiveButtons() == 4) ? 2.4f : 2.8f;
        return SimonClusterCenter() + glm::vec3(-backDist, 0.05f, 0.0f);
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

        glm::vec2 in(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { movement += fwd * speed * dt; player.isWalking = true; in.y += 1.0f; }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { movement -= fwd * speed * dt; player.isWalking = true; in.y -= 1.0f; }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { player.yaw += player.turnSpeed * dt; in.x -= 1.0f; }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { player.yaw -= player.turnSpeed * dt; in.x += 1.0f; }
        // Lerp suave del joystick hacia el input para que el puntito no
        // brinque, y vuelva al centro al soltar.
        gMoveInput   = gMoveInput * 0.7f + in * 0.3f;
        gIsSprinting = sprint && player.isWalking;

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
    UpdateDoorAnim(dt);

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
                      (currentZone == 3 && gEnergia.active && gMinigameCamLocked) ||
                      gCursorReleased;
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
        if (currentZone == 2 && gReciclaje.active)   UpdateReciclajeGame(deltaTime);
        if (currentZone == 3 && gEnergia.active)     UpdateEnergiaGame(deltaTime);
        if (currentZone == 4 && gHamburguesa.active) UpdateHamburguesaGame(deltaTime);
    }

    // Resetear luces extras por defecto
    if (gLights.size() > 2) gLights[2].Power = glm::vec4(0.0f);
    if (gLights.size() > 3) gLights[3].Power = glm::vec4(0.0f);
    if (gLights.size() > 4) gLights[4].Power = glm::vec4(0.0f);
    // Botones Simon de ENERGIA (slots 5..10) apagados salvo cuando estemos
    // dentro de la zona correspondiente y se reactiven mas abajo.
    for (size_t i = 5; i < gLights.size() && i < 11; ++i) {
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
            // Luz frontal sobre macetas 2 (z ~ -12.86)
            gLights[0].Position = glm::vec3(0.0f, 7.5f, -12.5f);
            gLights[0].Color = glm::vec4(0.75f, 1.0f, 0.75f, 1.0f);
            gLights[0].Power = glm::vec4(11.0f);
            gLights[0].distance = 40.0f;
            // Luz trasera sobre macetas 3 (z ~ -15.36) que antes quedaban
            // oscuras. Usa el slot 3 (capsula1) porque slot 1 lo sobrescribe
            // la linterna del jugador despues del switch.
            if (gLights.size() > 3) {
                gLights[3].Position = glm::vec3(0.0f, 7.5f, -15.5f);
                gLights[3].Color = glm::vec4(0.78f, 1.0f, 0.78f, 1.0f);
                gLights[3].Power = glm::vec4(10.0f);
                gLights[3].distance = 35.0f;
                gLights[3].type = 0;
            }
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

            // Botones Simon: cada uno tiene su propia LUZ TRASERA que
            // vive embebida en el cubo de la tapa, de modo que el cubo
            // siempre se ve auto-iluminado (luz propia) y al sonar en
            // el patron se prende mucho mas fuerte. Numero de slots =
            // activos. Distancia 1.5 m para que el cubo brille parejo
            // sin teniir media sala.
            {
                int nSimon = SimonActiveButtons();
                // Cubo del simon se desplaza -0.035 m en X sobre el
                // boton fisico, asi que la luz queda dentro del cubo.
                glm::vec3 lightOffset(-0.035f, 0.0f, 0.0f);
                for (int i = 0; i < nSimon && (size_t)(5 + i) < gLights.size(); ++i) {
                    float glow = gEnergia.buttonGlow[i];
                    // Base 25.2 (anterior 31.5 -20%) para que el cubo
                    // no quemen los colores en reposo. Glow boost se
                    // mantiene fuerte (76.5) para que el destello al
                    // sonar / pulsar siga siendo muy notorio.
                    float ambient = 25.2f + 2.88f * sinf(t * 1.5f + i);
                    float power = ambient + glow * 76.5f;
                    gLights[5 + i].Position = SimonButtonPos(i) + lightOffset;
                    gLights[5 + i].Color = SIMON_COLORS[i];
                    gLights[5 + i].Power = glm::vec4(power);
                    gLights[5 + i].distance = 1.5f;
                }
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


    // Material metalico opaco. Specular moderado para un highlight visible
    // pero sin look "espejado": el reflejo de cubemap (overlay fresnel) se
    // eliminó porque hacia ver el metal como cromo translucido. El metal
    // ahora se siente solido, con brillo direccional del phong y nada mas.
    glm::vec4 ambMetal = glm::vec4(0.08f, 0.08f, 0.09f, 1.0f);
    glm::vec4 diffMetal = glm::vec4(0.55f, 0.56f, 0.60f, 1.0f);
    glm::vec4 specMetal = glm::vec4(2.5f, 2.5f, 2.8f, 1.0f);
    int alphaMetal = 128;


    // ============ Dibujado de puerta con desliz blast-door ============
    // Tinte acero frio para compensar luces calidas de las salas.
    auto DrawZoneDoor = [&](int doorIdx, const ZoneDoor& d, float doorYaw) {
        if (!PUERTA_MODELO) return;
        float pulseGlobal = 0.80f + 0.12f * sinf(t * 2.2f);
        float dx = player.position.x - d.center.x;
        float dz = player.position.z - d.center.z;
        float dist2 = dx * dx + dz * dz;
        // Brillo cercano: hasta 1.3x cuando el jugador esta encima.
        // (`near`/`far` son macros de windef.h, no usar como variables)
        float proximity = glm::clamp(1.0f - dist2 / 9.0f, 0.0f, 1.0f);
        float pulse = pulseGlobal * (1.0f + proximity * 0.45f);

        glm::vec4 doorAmb  = glm::vec4(0.10f, 0.13f, 0.18f, 1.0f) * pulse;
        glm::vec4 doorDiff = glm::vec4(0.55f, 0.62f, 0.78f, 1.0f) * pulse;
        glm::vec4 doorSpec = glm::vec4(0.55f, 0.62f, 0.80f, 1.0f);
        phongShader->use();
        SetMaterial(phongShader, doorAmb, doorDiff, doorSpec, 48);

        // Suavizado (smoothstep) en la subida: arranque y final mas suaves
        // que un lerp lineal, da sensacion de servo mecanico.
        float a = (doorIdx >= 0 && doorIdx < 8) ? doorOpenAmount[doorIdx] : 0.0f;
        float aSmooth = a * a * (3.0f - 2.0f * a);
        float yOffset = aSmooth * DOOR_SLIDE_HEIGHT;

        glm::mat4 m(1.0f);
        m = glm::translate(m, d.center + glm::vec3(0.0f, yOffset, 0.0f));
        m = glm::rotate(m, glm::radians(doorYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(1.0f));
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
                DrawZoneDoor(i, zoneDoors[i], doorYaws[i]);
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

            // Cuando el minijuego de espectrografia esta activo, el meteoro
            // se tinta con la linea espectral en la que la barra se haya
            // detenido. Asi el color del cometa coincide con el del segmento
            // del espectro donde el jugador presiono SPACE.
            glm::vec4 meteorTint(1.0f);
            if (gEspectro.active) {
                glm::vec4 c = ESPECTRO_COLORS[gEspectro.currentColor];
                // alpha controla cuanto sustituye al color base de la textura
                meteorTint = glm::vec4(c.r * 1.6f, c.g * 1.6f, c.b * 1.6f, 0.85f);
            }
            proceduralShader->setVec4("uTint", meteorTint);

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
        DrawZoneDoor(4, zoneDoors[4], RETURN_DOOR_YAW[0]);
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        break;

    case 2: // RECICLAJE
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (RECICLAJE_OPACO) RECICLAJE_OPACO->Draw(*phongShader);
        // Pase metalico opaco. El overlay fresnel se quito porque el reflejo
        // del cubemap hacia ver el metal demasiado "transparente"/espejado;
        // ahora el highlight viene solo del specular phong.
        phongShader->setMat4("model", baseModelMat);
        SetMaterial(phongShader, ambMetal, diffMetal, specMetal, alphaMetal);
        if (RECICLAJE_METAL) RECICLAJE_METAL->Draw(*phongShader);

        // ====== MINIJUEGO DE APILAR CAJAS ======
        // Dibujamos la sala del minijuego siempre que este activo, pausado
        // con progreso parcial, o ya completado (la torre apilada queda
        // como prueba de la tarea cumplida).
        if (gReciclaje.active || gStats.recycleDone || gReciclaje.placedCount > 0)
            DrawReciclajeGame(projection, view);

        // Puerta de retorno al HUB (zoneDoors[5], borde oeste del modulo)
        DrawZoneDoor(5, zoneDoors[5], RETURN_DOOR_YAW[1]);
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        break;

    case 3: // ENERGIA
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (ENERGIA_OPACO) ENERGIA_OPACO->Draw(*phongShader);
        // Pase metalico opaco (sin overlay fresnel; ver nota en RECICLAJE).
        phongShader->setMat4("model", baseModelMat);
        SetMaterial(phongShader, ambMetal, diffMetal, specMetal, alphaMetal);
        if (ENERGIA_METALICO) ENERGIA_METALICO->Draw(*phongShader);

        // ====== MINIJUEGO SIMON (Energia) ======
        if (gEnergia.active) DrawEnergiaGame(projection, view);

        // Puerta de retorno al HUB (zoneDoors[6], borde sur del modulo)
        DrawZoneDoor(6, zoneDoors[6], RETURN_DOOR_YAW[2]);
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        break;

    case 4: // CAPSULAS
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (CAPSULAS_OPACO) CAPSULAS_OPACO->Draw(*phongShader);

        // ====== MINIJUEGO HAMBURGUESA (mesa de practicas) ======
        // Dibujamos la pila si el minijuego esta activo O si la comida ya
        // quedo preparada (la hamburguesa terminada se queda visible como
        // prueba de la tarea cumplida).
        if (gHamburguesa.active || gStats.foodPrepared || gHamburguesa.step > 0)
            DrawHamburguesaGame(projection, view);

        // Puerta de retorno al HUB (zoneDoors[7], borde este del modulo)
        DrawZoneDoor(7, zoneDoors[7], RETURN_DOOR_YAW[3]);
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
        { "Manual del Astronauta",    "Q" },
        { "Liberar / recapturar mouse","ALT" },
        { "Preparar comida (capsulas)",  "B" },
        { "Capas hamburguesa (capsulas)", "T Y U I O P" },
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

// Joystick virtual: circulo grande (base) + punto interior (stick) que se
// desplaza segun gMoveInput. Encima muestra etiqueta de SPRINT cuando se
// corre. Se dibuja como cuadrados aproximando un anillo (no tenemos primitive
// para circulos, asi que dibujamos 24 segmentos radiales). Pequenio,
// abajo-izquierda, justo debajo del panel de instrucciones.
void DrawJoystick() {
    if (!gText || !gText->ready) return;

    const float r = 38.0f;                   // radio del aro exterior
    // Esquina inferior derecha, ARRIBA del bloque de barras (las barras
    // ocupan ~y=18..110). Subimos el joystick a y=180 para que no se
    // empalme. El minimapa esta en la parte superior-derecha, asi que el
    // joystick queda en el hueco vertical libre del lado derecho.
    const float cx = SCR_WIDTH - 80.0f;
    const float cy = 195.0f;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Fondo cuadrado tras el joystick (panel oscuro con bracket).
    float pad = 14.0f;
    float bx = cx - r - pad, by = cy - r - pad;
    float bw = (r + pad) * 2.0f, bh = (r + pad) * 2.0f + 14.0f;
    DrawQuad2D(bx + 3.0f, by - 3.0f, bw, bh, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));
    DrawQuad2D(bx, by, bw, bh, glm::vec4(0.05f, 0.10f, 0.18f, 0.82f));
    DrawQuad2D(bx, by + bh - 3.0f, bw, 3.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.95f));
    DrawQuad2D(bx, by, bw, 2.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.55f));

    // Aro circular aproximado con 36 quads tangenciales (tickmarks).
    const int NSEG = 36;
    for (int i = 0; i < NSEG; ++i) {
        float a = (float)i / NSEG * 6.2831853f;
        float px = cx + cosf(a) * r;
        float py = cy + sinf(a) * r;
        float alpha = (i % 9 == 0) ? 1.0f : 0.55f;
        glm::vec4 col(0.55f, 0.75f, 1.0f, alpha);
        DrawQuad2D(px - 1.5f, py - 1.5f, 3.0f, 3.0f, col);
    }
    // Cruz interior tenue (referencia de centro).
    DrawQuad2D(cx - r + 4.0f, cy - 0.5f, (r * 2.0f) - 8.0f, 1.0f,
               glm::vec4(0.55f, 0.75f, 1.0f, 0.20f));
    DrawQuad2D(cx - 0.5f, cy - r + 4.0f, 1.0f, (r * 2.0f) - 8.0f,
               glm::vec4(0.55f, 0.75f, 1.0f, 0.20f));

    // Etiquetas W/A/S/D alrededor del aro.
    gText->RenderText("W", cx - 6.0f,  cy + r - 2.0f,  0.36f, glm::vec3(0.75f, 0.92f, 1.0f));
    gText->RenderText("S", cx - 6.0f,  cy - r - 14.0f, 0.36f, glm::vec3(0.75f, 0.92f, 1.0f));
    gText->RenderText("A", cx - r - 14.0f, cy - 8.0f,  0.36f, glm::vec3(0.75f, 0.92f, 1.0f));
    gText->RenderText("D", cx + r + 4.0f,  cy - 8.0f,  0.36f, glm::vec3(0.75f, 0.92f, 1.0f));

    // Stick (punto interior). Mapea gMoveInput.x al desplazamiento horizontal
    // y gMoveInput.y al vertical (positivo Y = arriba = W).
    float sx = cx + glm::clamp(gMoveInput.x, -1.0f, 1.0f) * (r * 0.65f);
    float sy = cy + glm::clamp(gMoveInput.y, -1.0f, 1.0f) * (r * 0.65f);
    float stickR = gIsSprinting ? 9.0f : 7.0f;
    // Halo
    glm::vec4 halo = gIsSprinting
        ? glm::vec4(1.0f, 0.65f, 0.20f, 0.45f)
        : glm::vec4(0.40f, 0.80f, 1.0f, 0.35f);
    DrawQuad2D(sx - stickR - 3.0f, sy - stickR - 3.0f,
               (stickR + 3.0f) * 2.0f, (stickR + 3.0f) * 2.0f, halo);
    // Cuerpo del stick
    glm::vec4 body = gIsSprinting
        ? glm::vec4(1.0f, 0.78f, 0.30f, 1.0f)
        : glm::vec4(0.65f, 0.95f, 1.0f, 1.0f);
    DrawQuad2D(sx - stickR, sy - stickR, stickR * 2.0f, stickR * 2.0f, body);
    // Reflejo
    DrawQuad2D(sx - stickR + 2.0f, sy + stickR - 4.0f, stickR, 2.0f,
               glm::vec4(1.0f, 1.0f, 1.0f, 0.7f));

    // Pie con estado: caminando / corriendo / quieto.
    const char* state = "QUIETO";
    glm::vec3 stateCol(0.70f, 0.78f, 0.92f);
    if (gIsSprinting) { state = "SPRINT"; stateCol = glm::vec3(1.0f, 0.78f, 0.30f); }
    else if (glm::length(gMoveInput) > 0.1f) { state = "CAMINA"; stateCol = glm::vec3(0.65f, 0.95f, 1.0f); }
    gText->RenderText(state, bx + (bw - 50.0f) * 0.5f, by + 6.0f, 0.36f, stateCol);

    glEnable(GL_DEPTH_TEST);
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
        out.push_back("Camina a una puerta luminosa para teletransportarte");
    }

    switch (currentZone) {
    case 0: // HUB
        if (d <= DIFF_EASY) {
            out.push_back("Las 4 puertas (N/S/E/O) llevan a cada zona");
            out.push_back("Atajos rapidos: 0=HUB 1=Invernadero 2=Reciclaje 3=Energia 4=Capsulas");
        }
        break;
    case 1: // INVERNADERO
        out.push_back("B = ritmo macetas (H J K L)  |  V = espectrografia meteorito (SPACE)");
        if (d <= DIFF_MEDIUM) out.push_back("G alterna vista cenital  |  P/O hace crecer/resetea el helecho");
        if (d <= DIFF_EASY)   out.push_back("Espectro: SPACE detiene la barra sobre la linea objetivo (H-a/H-b/H-g/H-d)");
        if (d <= DIFF_EASY)   out.push_back("Sweet spot: ancho en FACIL, mas estrecho en MEDIO/EXPERTO");
        if (d <= DIFF_EASY)   out.push_back("Solo se usan 2 macetas en FACIL; MEDIO y EXPERTO usan las 4");
        break;
    case 2: // RECICLAJE
        out.push_back("B = iniciar  |  E = recoger / soltar caja  |  Llevalas al cuadro dorado");
        if (d <= DIFF_MEDIUM) out.push_back("R reinicia las cajas  |  TAB interrumpe el minijuego");
        if (d <= DIFF_EASY)   out.push_back("Acercate a una caja para recogerla, parate en el cuadro dorado para apilarla");
        if (d <= DIFF_EASY)   out.push_back("Si sueltas fuera del cuadro la caja queda en el piso (desorden!)");
        if (d <= DIFF_EASY)   out.push_back("FACIL = 5 cajas, sin reloj; MEDIO/EXPERTO = 8/12 cajas con reloj");
        break;
    case 3: // ENERGIA
        out.push_back("B = iniciar Simon (la camara se fija a la pila)  |  Clic en la membrana o Z X C V (N M en MEDIO/EXPERTO)");
        if (d <= DIFF_EASY)   out.push_back("PRINCIPIANTE = 4 placas, MEDIO/EXPERTO = 6 placas (mas largo en EXPERTO)");
        if (d <= DIFF_EASY)   out.push_back("Memoriza el orden de las luces sobre los botones y repitelo");
        if (d <= DIFF_EASY)   out.push_back("Llega a Nivel 3 para estabilizar el reactor (Energia al 100%)");
        break;
    case 4: // CAPSULAS
        out.push_back("B = arma la hamburguesa en la mesa de practicas  |  T Y U I O P apilan las capas en orden");
        if (d <= DIFF_EASY) out.push_back("Receta: pan inferior -> carne -> queso -> jitomate -> lechuga -> pan superior");
        if (d <= DIFF_EASY) out.push_back("FACIL no tiene reloj; MEDIO/EXPERTO te ponen tiempo para servir la torta");
        if (d <= DIFF_EASY) out.push_back("Requiere haber regado las plantas del invernadero (cultivos frescos)");
        break;
    }

    // Recordatorio de dificultad
    const char* difLabel[3] = { "FACIL", "MEDIO", "EXPERTO" };
    char buf[96];
    snprintf(buf, sizeof(buf), "Dificultad: %s  (7/8/9 para cambiar)", difLabel[(int)d]);
    out.push_back(buf);
    return out;
}

// Pistas minimas siempre visibles cuando el panel esta colapsado. Son las
// teclas esenciales para no quedar atrapado y reciben adaptaciones por zona.
static std::vector<std::string> BuildCompactHelpLines() {
    std::vector<std::string> out;
    out.push_back("WASD mover  |  Shift correr  |  Espacio saltar  |  ESC menu");
    const char* zoneHint = "";
    switch (currentZone) {
        case 0: zoneHint = "HUB: camina a una puerta luminosa para entrar a un modulo";        break;
        case 1: zoneHint = "INVERNADERO: B = regar plantas  |  V = espectro del cometa  |  P = plantar (fractal)  |  R = reiniciar";       break;
        case 2: zoneHint = "RECICLAJE: B = iniciar  |  E = recoger / soltar  |  R = reiniciar";  break;
        case 3: zoneHint = "ENERGIA: B inicia Simon  |  Z X C V (N M)  |  R = reiniciar";       break;
        case 4: zoneHint = "CAPSULAS: B = armar hamburguesa  |  T Y U I O P apilan capas  |  R = reiniciar";  break;
    }
    out.push_back(zoneHint);
    out.push_back("Q manual de tareas  |  H expandir ayuda  |  ALT libera mouse");
    return out;
}

void DrawInstructionsPanel() {
    if (!gText || !gText->ready) return;
    std::vector<std::string> lines = gHelpExpanded ? BuildHelpLines()
                                                   : BuildCompactHelpLines();
    if (lines.empty()) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float pad = 16.0f;
    float lineH = 22.0f;
    float headerH = 24.0f;
    float panelH = lines.size() * lineH + 20.0f + headerH;
    float panelW = gHelpExpanded ? 520.0f : 460.0f;
    float x = pad;
    // Las barras de estatus ocupan ~y0=40 + 25 etiqueta + 18 barra + ~5 margen
    // = hasta ~y=95. Subimos el panel para que el borde inferior quede por
    // encima de ese bloque y nunca se empalmen.
    float y = 110.0f;

    // Sombra
    DrawQuad2D(x + 3.0f, y - 3.0f, panelW, panelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));
    // Fondo
    DrawQuad2D(x, y, panelW, panelH, glm::vec4(0.05f, 0.10f, 0.18f, 0.85f));
    // Lineas doradas arriba y abajo
    DrawQuad2D(x, y + panelH - 3.0f, panelW, 3.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.95f));
    DrawQuad2D(x, y,                 panelW, 2.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.55f));
    // Brackets dorados
    const float bL = 14.0f, bT = 3.0f;
    glm::vec4 brCol(1.0f, 0.85f, 0.30f, 0.95f);
    DrawQuad2D(x,                  y + panelH - bT, bL, bT, brCol);
    DrawQuad2D(x,                  y + panelH - bL, bT, bL, brCol);
    DrawQuad2D(x + panelW - bL,    y + panelH - bT, bL, bT, brCol);
    DrawQuad2D(x + panelW - bT,    y + panelH - bL, bT, bL, brCol);
    DrawQuad2D(x,                  y,               bL, bT, brCol);
    DrawQuad2D(x,                  y,               bT, bL, brCol);
    DrawQuad2D(x + panelW - bL,    y,               bL, bT, brCol);
    DrawQuad2D(x + panelW - bT,    y,               bT, bL, brCol);

    // Encabezado con titulo y hint de expansion.
    const char* title = gHelpExpanded ? ">> CONTROLES (expandido)"
                                       : ">> CONTROLES (compacto)";
    gText->RenderText(title, x + 14.0f, y + panelH - 22.0f, 0.38f,
                      glm::vec3(0.65f, 0.95f, 1.0f));
    gText->RenderText(gHelpExpanded ? "H ocultar" : "H expandir",
                      x + panelW - 105.0f, y + panelH - 22.0f, 0.34f,
                      glm::vec3(1.0f, 0.95f, 0.55f));

    for (size_t i = 0; i < lines.size(); ++i) {
        float lineY = y + panelH - 20.0f - headerH - (float)i * lineH;
        // Sombra del texto para legibilidad sobre fondo claro.
        gText->RenderText(lines[i], x + 14.0f + 1.0f, lineY - 1.0f, 0.40f,
                          glm::vec3(0.0f));
        gText->RenderText(lines[i], x + 14.0f, lineY, 0.40f,
                          glm::vec3(0.88f, 0.92f, 0.98f));
    }
    glEnable(GL_DEPTH_TEST);
}

// Panel inferior central con el mensaje de narrativa actual. Estilo "HUD de
// videojuego": entra deslizandose desde la izquierda, marco con corchetes en
// las 4 esquinas, stripe magenta al costado y un chevron ">>" pulsante. La
// duracion total es corta (NARRATIVE_DURATION) para que el jugador no se
// quede leyendo durante los minijuegos.
void DrawNarrativePanel() {
    if (gNarrativeQueue.empty()) return;
    if (!gText || !gText->ready) return;

    // gNarrativeTimer cuenta hacia 0; lived es el tiempo desde que aparecio.
    float t = gNarrativeTimer;
    float lived = NARRATIVE_DURATION - t;

    const float FADE_IN  = 0.18f;
    const float FADE_OUT = 0.28f;
    float fade;
    if (lived < FADE_IN)       fade = lived / FADE_IN;
    else if (t < FADE_OUT)     fade = t / FADE_OUT;
    else                       fade = 1.0f;
    fade = glm::clamp(fade, 0.0f, 1.0f);

    // Slide-in horizontal al aparecer (entra desde la izquierda).
    float slide = (lived < FADE_IN) ? (1.0f - lived / FADE_IN) : 0.0f;
    float slideOffset = -slide * 90.0f;

    // Pulso del chevron y del stripe magenta.
    float gt = (float)glfwGetTime();
    float pulse = 0.55f + 0.45f * sinf(gt * 6.5f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const std::string& msg = gNarrativeQueue.front();
    float scale = 0.58f;
    float w = gText->MeasureWidth(msg, scale);
    float panelH = 56.0f;
    float panelW = w + 110.0f; // espacio extra para chevron y padding
    if (panelW > (float)SCR_WIDTH - 40.0f) panelW = (float)SCR_WIDTH - 40.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f + slideOffset;
    float panelY = SCR_HEIGHT * 0.18f;

    // Cuerpo del panel (oscuro, casi opaco para que destaque sobre el juego).
    DrawQuad2D(panelX, panelY, panelW, panelH,
               glm::vec4(0.02f, 0.04f, 0.08f, 0.88f * fade));

    // Stripe magenta lateral (acento "alerta" estilo RPG/sci-fi).
    DrawQuad2D(panelX, panelY, 5.0f, panelH,
               glm::vec4(1.0f, 0.28f, 0.62f, fade * (0.75f + 0.25f * pulse)));

    // Barras cyan arriba y abajo.
    DrawQuad2D(panelX, panelY + panelH - 2.0f, panelW, 2.0f,
               glm::vec4(0.35f, 0.95f, 1.0f, fade));
    DrawQuad2D(panelX, panelY, panelW, 2.0f,
               glm::vec4(0.35f, 0.95f, 1.0f, fade * 0.65f));

    // Corchetes en las 4 esquinas (look de HUD videojuego).
    const float bL = 14.0f, bT = 3.0f;
    glm::vec4 cBracket(1.0f, 0.85f, 0.25f, fade);
    // sup-izq
    DrawQuad2D(panelX,                       panelY + panelH - bT, bL, bT, cBracket);
    DrawQuad2D(panelX,                       panelY + panelH - bL, bT, bL, cBracket);
    // sup-der
    DrawQuad2D(panelX + panelW - bL,         panelY + panelH - bT, bL, bT, cBracket);
    DrawQuad2D(panelX + panelW - bT,         panelY + panelH - bL, bT, bL, cBracket);
    // inf-izq
    DrawQuad2D(panelX,                       panelY,               bL, bT, cBracket);
    DrawQuad2D(panelX,                       panelY,               bT, bL, cBracket);
    // inf-der
    DrawQuad2D(panelX + panelW - bL,         panelY,               bL, bT, cBracket);
    DrawQuad2D(panelX + panelW - bT,         panelY,               bT, bL, cBracket);

    // Chevron ">>" pulsante a la izquierda del texto.
    glm::vec3 chCol = glm::vec3(0.30f, 0.95f, 1.0f) * (0.55f + 0.45f * pulse) * fade;
    gText->RenderText(">>", panelX + 18.0f, panelY + 15.0f, 0.70f, chCol);

    // Texto principal con sombra para legibilidad.
    float textX = panelX + 60.0f;
    float textY = panelY + 17.0f;
    gText->RenderText(msg, textX + 2.0f, textY - 2.0f, scale,
                      glm::vec3(0.0f, 0.0f, 0.0f) * fade);
    gText->RenderText(msg, textX, textY, scale,
                      glm::vec3(0.95f, 1.0f, 1.0f) * fade);

    glEnable(GL_DEPTH_TEST);
}

// Barras de estatus inferiores: representan la vida diaria del astronauta
// (agua, comida, oxigeno, energia de la base, tareas). Estilo HUD sci-fi:
// fondo oscuro tipo "consola", marco dorado, cada barra con su icono ASCII a
// la izquierda, segmentos verticales sobre el relleno (look de barra LED),
// sombra del texto y un pulso suave si la barra esta en zona critica (<25%).
void DrawStatusBars() {
    if (!gText || !gText->ready) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    struct Bar { const char* label; const char* icon; float value; glm::vec4 color; };
    Bar bars[5] = {
        { "AGUA",    "~",  gStats.agua,    glm::vec4(0.30f, 0.65f, 1.00f, 1.0f) },
        { "COMIDA",  "@",  gStats.comida,  glm::vec4(1.00f, 0.55f, 0.25f, 1.0f) },
        { "OXIGENO", "O2", gStats.oxigeno, glm::vec4(0.55f, 0.95f, 0.95f, 1.0f) },
        { "ENERGIA", "+",  gStats.energia, glm::vec4(1.00f, 0.85f, 0.20f, 1.0f) },
        { "TAREAS",  "*",  gStats.tareas,  glm::vec4(0.55f, 0.90f, 0.40f, 1.0f) },
    };

    const float barW   = 170.0f;
    const float barH   = 20.0f;
    const float gap    = 14.0f;
    const float totalW = 5 * barW + 4 * gap;
    float x0 = (SCR_WIDTH - totalW) * 0.5f;
    float y0 = 40.0f;
    float t  = (float)glfwGetTime();
    float critPulse = 0.7f + 0.3f * sinf(t * 4.5f);

    // Marco exterior tipo "consola": sombra inferior, fondo, doble linea
    // dorada (encima y debajo) y dos "esquinas" estilo bracket.
    float fx = x0 - 18.0f, fw = totalW + 36.0f;
    float fy = y0 - 22.0f, fh = barH + 50.0f;
    DrawQuad2D(fx + 4.0f, fy - 4.0f, fw, fh, glm::vec4(0.0f, 0.0f, 0.0f, 0.35f)); // sombra
    DrawQuad2D(fx, fy, fw, fh, glm::vec4(0.04f, 0.07f, 0.13f, 0.86f));
    DrawQuad2D(fx, fy + fh - 3.0f, fw, 3.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.95f));
    DrawQuad2D(fx, fy,             fw, 2.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.55f));
    // Brackets dorados en las 4 esquinas.
    const float bL = 16.0f, bT = 3.0f;
    glm::vec4 brCol(1.0f, 0.85f, 0.30f, 0.95f);
    DrawQuad2D(fx,              fy + fh - bT, bL, bT, brCol);
    DrawQuad2D(fx,              fy + fh - bL, bT, bL, brCol);
    DrawQuad2D(fx + fw - bL,    fy + fh - bT, bL, bT, brCol);
    DrawQuad2D(fx + fw - bT,    fy + fh - bL, bT, bL, brCol);
    DrawQuad2D(fx,              fy,           bL, bT, brCol);
    DrawQuad2D(fx,              fy,           bT, bL, brCol);
    DrawQuad2D(fx + fw - bL,    fy,           bL, bT, brCol);
    DrawQuad2D(fx + fw - bT,    fy,           bT, bL, brCol);

    // Etiqueta del bloque arriba del marco.
    gText->RenderText("// STATUS VITAL //", fx + 14.0f, fy + fh - 18.0f, 0.36f,
                      glm::vec3(1.0f, 0.95f, 0.55f));

    for (int i = 0; i < 5; ++i) {
        float bx = x0 + i * (barW + gap);
        float v  = glm::clamp(bars[i].value, 0.0f, 1.0f);
        bool critical = v < 0.25f;
        float alphaPulse = critical ? critPulse : 1.0f;

        // Fondo de la barra (interior, casi negro).
        DrawQuad2D(bx, y0, barW, barH, glm::vec4(0.05f, 0.07f, 0.10f, 0.95f));
        // Borde de la barra (linea sutil del color de la categoria).
        glm::vec4 edge = bars[i].color * 0.35f; edge.a = 0.7f;
        DrawQuad2D(bx,              y0,            barW, 1.0f, edge);
        DrawQuad2D(bx,              y0 + barH - 1, barW, 1.0f, edge);
        DrawQuad2D(bx,              y0,            1.0f, barH, edge);
        DrawQuad2D(bx + barW - 1.0f,y0,            1.0f, barH, edge);

        // Relleno principal.
        glm::vec4 fill = bars[i].color;
        fill.a *= alphaPulse;
        DrawQuad2D(bx + 1.0f, y0 + 1.0f, (barW - 2.0f) * v, barH - 2.0f, fill);
        // Brillo superior (gradiente fake con un quad mas claro).
        glm::vec4 hi = fill; hi.r = std::min(1.0f, hi.r + 0.25f);
                              hi.g = std::min(1.0f, hi.g + 0.25f);
                              hi.b = std::min(1.0f, hi.b + 0.25f);
                              hi.a *= 0.55f;
        DrawQuad2D(bx + 1.0f, y0 + barH * 0.55f, (barW - 2.0f) * v, barH * 0.40f, hi);
        // Segmentos verticales (look de barra LED): 10 separadores oscuros.
        for (int s = 1; s < 10; ++s) {
            DrawQuad2D(bx + (barW / 10.0f) * s - 1.0f, y0 + 2.0f,
                       1.0f, barH - 4.0f,
                       glm::vec4(0.0f, 0.0f, 0.0f, 0.35f));
        }

        // Icono al inicio del label.
        gText->RenderText(bars[i].icon, bx + 2.0f, y0 + barH + 6.0f, 0.42f,
                          glm::vec3(bars[i].color));
        // Etiqueta con sombra para legibilidad.
        gText->RenderText(bars[i].label, bx + 26.0f + 1.0f, y0 + barH + 6.0f - 1.0f, 0.36f,
                          glm::vec3(0.0f));
        gText->RenderText(bars[i].label, bx + 26.0f, y0 + barH + 6.0f, 0.36f,
                          glm::vec3(0.92f, 0.95f, 1.00f));
        // Porcentaje a la derecha.
        char pct[8];
        snprintf(pct, sizeof(pct), "%3d%%", (int)(v * 100.0f + 0.5f));
        glm::vec3 pctCol = critical ? glm::vec3(1.0f, 0.45f, 0.45f)
                                    : glm::vec3(1.0f, 0.95f, 0.55f);
        gText->RenderText(pct, bx + barW - 48.0f + 1.0f, y0 + barH + 6.0f - 1.0f, 0.36f,
                          glm::vec3(0.0f));
        gText->RenderText(pct, bx + barW - 48.0f, y0 + barH + 6.0f, 0.36f, pctCol);
    }

    glEnable(GL_DEPTH_TEST);
}

// Widget compacto siempre visible en el borde izquierdo medio. Muestra las
// 5 tareas con un cuadrito de paloma y el titulo abreviado. Sirve como
// recordatorio constante de que se puede pulsar Q para ver el manual
// completo. Estilo Genshin: panel oscuro semitransparente, marco dorado en
// las esquinas (brackets), barra de progreso global, paloma verde con check
// real y tarea actual resaltada con un >>.
void DrawQuestsWidget() {
    if (!gText || !gText->ready) return;

    struct Mini { const char* title; bool done; };
    const int N_QUESTS = 6;
    Mini quests[N_QUESTS] = {
        { "Plantar planta",      gStats.plantPlanted  },
        { "Regar plantas",       gStats.plantsWatered },
        { "Catalogar cometa",    gStats.cometDone     },
        { "Apilar cajas",        gStats.recycleDone   },
        { "Estabilizar reactor", gStats.simonDone     },
        { "Armar hamburguesa",   gStats.foodPrepared  },
    };
    int doneCount = 0;
    int firstPending = -1;
    for (int i = 0; i < N_QUESTS; ++i) {
        if (quests[i].done) doneCount++;
        else if (firstPending < 0) firstPending = i;
    }
    // Sincroniza la barra de TAREAS con el conteo real: avanza 1/N por
    // cada quest cumplida. Asi la barra siempre cuadra con el manual.
    gStats.tareas = (float)doneCount / (float)N_QUESTS;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float panelW  = 260.0f;
    const float headerH = 50.0f;
    const float rowH    = 24.0f;
    const float panelH  = headerH + N_QUESTS * rowH + 18.0f;
    const float x = 12.0f;
    const float y = (SCR_HEIGHT - panelH) * 0.55f;

    // Sombra del panel.
    DrawQuad2D(x + 4.0f, y - 4.0f, panelW, panelH, glm::vec4(0.0f, 0.0f, 0.0f, 0.40f));
    // Fondo con dos tonos verticales para dar sensacion de gradiente.
    DrawQuad2D(x, y, panelW, panelH * 0.5f,
               glm::vec4(0.06f, 0.10f, 0.18f, 0.88f));
    DrawQuad2D(x, y + panelH * 0.5f, panelW, panelH * 0.5f,
               glm::vec4(0.04f, 0.07f, 0.14f, 0.88f));
    // Linea horizontal dorada superior + inferior.
    DrawQuad2D(x, y + panelH - 3.0f, panelW, 3.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.95f));
    DrawQuad2D(x, y,                 panelW, 2.0f, glm::vec4(0.92f, 0.78f, 0.35f, 0.55f));
    // Brackets dorados en las 4 esquinas.
    const float bL = 16.0f, bT = 3.0f;
    glm::vec4 brCol(1.0f, 0.85f, 0.30f, 0.95f);
    DrawQuad2D(x,                 y + panelH - bT, bL, bT, brCol);
    DrawQuad2D(x,                 y + panelH - bL, bT, bL, brCol);
    DrawQuad2D(x + panelW - bL,   y + panelH - bT, bL, bT, brCol);
    DrawQuad2D(x + panelW - bT,   y + panelH - bL, bT, bL, brCol);
    DrawQuad2D(x,                 y,               bL, bT, brCol);
    DrawQuad2D(x,                 y,               bT, bL, brCol);
    DrawQuad2D(x + panelW - bL,   y,               bL, bT, brCol);
    DrawQuad2D(x + panelW - bT,   y,               bT, bL, brCol);

    // Encabezado: titulo (tipografia mayor) + contador done/5
    char header[48];
    snprintf(header, sizeof(header), "TAREAS  %d/5", doneCount);
    gText->RenderText(">> MANUAL", x + 14.0f, y + panelH - 22.0f, 0.34f,
                      glm::vec3(0.65f, 0.95f, 1.0f));
    gText->RenderText(header, x + 14.0f, y + panelH - 42.0f, 0.46f,
                      glm::vec3(1.0f, 0.95f, 0.55f));
    gText->RenderText("Q expandir", x + panelW - 110.0f, y + panelH - 22.0f, 0.32f,
                      glm::vec3(0.65f, 0.72f, 0.85f));

    // Barra de progreso global (fina, encima de la lista).
    float pgX = x + 14.0f, pgY = y + panelH - headerH - 4.0f;
    float pgW = panelW - 28.0f, pgH = 4.0f;
    DrawQuad2D(pgX, pgY, pgW, pgH, glm::vec4(0.10f, 0.13f, 0.18f, 0.9f));
    float frac = doneCount / 5.0f;
    DrawQuad2D(pgX, pgY, pgW * frac, pgH, glm::vec4(0.55f, 0.95f, 0.55f, 1.0f));

    // Filas: chip + titulo. Verde con OK si completa; gris con > si es la
    // siguiente pendiente; gris suave en las demas.
    for (int i = 0; i < N_QUESTS; ++i) {
        float ry = y + panelH - headerH - 14.0f - (i + 1) * rowH + 4.0f;

        // Fondo de fila zebra y resaltado para la pendiente actual.
        if (i == firstPending) {
            DrawQuad2D(x + 8.0f, ry - 2.0f, panelW - 16.0f, rowH - 2.0f,
                       glm::vec4(1.0f, 0.85f, 0.30f, 0.12f));
        } else if (i % 2 == 0) {
            DrawQuad2D(x + 8.0f, ry - 2.0f, panelW - 16.0f, rowH - 2.0f,
                       glm::vec4(1.0f, 1.0f, 1.0f, 0.03f));
        }

        // Casilla con paloma o cuadrito vacio.
        glm::vec4 chkBg = quests[i].done
            ? glm::vec4(0.30f, 0.85f, 0.40f, 1.0f)
            : glm::vec4(0.20f, 0.22f, 0.28f, 1.0f);
        DrawQuad2D(x + 14.0f, ry + 4.0f, 14.0f, 14.0f, chkBg);
        // Borde claro del cuadro
        glm::vec4 chkB = quests[i].done
            ? glm::vec4(0.65f, 1.0f, 0.65f, 1.0f)
            : glm::vec4(0.55f, 0.60f, 0.70f, 1.0f);
        DrawQuad2D(x + 14.0f, ry + 4.0f, 14.0f, 1.0f, chkB);
        DrawQuad2D(x + 14.0f, ry + 17.0f, 14.0f, 1.0f, chkB);
        DrawQuad2D(x + 14.0f, ry + 4.0f, 1.0f, 14.0f, chkB);
        DrawQuad2D(x + 27.0f, ry + 4.0f, 1.0f, 14.0f, chkB);
        // "X" dentro del cuadrito si esta completado (dos quads en diagonal).
        if (quests[i].done) {
            // Marca de check: dos trazos formando un tick.
            DrawQuad2D(x + 17.0f, ry + 9.0f, 5.0f, 1.5f,
                       glm::vec4(0.05f, 0.20f, 0.05f, 1.0f));
            DrawQuad2D(x + 20.0f, ry + 6.0f, 1.5f, 9.0f,
                       glm::vec4(0.05f, 0.20f, 0.05f, 1.0f));
        }

        // Flecha de "tarea actual"
        if (i == firstPending) {
            gText->RenderText(">", x + 36.0f, ry + 2.0f, 0.38f,
                              glm::vec3(1.0f, 0.85f, 0.30f));
        }

        glm::vec3 col = quests[i].done
            ? glm::vec3(0.62f, 0.92f, 0.62f)
            : (i == firstPending ? glm::vec3(1.0f, 0.95f, 0.65f)
                                  : glm::vec3(0.78f, 0.83f, 0.92f));
        gText->RenderText(quests[i].title, x + 48.0f, ry + 2.0f, 0.36f, col);
    }

    glEnable(GL_DEPTH_TEST);
}

// Manual del aventurero (estilo Genshin): lista de tareas/minijuegos con
// paloma cuando se completan. Se abre con Q.
void DrawQuestsMenu() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    DrawQuad2D(0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT,
               glm::vec4(0.0f, 0.0f, 0.05f, 0.72f));

    float panelW = 620.0f, panelH = 460.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f;
    float panelY = (SCR_HEIGHT - panelH) * 0.5f;
    DrawQuad2D(panelX, panelY, panelW, panelH, glm::vec4(0.05f, 0.10f, 0.18f, 0.94f));
    DrawQuad2D(panelX, panelY + panelH - 4.0f, panelW, 4.0f, glm::vec4(0.92f, 0.78f, 0.35f, 1.0f));
    DrawQuad2D(panelX, panelY, panelW, 4.0f, glm::vec4(0.92f, 0.78f, 0.35f, 1.0f));

    float cx = SCR_WIDTH * 0.5f;
    DrawTextCentered("MANUAL DEL ASTRONAUTA", cx, panelY + panelH - 60.0f, 0.90f,
                     glm::vec3(0.95f, 0.82f, 0.35f));
    DrawTextCentered("Tareas diarias en la base sustentable",
                     cx, panelY + panelH - 95.0f, 0.45f,
                     glm::vec3(0.75f, 0.80f, 0.90f));

    struct Quest { const char* title; const char* hint; bool done; };
    const int N_QUESTS_MENU = 6;
    Quest quests[N_QUESTS_MENU] = {
        { "Plantar la planta (Invernadero)",         "Crece el helecho fractal con P (3 iteraciones)", gStats.plantPlanted },
        { "Regar las plantas (Invernadero)",         "Ritmo OSU: H J K L con B",       gStats.plantsWatered },
        { "Catalogar el cometa (Invernadero)",       "Espectro: V para iniciar, SPACE", gStats.cometDone },
        { "Apilar cajas de reciclaje",               "Recoge con E y suelta en el cuadro dorado", gStats.recycleDone },
        { "Estabilizar el reactor (Energia)",        "Simon nivel 3 (clic o Z X C V)", gStats.simonDone },
        { "Armar la hamburguesa (Capsulas)",         "B + receta T Y U I O P en orden", gStats.foodPrepared },
    };

    float yTop = panelY + panelH - 140.0f;
    // Achicar un poco el lineH para que las 6 quests entren sin
    // pegarse al borde inferior del panel.
    float lineH = 48.0f;
    float labelX = panelX + 60.0f;

    for (int i = 0; i < N_QUESTS_MENU; ++i) {
        float y = yTop - i * lineH;
        DrawQuad2D(panelX + 30.0f, y - 16.0f, panelW - 60.0f, lineH - 8.0f,
                   glm::vec4(1.0f, 1.0f, 1.0f, 0.04f));
        // Cuadrito de paloma
        glm::vec4 chk = quests[i].done
            ? glm::vec4(0.45f, 0.90f, 0.45f, 1.0f)
            : glm::vec4(0.30f, 0.30f, 0.38f, 1.0f);
        DrawQuad2D(panelX + 36.0f, y + 4.0f, 18.0f, 18.0f, chk);
        if (gText && gText->ready) {
            if (quests[i].done) {
                gText->RenderText("OK", panelX + 38.0f, y + 6.0f, 0.34f,
                                  glm::vec3(0.10f, 0.20f, 0.10f));
            }
            glm::vec3 titleCol = quests[i].done
                ? glm::vec3(0.65f, 0.95f, 0.65f)
                : glm::vec3(0.92f, 0.95f, 1.00f);
            gText->RenderText(quests[i].title, labelX + 8.0f, y + 12.0f, 0.50f, titleCol);
            gText->RenderText(quests[i].hint,  labelX + 8.0f, y - 10.0f, 0.36f,
                              glm::vec3(0.60f, 0.66f, 0.78f));
        }
    }

    DrawTextCentered("Pulsa Q o ESC para cerrar", cx, panelY + 22.0f, 0.40f,
                     glm::vec3(0.55f, 0.60f, 0.70f));

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
    if (menuState == MENU_QUESTS) {
        DrawQuestsMenu();
        return;
    }
    if (menuState == MENU_PAUSE) {
        DrawPauseMenu();
        return;
    }

    // HUD en juego (menu cerrado)
    DrawMinimap();
    DrawInstructionsPanel();
    DrawStatusBars();
    DrawQuestsWidget();
    DrawJoystick();
    DrawZoneBanner();
    DrawNarrativePanel();
    if (gEspectro.active) DrawEspectroHUD();
    if (gEnergia.active)  DrawEnergiaHUD();
    if (gReciclaje.active) DrawReciclajeHUD();
    if (gHamburguesa.active) DrawHamburguesaHUD();
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
        } else if (menuState == MENU_QUESTS) {
            // El manual es independiente del menu de pausa: ESC lo cierra de
            // golpe en vez de regresar a la pausa, que no fue la pantalla
            // previa.
            menuState = MENU_NONE;
        } else {
            menuState = MENU_PAUSE; // desde un sub-menu, vuelve a pausa
            menuSelection = 0;
        }
    }
    keyEscLast = escNow;

    // --- Q: Manual del Astronauta (toggle global, abre y cierra) ---
    // Se procesa antes del early-return del menu para que Q tambien sirva
    // para cerrar el manual una vez abierto. Si hay otro menu activo, Q lo
    // reemplaza por el manual.
    static bool keyQLast = false;
    bool qNow = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
    if (qNow && !keyQLast) {
        if (menuState == MENU_QUESTS) {
            menuState = MENU_NONE;
        } else {
            menuState = MENU_QUESTS;
            menuSelection = 0;
        }
    }
    keyQLast = qNow;

    // --- ALT: libera / recaptura el mouse (cursor visible para mover la
    // ventana o hacer alt-tab sin que la camara se vuelva loca). Se procesa
    // antes del early-return para que tambien funcione con un menu abierto.
    static bool keyAltLast = false;
    bool altNow = (glfwGetKey(window, GLFW_KEY_LEFT_ALT)  == GLFW_PRESS) ||
                  (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
    if (altNow && !keyAltLast) {
        gCursorReleased = !gCursorReleased;
        PushNarrative(gCursorReleased
                      ? "Mouse liberado (ALT para recapturarlo)."
                      : "Mouse recapturado.");
    }
    keyAltLast = altNow;

    // --- H: expande / colapsa el panel de controles ---
    // Solo activo si el menu esta cerrado para no chocar con otras teclas.
    static bool keyHLast = false;
    bool hNow = glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS;
    // En invernadero H tambien es la tecla para "regar maceta izq". El OSU
    // solo lee H cuando esta active; aqui priorizamos el toggle solo si NO
    // estamos jugando OSU (asi no interrumpimos el ritmo).
    if (hNow && !keyHLast && menuState == MENU_NONE && !gInvernadero.active) {
        gHelpExpanded = !gHelpExpanded;
    }
    keyHLast = hNow;

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
    // de espectrografia bloqueamos 1..4 para que el jugador no salga sin
    // querer con la camara aun encadenada al meteoro; el 0 (regreso al HUB)
    // queda habilitado como escape rapido y TAB cierra el minijuego.
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
                // Al alcanzar la iteracion 3 el helecho ya tiene
                // suficientes ramas para sentirse "crecido": palomeamos
                // la tarea "Plantar planta" del manual.
                if (!gStats.plantPlanted && fractalIteration >= 3) {
                    gStats.plantPlanted = true;
                    PushNarrative("Helecho cultivado. Tarea 'Plantar planta' completada.");
                }
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
            StopActiveMinigames();
            InitInvernaderoGame();
            gInvernadero.active = true;
            gInvernadero.topDown = true;
            gCamModeBeforeLock = cameraMode;
            gMinigameCamLocked = true;
            PushNarrative("INVERNADERO: pulsa H J K L cuando la nota llegue a su maceta.");
            std::cout << "[Invernadero] Nueva partida (dificultad " << gInvernadero.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        // R = reiniciar el minijuego activo desde cero (mismas reglas y
        // dificultad). Util cuando el jugador falla y quiere empezar de
        // nuevo sin tener que pulsar TAB primero. Aplica al OSU y al
        // espectro (mas abajo se atiende espectro).
        static bool keyR_InvLast = false;
        bool rInv = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (rInv && !keyR_InvLast && gInvernadero.active) {
            InitInvernaderoGame();
            gInvernadero.active = true;
            gInvernadero.topDown = true;
            PushNarrative("Invernadero reiniciado. A regar de nuevo.");
            std::cout << "[Invernadero] RESET" << std::endl;
        }
        keyR_InvLast = rInv;

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
        // V = iniciar/reiniciar el desafio. Aparece una barra horizontal con
        //     el espectro visible y las cuatro lineas de la serie de Balmer.
        //     Un indicador la recorre; el jugador pulsa SPACE para detenerlo
        //     dentro del "sweet spot" sobre la linea objetivo. La dificultad
        //     controla el ancho de la zona valida.
        // TAB = interrumpir.
        static bool vLast = false;
        bool vKey = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
        if (vKey && !vLast) {
            StopActiveMinigames();
            InitEspectroGame();
            gEspectro.active = true;
            gCamModeBeforeLock = cameraMode;
            gMinigameCamLocked = true;
            PushNarrative("ESPECTROGRAFIA: pulsa SPACE para detener la barra sobre la linea objetivo.");
        }
        vLast = vKey;

        if (gEspectro.active) {
            // SPACE detiene el indicador. Solo aceptamos la pulsacion si la
            // ronda esta en curso (no en pausa de feedback ni terminada).
            static bool eSpaceLast = false;
            bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
            if (spaceNow && !eSpaceLast
                && !gEspectro.stopped
                && gEspectro.feedbackTimer <= 0.0f
                && gEspectro.roundIdx < gEspectro.rounds) {
                EspectroStopBar();
            }
            eSpaceLast = spaceNow;

            // R = reiniciar la espectrografia (vuelve a la ronda 1).
            static bool keyR_EspLast = false;
            bool rEsp = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
            if (rEsp && !keyR_EspLast) {
                InitEspectroGame();
                gEspectro.active = true;
                PushNarrative("Espectrografia reiniciada.");
                std::cout << "[Espectro] RESET" << std::endl;
            }
            keyR_EspLast = rEsp;

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

    // ZONA 2: RECICLAJE - Minijuego de apilar cajas. Las cajas estan
    // dispersas en el piso de la sala; el jugador camina hasta cada una
    // y pulsa E para recogerla. Vuelve al cuadro dorado del centro y
    // pulsa E para apilarla. Si suelta fuera del cuadro la caja queda
    // en el piso (desorden, suma a la metrica drops). La camara queda
    // libre para que el jugador pueda caminar mientras lleva la caja.
    if (currentZone == 2) {
        static bool keyBRecLast = false;
        bool bRec = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bRec && !keyBRecLast) {
            StopActiveMinigames();
            InitReciclajeGame();
            gReciclaje.active = true;
            PushNarrative("RECICLAJE: recoge las cajas tiradas con E y apilalas en el cuadro dorado.");
            std::cout << "[Reciclaje] Nueva partida (dificultad "
                      << gReciclaje.difficulty << ")" << std::endl;
        }
        keyBRecLast = bRec;

        if (gReciclaje.active) {
            // E = accion contextual (recoger si no lleva caja, soltar si lleva).
            static bool keyE_RecLast = false;
            bool eRec = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
            if (eRec && !keyE_RecLast) ReciclajeAction();
            keyE_RecLast = eRec;

            // R = reiniciar (vuelve a colocar todas las cajas en el piso).
            static bool keyR_RecLast = false;
            bool rRec = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
            if (rRec && !keyR_RecLast) {
                InitReciclajeGame();
                gReciclaje.active = true;
                PushNarrative("Cajas reiniciadas. Vuelve a recoger y apilar.");
                std::cout << "[Reciclaje] RESET" << std::endl;
            }
            keyR_RecLast = rRec;

            // TAB interrumpe el minijuego. El progreso queda visible.
            static bool tabIntRec = false;
            bool tabInt = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
            if (tabInt && !tabIntRec) {
                gReciclaje.active = false;
                PushNarrative("Minijuego de reciclaje interrumpido.");
            }
            tabIntRec = tabInt;
        }
    }

    // ZONA 3: ENERGIA - Minijuego Simon (memoria con botones de bateria)
    if (currentZone == 3) {
        // B = iniciar/reiniciar partida (bloquea la camara frente al reactor)
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            StopActiveMinigames();
            InitEnergiaGame();
            gEnergia.active = true;
            gCamModeBeforeLock = cameraMode;
            gMinigameCamLocked = true;
            PushNarrative("ENERGIA: memoriza el orden de las luces. Pulsa Z X C V o haz clic en el boton.");
            std::cout << "[Energia] Nueva partida (dificultad " << gEnergia.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        // R = reiniciar el Simon (vuelve al patron inicial de la dificultad).
        static bool keyR_EnLast = false;
        bool rEn = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (rEn && !keyR_EnLast && gEnergia.active) {
            InitEnergiaGame();
            gEnergia.active = true;
            PushNarrative("Simon reiniciado. A memorizar de nuevo.");
            std::cout << "[Energia] RESET" << std::endl;
        }
        keyR_EnLast = rEn;

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

        // Teclas extra para los botones 5 y 6 (solo activas en MEDIO/EXPERTO)
        static bool keyNLast = false, keyMLast = false;
        bool nKey = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
        bool mKey = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
        if (SimonActiveButtons() >= 6) {
            if (nKey && !keyNLast) EnergiaPress(4);
            if (mKey && !keyMLast) EnergiaPress(5);
        }
        keyNLast = nKey; keyMLast = mKey;

        // CLIC DEL MOUSE: cada boton Simon (membrana sobre la pila R_1) se
        // proyecta a pantalla y comparamos con la posicion del cursor. La
        // camara se aleja un poco mas en el modo 6 botones para que las
        // membranas no queden gigantes, asi mantenemos un radio fijo de
        // ~55 px que cubre el plano sin pisar al vecino (gana la mas cerca).
        // Solo aceptamos clics en fase 2 (turno del jugador).
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
                float bestDist = 55.0f; // radio en px
                int nSimon = SimonActiveButtons();
                for (int i = 0; i < nSimon; ++i) {
                    glm::vec4 clip = proj * vw * glm::vec4(SimonButtonPos(i), 1.0f);
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

    // ZONA 4: CAPSULAS - Minijuego "Armar Hamburguesa".
    // B inicia la preparacion en la mesa de practicas. La camara se queda
    // libre para que el jugador siga viendo la mesa mientras teclea las
    // capas en orden (PAN INFERIOR -> CARNE -> QUESO -> JITOMATE ->
    // LECHUGA -> PAN SUPERIOR). Al colocar la ultima capa se llenan las
    // barras de agua y comida igual que con la version vieja del cooking
    // timer. Requiere que las plantas ya esten regadas para mantener la
    // misma narrativa del proyecto (cultivos frescos como insumo).
    if (currentZone == 4) {
        static bool keyBCapLast = false;
        bool bCap = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bCap && !keyBCapLast && !gHamburguesa.active && !gStats.foodPrepared) {
            if (!gStats.plantsWatered) {
                PushNarrative("Tip: idealmente riegas las plantas antes para tener insumos.");
            }
            StopActiveMinigames();
            InitHamburguesaGame();
            gHamburguesa.active = true;
            PushNarrative("MESA DE PRACTICAS: arma la hamburguesa siguiendo la receta (T Y U I O P).");
            std::cout << "[Hamburguesa] Nueva partida (dificultad "
                      << gHamburguesa.difficulty
                      << ", plantsWatered=" << (gStats.plantsWatered ? 1 : 0)
                      << ")" << std::endl;
        }
        keyBCapLast = bCap;

        if (gHamburguesa.active) {
            // Teclas de ingrediente. Cada tecla apila la capa correspondiente
            // SOLO si es la que toca. Si es otra capa o ya esta colocada,
            // cuenta como miss y rompe el combo.
            static bool keyLast[HAMB_ING_COUNT] = { false, false, false, false, false, false };
            for (int i = 0; i < HAMB_ING_COUNT; ++i) {
                bool now = glfwGetKey(window, HAMB_KEYS[i]) == GLFW_PRESS;
                if (now && !keyLast[i]) HamburguesaPress(i);
                keyLast[i] = now;
            }

            // R = reiniciar la hamburguesa (vuelve al paso 0). Util
            // incluso despues de terminar para volver a armar otra.
            static bool keyR_HambLast = false;
            bool rHamb = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
            if (rHamb && !keyR_HambLast) {
                InitHamburguesaGame();
                gHamburguesa.active = true;
                PushNarrative("Hamburguesa reiniciada. Vuelve a apilar las capas.");
                std::cout << "[Hamburguesa] RESET" << std::endl;
            }
            keyR_HambLast = rHamb;

            // TAB interrumpe el minijuego y deja el progreso parcial visible
            // sobre la mesa (las capas ya colocadas no se borran, asi el
            // jugador puede continuar despues con B).
            static bool tabIntH = false;
            bool tabInt = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
            if (tabInt && !tabIntH) {
                gHamburguesa.active = false;
                PushNarrative("Minijuego de hamburguesa interrumpido.");
            }
            tabIntH = tabInt;
        }
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
    if (width <= 0 || height <= 0) return; // ventana minimizada: ignora
    glViewport(0, 0, width, height);
    // Actualiza SCR_WIDTH/HEIGHT para que todo el HUD se recompute (las
    // funciones de dibujo leen estas globales cada frame).
    SCR_WIDTH  = (unsigned int)width;
    SCR_HEIGHT = (unsigned int)height;
    hudProjection = glm::ortho(0.0f, (float)width, 0.0f, (float)height);
    if (gText) gText->SetProjection(width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoffset = (float)xpos - lastX; float yoffset = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;
    if (menuState != MENU_NONE) return;
    if (gCursorReleased) return; // ALT mantiene el cursor libre, no rotar camara
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
    case 1:
        gInvernadero.difficulty = d;
        // El invernadero hospeda dos minijuegos. La espectrografia tambien
        // depende de la dificultad de la zona porque el sweet spot cambia
        // de ancho con ella, asi que la sincronizamos aqui.
        gEspectro.difficulty = d;
        break;
    case 2: gReciclaje.difficulty   = d; break;
    case 3: gEnergia.difficulty     = d; break;
    case 4: gHamburguesa.difficulty = d; break;
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
    // 0 y 1), en MEDIO y EXPERTO los 4 carriles disponibles. El spacing
    // controla cada cuanto cae una nota; FACIL muy espaciado (lento),
    // EXPERTO un poco mas apretado (rapido).
    int    totalNotes;
    float  spacing;          // separacion media entre notas (s)
    int    numLanes;
    switch (gInvernadero.difficulty) {
    case DIFF_EASY:   totalNotes = 14; spacing = 1.9f;  numLanes = 2; break;
    case DIFF_MEDIUM: totalNotes = 30; spacing = 0.9f;  numLanes = 4; break;
    case DIFF_EXPERT: totalNotes = 48; spacing = 0.48f; numLanes = 4; break;
    default:          totalNotes = 14; spacing = 1.9f;  numLanes = 2; break;
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

        // Spawn de gotas de agua sobre la maceta acertada. Caen casi en
        // vertical con una leve dispersion horizontal sobre el area de la
        // maceta real (~0.6 m de diametro). Al impactar generan splashes.
        glm::vec3 base = InvLanePos(lane);
        for (int k = 0; k < 18; ++k) {
            WaterDrop d;
            float rx = ((float)rand() / RAND_MAX - 0.5f) * 0.55f;
            float rz = ((float)rand() / RAND_MAX - 0.5f) * 0.55f;
            d.pos = base + glm::vec3(rx, 2.6f + ((float)rand() / RAND_MAX) * 0.5f, rz);
            // Velocidad inicial: pequenia dispersion lateral para que el
            // chorro no se vea robotico, y vy negativa fuerte.
            float vxJ = ((float)rand() / RAND_MAX - 0.5f) * 0.4f;
            float vzJ = ((float)rand() / RAND_MAX - 0.5f) * 0.4f;
            d.vel = glm::vec3(vxJ, -2.2f - ((float)rand() / RAND_MAX) * 1.5f, vzJ);
            d.life = 1.4f;
            d.maxLife = d.life;
            d.lane = lane;
            d.landed = false;
            d.splash = false;
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
    // Integrar gotas de agua que cayeron al regar. La altura de aterrizaje
    // (top de la tierra de la maceta real) es ~0.55 m segun el .obj.
    const float kLandY = 0.55f;
    std::vector<WaterDrop> splashSpawn;
    for (auto& d : gInvernadero.drops) {
        if (!d.landed) {
            d.vel.y -= 9.0f * dt;
            d.pos   += d.vel * dt;
            d.life  -= dt;
            if (d.pos.y <= kLandY) {
                d.pos.y  = kLandY;
                d.vel    = glm::vec3(0.0f);
                d.landed = true;
                // Truncamos vida para que el splash dure poco y se desvanezca.
                d.life   = std::min(d.life, 0.35f);
                d.maxLife = 0.35f;
                // Splash: dos o tres mini gotas que rebotan hacia afuera y
                // mueren rapido. Solo si no es a su vez una salpicadura,
                // para no spawnear cadenas infinitas.
                if (!d.splash) {
                    int splashes = 2 + (rand() % 2);
                    for (int s = 0; s < splashes; ++s) {
                        WaterDrop sp;
                        float ang = ((float)rand() / RAND_MAX) * 6.2831853f;
                        float spd = 0.7f + ((float)rand() / RAND_MAX) * 0.6f;
                        sp.pos = d.pos + glm::vec3(0.0f, 0.02f, 0.0f);
                        sp.vel = glm::vec3(cosf(ang) * spd, 1.0f + ((float)rand() / RAND_MAX) * 0.6f, sinf(ang) * spd);
                        sp.life    = 0.30f + ((float)rand() / RAND_MAX) * 0.10f;
                        sp.maxLife = sp.life;
                        sp.lane    = d.lane;
                        sp.landed  = false;
                        sp.splash  = true;
                        splashSpawn.push_back(sp);
                    }
                }
            }
        }
        else {
            // Una vez asentada, solo vive su splash time y se va.
            d.life -= dt;
        }
    }
    for (auto& sp : splashSpawn) gInvernadero.drops.push_back(sp);
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
        // Exito si acerto >=60% de las notas. Marca plants_watered y
        // desbloquea la preparacion de comida en capsulas. Antes pedia
        // hits == notes (perfecto), pero un solo miss bloqueaba todo el
        // flujo y obligaba a reiniciar. Solo dispara una vez por
        // partida; el flag persiste hasta que se reinicia el minijuego.
        int total = (int)gInvernadero.notes.size();
        int needed = (total * 6) / 10; // 60%
        if (needed < 1) needed = 1;
        if (!gStats.plantsWatered && gInvernadero.hits >= needed) {
            gStats.plantsWatered = true;
            PushNarrative("Plantas regadas. Ya puedes preparar comida en CAPSULAS.");
            StopActiveMinigames();
        }
    }
}

void DrawInvernaderoGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!CUBO_MODELO) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    // Colores por carril: 4 colores distintos para que cada maceta sea
    // facilmente identificable y se vea que es translucida (no solida).
    glm::vec4 laneColors[4] = {
        glm::vec4(1.00f, 0.30f, 0.40f, 1.0f), // 0 = rojo / coral
        glm::vec4(0.35f, 0.95f, 0.40f, 1.0f), // 1 = verde
        glm::vec4(0.30f, 0.65f, 1.00f, 1.0f), // 2 = azul
        glm::vec4(1.00f, 0.85f, 0.20f, 1.0f)  // 3 = amarillo
    };

    int lanes = gInvernadero.activeLanes;
    if (lanes < 2) lanes = 2;
    if (lanes > 4) lanes = 4;

    // 1) Guia translucida de color sobre cada maceta ACTIVA. Solo color
    //    (sin textura del modelo) para que sea limpia y solo guie al
    //    jugador hacia la maceta correcta. Disco bajo dimensionado al area
    //    REAL de plantacion de la maceta (~0.55 m de diametro de tierra
    //    segun el .obj), no a todo el macetero. Asi cada carril cabe
    //    exactamente sobre la tierra y se ve "regado" la planta, no la
    //    sala completa.
    float laneCylSize = 0.62f;
    float laneCylH    = 0.35f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < lanes; i++) {
        glm::vec3 base = InvLanePos(i);
        // Apoyar el disco sobre la tierra de la maceta (~0.55 m segun .obj)
        // para que flote sutilmente encima en lugar de envolver todo el
        // macetero.
        base.y = 0.70f;
        float pulse = 0.50f + 0.25f * sinf(gInvernadero.gameTime * 1.8f + i);
        float waterBoost = gInvernadero.waterEffect[i];
        glm::vec4 c = laneColors[i];
        c.a = 0.35f + 0.20f * pulse + waterBoost * 0.30f;
        // Ambient alto y diffuse igual al color para que el cubo se vea
        // como un color plano translucido (no se modula con la textura).
        SetMaterial(phongShader, c, c, glm::vec4(0.0f), 1);
        phongShader->setFloat("transparency", c.a);

        glm::mat4 m(1.0f);
        m = glm::translate(m, base);
        m = glm::scale(m, glm::vec3(laneCylSize, laneCylH, laneCylSize));
        phongShader->setMat4("model", m);
        CUBO_MODELO->Draw(*phongShader);
    }

    // 2) Notas: pequenios cubos translucidos que se acercan horizontalmente
    //    desde el muro exterior de su fila. Aparecen lookAhead segundos
    //    antes y desaparecen 0.4 s despues del hitTime. Translucidos para
    //    que coincidan con el estilo de los marcadores.
    //    lookAhead depende de la dificultad para regular la velocidad
    //    visual de las notas: en FACIL viajan mas despacio, en EXPERTO
    //    un poco mas rapido que en MEDIO.
    float lookAhead;
    switch (gInvernadero.difficulty) {
    case DIFF_EASY:   lookAhead = 2.8f; break;
    case DIFF_MEDIUM: lookAhead = 2.0f; break;
    case DIFF_EXPERT: lookAhead = 1.7f; break;
    default:          lookAhead = 2.0f; break;
    }
    float scrollDist = 5.5f;
    for (auto& n : gInvernadero.notes) {
        if (n.hit || n.missed) continue;
        if (n.lane >= lanes) continue;
        float tToHit = n.hitTime - gInvernadero.gameTime;
        if (tToHit > lookAhead) continue;
        if (tToHit < -0.4f) continue;

        glm::vec3 target = InvLanePos(n.lane);
        glm::vec3 start  = target;
        start.x -= INV_SCROLL_DIR[n.lane] * scrollDist;
        float a = glm::clamp(1.0f - (tToHit / lookAhead), 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(start, target, a);
        // Volar las notas a la altura de la guia translucida (~0.7 m) para
        // que crucen por encima de la tierra de la maceta, no atravesando
        // el macetero.
        pos.y = 0.80f;

        glm::vec4 c = laneColors[n.lane];
        c.a = 0.80f; // translucido pero todavia bien visible
        // Color plano: ambient=diffuse=color, sin specular para que no se
        // vea la textura del modelo ni reflejos, solo un color limpio.
        SetMaterial(phongShader, c, c, glm::vec4(0.0f), 1);
        phongShader->setFloat("transparency", c.a);

        // Notas mas chicas para que se vean como punto de impacto sobre
        // la maceta y no como cubos que rebasan el diametro del carril.
        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::scale(m, glm::vec3(0.18f));
        phongShader->setMat4("model", m);
        CUBO_MODELO->Draw(*phongShader);
    }
    phongShader->setFloat("transparency", 1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // 3) Gotas de agua animadas: caen como cubos alargados azul agua y al
    //    impactar se aplanan como puddle/charco que se desvanece. Splashes
    //    son cubos pequenios que rebotan hacia afuera. Todo en azul agua
    //    con leve specular para brillo liquido.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (const WaterDrop& d : gInvernadero.drops) {
        float lifeT = (d.maxLife > 0.0f) ? glm::clamp(d.life / d.maxLife, 0.0f, 1.0f) : 0.0f;
        // Azul agua saturado, con leve variacion de cian segun vida.
        glm::vec4 c(0.20f + 0.10f * lifeT,
                    0.55f + 0.20f * lifeT,
                    0.95f,
                    glm::clamp(d.life * 1.8f, 0.0f, 0.85f));
        SetMaterial(phongShader, c, c, glm::vec4(0.8f, 0.9f, 1.0f, 1.0f), 64);
        phongShader->setFloat("transparency", c.a);

        glm::mat4 m(1.0f);
        if (d.landed) {
            // Charco aplanado que se expande mientras se desvanece.
            float spread = 0.18f + (1.0f - lifeT) * 0.25f;
            m = glm::translate(m, d.pos);
            m = glm::scale(m, glm::vec3(spread, 0.04f, spread));
        }
        else if (d.splash) {
            // Mini gota rebotando: cubito pequenio, sin estiramiento.
            m = glm::translate(m, d.pos);
            m = glm::scale(m, glm::vec3(0.06f));
        }
        else {
            // Gota cayendo: cubo alargado en y, con leve ondeo lateral
            // dependiente de posicion para que cada gota se mueva un poco
            // diferente. El estiramiento crece al caer.
            float wobble  = sinf(gInvernadero.gameTime * 6.0f + d.pos.x * 3.0f + d.pos.z * 2.0f) * 0.05f;
            float stretch = 0.16f + (1.0f - lifeT) * 0.14f;
            m = glm::translate(m, d.pos + glm::vec3(wobble, 0.0f, wobble * 0.5f));
            m = glm::scale(m, glm::vec3(0.09f, stretch, 0.09f));
        }
        phongShader->setMat4("model", m);
        CUBO_MODELO->Draw(*phongShader);
    }
    phongShader->setFloat("transparency", 1.0f);
    glDepthMask(GL_TRUE);
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
    gEnergia.hits = 0;
    gEnergia.misses = 0;
    gEnergia.combo = 0;
    gEnergia.maxCombo = 0;
    for (int i = 0; i < 6; i++) gEnergia.buttonGlow[i] = 0.0f;

    // Por dificultad: cuantos botones y que tan largo arranca el patron.
    //   PRINCIPIANTE: 4 botones, patron inicial 3 (cuadrado 2x2 clasico)
    //   MEDIO:        6 botones, patron inicial 4
    //   EXPERTO:      6 botones, patron inicial 8 (mas largo de entrada
    //                 para que el reto memoria sea mucho mayor)
    int startLen;
    switch (gEnergia.difficulty) {
    case DIFF_EASY:   startLen = 3; gEnergia.level = 1; break;
    case DIFF_MEDIUM: startLen = 4; gEnergia.level = 2; break;
    case DIFF_EXPERT: startLen = 8; gEnergia.level = 4; break;
    default:          startLen = 3; gEnergia.level = 1; break;
    }
    gEnergia.bestLevel = gEnergia.level;
    int nBtns = SimonActiveButtons();
    srand((unsigned)(glfwGetTime() * 1000.0));
    for (int i = 0; i < startLen; i++) gEnergia.pattern.push_back(rand() % nBtns);
}

void EnergiaPress(int button) {
    if (!gEnergia.active) return;
    if (gEnergia.phase != 2) return; // solo se aceptan inputs en fase de jugador
    // Validar contra los botones realmente activos (PRINCIPIANTE solo 4;
    // si llega N/M en ese modo, los ignoramos para no romper el patron).
    if (button < 0 || button >= SimonActiveButtons()) return;
    gEnergia.buttonGlow[button] = 1.0f;

    int expected = gEnergia.pattern[gEnergia.playerIndex];
    if (button == expected) {
        gEnergia.playerIndex++;
        gEnergia.hits++;
        gEnergia.combo++;
        if (gEnergia.combo > gEnergia.maxCombo) gEnergia.maxCombo = gEnergia.combo;
        // Score con bonus de racha: +50 base, +5 por cada eslabon de combo.
        gEnergia.score += 50 + gEnergia.combo * 5;
        std::cout << "[Simon] OK boton " << SIMON_NAMES[button]
                  << "  (" << gEnergia.playerIndex
                  << "/" << gEnergia.pattern.size() << ")"
                  << "  combo=" << gEnergia.combo << std::endl;
        if (gEnergia.playerIndex >= (int)gEnergia.pattern.size()) {
            gEnergia.level++;
            if (gEnergia.level > gEnergia.bestLevel) gEnergia.bestLevel = gEnergia.level;
            gEnergia.pattern.push_back(rand() % SimonActiveButtons());
            gEnergia.playerIndex = 0;
            gEnergia.phase = 1;            // volver a mostrar patron
            gEnergia.phaseTimer = 0.6f;     // pausa antes de mostrar
            gEnergia.showIndex = -1;
            char buf[80];
            snprintf(buf, sizeof(buf), "Patron correcto! Nivel %d. Memoriza el nuevo orden.", gEnergia.level);
            PushNarrative(buf);
            std::cout << "[Simon] NIVEL " << gEnergia.level
                      << "  Score=" << gEnergia.score
                      << "  Aciertos=" << gEnergia.hits
                      << "  MaxCombo=" << gEnergia.maxCombo << std::endl;
            // El reactor de la estacion se da por estable al alcanzar nivel 3.
            // Llena la barra de energia y palomea la tarea del manual.
            if (!gStats.simonDone && gEnergia.level >= 3) {
                gStats.simonDone = true;
                gStats.energia   = 1.0f;
                PushNarrative("Reactor estable. Energia de la base al 100%.");
                StopActiveMinigames();
            }
        } else {
            PushNarrative("Vas bien, sigue la secuencia.");
        }
    }
    else {
        gEnergia.misses++;
        gEnergia.combo = 0;
        char buf[120];
        snprintf(buf, sizeof(buf),
                 "Fallaste. Esperaba %s. Aciertos %d / Fallos %d / Score %d.",
                 SIMON_NAMES[expected], gEnergia.hits, gEnergia.misses, gEnergia.score);
        PushNarrative(buf);
        std::cout << "[Simon] FALLO. Esperaba " << SIMON_NAMES[expected]
                  << " y se presiono " << SIMON_NAMES[button]
                  << ".  Score final=" << gEnergia.score
                  << "  Hits=" << gEnergia.hits
                  << "  Misses=" << gEnergia.misses
                  << "  MaxCombo=" << gEnergia.maxCombo << std::endl;
        gEnergia.phase = 3;
        gEnergia.phaseTimer = 1.5f;
    }
}

void UpdateEnergiaGame(float dt) {
    // Decay mas lento que antes (2.5 -> 1.4/s) para que la membrana del
    // boton pulsado por el jugador linguer visiblemente. La fase de
    // mostrar patron sobrescribe este valor con 1.0 durante la ventana
    // "on" para que el patron NO desaparezca a mitad de cada beat.
    for (int i = 0; i < 6; i++) {
        gEnergia.buttonGlow[i] -= dt * 1.4f;
        if (gEnergia.buttonGlow[i] < 0.0f) gEnergia.buttonGlow[i] = 0.0f;
    }

    if (gEnergia.phase == 1) {
        // Fase de mostrar patron: ilumina uno por uno.
        // velocidad de showing segun dificultad. Subimos onTime para que
        // el jugador VEA con claridad que boton se prendio en lugar de
        // un parpadeo de medio segundo apenas iluminado.
        float onTime, offTime;
        switch (gEnergia.difficulty) {
        case DIFF_EASY:   onTime = 0.75f; offTime = 0.35f; break;
        case DIFF_MEDIUM: onTime = 0.55f; offTime = 0.25f; break;
        case DIFF_EXPERT: onTime = 0.35f; offTime = 0.15f; break;
        default:          onTime = 0.75f; offTime = 0.35f; break;
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

        // Mientras phaseTimer > offTime estamos en la VENTANA ON del
        // boton actual. Forzamos su glow a 1.0 cada frame para que el
        // boton no se apague a medias por el decay. Cuando phaseTimer
        // cae por debajo de offTime, ya estamos en el "gap" entre
        // botones y dejamos que decaiga normalmente.
        if (gEnergia.showIndex >= 0 &&
            gEnergia.showIndex < (int)gEnergia.pattern.size() &&
            gEnergia.phaseTimer > offTime) {
            int idx = gEnergia.pattern[gEnergia.showIndex];
            gEnergia.buttonGlow[idx] = 1.0f;
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

// HUD del minijuego Simon. Aparece solo mientras gEnergia.active == true.
// Mantiene el mismo estilo (panel oscuro con borde dorado + texto blanco)
// que DrawEspectroHUD para que los 4 minijuegos se sientan parte de la
// misma familia. Muestra: nivel actual, score, hits, fallos, combo, mejor
// racha, fase activa y el "color objetivo" durante la fase de input.
void DrawEnergiaHUD() {
    if (!hudShader) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float panelW = 540.0f;
    if (panelW > (float)SCR_WIDTH - 80.0f) panelW = (float)SCR_WIDTH - 80.0f;
    float panelH = 132.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f;
    float panelY = (float)SCR_HEIGHT - panelH - 26.0f;

    // Marco oscuro con borde dorado superior/inferior (igual que Espectro).
    DrawQuad2D(panelX - 6.0f, panelY - 6.0f, panelW + 12.0f, panelH + 12.0f,
               glm::vec4(0.03f, 0.05f, 0.10f, 0.85f));
    DrawQuad2D(panelX - 6.0f, panelY + panelH + 3.0f, panelW + 12.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));
    DrawQuad2D(panelX - 6.0f, panelY - 6.0f, panelW + 12.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));

    // Fila de chips de color (4 o 6 segun dificultad): se ilumina el que
    // esta activo en la fase de "mostrar patron" y los pulsados por el
    // jugador. Para no rebasar el panel cuando son 6, achicamos el ancho
    // del chip y reducimos la separacion.
    int nChips = SimonActiveButtons();
    float chipW = (nChips == 4) ? 80.0f : 70.0f;
    float chipH = 28.0f;
    float chipGap = (nChips == 4) ? 16.0f : 10.0f;
    float chipsTotal = chipW * (float)nChips + chipGap * (float)(nChips - 1);
    float chipX0 = panelX + (panelW - chipsTotal) * 0.5f;
    float chipY  = panelY + panelH - chipH - 14.0f;
    float tNow = (float)glfwGetTime();
    for (int i = 0; i < nChips; ++i) {
        float glow = gEnergia.buttonGlow[i];
        float pulse = 0.55f + 0.20f * sinf(tNow * 2.2f + i * 1.3f);
        glm::vec4 base = SIMON_COLORS[i];
        glm::vec4 bg(base.r * (0.35f + glow * 0.65f),
                     base.g * (0.35f + glow * 0.65f),
                     base.b * (0.35f + glow * 0.65f),
                     0.55f + glow * 0.40f);
        float cx = chipX0 + i * (chipW + chipGap);
        DrawQuad2D(cx, chipY, chipW, chipH, bg);
        // Borde brillante cuando glow alto.
        float edge = glm::clamp(glow * 0.9f + 0.18f * pulse, 0.0f, 1.0f);
        DrawQuad2D(cx, chipY + chipH - 3.0f, chipW, 3.0f,
                   glm::vec4(base.r, base.g, base.b, edge));
        DrawQuad2D(cx, chipY, chipW, 3.0f,
                   glm::vec4(base.r, base.g, base.b, edge));
        if (gText && gText->ready) {
            // Nombre + tecla rapida dentro del chip ("ROJO  Z").
            char buf[24];
            snprintf(buf, sizeof(buf), "%s %s", SIMON_NAMES[i], SIMON_KEY_HINT[i]);
            float scale = 0.32f;
            float w = gText->MeasureWidth(buf, scale);
            gText->RenderText(buf, cx + (chipW - w) * 0.5f, chipY + 7.0f,
                              scale, glm::vec3(0.98f, 0.98f, 1.00f));
        }
    }

    if (!gText || !gText->ready) {
        glEnable(GL_DEPTH_TEST);
        return;
    }

    // Linea 1: nivel, score, hits/misses, combo.
    char line1[160];
    int patLen = (int)gEnergia.pattern.size();
    snprintf(line1, sizeof(line1),
             "Nivel %d  |  Score %d  |  Aciertos %d  |  Fallos %d  |  Combo %d / Mejor %d",
             gEnergia.level, gEnergia.score, gEnergia.hits, gEnergia.misses,
             gEnergia.combo, gEnergia.maxCombo);
    float s1 = 0.36f;
    float w1 = gText->MeasureWidth(line1, s1);
    gText->RenderText(line1, panelX + (panelW - w1) * 0.5f, panelY + 56.0f,
                      s1, glm::vec3(0.96f, 0.96f, 1.0f));

    // Linea 2: estado / pista contextual segun fase.
    char line2[200];
    if (gEnergia.phase == 1) {
        int shown = gEnergia.showIndex + 1;
        if (shown < 0) shown = 0;
        if (shown > patLen) shown = patLen;
        snprintf(line2, sizeof(line2),
                 "MEMORIZA  -  Mostrando patron (%d/%d)   |   TAB para salir",
                 shown, patLen);
    } else if (gEnergia.phase == 2) {
        int next = gEnergia.playerIndex;
        const char* nextName = (next >= 0 && next < patLen)
            ? SIMON_NAMES[gEnergia.pattern[next]] : "-";
        snprintf(line2, sizeof(line2),
                 "TU TURNO  -  Paso %d/%d   Pulsa %s (clic o tecla)   |   TAB para salir",
                 next + 1, patLen, nextName);
    } else if (gEnergia.phase == 3) {
        snprintf(line2, sizeof(line2),
                 "FALLO  -  Reiniciando patron... (Mejor nivel %d)", gEnergia.bestLevel);
    } else {
        snprintf(line2, sizeof(line2),
                 "Pulsa B para empezar  |  TAB para salir");
    }
    float s2 = 0.32f;
    float w2 = gText->MeasureWidth(line2, s2);
    glm::vec3 stateCol = (gEnergia.phase == 3)
        ? glm::vec3(1.00f, 0.45f, 0.40f)
        : (gEnergia.phase == 2 ? glm::vec3(0.55f, 1.00f, 0.65f)
                                : glm::vec3(0.75f, 0.82f, 0.96f));
    gText->RenderText(line2, panelX + (panelW - w2) * 0.5f, panelY + 30.0f,
                      s2, stateCol);

    // Footer corto centrado: contador "boton X de N" estilo OSU.
    if (gEnergia.phase == 2 || gEnergia.phase == 1) {
        char line3[80];
        snprintf(line3, sizeof(line3), "Patron actual: %d luces", patLen);
        float s3 = 0.30f;
        float w3 = gText->MeasureWidth(line3, s3);
        gText->RenderText(line3, panelX + (panelW - w3) * 0.5f, panelY + 8.0f,
                          s3, glm::vec3(0.62f, 0.70f, 0.85f));
    }

    glEnable(GL_DEPTH_TEST);
}

void DrawEnergiaGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!CUBO_MODELO) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    // ============ Cubo de tapa sobre cada boton activo del Simon ============
    // Solido opaco, ligeramente offset hacia el jugador (-X). buttonGlow
    // modula brillo + infle del cubo cuando se ilumina en el patron.
    float t = (float)glfwGetTime();

    int nBtns = SimonActiveButtons();
    for (int i = 0; i < nBtns; ++i) {
        glm::vec3 pos = SimonButtonPos(i);
        float glow = gEnergia.buttonGlow[i];
        float pulse = 0.55f + 0.20f * sinf(t * 2.4f + i * 1.3f);
        glm::vec4 base = SIMON_COLORS[i];

        // Color BRILLO: la base constante baja 20% (estaba pasando a
        // blanco quemado en reposo). El boost de glow se conserva
        // intacto para que el destello al sonar / pulsar siga siendo
        // muy notorio sobre la base mas tenue.
        glm::vec4 diff = base * (11.52f + pulse * 1.44f + glow * 16.20f);
        diff.a = 1.0f;
        glm::vec4 amb  = base * (8.64f + 0.72f * pulse + glow * 8.10f);
        amb.a = 1.0f;
        glm::vec4 spec(0.95f, 0.95f, 0.95f, 1.0f);
        SetMaterial(phongShader, amb, diff, spec, 32);
        phongShader->setFloat("transparency", 1.0f);

        // Cubo de tapa: 4.5 cm de arista (0.8x del anterior 5.6 cm).
        // Sigue colocado sobre el boton fisico (6 cm) pero ya queda
        // mas como una "luz incrustada" que como una tapa.
        float size = 0.045f + 0.0024f * pulse + glow * 0.0032f;
        // Empujamos el cubo 3.5 cm hacia el jugador (-X) para que la
        // cara trasera del cubo quede pegada a la cara visible del
        // boton fisico (x = 5.076). Asi el cubo coincide en tamano y
        // posicion con el boton real sin flotar ni hundirse en la pila.
        glm::vec3 capOffset(-0.035f, 0.0f, 0.0f);

        glm::mat4 m(1.0f);
        m = glm::translate(m, pos + capOffset);
        m = glm::scale(m, glm::vec3(size));
        phongShader->setMat4("model", m);
        CUBO_MODELO->Draw(*phongShader);
    }
}

// ============ RECICLAJE ============
// Minijuego de "ordenar el almacen": el jugador recoge cajas tiradas
// por el piso (con E si esta cerca de una) y las apila en el cuadro
// dorado del centro. Si suelta fuera del cuadro la caja queda en el
// piso (desorden!). Al apilar todas: tarea recycleDone + bono a las
// barras de energia y oxigeno.

// Predicado: el jugador esta sobre el cuadro de apilado?
static bool PlayerInStackZone() {
    float dx = player.position.x - RECICLAJE_STACK_CENTER.x;
    float dz = player.position.z - RECICLAJE_STACK_CENTER.z;
    return (dx >= -RECICLAJE_STACK_HALF_X && dx <= RECICLAJE_STACK_HALF_X &&
            dz >= -RECICLAJE_STACK_HALF_Z && dz <= RECICLAJE_STACK_HALF_Z);
}

// Encuentra la caja sin apilar mas cercana al jugador dentro del rango
// de pickup. Devuelve -1 si no hay ninguna a tiro.
static int NearestPickableBox() {
    int best = -1;
    float bestD2 = RECICLAJE_PICKUP_RANGE * RECICLAJE_PICKUP_RANGE;
    for (int i = 0; i < (int)gReciclaje.boxes.size(); ++i) {
        const ReciclajeBox& b = gReciclaje.boxes[i];
        if (b.placed) continue;
        float dx = b.pos.x - player.position.x;
        float dz = b.pos.z - player.position.z;
        float d2 = dx * dx + dz * dz;
        if (d2 < bestD2) { bestD2 = d2; best = i; }
    }
    return best;
}

void InitReciclajeGame() {
    gReciclaje.boxes.clear();
    gReciclaje.placedCount   = 0;
    gReciclaje.carrying      = -1;
    gReciclaje.score         = 0;
    gReciclaje.drops         = 0;
    gReciclaje.bestStack     = 0;
    gReciclaje.layerAnim     = 0.0f;
    gReciclaje.feedbackTimer = 0.0f;
    gReciclaje.feedbackKind  = 0;
    gReciclaje.finished      = false;
    gReciclaje.failedByTime  = false;

    int total;
    switch (gReciclaje.difficulty) {
    case DIFF_EASY:   total = 5;  gReciclaje.timeLimit = 0.0f;  break;
    case DIFF_MEDIUM: total = 8;  gReciclaje.timeLimit = 45.0f; break;
    case DIFF_EXPERT: total = 12; gReciclaje.timeLimit = 45.0f; break;
    default:          total = 5;  gReciclaje.timeLimit = 0.0f;  break;
    }
    gReciclaje.timeLeft = gReciclaje.timeLimit;

    // Cajas dispersas por el piso, evitando el cuadro de apilado (radio
    // de seguridad ~1.4 m) y manteniendo cierta separacion entre cajas
    // para que no aparezcan amontonadas en el mismo punto. Seed fijo
    // para que las posiciones sean reproducibles entre partidas de la
    // misma dificultad.
    srand(98765 + (int)gReciclaje.difficulty * 11);
    int attemptsTotal = 0;
    while ((int)gReciclaje.boxes.size() < total && attemptsTotal < 400) {
        attemptsTotal++;
        // Area util de la sala (X 6.5..12.0, Z -3.0..3.0).
        float fx = 6.5f + (rand() / (float)RAND_MAX) * 5.5f;
        float fz = -3.0f + (rand() / (float)RAND_MAX) * 6.0f;
        // Evita el cuadro de apilado.
        float dxs = fx - RECICLAJE_STACK_CENTER.x;
        float dzs = fz - RECICLAJE_STACK_CENTER.z;
        if (dxs * dxs + dzs * dzs < 1.6f * 1.6f) continue;
        // Evita encimarse con otra caja.
        bool tooClose = false;
        for (const auto& other : gReciclaje.boxes) {
            float dx = fx - other.pos.x;
            float dz = fz - other.pos.z;
            if (dx * dx + dz * dz < 0.55f * 0.55f) { tooClose = true; break; }
        }
        if (tooClose) continue;

        ReciclajeBox b;
        b.pos      = glm::vec3(fx, 0.18f, fz);
        b.color    = rand() % MAT_COUNT;
        b.placed   = false;
        b.stackIdx = -1;
        gReciclaje.boxes.push_back(b);
    }
    std::cout << "[Reciclaje] " << gReciclaje.boxes.size()
              << " cajas dispersas (dif " << gReciclaje.difficulty << ")" << std::endl;
}

void ReciclajeAction() {
    if (!gReciclaje.active || gReciclaje.finished) return;

    if (gReciclaje.carrying < 0) {
        // RECOGER: agarra la caja sin apilar mas cercana, si esta a tiro.
        int idx = NearestPickableBox();
        if (idx < 0) {
            gReciclaje.feedbackKind  = 3;
            gReciclaje.feedbackTimer = 0.45f;
            PushNarrative("No hay ninguna caja cerca. Acercate a una.");
            return;
        }
        gReciclaje.carrying = idx;
        char buf[80];
        snprintf(buf, sizeof(buf), "Caja %s recogida. Llevala al cuadro dorado.",
                 RECICLAJE_NAMES[gReciclaje.boxes[idx].color]);
        PushNarrative(buf);
        std::cout << "[Reciclaje] PICKUP idx=" << idx
                  << " color=" << RECICLAJE_NAMES[gReciclaje.boxes[idx].color] << std::endl;
        return;
    }

    // SOLTAR. Si el jugador esta dentro del cuadro, la caja se apila;
    // si no, se queda en el piso a los pies del jugador (desorden).
    int i = gReciclaje.carrying;
    if (PlayerInStackZone()) {
        gReciclaje.boxes[i].placed   = true;
        gReciclaje.boxes[i].stackIdx = gReciclaje.placedCount;
        gReciclaje.placedCount++;
        gReciclaje.carrying = -1;
        gReciclaje.layerAnim = 1.0f;
        gReciclaje.feedbackKind  = 1;
        gReciclaje.feedbackTimer = 0.5f;
        if (gReciclaje.placedCount > gReciclaje.bestStack)
            gReciclaje.bestStack = gReciclaje.placedCount;
        // Score base + bonus por apilar sin "desorden".
        gReciclaje.score += 120 + std::max(0, 20 - gReciclaje.drops * 5);
        std::cout << "[Reciclaje] STACK " << gReciclaje.placedCount
                  << "/" << gReciclaje.boxes.size()
                  << "  drops=" << gReciclaje.drops << std::endl;

        if (gReciclaje.placedCount >= (int)gReciclaje.boxes.size()) {
            gReciclaje.finished = true;
            if (!gStats.recycleDone) {
                gStats.recycleDone = true;
                gStats.energia = std::min(1.0f, gStats.energia + 0.30f);
                gStats.oxigeno = std::min(1.0f, gStats.oxigeno + 0.20f);
            }
            char buf[160];
            snprintf(buf, sizeof(buf),
                     "Almacen ordenado! Apiladas %d cajas con %d caidas. Score %d.",
                     gReciclaje.placedCount, gReciclaje.drops, gReciclaje.score);
            PushNarrative(buf);
            std::cout << "[Reciclaje] COMPLETA. Score=" << gReciclaje.score
                      << "  Drops=" << gReciclaje.drops << std::endl;
            StopActiveMinigames();
        } else {
            char buf[120];
            int falta = (int)gReciclaje.boxes.size() - gReciclaje.placedCount;
            snprintf(buf, sizeof(buf),
                     "Apilada (%d/%d). Quedan %d cajas por levantar.",
                     gReciclaje.placedCount, (int)gReciclaje.boxes.size(), falta);
            PushNarrative(buf);
        }
    } else {
        // Fuera de zona: la caja vuelve al piso a los pies del jugador.
        gReciclaje.boxes[i].pos = glm::vec3(player.position.x, 0.18f, player.position.z);
        gReciclaje.carrying = -1;
        gReciclaje.drops++;
        gReciclaje.feedbackKind  = 2;
        gReciclaje.feedbackTimer = 0.55f;
        PushNarrative("Caja soltada fuera del cuadro. Hay que llevarla al cuadro dorado.");
        std::cout << "[Reciclaje] DROP fuera de zona. drops=" << gReciclaje.drops << std::endl;
    }
}

void UpdateReciclajeGame(float dt) {
    if (!gReciclaje.active) return;
    if (gReciclaje.layerAnim > 0.0f) {
        gReciclaje.layerAnim -= dt * 2.8f;
        if (gReciclaje.layerAnim < 0.0f) gReciclaje.layerAnim = 0.0f;
    }
    if (gReciclaje.feedbackTimer > 0.0f) {
        gReciclaje.feedbackTimer -= dt;
        if (gReciclaje.feedbackTimer <= 0.0f) {
            gReciclaje.feedbackTimer = 0.0f;
            gReciclaje.feedbackKind  = 0;
        }
    }
    // Reloj solo en MEDIO/EXPERTO. Timeout = fallo; progreso visible.
    if (!gReciclaje.finished && gReciclaje.timeLimit > 0.0f) {
        gReciclaje.timeLeft -= dt;
        if (gReciclaje.timeLeft <= 0.0f) {
            gReciclaje.timeLeft = 0.0f;
            gReciclaje.failedByTime = true;
            gReciclaje.finished = true;
            PushNarrative("Se acabo el tiempo. Pulsa B o R para reintentar.");
            std::cout << "[Reciclaje] TIMEOUT. Apiladas=" << gReciclaje.placedCount
                      << " de " << gReciclaje.boxes.size() << std::endl;
        }
    }
}

void DrawReciclajeGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!CAJA_MODELO) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    float tNow = (float)glfwGetTime();

    // (1) Cuadro dorado pulsante en el suelo: marca la zona de apilado.
    // Usamos el cubo escalado muy plano. El color pulsa entre dorado y
    // verde-amarillo segun si el jugador esta dentro o lleva una caja.
    Model* baseModel = CUBO_MODELO ? CUBO_MODELO : CAJA_MODELO;
    if (baseModel) {
        bool playerIn = PlayerInStackZone();
        bool carrying = (gReciclaje.carrying >= 0);
        float pulse = 0.55f + 0.45f * sinf(tNow * 2.8f);
        glm::vec4 cBase = (playerIn && carrying)
            ? glm::vec4(0.40f, 0.95f, 0.45f, 1.0f)   // verde: listo para soltar
            : glm::vec4(0.95f, 0.78f, 0.30f, 1.0f);  // dorado: zona objetivo
        glm::vec4 amb = cBase * (0.45f + 0.15f * pulse); amb.a = 1.0f;
        glm::vec4 diff = cBase * (0.85f + 0.15f * pulse); diff.a = 1.0f;
        SetMaterial(phongShader, amb, diff, glm::vec4(0.30f), 16);
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(RECICLAJE_STACK_CENTER.x,
                                        0.02f,
                                        RECICLAJE_STACK_CENTER.z));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(RECICLAJE_STACK_HALF_X * 2.0f,
                                    RECICLAJE_STACK_HALF_Z * 2.0f,
                                    0.04f));
        phongShader->setMat4("model", m);
        baseModel->Draw(*phongShader);
    }

    // (2) Cajas en el piso (no apiladas y no cargadas). Cada una se
    // dibuja en su posicion actual; las que estan dentro del rango de
    // pickup reciben un leve "lift" + brillo para sugerir interaccion.
    int nearest = -1;
    if (gReciclaje.active && gReciclaje.carrying < 0) nearest = NearestPickableBox();
    for (int i = 0; i < (int)gReciclaje.boxes.size(); ++i) {
        const ReciclajeBox& b = gReciclaje.boxes[i];
        if (b.placed) continue;
        if (i == gReciclaje.carrying) continue;
        glm::vec4 base = RECICLAJE_COLORS[b.color];
        glm::vec4 amb  = base * 0.40f; amb.a = 1.0f;
        glm::vec4 diff = base * 1.00f; diff.a = 1.0f;
        glm::vec4 spec = glm::vec4(0.55f);
        if (i == nearest) {
            // Caja a tiro: highlight blanco para que se distinga.
            float k = 0.6f + 0.4f * sinf(tNow * 5.0f);
            amb  = glm::mix(amb,  glm::vec4(1.0f, 1.0f, 0.7f, 1.0f), 0.30f * k);
            diff = glm::mix(diff, glm::vec4(1.0f, 1.0f, 0.7f, 1.0f), 0.30f * k);
        }
        SetMaterial(phongShader, amb, diff, spec, 24);

        float liftY = (i == nearest)
            ? (0.04f + 0.04f * sinf(tNow * 5.0f))
            : 0.0f;
        glm::mat4 m(1.0f);
        m = glm::translate(m, b.pos + glm::vec3(0.0f, liftY, 0.0f));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(RECICLAJE_BOX_SCALE));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }

    // (3) Caja cargada: flotando justo enfrente y arriba del jugador.
    if (gReciclaje.carrying >= 0) {
        int idx = gReciclaje.carrying;
        const ReciclajeBox& b = gReciclaje.boxes[idx];
        glm::vec4 base = RECICLAJE_COLORS[b.color];
        glm::vec4 amb  = base * 0.45f; amb.a = 1.0f;
        glm::vec4 diff = base * 1.05f; diff.a = 1.0f;
        SetMaterial(phongShader, amb, diff, glm::vec4(0.65f), 32);

        // Posicion encima de la cabeza, con un pequeno bobbing.
        float bob = 0.04f * sinf(tNow * 3.5f);
        glm::vec3 p = player.position + glm::vec3(0.0f, 1.85f + bob, 0.0f);
        glm::mat4 m(1.0f);
        m = glm::translate(m, p);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, tNow * 0.6f, glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, glm::vec3(RECICLAJE_BOX_SCALE));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }

    // (4) Cajas ya apiladas dentro del cuadro. La ultima recibe un drop
    // proporcional a layerAnim para que se vea aterrizando.
    for (int i = 0; i < (int)gReciclaje.boxes.size(); ++i) {
        const ReciclajeBox& b = gReciclaje.boxes[i];
        if (!b.placed) continue;
        glm::vec4 base = RECICLAJE_COLORS[b.color];
        glm::vec4 amb  = base * 0.40f; amb.a = 1.0f;
        glm::vec4 diff = base * 1.00f; diff.a = 1.0f;
        SetMaterial(phongShader, amb, diff, glm::vec4(0.55f), 24);

        bool isTop = (b.stackIdx == gReciclaje.placedCount - 1);
        float drop = isTop ? gReciclaje.layerAnim * 0.25f : 0.0f;
        glm::vec3 p = RECICLAJE_STACK_CENTER +
                      glm::vec3(0.0f, b.stackIdx * RECICLAJE_LAYER_H + drop, 0.0f);
        glm::mat4 m(1.0f);
        m = glm::translate(m, p);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(RECICLAJE_BOX_SCALE));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }
}

// HUD del minijuego de reciclaje. Mismo estilo (panel oscuro con borde
// dorado + linea de estado) que el HUD de hamburguesa y simon, para
// que el modulo se sienta parte del mismo set.
void DrawReciclajeHUD() {
    if (!hudShader) return;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float panelW = 620.0f;
    if (panelW > (float)SCR_WIDTH - 80.0f) panelW = (float)SCR_WIDTH - 80.0f;
    float panelH = 118.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f;
    float panelY = (float)SCR_HEIGHT - panelH - 26.0f;

    DrawQuad2D(panelX - 6.0f, panelY - 6.0f, panelW + 12.0f, panelH + 12.0f,
               glm::vec4(0.03f, 0.05f, 0.10f, 0.85f));
    DrawQuad2D(panelX - 6.0f, panelY + panelH + 3.0f, panelW + 12.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));
    DrawQuad2D(panelX - 6.0f, panelY - 6.0f, panelW + 12.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));

    int total = (int)gReciclaje.boxes.size();
    int placed = gReciclaje.placedCount;
    float tNow = (float)glfwGetTime();

    // Barra de progreso: relleno proporcional a placed/total con un
    // tinte dorado que pulsa cuando el jugador esta dentro del cuadro.
    float barX = panelX + 20.0f;
    float barW = panelW - 40.0f;
    float barH = 16.0f;
    float barY = panelY + panelH - barH - 14.0f;
    DrawQuad2D(barX, barY, barW, barH, glm::vec4(0.10f, 0.13f, 0.18f, 0.85f));
    float frac = (total > 0) ? (float)placed / (float)total : 0.0f;
    if (frac > 0.0f) {
        float pulse = 0.85f + 0.15f * sinf(tNow * 3.0f);
        glm::vec4 fill = glm::vec4(0.30f, 0.85f, 0.45f, 1.0f) * pulse;
        fill.a = 1.0f;
        DrawQuad2D(barX, barY, barW * frac, barH, fill);
    }

    if (!gText || !gText->ready) {
        glEnable(GL_DEPTH_TEST);
        return;
    }

    // Linea 1: contadores principales + reloj cuando aplica.
    char line1[220];
    if (gReciclaje.timeLimit > 0.0f) {
        snprintf(line1, sizeof(line1),
                 "Apiladas %d/%d  |  Score %d  |  Desorden %d  |  Mejor torre %d  |  Tiempo %.1fs",
                 placed, total, gReciclaje.score,
                 gReciclaje.drops, gReciclaje.bestStack,
                 gReciclaje.timeLeft);
    } else {
        snprintf(line1, sizeof(line1),
                 "Apiladas %d/%d  |  Score %d  |  Desorden %d  |  Mejor torre %d",
                 placed, total, gReciclaje.score,
                 gReciclaje.drops, gReciclaje.bestStack);
    }
    float s1 = 0.30f;
    float w1 = gText->MeasureWidth(line1, s1);
    if (w1 > panelW - 20.0f) { s1 = 0.26f; w1 = gText->MeasureWidth(line1, s1); }
    gText->RenderText(line1, panelX + (panelW - w1) * 0.5f, panelY + 56.0f,
                      s1, glm::vec3(0.96f, 0.96f, 1.0f));

    // Linea 2: pista contextual segun el estado actual.
    char line2[220];
    glm::vec3 stateCol = glm::vec3(0.75f, 0.82f, 0.96f);
    if (gReciclaje.finished && !gReciclaje.failedByTime) {
        snprintf(line2, sizeof(line2),
                 "ALMACEN ORDENADO  -  Score final %d   |   TAB para cerrar",
                 gReciclaje.score);
        stateCol = glm::vec3(0.60f, 0.95f, 0.65f);
    } else if (gReciclaje.failedByTime) {
        snprintf(line2, sizeof(line2),
                 "TIEMPO AGOTADO  -  Pulsa B o R para reintentar");
        stateCol = glm::vec3(0.98f, 0.55f, 0.45f);
    } else if (gReciclaje.feedbackKind == 2) {
        snprintf(line2, sizeof(line2),
                 "Caja fuera del cuadro: vuelvela al cuadro dorado!");
        stateCol = glm::vec3(0.98f, 0.55f, 0.45f);
    } else if (gReciclaje.feedbackKind == 3) {
        snprintf(line2, sizeof(line2),
                 "Nada que recoger: acercate a una caja del piso");
        stateCol = glm::vec3(0.98f, 0.78f, 0.45f);
    } else if (gReciclaje.carrying >= 0) {
        bool inZone = PlayerInStackZone();
        if (inZone) {
            snprintf(line2, sizeof(line2),
                     "Dentro del cuadro: pulsa E para soltar la caja");
            stateCol = glm::vec3(0.60f, 0.95f, 0.65f);
        } else {
            int color = gReciclaje.boxes[gReciclaje.carrying].color;
            snprintf(line2, sizeof(line2),
                     "Llevas %s: camina al cuadro dorado y pulsa E",
                     RECICLAJE_NAMES[color]);
        }
    } else if (placed < total) {
        snprintf(line2, sizeof(line2),
                 "Camina hacia una caja del piso y pulsa E para recogerla");
    } else {
        snprintf(line2, sizeof(line2), "Listo");
    }
    float s2 = 0.30f;
    float w2 = gText->MeasureWidth(line2, s2);
    if (w2 > panelW - 20.0f) { s2 = 0.26f; w2 = gText->MeasureWidth(line2, s2); }
    gText->RenderText(line2, panelX + (panelW - w2) * 0.5f, panelY + 28.0f,
                      s2, stateCol);

    glEnable(GL_DEPTH_TEST);
}

// ============ HAMBURGUESA (CAPSULAS) ============
// Armado de hamburguesa por capas (T Y U I O P en orden). MEDIO/EXPERTO
// tienen reloj. Al completar las 6 capas: foodPrepared=true, agua y
// comida saltan a 1.0 y la hamburguesa queda visible sobre la mesa.

void InitHamburguesaGame() {
    gHamburguesa.step       = 0;
    gHamburguesa.score      = 0;
    gHamburguesa.hits       = 0;
    gHamburguesa.misses     = 0;
    gHamburguesa.combo      = 0;
    gHamburguesa.maxCombo   = 0;
    gHamburguesa.bestStep   = 0;
    gHamburguesa.feedbackTimer = 0.0f;
    gHamburguesa.lastWrong  = false;
    gHamburguesa.lastWrongIng = -1;
    gHamburguesa.finished   = false;
    gHamburguesa.failedByTime = false;
    for (int i = 0; i < HAMB_ING_COUNT; ++i) gHamburguesa.layerAnim[i] = 0.0f;

    // Reloj segun dificultad (0 = sin reloj). En FACIL es solo practicar
    // la receta; en MEDIO/EXPERTO la dinamica se aprieta para que el
    // armado se sienta como un servicio rapido.
    switch (gHamburguesa.difficulty) {
    case DIFF_EASY:   gHamburguesa.timeLimit = 0.0f;  break;
    case DIFF_MEDIUM: gHamburguesa.timeLimit = 18.0f; break;
    case DIFF_EXPERT: gHamburguesa.timeLimit = 10.0f; break;
    default:          gHamburguesa.timeLimit = 0.0f;  break;
    }
    gHamburguesa.timeLeft = gHamburguesa.timeLimit;
}

void HamburguesaPress(int ingredient) {
    if (!gHamburguesa.active) return;
    if (gHamburguesa.finished) return;
    if (ingredient < 0 || ingredient >= HAMB_ING_COUNT) return;

    int expected = gHamburguesa.step;
    if (ingredient == expected) {
        // Acierto: apila la capa y avanza la receta.
        gHamburguesa.layerAnim[ingredient] = 1.0f;
        gHamburguesa.step++;
        gHamburguesa.hits++;
        gHamburguesa.combo++;
        if (gHamburguesa.combo > gHamburguesa.maxCombo)
            gHamburguesa.maxCombo = gHamburguesa.combo;
        if (gHamburguesa.step > gHamburguesa.bestStep)
            gHamburguesa.bestStep = gHamburguesa.step;
        // Score base 100 + bonus de combo (la receta es solo 6 pasos asi
        // que damos mas peso por acierto que en Simon, sin pasar de 200).
        gHamburguesa.score += 100 + gHamburguesa.combo * 10;
        gHamburguesa.lastWrong = false;
        std::cout << "[Hamburguesa] OK " << HAMB_NAMES[ingredient]
                  << "  (" << gHamburguesa.step << "/" << HAMB_ING_COUNT << ")"
                  << "  combo=" << gHamburguesa.combo << std::endl;

        if (gHamburguesa.step >= HAMB_ING_COUNT) {
            // Hamburguesa terminada: palomea la tarea de comida y llena
            // las barras de agua y comida (misma recompensa que tenia el
            // cooking timer original).
            gHamburguesa.finished = true;
            if (!gStats.foodPrepared) {
                gStats.foodPrepared = true;
                gStats.agua   = 1.0f;
                gStats.comida = 1.0f;
            }
            char buf[120];
            snprintf(buf, sizeof(buf),
                     "Hamburguesa lista! Score %d, racha %d. Agua y comida al 100%%.",
                     gHamburguesa.score, gHamburguesa.maxCombo);
            PushNarrative(buf);
            std::cout << "[Hamburguesa] COMPLETA. Score=" << gHamburguesa.score
                      << "  MaxCombo=" << gHamburguesa.maxCombo
                      << "  Misses=" << gHamburguesa.misses << std::endl;
            StopActiveMinigames();
        } else {
            char buf[120];
            snprintf(buf, sizeof(buf),
                     "Bien! Siguiente capa: %s (tecla %s).",
                     HAMB_NAMES[gHamburguesa.step],
                     HAMB_KEY_HINT[gHamburguesa.step]);
            PushNarrative(buf);
        }
    } else {
        // Fallo: si la capa ya estaba colocada se llama "capa repetida".
        gHamburguesa.misses++;
        gHamburguesa.combo = 0;
        gHamburguesa.lastWrong = true;
        gHamburguesa.lastWrongIng = ingredient;
        gHamburguesa.feedbackTimer = 0.6f;
        const char* why = (ingredient < gHamburguesa.step)
            ? "esa capa ya esta puesta"
            : "todavia no toca esa capa";
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "Fallaste: %s, %s. Esperaba %s (tecla %s).",
                 HAMB_NAMES[ingredient], why,
                 HAMB_NAMES[expected], HAMB_KEY_HINT[expected]);
        PushNarrative(buf);
        std::cout << "[Hamburguesa] FALLO " << HAMB_NAMES[ingredient]
                  << " (esperaba " << HAMB_NAMES[expected] << ")."
                  << "  Misses=" << gHamburguesa.misses << std::endl;
    }
}

void UpdateHamburguesaGame(float dt) {
    if (!gHamburguesa.active) return;
    // Animacion de apilado: cada capa cae suavemente al colocarse (la
    // animacion va de 1.0 a 0.0 mientras "asienta" el ingrediente).
    for (int i = 0; i < HAMB_ING_COUNT; ++i) {
        if (gHamburguesa.layerAnim[i] > 0.0f) {
            gHamburguesa.layerAnim[i] -= dt * 2.5f;
            if (gHamburguesa.layerAnim[i] < 0.0f) gHamburguesa.layerAnim[i] = 0.0f;
        }
    }
    if (gHamburguesa.feedbackTimer > 0.0f) {
        gHamburguesa.feedbackTimer -= dt;
        if (gHamburguesa.feedbackTimer <= 0.0f) {
            gHamburguesa.feedbackTimer = 0.0f;
            gHamburguesa.lastWrong = false;
        }
    }
    // Reloj (solo MEDIO/EXPERTO). Si se agota antes de cerrar la receta,
    // la partida se cuenta como fallida: NO marcamos foodPrepared y
    // dejamos el progreso visible para que el jugador pueda reintentar.
    if (!gHamburguesa.finished && gHamburguesa.timeLimit > 0.0f) {
        gHamburguesa.timeLeft -= dt;
        if (gHamburguesa.timeLeft <= 0.0f) {
            gHamburguesa.timeLeft = 0.0f;
            gHamburguesa.failedByTime = true;
            gHamburguesa.finished = true;
            PushNarrative("Se acabo el tiempo. Pulsa B para reintentar.");
            std::cout << "[Hamburguesa] TIMEOUT. Capas colocadas="
                      << gHamburguesa.step << std::endl;
        }
    }
}

void DrawHamburguesaGame(const glm::mat4& projection, const glm::mat4& view) {
    if (!HAMB_PLATO && !HAMB_PAN_INFERIOR) return;
    phongShader->use();
    phongShader->setMat4("projection", projection);
    phongShader->setMat4("view", view);

    // (1) Plato siempre visible cuando hay minijuego activo, partida
    // pausada por TAB con progreso, o hamburguesa ya completada (queda
    // como prueba sobre la mesa). Asi el jugador siempre ve donde va a
    // armar el burger.
    if (HAMB_PLATO) {
        glm::vec3 platePos = HAMB_TABLE_TOP + glm::vec3(0.0f, HAMB_PLATE_LIFT, 0.0f);
        SetMaterial(phongShader,
                    glm::vec4(0.30f, 0.30f, 0.32f, 1.0f),
                    glm::vec4(0.92f, 0.92f, 0.95f, 1.0f),
                    glm::vec4(0.65f, 0.65f, 0.70f, 1.0f), 32);
        glm::mat4 m(1.0f);
        m = glm::translate(m, platePos);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(HAMB_MODEL_SCALE));
        phongShader->setMat4("model", m);
        HAMB_PLATO->Draw(*phongShader);
    }

    // (2) Capas apiladas. Se dibujan SOLO las que ya estan colocadas
    // (i < step). La animacion layerAnim[i] anade un pequeno salto al
    // momento de aterrizar (decay 2.5/seg en Update).
    float yAccum = HAMB_PLATE_LIFT + 0.01f; // espacio entre plato y base
    Model* layerModels[HAMB_ING_COUNT] = {
        HAMB_PAN_INFERIOR, HAMB_CARNE, HAMB_QUESO, HAMB_JITOMATE,
        HAMB_LECHUGA, HAMB_PAN_SUPERIOR
    };
    for (int i = 0; i < HAMB_ING_COUNT; ++i) {
        if (i >= gHamburguesa.step) break;
        if (!layerModels[i]) { yAccum += (HAMB_LAYER_H[i] * HAMB_MODEL_SCALE); continue; }

        float drop = gHamburguesa.layerAnim[i] * 0.08f;
        glm::vec3 pos = HAMB_TABLE_TOP + glm::vec3(0.0f, yAccum + drop, 0.0f);

        glm::vec4 amb  = HAMB_CHIP_COLORS[i] * 0.35f; amb.a = 1.0f;
        glm::vec4 diff = HAMB_CHIP_COLORS[i] * 0.95f; diff.a = 1.0f;
        SetMaterial(phongShader, amb, diff,
                    glm::vec4(0.55f, 0.55f, 0.55f, 1.0f), 16);

        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(HAMB_MODEL_SCALE));
        phongShader->setMat4("model", m);
        layerModels[i]->Draw(*phongShader);

        yAccum += (HAMB_LAYER_H[i] * HAMB_MODEL_SCALE);
    }
}

// HUD del minijuego de hamburguesa. Mismo estilo (panel oscuro con borde
// dorado + fila de chips de color + linea de estado) que Espectro y Simon
// para que el modulo de comida se sienta parte del mismo set.
void DrawHamburguesaHUD() {
    if (!hudShader) return;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float panelW = 620.0f;
    if (panelW > (float)SCR_WIDTH - 80.0f) panelW = (float)SCR_WIDTH - 80.0f;
    float panelH = 138.0f;
    float panelX = (SCR_WIDTH - panelW) * 0.5f;
    float panelY = (float)SCR_HEIGHT - panelH - 26.0f;

    DrawQuad2D(panelX - 6.0f, panelY - 6.0f, panelW + 12.0f, panelH + 12.0f,
               glm::vec4(0.03f, 0.05f, 0.10f, 0.85f));
    DrawQuad2D(panelX - 6.0f, panelY + panelH + 3.0f, panelW + 12.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));
    DrawQuad2D(panelX - 6.0f, panelY - 6.0f, panelW + 12.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));

    // Fila de chips por ingrediente. Cada chip muestra el nombre y la
    // tecla. El chip de la SIGUIENTE capa pulsa para guiar al jugador;
    // los chips de capas YA colocadas se ven mas claros (cumplidos).
    int n = HAMB_ING_COUNT;
    float chipW = 90.0f;
    float chipH = 30.0f;
    float chipGap = 6.0f;
    float chipsTotal = chipW * n + chipGap * (n - 1);
    if (chipsTotal > panelW - 20.0f) {
        chipW   = (panelW - 20.0f - chipGap * (n - 1)) / n;
        chipsTotal = chipW * n + chipGap * (n - 1);
    }
    float chipX0 = panelX + (panelW - chipsTotal) * 0.5f;
    float chipY  = panelY + panelH - chipH - 14.0f;
    float tNow = (float)glfwGetTime();
    for (int i = 0; i < n; ++i) {
        glm::vec4 base = HAMB_CHIP_COLORS[i];
        bool placed = (i < gHamburguesa.step);
        bool isNext = (i == gHamburguesa.step) && !gHamburguesa.finished;
        float pulse = 0.55f + 0.20f * sinf(tNow * 2.6f + i * 1.1f);

        glm::vec4 bg = base;
        if (placed) {
            bg.r *= 0.85f; bg.g *= 0.85f; bg.b *= 0.85f; bg.a = 0.85f;
        } else if (isNext) {
            float k = 0.70f + 0.30f * pulse;
            bg.r *= k; bg.g *= k; bg.b *= k;
            bg.a = 0.95f;
        } else {
            bg.r *= 0.30f; bg.g *= 0.30f; bg.b *= 0.30f; bg.a = 0.55f;
        }
        float cx = chipX0 + i * (chipW + chipGap);
        DrawQuad2D(cx, chipY, chipW, chipH, bg);
        // Borde superior/inferior con color saturado en el chip activo.
        float edge = isNext ? (0.55f + 0.45f * pulse) : (placed ? 0.45f : 0.18f);
        DrawQuad2D(cx, chipY + chipH - 3.0f, chipW, 3.0f,
                   glm::vec4(base.r, base.g, base.b, edge));
        DrawQuad2D(cx, chipY, chipW, 3.0f,
                   glm::vec4(base.r, base.g, base.b, edge));

        if (gText && gText->ready) {
            char buf[40];
            // Nombre + tecla compactos. En chips estrechos solo cabe la
            // tecla y el primer caracter del nombre, asi que abreviamos.
            const char* nm = HAMB_NAMES[i];
            char shortNm[14];
            // Tomar la primera palabra (hasta espacio) o primeras 8 letras.
            int k = 0;
            for (int c = 0; nm[c] && k < 12 && nm[c] != ' '; ++c) shortNm[k++] = nm[c];
            shortNm[k] = '\0';
            snprintf(buf, sizeof(buf), "%s %s", shortNm, HAMB_KEY_HINT[i]);
            float scale = 0.28f;
            float w = gText->MeasureWidth(buf, scale);
            // Achicar si no entra
            if (w > chipW - 6.0f) {
                scale = 0.24f;
                w = gText->MeasureWidth(buf, scale);
            }
            gText->RenderText(buf, cx + (chipW - w) * 0.5f, chipY + 8.0f,
                              scale, glm::vec3(0.98f, 0.98f, 1.00f));
        }
    }

    if (!gText || !gText->ready) {
        glEnable(GL_DEPTH_TEST);
        return;
    }

    // Linea 1: score / hits / misses / combo / progreso.
    char line1[200];
    if (gHamburguesa.timeLimit > 0.0f) {
        snprintf(line1, sizeof(line1),
                 "Capas %d/%d  |  Score %d  |  Aciertos %d  |  Fallos %d  |  Combo %d / Mejor %d  |  Tiempo %.1fs",
                 gHamburguesa.step, HAMB_ING_COUNT,
                 gHamburguesa.score, gHamburguesa.hits, gHamburguesa.misses,
                 gHamburguesa.combo, gHamburguesa.maxCombo,
                 gHamburguesa.timeLeft);
    } else {
        snprintf(line1, sizeof(line1),
                 "Capas %d/%d  |  Score %d  |  Aciertos %d  |  Fallos %d  |  Combo %d / Mejor %d",
                 gHamburguesa.step, HAMB_ING_COUNT,
                 gHamburguesa.score, gHamburguesa.hits, gHamburguesa.misses,
                 gHamburguesa.combo, gHamburguesa.maxCombo);
    }
    float s1 = 0.33f;
    float w1 = gText->MeasureWidth(line1, s1);
    if (w1 > panelW - 20.0f) { s1 = 0.28f; w1 = gText->MeasureWidth(line1, s1); }
    gText->RenderText(line1, panelX + (panelW - w1) * 0.5f, panelY + 56.0f,
                      s1, glm::vec3(0.96f, 0.96f, 1.0f));

    // Linea 2: estado / receta.
    char line2[200];
    glm::vec3 stateCol = glm::vec3(0.75f, 0.82f, 0.96f);
    if (gHamburguesa.finished && !gHamburguesa.failedByTime) {
        snprintf(line2, sizeof(line2),
                 "HAMBURGUESA LISTA  -  Score final %d   |   TAB para cerrar",
                 gHamburguesa.score);
        stateCol = glm::vec3(0.55f, 1.00f, 0.65f);
    } else if (gHamburguesa.failedByTime) {
        snprintf(line2, sizeof(line2),
                 "Tiempo agotado en la capa %d/%d   |   Pulsa B para reintentar",
                 gHamburguesa.step, HAMB_ING_COUNT);
        stateCol = glm::vec3(1.00f, 0.45f, 0.40f);
    } else if (gHamburguesa.lastWrong) {
        snprintf(line2, sizeof(line2),
                 "FALLO  -  Te toca la capa %s (tecla %s)",
                 HAMB_NAMES[gHamburguesa.step], HAMB_KEY_HINT[gHamburguesa.step]);
        stateCol = glm::vec3(1.00f, 0.55f, 0.45f);
    } else {
        snprintf(line2, sizeof(line2),
                 "RECETA  -  Siguiente capa: %s (tecla %s)   |   TAB para salir",
                 HAMB_NAMES[gHamburguesa.step], HAMB_KEY_HINT[gHamburguesa.step]);
    }
    float s2 = 0.32f;
    float w2 = gText->MeasureWidth(line2, s2);
    if (w2 > panelW - 20.0f) { s2 = 0.28f; w2 = gText->MeasureWidth(line2, s2); }
    gText->RenderText(line2, panelX + (panelW - w2) * 0.5f, panelY + 30.0f,
                      s2, stateCol);

    // Footer: orden de la receta resumido.
    char line3[160];
    snprintf(line3, sizeof(line3),
             "Receta: plato -> pan inf -> carne -> queso -> jitomate -> lechuga -> pan sup");
    float s3 = 0.26f;
    float w3 = gText->MeasureWidth(line3, s3);
    if (w3 > panelW - 20.0f) { s3 = 0.22f; w3 = gText->MeasureWidth(line3, s3); }
    gText->RenderText(line3, panelX + (panelW - w3) * 0.5f, panelY + 8.0f,
                      s3, glm::vec3(0.62f, 0.70f, 0.85f));

    glEnable(GL_DEPTH_TEST);
}

// ============ ESPECTROGRAFIA DEL METEORITO ============
// El minijuego "espectro" se desarrolla en la zona invernadero a la par del
// rytmo de plantas: sirve para que el jugador identifique las cuatro lineas
// espectrales mas comunes en astronomia. ESPECTRO_COLORS/NAMES estan
// declarados arriba para que SpawnMoonTrail pueda usarlos.

// Posicion en la barra (0..1) de una linea espectral concreta. La barra
// representa el espectro visible 400..700 nm con escala lineal en nm.
static float EspectroLinePos(int idx) {
    float wl = ESPECTRO_NM[idx];
    return glm::clamp((wl - 400.0f) / 300.0f, 0.0f, 1.0f);
}

// Selecciona una nueva linea objetivo distinta a la anterior y reinicia el
// indicador para que arranque desde el extremo opuesto. Asi cada ronda se
// siente como un barrido nuevo y el jugador no puede limitarse a soltar la
// tecla en el mismo punto del recorrido.
static void EspectroNextRound() {
    int prev = gEspectro.targetLine;
    int next;
    do { next = rand() % 4; } while (next == prev && gEspectro.roundIdx > 0);
    gEspectro.targetLine  = next;
    gEspectro.sweetCenter = EspectroLinePos(next);
    gEspectro.barPos      = (gEspectro.sweetCenter > 0.5f) ? 0.0f : 1.0f;
    gEspectro.barDir      = (gEspectro.sweetCenter > 0.5f) ? +1 : -1;
    gEspectro.stopped     = false;
    gEspectro.feedbackTimer = 0.0f;
}

void InitEspectroGame() {
    gEspectro.currentColor = 0;
    gEspectro.score        = 0;
    gEspectro.hits         = 0;
    gEspectro.misses       = 0;
    gEspectro.combo        = 0;
    gEspectro.maxCombo     = 0;
    gEspectro.roundIdx     = 0;
    gEspectro.stopped      = false;
    gEspectro.feedbackTimer = 0.0f;
    gEspectro.targetLine   = -1;

    // La dificultad ajusta dos cosas: cuantas rondas hay que jugar y, sobre
    // todo, cuanto mide el sweet spot. La velocidad del indicador tambien
    // sube en EXPERT para que la ventana de reaccion sea mas tensa.
    switch (gEspectro.difficulty) {
    case DIFF_EASY:
        gEspectro.rounds    = 5;
        gEspectro.sweetHalf = 0.060f; // ~12% de la barra
        gEspectro.barSpeed  = 0.45f;
        break;
    case DIFF_MEDIUM:
        gEspectro.rounds    = 7;
        gEspectro.sweetHalf = 0.035f; // ~7%
        gEspectro.barSpeed  = 0.75f;
        break;
    case DIFF_EXPERT:
        gEspectro.rounds    = 10;
        gEspectro.sweetHalf = 0.020f; // ~4%
        gEspectro.barSpeed  = 1.10f;
        break;
    default:
        gEspectro.rounds    = 5;
        gEspectro.sweetHalf = 0.060f;
        gEspectro.barSpeed  = 0.45f;
        break;
    }

    srand((unsigned)time(NULL));
    EspectroNextRound();
}

// Compatibilidad con el resto del codigo: si algo intenta fijar el color
// (p.ej. la estela del meteorito al spawnear), respetamos el indice. La
// mecanica nueva escribe currentColor desde UpdateEspectroGame cuando el
// jugador detiene la barra.
void EspectroSetColor(int idx) {
    if (!gEspectro.active) return;
    if (idx < 0 || idx > 3) return;
    gEspectro.currentColor = idx;
}

// Detiene el indicador y evalua si cayo dentro del sweet spot. La linea
// "donde cayo" se decide por proximidad: la que tenga su centro mas cercano
// a la posicion final del indicador determina el color del meteoro.
static void EspectroStopBar() {
    if (gEspectro.stopped) return;

    gEspectro.stopped = true;

    int closest = 0;
    float bestD = 999.0f;
    for (int i = 0; i < 4; ++i) {
        float d = std::fabs(gEspectro.barPos - EspectroLinePos(i));
        if (d < bestD) { bestD = d; closest = i; }
    }
    gEspectro.currentColor = closest;

    float diff = std::fabs(gEspectro.barPos - gEspectro.sweetCenter);
    bool hit  = (diff <= gEspectro.sweetHalf);
    gEspectro.lastHit = hit;

    if (hit) {
        gEspectro.hits++;
        gEspectro.combo++;
        if (gEspectro.combo > gEspectro.maxCombo) gEspectro.maxCombo = gEspectro.combo;
        // Cuanto mas cerca del centro, mas puntos. Bonus por combo encadenado.
        float accuracy = 1.0f - (diff / gEspectro.sweetHalf);
        int gained = 80 + (int)(accuracy * 70.0f) + gEspectro.combo * 10;
        gEspectro.score += gained;
        char buf[140];
        snprintf(buf, sizeof(buf),
                 "%s alineada (%.0f%%). Combo x%d. +%d pts.",
                 ESPECTRO_NAMES[gEspectro.targetLine],
                 accuracy * 100.0f, gEspectro.combo, gained);
        PushNarrative(buf);
    } else {
        gEspectro.combo = 0;
        gEspectro.misses++;
        char buf[140];
        snprintf(buf, sizeof(buf),
                 "Fuera del sweet spot. Caiste en %s, esperaba %s.",
                 ESPECTRO_NAMES[closest], ESPECTRO_NAMES[gEspectro.targetLine]);
        PushNarrative(buf);
    }

    // Pequena pausa para que se vea el resultado antes de pasar a la siguiente
    // ronda. Si era la ultima ronda, mostramos el resumen y dejamos la barra
    // congelada hasta que el jugador pulse TAB para salir.
    gEspectro.feedbackTimer = 1.0f;
    gEspectro.roundIdx++;
    if (gEspectro.roundIdx >= gEspectro.rounds) {
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "Espectro completo. Aciertos %d/%d. Score %d. Combo max %d.",
                 gEspectro.hits, gEspectro.rounds,
                 gEspectro.score, gEspectro.maxCombo);
        PushNarrative(buf);

        // Marca el cometa como terminado si tuvo mas aciertos que fallos.
        // (gStats.tareas se recalcula en DrawQuestsWidget como
        // doneCount/N_QUESTS, asi que aqui basta con setear el flag.)
        if (!gStats.cometDone && gEspectro.hits > gEspectro.misses) {
            gStats.cometDone = true;
            PushNarrative("Cometa catalogado. Tarea cientifica registrada.");
            StopActiveMinigames();
        }
    }
}

void UpdateEspectroGame(float dt) {
    // Si terminamos todas las rondas, no movemos mas la barra.
    if (gEspectro.roundIdx >= gEspectro.rounds) {
        if (gEspectro.feedbackTimer > 0.0f) gEspectro.feedbackTimer -= dt;
        return;
    }

    // Mientras se muestra el feedback de la ronda anterior, esperamos.
    if (gEspectro.feedbackTimer > 0.0f) {
        gEspectro.feedbackTimer -= dt;
        if (gEspectro.feedbackTimer <= 0.0f && gEspectro.stopped) {
            EspectroNextRound();
        }
        return;
    }

    // Ping-pong del indicador entre 0 y 1.
    if (!gEspectro.stopped) {
        gEspectro.barPos += gEspectro.barDir * gEspectro.barSpeed * dt;
        if (gEspectro.barPos >= 1.0f) {
            gEspectro.barPos = 1.0f;
            gEspectro.barDir = -1;
        } else if (gEspectro.barPos <= 0.0f) {
            gEspectro.barPos = 0.0f;
            gEspectro.barDir = +1;
        }
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

void DrawEspectroGame(const glm::mat4& /*projection*/, const glm::mat4& /*view*/) {
    // Sin halo 3D alrededor del meteoro: el feedback de la ronda vive entero
    // en el HUD (barra del espectro) y en el tinte del propio cometa, asi la
    // escena no se llena de cajas flotantes que distraen del cielo.
}

// Aproximacion sencilla de longitud de onda visible a RGB. Sirve para pintar
// la barra como un gradiente que el jugador reconoce como espectro visible
// (violeta -> azul -> verde -> amarillo -> naranja -> rojo). No pretende ser
// colorimetria exacta, solo una representacion clara para el HUD.
static glm::vec3 WavelengthToRGB(float wl) {
    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (wl >= 380.0f && wl < 440.0f) {
        r = -(wl - 440.0f) / (440.0f - 380.0f);
        g = 0.0f;
        b = 1.0f;
    } else if (wl < 490.0f) {
        r = 0.0f;
        g = (wl - 440.0f) / (490.0f - 440.0f);
        b = 1.0f;
    } else if (wl < 510.0f) {
        r = 0.0f;
        g = 1.0f;
        b = -(wl - 510.0f) / (510.0f - 490.0f);
    } else if (wl < 580.0f) {
        r = (wl - 510.0f) / (580.0f - 510.0f);
        g = 1.0f;
        b = 0.0f;
    } else if (wl < 645.0f) {
        r = 1.0f;
        g = -(wl - 645.0f) / (645.0f - 580.0f);
        b = 0.0f;
    } else if (wl <= 780.0f) {
        r = 1.0f;
        g = 0.0f;
        b = 0.0f;
    }
    // Atenua los extremos del visible que el ojo casi no detecta.
    float factor = 1.0f;
    if (wl < 420.0f)      factor = 0.3f + 0.7f * (wl - 380.0f) / 40.0f;
    else if (wl > 700.0f) factor = 0.3f + 0.7f * (780.0f - wl) / 80.0f;
    factor = glm::clamp(factor, 0.0f, 1.0f);
    return glm::vec3(r, g, b) * factor;
}

// Barra del espectro visible con las cuatro lineas del hidrogeno, el sweet
// spot resaltado y el indicador moviendose en ping-pong. Se dibuja en la
// franja superior central de la pantalla durante el minijuego de espectro.
void DrawEspectroHUD() {
    if (!hudShader) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Geometria de la barra
    float barW = 720.0f;
    if (barW > (float)SCR_WIDTH - 80.0f) barW = (float)SCR_WIDTH - 80.0f;
    float barH = 56.0f;
    float barX = (SCR_WIDTH - barW) * 0.5f;
    float barY = (float)SCR_HEIGHT - 110.0f;

    // Marco / fondo oscuro
    DrawQuad2D(barX - 8.0f, barY - 28.0f, barW + 16.0f, barH + 64.0f,
               glm::vec4(0.03f, 0.05f, 0.10f, 0.85f));
    DrawQuad2D(barX - 8.0f, barY + barH + 33.0f, barW + 16.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));
    DrawQuad2D(barX - 8.0f, barY - 28.0f, barW + 16.0f, 3.0f,
               glm::vec4(0.92f, 0.78f, 0.35f, 0.9f));

    // Gradiente del espectro: 60 segmentos pintados por longitud de onda.
    const int N = 60;
    float segW = barW / (float)N;
    for (int i = 0; i < N; ++i) {
        float u = (i + 0.5f) / (float)N;
        float wl = 400.0f + u * 300.0f;
        glm::vec3 rgb = WavelengthToRGB(wl);
        // Atenuamos un poco para simular un fondo tipo espectro continuo,
        // sobre el cual las lineas de hidrogeno destacan brillantes.
        DrawQuad2D(barX + i * segW, barY, segW + 0.5f, barH,
                   glm::vec4(rgb * 0.85f, 1.0f));
    }

    // Sweet spot: toda la zona valida se pinta como un rectangulo blanco
    // translucido (sin bordes laterales). Pulsamos suavemente la opacidad
    // para que la zona "respire" y se distinga del resto del espectro sin
    // tapar los colores de la barra.
    float spotX = barX + (gEspectro.sweetCenter - gEspectro.sweetHalf) * barW;
    float spotW = (2.0f * gEspectro.sweetHalf) * barW;
    float pulse = 0.55f + 0.20f * sinf((float)glfwGetTime() * 4.0f);
    DrawQuad2D(spotX, barY - 6.0f, spotW, barH + 12.0f,
               glm::vec4(1.0f, 1.0f, 1.0f, 0.42f * pulse));

    // Lineas de la serie de Balmer. La linea objetivo de la ronda actual se
    // dibuja mas ancha, mas opaca y con un halo blanco para que el jugador
    // identifique de un vistazo donde tiene que pegarle.
    for (int i = 0; i < 4; ++i) {
        float lp = EspectroLinePos(i);
        float lx = barX + lp * barW;
        glm::vec4 lc = ESPECTRO_COLORS[i];
        bool isTarget = (i == gEspectro.targetLine);

        if (isTarget) {
            // Halo blanco amplio alrededor de la linea objetivo
            float haloPulse = 0.65f + 0.25f * sinf((float)glfwGetTime() * 5.0f);
            DrawQuad2D(lx - 7.0f, barY - 8.0f, 14.0f, barH + 16.0f,
                       glm::vec4(1.0f, 1.0f, 1.0f, 0.35f * haloPulse));
            // Linea principal mas gruesa y mas saturada
            DrawQuad2D(lx - 2.5f, barY - 8.0f, 5.0f, barH + 16.0f,
                       glm::vec4(glm::min(lc.r * 1.6f, 1.0f),
                                 glm::min(lc.g * 1.6f, 1.0f),
                                 glm::min(lc.b * 1.6f, 1.0f),
                                 1.0f));
        } else {
            DrawQuad2D(lx - 1.0f, barY - 4.0f, 2.5f, barH + 8.0f,
                       glm::vec4(lc.r, lc.g, lc.b, 0.95f));
        }

        if (gText && gText->ready) {
            float scale = isTarget ? 0.40f : 0.32f;
            float w = gText->MeasureWidth(ESPECTRO_NAMES[i], scale);
            glm::vec3 textCol = isTarget
                ? glm::vec3(1.0f, 1.0f, 1.0f)
                : glm::vec3(lc.r, lc.g, lc.b) * 1.1f;
            gText->RenderText(ESPECTRO_NAMES[i], lx - w * 0.5f,
                              barY + barH + 8.0f, scale, textCol);
        }
    }

    // Indicador del jugador: barra vertical fina con halo.
    float indX = barX + gEspectro.barPos * barW;
    glm::vec4 indCol = gEspectro.stopped
        ? (gEspectro.lastHit ? glm::vec4(0.30f, 1.00f, 0.45f, 1.0f)
                              : glm::vec4(1.00f, 0.35f, 0.30f, 1.0f))
        : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    DrawQuad2D(indX - 6.0f, barY - 10.0f, 12.0f, barH + 20.0f,
               glm::vec4(indCol.r, indCol.g, indCol.b, 0.25f));
    DrawQuad2D(indX - 2.0f, barY - 10.0f, 4.0f, barH + 20.0f, indCol);

    // Texto: ronda actual, objetivo y puntuacion arriba de la barra; pista
    // de teclas debajo. Asi el jugador entiende el estado sin abrir la guia.
    if (gText && gText->ready) {
        char buf[160];
        if (gEspectro.roundIdx >= gEspectro.rounds) {
            snprintf(buf, sizeof(buf), "FIN  -  Aciertos %d/%d  -  Score %d",
                     gEspectro.hits, gEspectro.rounds, gEspectro.score);
        } else {
            snprintf(buf, sizeof(buf),
                     "Ronda %d/%d   Objetivo: %s   Score %d   Combo %d",
                     gEspectro.roundIdx + 1, gEspectro.rounds,
                     ESPECTRO_NAMES[gEspectro.targetLine],
                     gEspectro.score, gEspectro.combo);
        }
        float scale = 0.42f;
        float w = gText->MeasureWidth(buf, scale);
        gText->RenderText(buf, (SCR_WIDTH - w) * 0.5f, barY - 22.0f,
                          scale, glm::vec3(0.95f, 0.95f, 1.0f));

        const char* hint = (gEspectro.roundIdx >= gEspectro.rounds)
            ? "TAB para salir"
            : "SPACE = detener la barra   |   TAB = salir";
        scale = 0.32f;
        w = gText->MeasureWidth(hint, scale);
        gText->RenderText(hint, (SCR_WIDTH - w) * 0.5f, barY + barH + 38.0f,
                          scale, glm::vec3(0.70f, 0.78f, 0.92f));
    }

    glEnable(GL_DEPTH_TEST);
}