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

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "rainbowDice.hpp"
#include "dice.hpp"
#include "rainbowDiceGlobal.hpp"

class RainbowDiceGL : public RainbowDice {
public:
    virtual void initWindow(WindowType *window);

    virtual void initPipeline();

    virtual void initThread();

    virtual void cleanupThread();

    virtual void cleanup();

    virtual void drawFrame();

    virtual bool updateUniformBuffer();

    virtual bool allStopped();

    virtual void destroyWindow();

    virtual std::vector<std::string> getDiceResults();

    virtual void loadObject(std::vector<std::string> &symbols);

    virtual void destroyModels();

    virtual void recreateModels();

    virtual void recreateSwapChain();

    virtual void updateAcceleration(float x, float y, float z);

    virtual void resetPositions();

    virtual void resetPositions(std::set<int> &dice);

    virtual ~RainbowDiceGL() {}
private:
    WindowType *window;
    EGLContext context;
    EGLConfig config;
    EGLSurface surface;
    EGLDisplay display;
    GLuint programID;
    GLint MatrixID;
    std::vector<GLuint> texture;

    struct Dice {
        GLuint vertexBuffer;
        GLuint indexBuffer;
        std::shared_ptr<DicePhysicsModel> die;

        Dice(std::vector<std::string> &symbols, glm::vec3 position) : die(nullptr)
        {
            long nbrSides = symbols.size();
            if (nbrSides == 4) {
                die.reset((DicePhysicsModel*)new DiceModelTetrahedron(symbols, position));
            } else if (nbrSides == 12) {
                die.reset((DicePhysicsModel*)new DiceModelDodecahedron(symbols, position));
            } else if (nbrSides == 20) {
                die.reset((DicePhysicsModel*)new DiceModelIcosahedron(symbols, position));
            } else if (6 % nbrSides == 0) {
                die.reset((DicePhysicsModel*)new DiceModelCube(symbols, position));
            } else {
                die.reset((DicePhysicsModel*)new DiceModelHedron(symbols, position));
            }
        }

        void loadModel(int width, int height) {
            die->loadModel();
            die->setView();
            die->updatePerspectiveMatrix(width, height);
        }
    };
    std::vector<Dice> dice;

    GLuint loadShaders();
};