/**
 * Copyright 2018 Cerulean Quasar. All Rights Reserved.
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

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <native_window.h>
#include <string.h>
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
        ANativeWindow_setBuffersGeometry(m_window, 0, 0, format);

        if ((m_surface = eglCreateWindowSurface(m_display, m_config, m_window, nullptr)) ==
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

        if (!eglQuerySurface(m_display, m_surface, EGL_WIDTH, &m_width) ||
            !eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &m_height)) {
            destroySurface();
            throw std::runtime_error("Could not get width and height of surface");
        }

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
        // Accept fragment if it is closer to the camera than the former one.
        glDepthFunc(GL_LESS);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

        /* release the java window object */
        ANativeWindow_release(m_window);
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

std::string const SHADER_VERT_FILE("shaderGL.vert");
std::string const SHADER_FRAG_FILE("shaderGL.frag");

void RainbowDiceGL::init() {
    programID = loadShaders(SHADER_VERT_FILE, SHADER_FRAG_FILE);

    // set the shader to use
    glUseProgram(programID);
}

void RainbowDiceGL::initModels() {
    for (auto &&die : dice) {
        die->loadModel(m_surface.width(), m_surface.height(), m_textureAtlas);
    }

    for (auto &&die : dice) {
        // the vertex buffer
        glGenBuffers(1, &die->vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, die->vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * die->die->vertices.size(),
                     die->die->vertices.data(), GL_STATIC_DRAW);

        // the index buffer
        glGenBuffers(1, &die->indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, die->indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * die->die->indices.size(),
                     die->die->indices.data(), GL_STATIC_DRAW);
    }

    // needed because we are going to switch to another thread now
    m_surface.cleanupThread();
}

void RainbowDiceGL::drawFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLint textureID = glGetUniformLocation(programID, "texSampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureAtlas->texture());
    glUniform1i(textureID, 0);

    GLint viewPos = glGetUniformLocation(programID, "viewPosition");
    glUniform3fv(viewPos, 1, &viewPoint[0]);

    for (auto && die : dice) {
        // Send our transformation to the currently bound shader, in the "MVP"
        // uniform. This is done in the main loop since each model will have a
        // different MVP matrix (At least for the M part)

        // the projection matrix
        GLint MatrixID = glGetUniformLocation(programID, "proj");
        glm::mat4 matrix = die->die->ubo.proj;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &matrix[0][0]);

        // view matrix
        MatrixID = glGetUniformLocation(programID, "view");
        matrix = die->die->ubo.view;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &matrix[0][0]);

        // model matrix
        MatrixID = glGetUniformLocation(programID, "model");
        matrix = die->die->ubo.model;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &matrix[0][0]);

        // the model matrix for the normal vector
        MatrixID = glGetUniformLocation(programID, "normalMatrix");
        matrix = glm::transpose(glm::inverse(die->die->ubo.model));
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &matrix[0][0]);

        // 1st attribute buffer : colors
        GLint colorID = glGetAttribLocation(programID, "inColor");
        glBindBuffer(GL_ARRAY_BUFFER, die->vertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, die->indexBuffer);
        glVertexAttribPointer(
                colorID,                          // The position of the attribute in the shader.
                3,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                sizeof(Vertex),                   // stride
                (void *) (offsetof(Vertex, color))// array buffer offset
        );
        glEnableVertexAttribArray(colorID);

        // attribute buffer : vertices for die
        GLint position = glGetAttribLocation(programID, "inPosition");
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
        GLint texCoordID = glGetAttribLocation(programID, "inTexCoord");
        glVertexAttribPointer(
            texCoordID,                       // The position of the attribute in the shader
            2,                                // size
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            sizeof (Vertex),                  // stride
            (void*) offsetof (Vertex, texCoord)  // array buffer offset
        );
        glEnableVertexAttribArray(texCoordID);

        // attribute buffer : vertices for die
        GLint normalID = glGetAttribLocation(programID, "inNormal");
        glVertexAttribPointer(
                normalID,                        // The position of the attribute in the shader.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(Vertex),                  // stride
                (void *) (offsetof(Vertex, normal)) // array buffer offset
        );
        glEnableVertexAttribArray(normalID);

        // attribute buffer : vertices for die
        GLint cornerNormalID = glGetAttribLocation(programID, "inCornerNormal");
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
        GLint corner1ID = glGetAttribLocation(programID, "inCorner1");
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
        GLint corner2ID = glGetAttribLocation(programID, "inCorner2");
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
        GLint corner3ID = glGetAttribLocation(programID, "inCorner3");
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
        GLint corner4ID = glGetAttribLocation(programID, "inCorner4");
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
        GLint corner5ID = glGetAttribLocation(programID, "inCorner5");
        glVertexAttribPointer(
                corner5ID,                        // The position of the attribute in the shader.
                3,                               // size
                GL_FLOAT,                        // type
                GL_FALSE,                        // normalized?
                sizeof(Vertex),                  // stride
                (void *) (offsetof(Vertex, corner5)) // array buffer offset
        );
        glEnableVertexAttribArray(corner5ID);

        // Draw the triangles !
        //glDrawArrays(GL_TRIANGLES, 0, dice[0].die->vertices.size() /* total number of vertices*/);
        glDrawElements(GL_TRIANGLES, die->die->indices.size(), GL_UNSIGNED_INT, 0);

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
    }
    eglSwapBuffers(m_surface.display(), m_surface.surface());
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

