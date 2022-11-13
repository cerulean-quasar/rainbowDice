/**
 * Copyright 2019 Cerulean Quasar. All Rights Reserved.
 *
 *  This file is part of RainbowDice.
 *
 *  RainbowDice is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RainbowDice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RainbowDice.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <string>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/native_window.h>
#include <cstring>
#include <jni.h>
#include "rainbowDiceGL.hpp"
#include "rainbowDiceGlobal.hpp"
#include "android.hpp"

namespace graphicsGL {
    void Surface::createSurface() {
        /*
        static EGLint const attribute_list[] = {
                EGL_RED_SIZE, 1,
                EGL_GREEN_SIZE, 1,
                EGL_BLUE_SIZE, 1,
                EGL_NONE
        };
         */
        const EGLint attribute_list[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
                EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_DEPTH_SIZE, 24,
                EGL_NONE};

        // Initialize EGL
        if ((m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
            destroySurface();
            throw std::runtime_error("Could not open display");
        }

        EGLint majorVersion;
        EGLint minorVersion;
        if (!eglInitialize(m_display, &majorVersion, &minorVersion)) {
            destroySurface();
            throw std::runtime_error("Could not initialize display");
        }

        EGLint nbr_config;
        if (!eglChooseConfig(m_display, attribute_list, &m_config, 1, &nbr_config)) {
            destroySurface();
            throw std::runtime_error("Could not get config");
        }

        if (nbr_config == 0) {
            destroySurface();
            throw std::runtime_error("Got 0 configs.");
        }

        int32_t format;
        if (!eglGetConfigAttrib(m_display, m_config, EGL_NATIVE_VISUAL_ID, &format)) {
            destroySurface();
            throw std::runtime_error("Could not get display format");
        }
        ANativeWindow_setBuffersGeometry(m_window.get(), 0, 0, format);

        if ((m_surface = eglCreateWindowSurface(m_display, m_config, m_window.get(), nullptr)) ==
            EGL_NO_SURFACE) {
            destroySurface();
            throw std::runtime_error("Could not create surface");
        }

        EGLint contextAttributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        if ((m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, contextAttributes)) ==
            EGL_NO_CONTEXT) {
            destroySurface();
            throw std::runtime_error("Could not create context");
        }

        if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
            destroySurface();
            throw std::runtime_error("Could not set the surface to current");
        }

        EGLint width, height;
        if (!eglQuerySurface(m_display, m_surface, EGL_WIDTH, &width) ||
            !eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &height)) {
            destroySurface();
            throw std::runtime_error("Could not get width and height of surface");
        }

        m_width = static_cast<uint32_t>(width);
        m_height = static_cast<uint32_t>(height);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
        // Accept fragment if it is closer to the camera than the former one.
        glDepthFunc(GL_LESS);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glViewport(0, 0, m_width, m_height);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void Surface::destroySurface() {
        if (m_display != EGL_NO_DISPLAY) {
            eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (m_context != EGL_NO_CONTEXT) {
                eglDestroyContext(m_display, m_context);
            }
            if (m_surface != EGL_NO_SURFACE) {
                eglDestroySurface(m_display, m_surface);
            }
            eglTerminate(m_display);
        }
        m_display = EGL_NO_DISPLAY;
        m_context = EGL_NO_CONTEXT;
        m_surface = EGL_NO_SURFACE;
    }

    void Surface::initThread() {
        if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
            throw std::runtime_error("Could not set the surface to current");
        }
    }

    void Surface::cleanupThread() {
        if (!eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
            throw std::runtime_error("Could not unset the surface to current");
        }
    }

} /* namespace graphicsGL */

char constexpr const *SHADER_VERT_FILE = "shaderGL.vert";
char constexpr const *SHADER_FRAG_FILE = "shaderGL.frag";
char constexpr const *SHADER_LINES_VERT_FILE = "shaderLinesGL.vert";
char constexpr const *SHADER_LINES_FRAG_FILE = "shaderLinesGL.frag";

template <>
bool DiceGraphics<GLGraphics>::isGL() {
    return true;
}

void RainbowDiceGL::init() {
    m_programID = loadShaders(SHADER_VERT_FILE, SHADER_FRAG_FILE);
    m_programLoaded = true;

    m_programIDDiceBox = loadShaders(SHADER_LINES_VERT_FILE, SHADER_LINES_FRAG_FILE);
    m_programLoadedDiceBox = true;
}

void RainbowDiceGL::initModels() {
}

