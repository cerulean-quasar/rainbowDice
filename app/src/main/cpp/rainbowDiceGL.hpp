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
#include <list>
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

    virtual void resetToStoppedPositions(std::vector<std::string> const &symbols);

    virtual void addRollingDiceAtIndices(std::set<int> &diceIndices);

    virtual ~RainbowDiceGL() {}
private:
    WindowType *window;
    EGLContext context;
    EGLConfig config;
    EGLSurface surface;
    EGLDisplay display;
    GLuint programID;
    std::vector<GLuint> texture;

    struct Dice {
        GLuint vertexBuffer;
        GLuint indexBuffer;
        std::shared_ptr<DicePhysicsModel> die;

        // If the die is being rerolled, then don't count it in the results list returned because
        // the GUI is expecting a new list of results that only contains one set of rolls for a die
        // and then uses the index to determine what the reroll value was.
        bool isBeingReRolled;

        Dice(std::vector<std::string> &symbols, glm::vec3 position) : die(nullptr)
        {
            isBeingReRolled = false;
            long nbrSides = symbols.size();
            if (nbrSides == 4) {
                die.reset((DicePhysicsModel*)new DiceModelTetrahedron(symbols, position, true));
            } else if (nbrSides == 12) {
                die.reset((DicePhysicsModel*)new DiceModelDodecahedron(symbols, position, true));
            } else if (nbrSides == 20) {
                die.reset((DicePhysicsModel*)new DiceModelIcosahedron(symbols, position, true));
            } else if (6 % nbrSides == 0) {
                die.reset((DicePhysicsModel*)new DiceModelCube(symbols, position, true));
            } else {
                die.reset((DicePhysicsModel*)new DiceModelHedron(symbols, position, nbrSides, true));
            }
        }

        void loadModel(int width, int height) {
            die->loadModel();
            die->setView();
            die->updatePerspectiveMatrix(width, height);
        }

        ~Dice() {
            glDeleteBuffers(1, &vertexBuffer);
            glDeleteBuffers(1, &indexBuffer);
        }
    };

    typedef std::list<std::shared_ptr<Dice> > DiceList;
    DiceList dice;

    GLuint loadShaders(std::string vertexShaderFile, std::string fragmentShaderFile);
};