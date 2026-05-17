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
bool showControlsHint = true;

// Banner grande que aparece unos segundos al cambiar de zona ("INVERNADERO")
std::string gZoneBanner;
float gZoneBannerTimer = 0.0f;
const float ZONE_BANNER_DURATION = 2.5f;

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

struct InvernaderoGame {
    Difficulty difficulty = DIFF_EASY;
    bool active = false;
    bool topDown = false;
    float gameTime = 0.0f;
    std::vector<OsuNote> notes;
    int score = 0;
    int combo = 0;
    int maxCombo = 0;
    int hits = 0;
    int misses = 0;
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

InvernaderoGame gInvernadero;
SimonGame gEnergia;
ReciclajeGame gReciclaje;
CapsulasGame gCapsulas;

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

const glm::vec3 INV_PLANT_POSITIONS[4] = {
    glm::vec3(-3.0f, 0.0f, -10.0f),
    glm::vec3(-1.0f, 0.0f, -10.0f),
    glm::vec3( 1.0f, 0.0f, -10.0f),
    glm::vec3( 3.0f, 0.0f, -10.0f)
};

const glm::vec3 SIMON_BUTTON_POSITIONS[4] = {
    glm::vec3(-3.0f, 1.6f, 11.5f),
    glm::vec3(-1.0f, 1.6f, 11.5f),
    glm::vec3( 1.0f, 1.6f, 11.5f),
    glm::vec3( 3.0f, 1.6f, 11.5f)
};

const glm::vec3 RECYCLE_BIN_POSITIONS[3] = {
    glm::vec3(11.5f, 0.5f, -2.5f),  // METAL
    glm::vec3(11.5f, 0.5f,  0.0f),  // VIDRIO
    glm::vec3(11.5f, 0.5f,  2.5f)   // CARTON
};

bool keyZLast = false, keyXLast = false, keyCLast = false, keyVLast = false;
bool keyGLast = false, keyBLast = false, keyELast = false;
bool keyTabLast = false;
bool keyDiff5Last = false, keyDiff6Last = false, keyDiff7Last = false;

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
        if (maxAABB.y < 0.2f) continue;
        float anchoX = maxAABB.x - minAABB.x;
        float anchoZ = maxAABB.z - minAABB.z;
        if (anchoX > 12.0f || anchoZ > 12.0f) continue;
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

    glm::vec4 baseColor = GetMoonTrailBaseColor(moonTrailMode);

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

        // Variaciones sutiles de color
        float colorVariation = 0.82f + r * 0.25f;
        glm::vec3 variedColor = glm::vec3(baseColor) * colorVariation;

        variedColor.r += (r - 0.5f) * 0.06f;
        variedColor.g += (r2 - 0.5f) * 0.06f;
        variedColor.b += (r - 0.5f) * 0.04f;

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

