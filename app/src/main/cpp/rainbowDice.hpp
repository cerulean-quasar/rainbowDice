#ifndef RAINBOWDICE_HPP
#define RAINBOWDICE_HPP
#include <set>
#include <vector>
#include "rainbowDiceGlobal.hpp"

class RainbowDice {
public:
    virtual void initModels()=0;

    virtual void initThread()=0;

    virtual void drawFrame()=0;

    virtual bool updateUniformBuffer()=0;

    virtual bool allStopped()=0;

    virtual std::vector<std::vector<uint32_t>> getDiceResults()=0;

    virtual bool needsReroll()=0;

    virtual void loadObject(std::vector<std::string> const &symbols,
                            std::vector<uint32_t> const &rerollSymbol,
                            std::vector<float> const &color)=0;

    virtual void recreateModels()=0;

    virtual void recreateSwapChain()=0;

    virtual void updateAcceleration(float x, float y, float z)=0;

    virtual void resetPositions()=0;

    virtual void resetPositions(std::set<int> &dice)=0;

    virtual void resetToStoppedPositions(std::vector<uint32_t > const &inUpFaceIndices)=0;

    virtual void addRollingDice()=0;

    virtual void cleanupThread()=0;

    virtual ~RainbowDice() { }
};
#endif