void RainbowDiceGL::drawFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the dice shader.
    glUseProgram(m_programID);

    GLint textureID = glGetUniformLocation(m_programID, "texSampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture->texture());
    glUniform1i(textureID, 0);

    GLint viewPos = glGetUniformLocation(m_programID, "viewPosition");
    glUniform3fv(viewPos, 1, &m_viewPoint[0]);

    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            // Send our transformation to the currently bound shader, in the "MVP"
            // uniform. This is done in the main loop since each model will have a
            // different MVP matrix (At least for the M part)

            // the projection matrix
            GLint MatrixID = glGetUniformLocation(m_programID, "proj");
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &m_proj[0][0]);

            // view matrix
            MatrixID = glGetUniformLocation(m_programID, "view");
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &m_view[0][0]);

            // model matrix
            MatrixID = glGetUniformLocation(m_programID, "model");
            glm::mat4 model = die->die()->model();
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &model[0][0]);

            // the model matrix for the normal vector
            MatrixID = glGetUniformLocation(m_programID, "normalMatrix");
            glm::mat4 matrix = glm::transpose(glm::inverse(model));
            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &matrix[0][0]);

            // whether the die is selected
            GLint var = glGetUniformLocation(m_programID, "isSelected");
            glUniform1i(var, die->isSelected() ? 1 : 0);

            // the width of the edges of the die
            var = glGetUniformLocation(m_programID, "edgeWidth");
            glUniform1f(var, die->die()->edgeWidth());

            // 1st attribute buffer : colors
            GLint colorID = glGetAttribLocation(m_programID, "inColor");
            glBindBuffer(GL_ARRAY_BUFFER, die->vertexBuffer());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, die->indexBuffer());
            glVertexAttribPointer(
                    colorID,                          // The position of the attribute in the shader.
                    4,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    sizeof(Vertex),                   // stride
                    (void *) (offsetof(Vertex, color))// array buffer offset
            );
            glEnableVertexAttribArray(colorID);

            // attribute buffer : vertices for die
            GLint position = glGetAttribLocation(m_programID, "inPosition");
            glVertexAttribPointer(
                    position,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, pos)) // array buffer offset
            );
            glEnableVertexAttribArray(position);

            // Send in the texture coordinates
            GLint texCoordID = glGetAttribLocation(m_programID, "inTexCoord");
            glVertexAttribPointer(
                    texCoordID,                       // The position of the attribute in the shader
                    2,                                // size
                    GL_FLOAT,                         // type
                    GL_FALSE,                         // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) offsetof (Vertex, texCoord)  // array buffer offset
            );
            glEnableVertexAttribArray(texCoordID);

            // attribute buffer : normal vector to the face
            GLint normalID = glGetAttribLocation(m_programID, "inNormal");
            glVertexAttribPointer(
                    normalID,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, normal)) // array buffer offset
            );
            glEnableVertexAttribArray(normalID);

            // attribute buffer : normal vector to the corner
            GLint cornerNormalID = glGetAttribLocation(m_programID, "inCornerNormal");
            glVertexAttribPointer(
                    cornerNormalID,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, cornerNormal)) // array buffer offset
            );
            glEnableVertexAttribArray(cornerNormalID);

            // attribute buffer : vertices for die
            GLint corner1ID = glGetAttribLocation(m_programID, "inCorner1");
            glVertexAttribPointer(
                    corner1ID,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, corner1)) // array buffer offset
            );
            glEnableVertexAttribArray(corner1ID);

            // attribute buffer : vertices for die
            GLint corner2ID = glGetAttribLocation(m_programID, "inCorner2");
            glVertexAttribPointer(
                    corner2ID,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, corner2)) // array buffer offset
            );
            glEnableVertexAttribArray(corner2ID);

            // attribute buffer : vertices for die
            GLint corner3ID = glGetAttribLocation(m_programID, "inCorner3");
            glVertexAttribPointer(
                    corner3ID,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, corner3)) // array buffer offset
            );
            glEnableVertexAttribArray(corner3ID);

            // attribute buffer : vertices for die
            GLint corner4ID = glGetAttribLocation(m_programID, "inCorner4");
            glVertexAttribPointer(
                    corner4ID,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, corner4)) // array buffer offset
            );
            glEnableVertexAttribArray(corner4ID);

            // attribute buffer : vertices for die
            GLint corner5ID = glGetAttribLocation(m_programID, "inCorner5");
            glVertexAttribPointer(
                    corner5ID,                        // The position of the attribute in the shader.
                    3,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, corner5)) // array buffer offset
            );
            glEnableVertexAttribArray(corner5ID);

            // attribute buffer : mode for the way the nearness to edges is detected.
            GLint modeID = glGetAttribLocation(m_programID, "inMode");
            glVertexAttribPointer(
                    modeID,                          // The position of the attribute in the shader.
                    1,                               // size
                    GL_FLOAT,                        // type
                    GL_FALSE,                        // normalized?
                    sizeof(Vertex),                  // stride
                    (void *) (offsetof(Vertex, mode)) // array buffer offset
            );
            glEnableVertexAttribArray(modeID);

            // Draw the triangles !
            //glDrawArrays(GL_TRIANGLES, 0, dice[0].die->vertices.size() /* total number of vertices*/);
            glDrawElements(GL_TRIANGLES, die->die()->getIndices().size(), GL_UNSIGNED_INT, 0);

            glDisableVertexAttribArray(position);
            glDisableVertexAttribArray(colorID);
            glDisableVertexAttribArray(texCoordID);
            glDisableVertexAttribArray(normalID);
            glDisableVertexAttribArray(cornerNormalID);
            glDisableVertexAttribArray(corner1ID);
            glDisableVertexAttribArray(corner2ID);
            glDisableVertexAttribArray(corner3ID);
            glDisableVertexAttribArray(corner4ID);
            glDisableVertexAttribArray(corner5ID);
            glDisableVertexAttribArray(modeID);
        }
    }

    if (m_diceBox != nullptr && anyRolling()) {
        // Use the dice box shader.
        glUseProgram(m_programIDDiceBox);
        viewPos = glGetUniformLocation(m_programIDDiceBox, "viewPosition");
        glUniform3fv(viewPos, 1, &m_viewPoint[0]);

        // the projection matrix
        GLint MatrixID = glGetUniformLocation(m_programIDDiceBox, "proj");
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &m_proj[0][0]);

        // view matrix
        MatrixID = glGetUniformLocation(m_programIDDiceBox, "view");
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &m_view[0][0]);

        // model matrix
        MatrixID = glGetUniformLocation(m_programIDDiceBox, "model");
        glm::mat4 model = glm::mat4(1);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &model[0][0]);

        // the model matrix for the normal vector
        MatrixID = glGetUniformLocation(m_programIDDiceBox, "normalMatrix");
        glm::mat4 matrix = glm::transpose(glm::inverse(model));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &matrix[0][0]);

        // 1st attribute buffer : colors
        GLint colorID = glGetAttribLocation(m_programIDDiceBox, "inColor");
        glBindBuffer(GL_ARRAY_BUFFER, m_diceBox->vertexBuffer());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_diceBox->indexBuffer());
        glVertexAttribPointer(
                colorID,                          // The color of the attribute in the shader.
                4,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                sizeof(VertexSquareOutline),                   // stride
                (void *) (offsetof(VertexSquareOutline, color))// array buffer offset
        );
        glEnableVertexAttribArray(colorID);

        // attribute buffer : vertices for die
        GLint position = glGetAttribLocation(m_programIDDiceBox, "inPosition");
        glVertexAttribPointer(
                position,                        // The position of the attribute in the shader.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(VertexSquareOutline),                  // stride
                (void *) (offsetof(VertexSquareOutline, pos)) // array buffer offset
        );
        glEnableVertexAttribArray(position);

        // attribute buffer : normal vector to the face
        GLint normalID = glGetAttribLocation(m_programIDDiceBox, "inNormal");
        glVertexAttribPointer(
                normalID,                        // The normal vector to the fragment.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(VertexSquareOutline),                  // stride
                (void *) (offsetof(VertexSquareOutline, normal)) // array buffer offset
        );
        glEnableVertexAttribArray(normalID);

        // attribute buffer : vertices for die
        GLint corner1ID = glGetAttribLocation(m_programIDDiceBox, "inCorner1");
        glVertexAttribPointer(
                corner1ID,                       // The position of the first corner of the square.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(VertexSquareOutline),                  // stride
                (void *) (offsetof(VertexSquareOutline, corner1)) // array buffer offset
        );
        glEnableVertexAttribArray(corner1ID);

        // attribute buffer : vertices for die
        GLint corner2ID = glGetAttribLocation(m_programIDDiceBox, "inCorner2");
        glVertexAttribPointer(
                corner2ID,                       // The position of the second corner of the square.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(VertexSquareOutline),                  // stride
                (void *) (offsetof(VertexSquareOutline, corner2)) // array buffer offset
        );
        glEnableVertexAttribArray(corner2ID);

        // attribute buffer : vertices for die
        GLint corner3ID = glGetAttribLocation(m_programIDDiceBox, "inCorner3");
        glVertexAttribPointer(
                corner3ID,                       // The position of the third corner of the square.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(VertexSquareOutline),                  // stride
                (void *) (offsetof(VertexSquareOutline, corner3)) // array buffer offset
        );
        glEnableVertexAttribArray(corner3ID);

        // attribute buffer : vertices for die
        GLint corner4ID = glGetAttribLocation(m_programIDDiceBox, "inCorner4");
        glVertexAttribPointer(
                corner4ID,                       // The position of the forth corner of the square.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(VertexSquareOutline),                  // stride
                (void *) (offsetof(VertexSquareOutline, corner4)) // array buffer offset
        );
        glEnableVertexAttribArray(corner4ID);

        // Draw the triangles !
        glDrawElements(GL_TRIANGLES, m_diceBox->nbrIndices(), GL_UNSIGNED_INT, 0);

        glDisableVertexAttribArray(position);
        glDisableVertexAttribArray(colorID);
        glDisableVertexAttribArray(normalID);
        glDisableVertexAttribArray(corner1ID);
        glDisableVertexAttribArray(corner2ID);
        glDisableVertexAttribArray(corner3ID);
        glDisableVertexAttribArray(corner4ID);
    }

    eglSwapBuffers(m_surface->display(), m_surface->surface());
}