    // Al entrar a una zona con minijuego, inicializarlo automaticamente
    if (z == 1) {
        InitInvernaderoGame();
        gInvernadero.active = true;
        gInvernadero.topDown = true;
    }
    if (z == 2) {
        InitReciclajeGame();
        gReciclaje.active = true;
    }
    if (z == 3) {
        InitEnergiaGame();
        gEnergia.active = true;
    }
    if (z == 4) {
        InitCapsulasGame();
        gCapsulas.active = true;
    }
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
glm::mat4 ComputeView() {
    if (currentZone == 1 && gInvernadero.active && gInvernadero.topDown) {
        glm::vec3 center(0.0f, 0.0f, -10.0f);
        glm::vec3 eye(0.0f, 14.0f, -9.99f);
        return glm::lookAt(eye, center, glm::vec3(0.0f, 0.0f, -1.0f));
    }
    if (cameraMode == CAM_FREE) return camera.GetViewMatrix();
    glm::vec3 fwd = glm::vec3(sinf(glm::radians(player.yaw)), 0.0f, cosf(glm::radians(player.yaw)));
    glm::vec3 eye, target;
    const float headHeight = 1.7f, distBack = 4.5f, distFront = 4.5f, camHeight = 2.8f;
    switch (cameraMode) {
    case CAM_FIRST:  eye = player.position + glm::vec3(0.0f, headHeight, 0.0f); target = eye + fwd; break;
    case CAM_THIRD:  eye = player.position - fwd * distBack + glm::vec3(0.0f, camHeight, 0.0f); target = player.position + glm::vec3(0.0f, 1.2f, 0.0f); break;
    case CAM_SECOND: eye = player.position + fwd * distFront + glm::vec3(0.0f, camHeight, 0.0f); target = player.position + glm::vec3(0.0f, 1.2f, 0.0f); break;
    default: return camera.GetViewMatrix();
    }
    return glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 ComputeEyePos() {
    if (currentZone == 1 && gInvernadero.active && gInvernadero.topDown) {
        return glm::vec3(0.0f, 14.0f, -9.99f);
    }
    if (cameraMode == CAM_FREE) return camera.Position;
    glm::vec3 fwd = glm::vec3(sinf(glm::radians(player.yaw)), 0.0f, cosf(glm::radians(player.yaw)));
    if (cameraMode == CAM_FIRST)  return player.position + glm::vec3(0, 1.7f, 0);
    if (cameraMode == CAM_THIRD)  return player.position - fwd * 4.5f + glm::vec3(0, 2.8f, 0);
    if (cameraMode == CAM_SECOND) return player.position + fwd * 4.5f + glm::vec3(0, 2.8f, 0);
    return camera.Position;
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

    // El avatar solo se mueve cuando NO estamos en camara libre (para evitar
    // que WASD controle simultaneamente al jugador y a la camara libre).
    if (cameraMode != CAM_FREE) {
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

    if (menuState == MENU_NONE && gZoneBannerTimer > 0.0f) {
        gZoneBannerTimer -= deltaTime;
    }

    // --- Logica de minijuegos --- (pausada cuando el menu esta abierto)
    if (menuState == MENU_NONE) {
        if (currentZone == 1 && gInvernadero.active) UpdateInvernaderoGame(deltaTime);
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


    glm::vec4 ambMetal = glm::vec4(0.03f, 0.03f, 0.03f, 1.0f);
    glm::vec4 diffMetal = glm::vec4(0.45f, 0.45f, 0.48f, 1.0f);
    glm::vec4 specMetal = glm::vec4(12.0f, 12.0f, 12.0f, 1.0f);
    int alphaMetal = 2048;


    switch (currentZone) {
    case 0: // HUB
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (HUB_OPACO) HUB_OPACO->Draw(*phongShader);

        // Puertas N/S/E/O del HUB: una puerta visible en cada hitbox de
        // teletransporte. La que esta mas cerca del jugador "se ilumina"
        // para indicar que esta a punto de activarse.
        if (PUERTA_MODELO) {
            float pulseGlobal = 0.85f + 0.15f * sinf(t * 2.2f);

            // 0=S, 1=E, 2=N, 3=O. Yaw para que la cara de la puerta encare
            // hacia el HUB (jugador) y el modelo quede paralelo a su muro.
            const float doorYaws[4] = { 180.0f, -90.0f, 0.0f, 90.0f };

            for (int i = 0; i < 4; ++i) {
                const ZoneDoor& d = zoneDoors[i];
                if (d.fromZone != 0) continue;

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
                SetMaterial(phongShader, doorAmb, doorDiff, doorSpec, 64);

                glm::mat4 m(1.0f);
                m = glm::translate(m, d.center);
                m = glm::rotate(m, glm::radians(doorYaws[i]), glm::vec3(0.0f, 1.0f, 0.0f));
                m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                m = glm::scale(m, glm::vec3(0.6f));
                phongShader->setMat4("model", m);
                PUERTA_MODELO->Draw(*phongShader);
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

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        fresnelShader->use();
        fresnelShader->setMat4("projection", projection);
        fresnelShader->setMat4("view", view);
        fresnelShader->setMat4("model", baseModelMat);
        fresnelShader->setVec3("cameraPos", eyePos);
        fresnelShader->setVec3("baseColor", glm::vec3(0.30f, 0.55f, 0.70f));
        fresnelShader->setFloat("transparency", 0.35f);
        fresnelShader->setFloat("alphaMult", 0.35f);
        fresnelShader->setFloat("opacity", 0.35f);
        if (INVERNADERO_TRASLUCIDO) INVERNADERO_TRASLUCIDO->Draw(*fresnelShader);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

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
        break;

    case 2: // RECICLAJE
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (RECICLAJE_OPACO) RECICLAJE_OPACO->Draw(*phongShader);
        SetMaterial(phongShader, ambMetal, diffMetal, specMetal, alphaMetal);
        if (RECICLAJE_METAL) RECICLAJE_METAL->Draw(*phongShader);

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
        break;

    case 3: // ENERGIA
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (ENERGIA_OPACO) ENERGIA_OPACO->Draw(*phongShader);
        SetMaterial(phongShader, ambMetal, diffMetal, specMetal, alphaMetal);
        if (ENERGIA_METALICO) ENERGIA_METALICO->Draw(*phongShader);

        // ====== MINIJUEGO SIMON (Energia) ======
        if (gEnergia.active) DrawEnergiaGame(projection, view);
        break;

    case 4: // CAPSULAS
        SetMaterial(phongShader, ambOpaque, diffOpaque, specOpaque, alphaOpaque);
        if (CAPSULAS_OPACO) CAPSULAS_OPACO->Draw(*phongShader);

        // ====== MINIJUEGO BUSCAR AL INTRUSO (Capsulas) ======
        if (gCapsulas.active) DrawCapsulasGame(projection, view);
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
    gText->Init(fontTries, 32, "shaders/text.vs", "shaders/text.fs",
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

void DrawPauseMenu() {
    // Fondo oscuro semitransparente
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    DrawQuad2D(0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, glm::vec4(0.0f, 0.0f, 0.05f, 0.65f));

    // Panel central
    float panelW = 420.0f, panelH = 380.0f;
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

    const char* options[4] = { "Continuar", "Controles", "Mapa", "Salir" };
    float yBase = panelY + panelH - 170.0f;
    float lineH = 50.0f;
    for (int i = 0; i < 4; ++i) {
        float y = yBase - i * lineH;
        if (i == menuSelection) {
            DrawQuad2D(panelX + 30.0f, y - 10.0f, panelW - 60.0f, 38.0f,
                       glm::vec4(0.92f, 0.78f, 0.35f, 0.28f));
        }
        glm::vec3 col = (i == menuSelection)
            ? glm::vec3(1.0f, 0.95f, 0.55f)
            : glm::vec3(0.80f, 0.85f, 0.92f);
        DrawTextCentered(options[i], cx, y, 0.70f, col);
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
        { "Dificultad zona",          "5 / 6 / 7" }
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
        out.push_back("B = iniciar partida de ritmo  |  H J K L = regar maceta");
        if (d <= DIFF_MEDIUM) out.push_back("G alterna vista cenital  |  P/O hace crecer/resetea el helecho");
        if (d <= DIFF_EASY)   out.push_back("Riega cuando la nota llegue al circulo");
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
    snprintf(buf, sizeof(buf), "Dificultad: %s  (5/6/7 para cambiar)", difLabel[(int)d]);
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
        bool up = (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) ||
                  (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS);
        bool dn = (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) ||
                  (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS);
        bool enter = (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) ||
                     (glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) ||
                     (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

        if (menuState == MENU_PAUSE) {
            if (up && !kUpLast) menuSelection = (menuSelection + 3) % 4;
            if (dn && !kDnLast) menuSelection = (menuSelection + 1) % 4;
            if (enter && !kEnterLast) {
                switch (menuSelection) {
                case 0: menuState = MENU_NONE; break;                   // Continuar
                case 1: menuState = MENU_CONTROLS; break;               // Controles
                case 2: showMinimap = !showMinimap;                     // Mapa (toggle)
                        menuState = MENU_NONE; break;
                case 3: glfwSetWindowShouldClose(window, true); break;  // Salir
                }
            }
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

    // 3. Controles universales: Teletransporte a Zonas
    int zoneKeys[5] = { GLFW_KEY_0,    GLFW_KEY_1,    GLFW_KEY_2,    GLFW_KEY_3,    GLFW_KEY_4 };
    int zoneKeysKP[5] = { GLFW_KEY_KP_0, GLFW_KEY_KP_1, GLFW_KEY_KP_2, GLFW_KEY_KP_3, GLFW_KEY_KP_4 };

    for (int i = 0; i < 5; ++i) {
        bool now = (glfwGetKey(window, zoneKeys[i]) == GLFW_PRESS) ||
            (glfwGetKey(window, zoneKeysKP[i]) == GLFW_PRESS);

        if (now && !keyZoneLast[i]) ChangeZone(i);
        keyZoneLast[i] = now;
    }

    // Dificultad de minijuego de la zona actual: 5=Facil, 6=Medio, 7=Experto
    bool d5 = glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS;
    bool d6 = glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS;
    bool d7 = glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS;
    if (d5 && !keyDiff5Last) SetCurrentZoneDifficulty(DIFF_EASY);
    if (d6 && !keyDiff6Last) SetCurrentZoneDifficulty(DIFF_MEDIUM);
    if (d7 && !keyDiff7Last) SetCurrentZoneDifficulty(DIFF_EXPERT);
    keyDiff5Last = d5; keyDiff6Last = d6; keyDiff7Last = d7;

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
        // la tecla C se reserva al minijuego Simon en otra zona)
        static bool nLast = false;
        static bool mLast = false;
        static bool yLast = false;
        bool nKey = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
        bool mKey = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
        bool yKey = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
        if (nKey && !nLast) { moonTrailMode = 0; std::cout << "CYAN"    << std::endl; }
        if (mKey && !mLast) { moonTrailMode = 1; std::cout << "MAGENTA" << std::endl; }
        if (yKey && !yLast) { moonTrailMode = 2; std::cout << "YELLOW"  << std::endl; }
        nLast = nKey;
        mLast = mKey;
        yLast = yKey;

        // --- Minijuego OSU ---
        // B = iniciar/reiniciar partida, V = alternar vista cenital
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            InitInvernaderoGame();
            gInvernadero.active = true;
            gInvernadero.topDown = true;
            std::cout << "[Invernadero] Nueva partida (dificultad " << gInvernadero.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

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
        // B = iniciar/reiniciar partida
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            InitEnergiaGame();
            gEnergia.active = true;
            std::cout << "[Energia] Nueva partida (dificultad " << gEnergia.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        // Botones: Z X C V (las 4 baterias)
        bool zKey = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
        bool xKey = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
        bool cKey = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
        bool vKey = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;
        if (zKey && !keyZLast) EnergiaPress(0);
        if (xKey && !keyXLast) EnergiaPress(1);
        if (cKey && !keyCLast) EnergiaPress(2);
        if (vKey && !keyVLast) EnergiaPress(3);
        keyZLast = zKey; keyXLast = xKey; keyCLast = cKey; keyVLast = vKey;

        // ---- INTERACCION CON CLIC DEL MOUSE (LISTA, COMENTADA) ----
        // Cuando se implemente, habilitar cursor visible al entrar a zona 3 y:
        // if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        //     double mx, my; glfwGetCursorPos(window, &mx, &my);
        //     // Proyectar cada SIMON_BUTTON_POSITIONS[i] al espacio pantalla con
        //     // projection*view, comparar (mx,my) con la proyeccion y disparar
        //     // EnergiaPress(i) si la distancia es menor a un radio en pixeles.
        // }
    }

    // ZONA 4: CAPSULAS - Buscar al intruso
    // E = revisar la capsula mas cercana al avatar
    // B = nueva partida con la dificultad actual
    if (currentZone == 4) {
        bool bKey = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
        if (bKey && !keyBLast) {
            InitCapsulasGame();
            gCapsulas.active = true;
            std::cout << "[Capsulas] Nueva partida (dificultad " << gCapsulas.difficulty << ")" << std::endl;
        }
        keyBLast = bKey;

        bool eKey = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        if (eKey && !keyELast) {
            CapsulasCheckNearest();
        }
        keyELast = eKey;
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
    if (cameraMode == CAM_FREE) camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (cameraMode == CAM_FREE) camera.ProcessMouseScroll((float)yoffset);
}

// =============================================================================
// ============         IMPLEMENTACION DE LOS MINIJUEGOS            ============
// =============================================================================

void SetCurrentZoneDifficulty(Difficulty d) {
    switch (currentZone) {
    case 1:
        gInvernadero.difficulty = d;
        std::cout << "[Invernadero] Dificultad: " << d << std::endl;
        InitInvernaderoGame();
        gInvernadero.active = true;
        break;
    case 2:
        gReciclaje.difficulty = d;
        std::cout << "[Reciclaje] Dificultad: " << d << std::endl;
        InitReciclajeGame();
        gReciclaje.active = true;
        break;
    case 3:
        gEnergia.difficulty = d;
        std::cout << "[Energia] Dificultad: " << d << std::endl;
        InitEnergiaGame();
        gEnergia.active = true;
        break;
    case 4:
        gCapsulas.difficulty = d;
        std::cout << "[Capsulas] Dificultad: " << d << std::endl;
        InitCapsulasGame();
        gCapsulas.active = true;
        break;
    default: break;
    }
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

    // Parametros segun dificultad
    int    totalNotes;
    float  spacing;          // separacion media entre notas (s)
    switch (gInvernadero.difficulty) {
    case DIFF_EASY:   totalNotes = 20; spacing = 1.4f; break;
    case DIFF_MEDIUM: totalNotes = 30; spacing = 0.9f; break;
    case DIFF_EXPERT: totalNotes = 45; spacing = 0.55f; break;
    default:          totalNotes = 20; spacing = 1.4f; break;
    }

    float t = 2.5f; // primer nota tras 2.5s de margen
    int lastLane = -1;
    srand(12345); // patron reproducible
    for (int i = 0; i < totalNotes; i++) {
        OsuNote n;
        int lane;
        do { lane = rand() % 4; } while (lane == lastLane && (rand() % 3 != 0));
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
        std::cout << "[OSU] HIT  carril " << lane << "  combo=" << gInvernadero.combo
                  << "  score=" << gInvernadero.score << std::endl;
    }
    else {
        gInvernadero.combo = 0;
        gInvernadero.misses++;
        std::cout << "[OSU] MISS carril " << lane << std::endl;
    }
}

void UpdateInvernaderoGame(float dt) {
    gInvernadero.gameTime += dt;
    for (int i = 0; i < 4; i++) {
        gInvernadero.waterEffect[i] -= dt * 2.0f;
        if (gInvernadero.waterEffect[i] < 0.0f) gInvernadero.waterEffect[i] = 0.0f;
    }
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

    // Colores por carril
    glm::vec4 laneColors[4] = {
        glm::vec4(0.9f, 0.2f, 0.2f, 1.0f), // rojo
        glm::vec4(0.95f,0.7f, 0.1f, 1.0f), // amarillo
        glm::vec4(0.2f, 0.85f,0.3f, 1.0f), // verde
        glm::vec4(0.3f, 0.5f, 1.0f, 1.0f)  // azul
    };

    // 1) Marcadores fijos en cada planta (objetivos)
    for (int i = 0; i < 4; i++) {
        glm::vec3 base = INV_PLANT_POSITIONS[i];
        base.y = 0.05f;
        float pulse = 0.85f + 0.15f * sinf(gInvernadero.gameTime * 3.0f + i);
        float waterBoost = gInvernadero.waterEffect[i];
        glm::vec4 c = laneColors[i] * (pulse + waterBoost);
        SetMaterial(phongShader, c * 0.4f, c, glm::vec4(0.4f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, base);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.5f + waterBoost * 0.25f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }

    // 2) Notas cayendo: visibles 2s antes del hitTime, se acercan al objetivo
    float lookAhead = 2.0f;
    for (auto& n : gInvernadero.notes) {
        if (n.hit || n.missed) continue;
        float tToHit = n.hitTime - gInvernadero.gameTime;
        if (tToHit > lookAhead) continue;
        if (tToHit < -0.4f) continue;

        // En top-down: las notas se desplazan en Z (alejadas -> al objetivo)
        glm::vec3 target = INV_PLANT_POSITIONS[n.lane];
        glm::vec3 start  = target + glm::vec3(0.0f, 0.0f, 6.0f); // mas cerca del jugador
        float a = glm::clamp(1.0f - (tToHit / lookAhead), 0.0f, 1.0f);
        glm::vec3 pos = glm::mix(start, target, a);
        pos.y = 0.4f;

        glm::vec4 c = laneColors[n.lane];
        SetMaterial(phongShader, c * 0.3f, c, glm::vec4(0.5f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.35f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
    }
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
            std::cout << "[Simon] NIVEL " << gEnergia.level
                      << "  Score=" << gEnergia.score << std::endl;
        }
    }
    else {
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
    // Las pilas/botones de la zona ENERGIA son parte del modelo
    // ENERGIA_OPACO; el "brillo" del minijuego Simon viene de los 4 point
    // lights configurados en Update (slots 5..8 de gLights). Aqui no hace
    // falta dibujar geometria extra: los botones existentes del modelo
    // quedan iluminados con el color del estado actual del juego.
    (void)projection;
    (void)view;
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

    // Items en el piso
    for (auto& it : gReciclaje.items) {
        if (it.sorted) continue;
        glm::vec4 c = matColors[(int)it.type];
        SetMaterial(phongShader, c * 0.3f, c, glm::vec4(0.4f), 32);

        glm::mat4 m(1.0f);
        m = glm::translate(m, it.position);
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.28f));
        phongShader->setMat4("model", m);
        CAJA_MODELO->Draw(*phongShader);
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
        std::cout << "[Capsulas] Acercate mas a una capsula para revisarla." << std::endl;
        return;
    }

    gCapsulas.lastChecked = best;
    gCapsulas.lastCheckedTimer = 0.6f;

    if (best == gCapsulas.intruderIdx) {
        gCapsulas.win = true;
        std::cout << "[Capsulas] INTRUSO ATRAPADO en capsula " << best
                  << "  Tiempo restante=" << gCapsulas.timeLeft << "s" << std::endl;
    }
    else {
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