bool RainbowDiceGL::updateUniformBuffer() {
    for (DiceList::iterator iti = dice.begin(); iti != dice.end(); iti++) {
        if (!iti->get()->die->isStopped()) {
            for (DiceList::iterator itj = iti; itj != dice.end(); itj++) {
                if (!itj->get()->die->isStopped() && iti != itj) {
                    iti->get()->die->calculateBounce(itj->get()->die.get());
                }
            }
        }
    }

    bool needsRedraw = false;
    uint32_t i = 0;
    for (auto &&die : dice) {
        if (die->die->updateModelMatrix()) {
            needsRedraw = true;
        }
        if (die->die->isStopped() && !die->die->isStoppedAnimationStarted()) {
            float width = screenWidth;
            float height = screenHeight;
            uint32_t nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
            uint32_t stoppedX = i%nbrX;
            uint32_t stoppedY = i/nbrX;
            float x = -width/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = -height/2 + (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die->animateMove(x, y);
        }
        i++;
    }

    return needsRedraw;
}

bool RainbowDiceGL::allStopped() {
    for (auto &&die : dice) {
        if (!die->die->isStopped() || !die->die->isStoppedAnimationDone()) {
            return false;
        }
    }
    return true;
}

std::vector<std::vector<std::string>> RainbowDiceGL::getDiceResults() {
    std::vector<std::vector<std::string>> results;
    for (DiceList::iterator dieIt = dice.begin(); dieIt != dice.end(); dieIt++) {
        if (!dieIt->get()->die->isStopped()) {
            // Should not happen
            throw std::runtime_error("Not all die are stopped!");
        }
        std::vector<std::string> dieResults;
        while (dieIt->get()->isBeingReRolled) {
            dieResults.push_back(dieIt->get()->die->getResult());
            dieIt++;
        }
        dieResults.push_back(dieIt->get()->die->getResult());
        results.push_back(dieResults);
    }

    return results;
}

bool RainbowDiceGL::needsReroll() {
    for (auto const &die : dice) {
        if (die->needsReroll()) {
            return true;
        }
    }

    return false;
}

void RainbowDiceGL::loadObject(std::vector<std::string> const &symbols, std::string const &inRerollSymbol) {
    glm::vec3 position(0.0f, 0.0f, -1.0f);
    std::shared_ptr<DiceGL> o(new DiceGL(symbols, inRerollSymbol, position));
    dice.push_back(o);
}

void RainbowDiceGL::recreateModels() {
    for (auto die : dice) {
        // create 1 buffer for the vertices (includes color and texture coordinates and which texture to use too).
        glGenBuffers(1, &die->vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, die->vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof (Vertex) * die->die->vertices.size(), die->die->vertices.data(), GL_STATIC_DRAW);

        // create 1 buffer for the indices
        glGenBuffers(1, &die->indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, die->indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (Vertex) * die->die->indices.size(), die->die->indices.data(), GL_STATIC_DRAW);
    }
}

void RainbowDiceGL::recreateSwapChain() {
    // do nothing
}

void RainbowDiceGL::updateAcceleration(float x, float y, float z) {
    for (auto && die : dice) {
        die->die->updateAcceleration(x, y, z);
    }
}

void RainbowDiceGL::resetPositions() {
    for (auto && die: dice) {
        die->die->resetPosition();
    }
}

void RainbowDiceGL::resetPositions(std::set<int> &diceIndices) {
    int i = 0;
    for (auto &&die : dice) {
        if (diceIndices.find(i) != diceIndices.end()) {
            die->die->resetPosition();
        }
        i++;
    }
}
void RainbowDiceGL::addRollingDice() {
    for (DiceList::iterator diceIt = dice.begin(); diceIt != dice.end(); diceIt++) {
        // Skip dice already being rerolled or does not get rerolled.
        if (diceIt->get()->isBeingReRolled || diceIt->get()->rerollSymbol.size() == 0) {
            continue;
        }

        std::string result = diceIt->get()->die->getResult();
        if (result != diceIt->get()->rerollSymbol) {
            continue;
        }

        DiceGL *currentDie = diceIt->get();
        glm::vec3 position(0.0f, 0.0f, -1.0f);
        std::shared_ptr<DiceGL> die(new DiceGL(currentDie->die->symbols, currentDie->rerollSymbol,
                                               position));
        diceIt->get()->isBeingReRolled = true;
        int32_t w = m_surface.width();
        int32_t h = m_surface.height();
        die->loadModel(w, h, m_textureAtlas);

        // the vertex buffer
        glGenBuffers(1, &die->vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, die->vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * die->die->vertices.size(),
                     die->die->vertices.data(), GL_STATIC_DRAW);

        // the index buffer
        glGenBuffers(1, &die->indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, die->indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * die->die->indices.size(),
                     die->die->indices.data(), GL_STATIC_DRAW);
        die->die->resetPosition();

        diceIt++;
        dice.insert(diceIt, die);
        diceIt--;
    }

    int i=0;
    for (auto &&die : dice) {
        if (die->die->isStopped()) {
            float width = screenWidth;
            float height = screenHeight;
            uint32_t nbrX = static_cast<uint32_t>(width/(2*DicePhysicsModel::stoppedRadius));
            uint32_t stoppedX = i%nbrX;
            uint32_t stoppedY = i/nbrX;
            float x = -width/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
            float y = -height/2 + (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;
            die->die->animateMove(x, y);
        }
        i++;
    }
}

void RainbowDiceGL::resetToStoppedPositions(std::vector<std::string> const &symbols) {
    uint32_t i = 0;
    for (auto &&die : dice) {
        uint32_t nbrX = static_cast<uint32_t>(screenWidth/(2*DicePhysicsModel::stoppedRadius));
        uint32_t stoppedX = i%nbrX;
        uint32_t stoppedY = i/nbrX;
        float x = -screenWidth/2 + (2*stoppedX + 1) * DicePhysicsModel::stoppedRadius;
        float y = -screenHeight/2 + (2*stoppedY + 1) * DicePhysicsModel::stoppedRadius;

        die->die->positionDice(symbols[i++], x, y);
    }
}