GLuint RainbowDiceGL::loadShaders(std::string const &vertexShaderFile, std::string const &fragmentShaderFile) {
    GLint Result = GL_TRUE;
    GLint InfoLogLength = 0;

    std::vector<char> vertexShader = readFile(vertexShaderFile);
    std::vector<char> fragmentShader = readFile(fragmentShaderFile);

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Compile Vertex Shader
    char const * VertexSourcePointer = vertexShader.data();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (Result == GL_FALSE) {
        if (InfoLogLength > 0) {
            std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(VertexShaderID, InfoLogLength, &InfoLogLength,
                               VertexShaderErrorMessage.data());
            if (VertexShaderErrorMessage[0] == '\0') {
                throw std::runtime_error("vertex shader compile error");
            } else {
                throw std::runtime_error(std::string("vertex shader compile error: ") + VertexShaderErrorMessage.data());
            }
        } else {
            throw std::runtime_error("vertex shader compile error");
        }
    }

    // Compile Fragment Shader
    char const * FragmentSourcePointer = fragmentShader.data();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (Result == GL_FALSE) {
        if (InfoLogLength > 0) {
            std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL,
                               FragmentShaderErrorMessage.data());
            if (FragmentShaderErrorMessage[0] == '\0') {
                throw std::runtime_error("Fragment shader compile error.");
            } else {
                throw std::runtime_error(std::string("Fragment shader compile error: ") + FragmentShaderErrorMessage.data());
            }
        } else {
            throw std::runtime_error("Fragment shader compile error.");
        }
    }

    // Link the program
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (Result == GL_FALSE) {
        if (InfoLogLength > 0) {
            std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
            glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, ProgramErrorMessage.data());
            throw std::runtime_error(ProgramErrorMessage.data());
        } else {
            throw std::runtime_error("Linker error.");
        }
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

