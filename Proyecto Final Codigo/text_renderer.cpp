#include <text_renderer.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

bool TextRenderer::Init(const std::vector<std::string>& fontPaths, unsigned int pixelHeight,
                        const char* vsPath, const char* fsPath,
                        int screenWidth, int screenHeight) {
    if (ready) return true;

    // Cargar el primer .ttf de la lista que se pueda abrir.
    std::vector<unsigned char> ttfData;
    std::string usedPath;
    for (const std::string& p : fontPaths) {
        std::ifstream file(p, std::ios::binary | std::ios::ate);
        if (!file.is_open()) continue;
        std::streamsize size = file.tellg();
        if (size <= 0) continue;
        file.seekg(0, std::ios::beg);
        std::vector<unsigned char> buf((size_t)size);
        if (file.read((char*)buf.data(), size)) {
            ttfData = std::move(buf);
            usedPath = p;
            break;
        }
    }
    if (ttfData.empty()) {
        std::cout << "[TextRenderer] No se pudo abrir ninguna fuente .ttf de la lista" << std::endl;
        return false;
    }

    stbtt_fontinfo font;
    int fontOffset = stbtt_GetFontOffsetForIndex(ttfData.data(), 0);
    if (!stbtt_InitFont(&font, ttfData.data(), fontOffset)) {
        std::cout << "[TextRenderer] stbtt_InitFont fallo en " << usedPath << std::endl;
        return false;
    }

    textShader = new Shader(vsPath, fsPath);
    SetProjection(screenWidth, screenHeight);

    float fontScale = stbtt_ScaleForPixelHeight(&font, (float)pixelHeight);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 32; c < 128; c++) {
        int w = 0, h = 0, xoff = 0, yoff = 0;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(
            &font, 0, fontScale, c, &w, &h, &xoff, &yoff);

        unsigned int texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        if (bitmap && w > 0 && h > 0) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED,
                         GL_UNSIGNED_BYTE, bitmap);
        } else {
            // Caracteres como ' ' no tienen bitmap; subimos un pixel vacio.
            unsigned char blank = 0;
            w = h = 1; xoff = 0; yoff = 0;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED,
                         GL_UNSIGNED_BYTE, &blank);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int advWidthUnscaled = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&font, c, &advWidthUnscaled, &lsb);

        Character ch;
        ch.TextureID = texture;
        ch.Size      = glm::ivec2(w, h);
        // FreeType usa bitmap_top positivo (altura sobre baseline). En stbtt
        // yoff es la y-down del top del glifo, asi que invertimos el signo
        // para mantener compatibilidad con RenderText.
        ch.Bearing   = glm::ivec2(xoff, -yoff);
        // RenderText hace (Advance >> 6) * scale, asumiendo punto fijo 26.6
        // como devuelve FreeType. Convertimos el avance en pixeles de stbtt
        // al mismo formato multiplicando por 64.
        ch.Advance   = (unsigned int)(advWidthUnscaled * fontScale * 64.0f);
        Characters[(char)c] = ch;

        if (bitmap) stbtt_FreeBitmap(bitmap, nullptr);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ready = true;
    std::cout << "[TextRenderer] OK (" << Characters.size() << " glyphs, font="
              << usedPath << ")" << std::endl;
    return true;
}

void TextRenderer::SetProjection(int w, int h) {
    screenW = w;
    screenH = h;
    projection = glm::ortho(0.0f, (float)w, 0.0f, (float)h);
}

void TextRenderer::RenderText(const std::string& text, float x, float y,
                              float scale, glm::vec3 color) {
    if (!ready || !textShader) return;

    GLboolean prevBlend  = glIsEnabled(GL_BLEND);
    GLboolean prevDepth  = glIsEnabled(GL_DEPTH_TEST);
    GLint prevBlendSrc = GL_SRC_ALPHA, prevBlendDst = GL_ONE_MINUS_SRC_ALPHA;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &prevBlendDst);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    textShader->use();
    textShader->setMat4("projection", projection);
    textShader->setVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    textShader->setInt("text", 0);
    glBindVertexArray(VAO);

    for (char c : text) {
        auto it = Characters.find(c);
        if (it == Characters.end()) continue;
        const Character& ch = it->second;

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // stbtt entrega el bitmap top-down; el shader espera UV con (0,0) arriba
        // y (1,1) abajo igual que con FreeType, asi que mantenemos el mismo
        // mapeo (la textura ya quedo guardada en ese orden al hacer glTexImage2D).
        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },

            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (prevDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (!prevBlend) glDisable(GL_BLEND);
    glBlendFunc(prevBlendSrc, prevBlendDst);
}

float TextRenderer::MeasureWidth(const std::string& text, float scale) {
    float w = 0.0f;
    for (char c : text) {
        auto it = Characters.find(c);
        if (it == Characters.end()) continue;
        w += (it->second.Advance >> 6) * scale;
    }
    return w;
}

float TextRenderer::LineHeight(float scale) {
    auto it = Characters.find('H');
    if (it == Characters.end()) return 16.0f * scale;
    return it->second.Size.y * scale * 1.5f;
}

void TextRenderer::Shutdown() {
    for (auto& kv : Characters) {
        if (kv.second.TextureID) glDeleteTextures(1, &kv.second.TextureID);
    }
    Characters.clear();
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    VAO = VBO = 0;
    if (textShader) { delete textShader; textShader = nullptr; }
    ready = false;
}
