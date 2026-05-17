#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>  // shader_m.h usa glm::value_ptr y no lo incluye

#include <map>
#include <string>
#include <vector>

#include <shader_m.h>

struct Character {
    unsigned int TextureID = 0;
    glm::ivec2   Size      = glm::ivec2(0);
    glm::ivec2   Bearing   = glm::ivec2(0);
    unsigned int Advance   = 0;
};

class TextRenderer {
public:
    std::map<char, Character> Characters;
    unsigned int VAO = 0, VBO = 0;
    Shader* textShader = nullptr;
    glm::mat4 projection = glm::mat4(1.0f);
    int screenW = 0, screenH = 0;
    bool ready = false;

    // Intenta cada ruta de la lista hasta encontrar una fuente cargable. Si
    // ninguna funciona Init devuelve false y RenderText es no-op.
    bool Init(const std::vector<std::string>& fontPaths, unsigned int pixelHeight,
              const char* vsPath, const char* fsPath,
              int screenWidth, int screenHeight);

    void SetProjection(int w, int h);

    void RenderText(const std::string& text, float x, float y,
                    float scale, glm::vec3 color);

    float MeasureWidth(const std::string& text, float scale);
    float LineHeight(float scale);

    void Shutdown();
};

#endif
