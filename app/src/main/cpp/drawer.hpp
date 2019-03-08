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
#ifndef RAINBOWDICE_DRAWER_HPP
#define RAINBOWDICE_DRAWER_HPP

#include <atomic>
#include <mutex>
#include <queue>
#include <vector>
#include "dice.hpp"
#include "rainbowDice.hpp"
#include "text.hpp"
#include "native-lib.hpp"
#include "diceDescription.hpp"

class DrawEvent {
public:
    enum evtype {
        stopDrawing,
        surfaceDirty,
        surfaceChanged,
        diceChange,
        drawStoppedDice,
        scrollSurface,
        scaleSurface
    };

    // returns true if the surface needs redrawing after this event.
    virtual bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) = 0;

    virtual evtype type() = 0;
    virtual ~DrawEvent() = default;
};

class StopDrawingEvent : public DrawEvent {
public:
    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) override {
        return false;
    }

    evtype type() override { return stopDrawing; }

    ~StopDrawingEvent() override = default;
};

class SurfaceChangedEvent : public DrawEvent {
    uint32_t m_width;
    uint32_t m_height;
public:
    SurfaceChangedEvent(uint32_t width, uint32_t height)
        : m_width(width),
        m_height(height) {
    }
    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) override {
        diceGraphics->recreateSwapChain(m_width, m_height);

        // redraw the frame right away and then return false because this event means that we
        // should redraw immediately and not wait for some number of events to come up before we
        // do the redraw.
        diceGraphics->drawFrame();
        return false;
    }

    evtype type() override { return surfaceChanged; }

    ~SurfaceChangedEvent() override = default;
};

class ScrollEvent : public DrawEvent {
private:
    float m_distanceX;
    float m_distanceY;
public:
    ScrollEvent(float inDistanceX, float inDistanceY)
        : m_distanceX{inDistanceX},
          m_distanceY{inDistanceY} {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) override {
        diceGraphics->scroll(m_distanceX, m_distanceY);
        return true;
    }

    evtype type() override { return scrollSurface; }

    ~ScrollEvent() override = default;
};

class ScaleEvent : public DrawEvent {
private:
    float m_scaleFactor;
public:
    ScaleEvent(float inScaleFactor)
            : m_scaleFactor{inScaleFactor} {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) override {
        diceGraphics->scale(m_scaleFactor);
        return true;
    }

    evtype type() override { return scaleSurface; }

    ~ScaleEvent() override = default;
};

class DrawStoppedDiceEvent : public DrawEvent {
    std::vector<std::shared_ptr<DiceDescription>> m_dice;
    std::shared_ptr<TextureAtlas> m_texture;
    std::vector<uint32_t> m_upFaceIndices;
public:
    DrawStoppedDiceEvent(std::vector<std::shared_ptr<DiceDescription>> inDice,
                    std::shared_ptr<TextureAtlas> inTexture,
                    std::vector<uint32_t> inUpFaceIndices)
            : m_dice{std::move(inDice)},
              m_texture{std::move(inTexture)},
              m_upFaceIndices{std::move(inUpFaceIndices)}
    {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) override {
        diceGraphics->setTexture(m_texture);
        diceGraphics->setDice("", m_dice);
        diceGraphics->initModels();
        diceGraphics->resetToStoppedPositions(m_upFaceIndices);

        // redraw the frame right away and then return false because this event means that we
        // should redraw immediately and not wait for some number of events to come up before we
        // do the redraw.
        diceGraphics->drawFrame();
        return false;
    }

    evtype type() override { return drawStoppedDice; }

    ~DrawStoppedDiceEvent() override = default;
};

class DiceChangeEvent : public DrawEvent {
    std::string m_diceName;
    std::vector<std::shared_ptr<DiceDescription>> m_dice;
    std::shared_ptr<TextureAtlas> m_texture;
public:
    DiceChangeEvent(std::string inDiceName, std::vector<std::shared_ptr<DiceDescription>> inDice,
        std::shared_ptr<TextureAtlas> inTexture)
        : m_diceName{std::move(inDiceName)},
        m_dice{std::move(inDice)},
        m_texture{std::move(inTexture)}
    {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) override {
        diceGraphics->setTexture(m_texture);
        diceGraphics->setDice(m_diceName, m_dice);
        diceGraphics->initModels();
        diceGraphics->resetPositions();
        return true;
    }

    evtype type() override { return diceChange; }

    ~DiceChangeEvent() override = default;
};

class SurfaceDirtyEvent : public DrawEvent {
    std::shared_ptr<WindowType> m_surface;
public:
    explicit SurfaceDirtyEvent(std::shared_ptr<WindowType> inSurface)
        : m_surface{std::move(inSurface)} {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics) override {
        diceGraphics->newSurface(m_surface);

        // redraw the frame right away and then return false because this event means that we
        // should redraw immediately and not wait for some number of events to come up before we
        // do the redraw.
        diceGraphics->drawFrame();
        return false;
    }

    evtype type() override { return surfaceDirty; }

    ~SurfaceDirtyEvent() override = default;
};

/* Used to communicate between the gui thread and the drawing thread.
 */
class DiceChannel {
private:
    std::mutex m_eventLock;
    std::condition_variable m_eventConditionVariable;

    std::shared_ptr<WindowType> m_nextSurface;
    std::queue<std::shared_ptr<DrawEvent>> m_drawEventQueue;
    bool m_stopDrawing;
    bool m_surfaceDirty;

public:
    DiceChannel()
            : m_stopDrawing{false},
              m_surfaceDirty{false}
    {}

    std::shared_ptr<DrawEvent> getEventNoWait();
    std::shared_ptr<DrawEvent> getEvent();

    // blocks on sending event
    void sendEvent(std::shared_ptr<DrawEvent> const &event);
    void sendStopDrawingEvent();
    void sendSurfaceDirtyEvent(std::shared_ptr<WindowType> inSurface);
};

DiceChannel &diceChannel();

class DiceWorker {
    static constexpr uint32_t m_maxEventsBeforeRedraw = 128;

    bool m_tryVulkan;
    std::unique_ptr<RainbowDice> m_diceGraphics;
    std::unique_ptr<Notify> m_notify;
public:
    DiceWorker(std::shared_ptr<WindowType> &inSurface,
               std::unique_ptr<Notify> &inNotify)
            : m_tryVulkan{true},
              m_diceGraphics{},
              m_notify{std::move(inNotify)}
     {
#ifndef CQ_ENABLE_VULKAN
        m_tryVulkan = false;
#endif
        initDiceGraphics(inSurface);
    }

    void initDiceGraphics(std::shared_ptr<WindowType> &surface);
    void waitingLoop();
    std::shared_ptr<DrawEvent> drawingLoop();
};

#endif // RAINBOWDICE_DRAWER_HPP
