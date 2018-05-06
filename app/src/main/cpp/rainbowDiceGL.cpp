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

GLuint texturetest;

void RainbowDiceGL::initWindow(WindowType *inWindow){
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
        EGL_NONE };

    window = inWindow;

    // Initialize EGL
    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        destroyWindow();
        throw std::runtime_error("Could not open display");
    }

    EGLint majorVersion;
    EGLint minorVersion;
    if (!eglInitialize(display, &majorVersion, &minorVersion)) {
        destroyWindow();
        throw std::runtime_error("Could not initialize display");
    }

    EGLint nbr_config;
    if (!eglChooseConfig(display, attribute_list, &config, 1, &nbr_config)) {
        destroyWindow();
        throw std::runtime_error("Could not get config");
    }

    if (nbr_config == 0) {
        destroyWindow();
        throw std::runtime_error("Got 0 configs.");
    }

    int32_t format;
    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        destroyWindow();
        throw std::runtime_error("Could not get display format");
    }
    ANativeWindow_setBuffersGeometry(window, 0, 0, format);

    if ((surface = eglCreateWindowSurface(display, config, window, nullptr)) == EGL_NO_SURFACE) {
        destroyWindow();
        throw std::runtime_error("Could not create surface");
    }

    EGLint contextAttributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    if ((context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttributes)) == EGL_NO_CONTEXT) {
        destroyWindow();
        throw std::runtime_error("Could not create context");
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        destroyWindow();
        throw std::runtime_error("Could not set the surface to current");
    }

    int width;
    int height;
    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        destroyWindow();
        throw std::runtime_error("Could not get width and height of surface");
    }

    // The clear background color is transparent black.
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it is closer to the camera than the former one.
    glDepthFunc(GL_LESS);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    glViewport(0, 0, width, height);
}

void RainbowDiceGL::initPipeline() {
    programID = loadShaders();

    int32_t w = ANativeWindow_getWidth(window);
    int32_t h = ANativeWindow_getHeight(window);
    for (auto die : dice) {
        die.loadModel(w, h);
    }

    // Get a handle for our "MVP" uniform
    // Only during the initialisation
    MatrixID = glGetUniformLocation(programID, "MVP");

    for (int i=0; i < dice.size(); i++) {
        // the vertex buffer
        glGenBuffers(1, &dice[i].vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, dice[i].vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * dice[i].die->vertices.size(),
                     dice[i].die->vertices.data(), GL_STATIC_DRAW);

        // the index buffer
        glGenBuffers(1, &dice[i].indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dice[i].indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * dice[i].die->indices.size(),
                     dice[i].die->indices.data(), GL_STATIC_DRAW);
    }

    // set the shader to use
    glUseProgram(programID);

    // load the textures
    glGenTextures(1, &texturetest);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturetest);

    // when sampling outside of the texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // when the texture is scaled up or down
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*GL_LINEAR_MIPMAP_LINEAR*/GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*GL_LINEAR*/ GL_NEAREST);

    std::vector<char> &texture = texAtlas->getImage();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texAtlas->getImageWidth(), texAtlas->getTextureHeight(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, texture.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    // needed because we are going to switch to another thread now
    if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        destroyWindow();
        throw std::runtime_error("Could not unset the surface to current");
    }
}

void RainbowDiceGL::initThread() {
    if (!eglMakeCurrent(display, surface, surface, context)) {
        throw std::runtime_error("Could not set the surface to current");
    }
}

void RainbowDiceGL::cleanupThread() {
    if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
        throw std::runtime_error("Could not unset the surface to current");
    }
}

void RainbowDiceGL::cleanup() {
    for (auto die : dice) {
        glDeleteBuffers(1, &die.vertexBuffer);
        glDeleteBuffers(1, &die.indexBuffer);
    }
    glDeleteTextures(1, &texturetest);
    destroyWindow();
}

void RainbowDiceGL::drawFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLint textureID = glGetUniformLocation(programID, "texSampler");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturetest);
    glUniform1i(textureID, 0);

    for (int i=0; i<dice.size(); i++) {
        // Send our transformation to the currently bound shader, in the "MVP"
        // uniform. This is done in the main loop since each model will have a
        // different MVP matrix (At least for the M part)
        glm::mat4 mvp = dice[i].die->ubo.proj * dice[i].die->ubo.view * dice[i].die->ubo.model;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

        // 1st attribute buffer : colors
        GLint colorID = glGetAttribLocation(programID, "inColor");
        glBindBuffer(GL_ARRAY_BUFFER, dice[i].vertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dice[i].indexBuffer);
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

        // Draw the triangles !
        //glDrawArrays(GL_TRIANGLES, 0, dice[0].die->vertices.size() /* total number of vertices*/);
        glDrawElements(GL_TRIANGLES, dice[i].die->indices.size(), GL_UNSIGNED_INT, 0);

        glDisableVertexAttribArray(position);
        glDisableVertexAttribArray(colorID);
        glDisableVertexAttribArray(texCoordID);
    }
    eglSwapBuffers(display, surface);
}

GLuint RainbowDiceGL::loadShaders() {
    GLint Result = GL_TRUE;
    GLint InfoLogLength = 0;

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

void RainbowDiceGL::updateUniformBuffer() {
    for (int i = 0; i < dice.size(); i++) {
        for (int j = i+1; j < dice.size(); j++) {
            dice[i].die->calculateBounce(dice[j].die.get());
        }
    }
    for (auto die : dice) {
        die.die->updateModelMatrix();
    }
}

bool RainbowDiceGL::allStopped() {
    for (auto die : dice) {
        if (!die.die->isStopped()) {
            return false;
        }
    }
    return true;
}
void RainbowDiceGL::destroyWindow() {
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;

    /* release the java window object */
    ANativeWindow_release(window);
}

std::vector<std::string> RainbowDiceGL::getDiceResults() {
    std::vector<std::string> results;
    for (auto die : dice) {
        if (!die.die->isStopped()) {
            // Should not happen
            throw std::runtime_error("Not all die are stopped!");
        }
        results.push_back(die.die->getResult());
    }

    return results;
}

void RainbowDiceGL::loadObject(std::vector<std::string> &symbols) {
    Dice o(symbols, glm::vec3(0.0f, 0.0f, -1.0f));
    dice.push_back(o);
}

void RainbowDiceGL::destroyModels() {
    dice.clear();
}

void RainbowDiceGL::recreateModels() {
    for (auto die : dice) {
        // create 1 buffer for the vertices (includes color and texture coordinates and which texture to use too).
        glGenBuffers(1, &die.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, die.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof (Vertex) * die.die->vertices.size(), die.die->vertices.data(), GL_STATIC_DRAW);

        // create 1 buffer for the indices
        glGenBuffers(1, &die.indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, die.indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof (Vertex) * die.die->indices.size(), die.die->indices.data(), GL_STATIC_DRAW);
    }
}

void RainbowDiceGL::recreateSwapChain() {
    // do nothing
}

void RainbowDiceGL::updateAcceleration(float x, float y, float z) {
    for (auto die : dice) {
        die.die->updateAcceleration(x, y, z);
    }
}

void RainbowDiceGL::resetPositions() {
    for (auto die: dice) {
        die.die->resetPosition();
    }
}

void RainbowDiceGL::resetPositions(std::set<int> &diceIndices) {
    for (auto i : diceIndices) {
        dice[i].die->resetPosition();
    }
}
