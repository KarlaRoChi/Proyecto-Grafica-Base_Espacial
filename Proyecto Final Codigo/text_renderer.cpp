#include <text_renderer.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

bool TextRenderer::Init(const std::string& fontPath, unsigned int pixelHeight,
                        const char* vsPath, const char* fsPath,
                        int screenWidth, int screenHeight) {
    if (ready) return true;

    textShader = new Shader(vsPath, fsPath);
    SetProjection(screenWidth, screenHeight);

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "[TextRenderer] FT_Init_FreeType FAILED" << std::endl;
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cout << "[TextRenderer] FT_New_Face FAILED: " << fontPath << std::endl;
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, pixelHeight);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 32; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "[TextRenderer] FT_Load_Char failed for: " << (int)c << std::endl;
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character ch;
        ch.TextureID = texture;
        ch.Size      = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        ch.Bearing   = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
        ch.Advance   = (unsigned int)face->glyph->advance.x;
        Characters[(char)c] = ch;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

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
    std::cout << "[TextRenderer] OK (" << Characters.size() << " glyphs)" << std::endl;
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
