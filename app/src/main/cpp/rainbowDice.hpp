#ifndef RAINBOWDICE_HPP
#define RAINBOWDICE_HPP
#include <set>
#include <vector>
#include "rainbowDiceGlobal.hpp"

class RainbowDice {
public:
    virtual void initWindow(WindowType *window) = 0;

    virtual void initPipeline()=0;

    virtual void initThread()=0;

    virtual void cleanup()=0;

    virtual void drawFrame()=0;

    virtual bool updateUniformBuffer()=0;

    virtual bool allStopped()=0;

    virtual void destroyWindow()=0;

    virtual std::vector<std::string> getDiceResults()=0;

    virtual void loadObject(std::vector<std::string> &symbols)=0;

    virtual void destroyModels()=0;

    virtual void recreateModels()=0;

    virtual void recreateSwapChain()=0;

    virtual void updateAcceleration(float x, float y, float z)=0;

    virtual void resetPositions()=0;

    virtual void resetPositions(std::set<int> &dice)=0;

    virtual void resetToStoppedPositions(std::vector<std::string> const &symbols)=0;

    virtual void addRollingDiceAtIndices(std::set<int> &diceIndices)=0;

    virtual void cleanupThread()=0;

    virtual ~RainbowDice() { }
};
#endif