void RainbowDiceGL::recreateModels() {
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            die->createGLResources();
        }
    }

    if (m_diceBox != nullptr) {
        m_diceBox->createGLResources();
    }
}

void RainbowDiceGL::destroyModelGLResources() {
    for (auto const &dice : m_dice) {
        for (auto const &die : dice) {
            die->destroyGLResources();
        }
    }

    if (m_diceBox != nullptr) {
        m_diceBox->destroyGLResources();
    }
}

void RainbowDiceGL::recreateSwapChain(uint32_t width, uint32_t height) {
    // destroy all GL resources
    destroyModelGLResources();
    if (m_texture != nullptr) {
        m_texture->destroyGLResources();
    }
    destroyGLResources();
    auto window = m_surface->window();
    m_surface.reset();

    // recreate everything from scratch and update the perspective matrix
    m_surface = std::make_shared<graphicsGL::Surface>(window);
    init();
    updatePerspectiveMatrix(m_surface->width(), m_surface->height());
    if (m_diceBox != nullptr) {
        m_diceBox->updateMaxXYZ(m_screenWidth/2.0f, m_screenHeight/2.0f, M_maxDicePosZ);
    }
    recreateModels();
    if (m_texture != nullptr) {
        m_texture->initGLResources();
    }

    // move dice to the new position on the screen according to the new screen size.
    if (m_drawRollingDice) {
        animateMoveStoppedDice();
    } else {
        // This will move the dice to stopped positions without animation.  We do not animate any
        // moving if the dice are not rolling dice.
        moveDiceToStoppedPositions();
    }
}

std::shared_ptr<DiceGL> RainbowDiceGL::createDie(std::vector<std::string> const &symbols,
                                  std::vector<uint32_t> const &inRerollIndices,
                                  std::vector<float> const &color) {
    return std::make_shared<DiceGL>(symbols, inRerollIndices, color, m_texture->textureAtlas());
}

std::shared_ptr<DiceGL> RainbowDiceGL::createDie(std::shared_ptr<DiceGL> const &inDice) {
    return std::make_shared<DiceGL>(inDice->die()->getSymbols(), inDice->rerollIndices(),
            inDice->die()->dieColor(), m_texture->textureAtlas());
}
