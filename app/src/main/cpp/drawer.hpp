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
#include <bitset>
#include "dice.hpp"
#include "rainbowDice.hpp"
#include "text.hpp"
#include "native-lib.hpp"
#include "diceDescription.hpp"

class DrawEvent {
public:
    enum evtype {
        stopDrawing,
        surfaceChanged,
        diceChange,
        drawStoppedDice,
        scrollSurface,
        scaleSurface,
        tapDice,
        rerollSelected,
        addRerollSelected,
        deleteSelected,
        resetView
    };

    // returns true if the surface needs redrawing after this event.
    virtual bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
            std::shared_ptr<Notify> &notify) = 0;

    virtual evtype type() = 0;
    virtual ~DrawEvent() = default;
};

class StopDrawingEvent : public DrawEvent {
public:
    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
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
    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        // giggle the device so that when the swapchain is recreated, it gets the correct width and
        // height.
        if (diceGraphics->hasDice()) {
            diceGraphics->updateUniformBuffer();
            diceGraphics->drawFrame();
        }

        diceGraphics->recreateSwapChain(m_width, m_height);

        // redraw the frame right away and then return false because this event means that we
        // should redraw immediately and not wait for some number of events to come up before we
        // do the redraw.
        if (diceGraphics->hasDice()) {
            diceGraphics->drawFrame();
        }
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

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
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
    explicit ScaleEvent(float inScaleFactor)
            : m_scaleFactor{inScaleFactor} {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        diceGraphics->scale(m_scaleFactor);
        return true;
    }

    evtype type() override { return scaleSurface; }

    ~ScaleEvent() override = default;
};

class TapDiceEvent : public DrawEvent {
private:
    float m_x;
    float m_y;
public:
    TapDiceEvent(float inX, float inY)
            : m_x{inX},
              m_y{inY} {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        bool before = diceGraphics->diceSelected();
        bool needsRedraw = diceGraphics->tapDice(m_x, m_y);
        bool after = diceGraphics->diceSelected();
        if (before != after) {
            notify->sendSelected(after);
        }
        return needsRedraw;
    }

    evtype type() override { return tapDice; }

    ~TapDiceEvent() override = default;
};

class RerollSelected : public DrawEvent {
public:
    RerollSelected() = default;

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        if (diceGraphics->diceSelected()) {
            bool hasResult = diceGraphics->rerollSelected();
            notify->sendSelected(false);
            if (hasResult) {
                std::vector<std::vector<uint32_t>> results = diceGraphics->getDiceResults();
                notify->sendResult(diceGraphics->diceName(), diceGraphics->isModifiedRoll(),
                                   results, diceGraphics->getDiceDescriptions());
                return true;
            } else {
                // Return false here because we will enter the drawing loop and then the dice will get
                // redrawn.  If we return true here, then the dice will display, but then there will be
                // a longish pause (about half a second) as the sensors get initialized at the top of the
                // drawing loop.
                return false;
            }
        } else {
            return false;
        }
    }

    evtype type() override { return rerollSelected; }

    ~RerollSelected() override = default;
};

class AddRerollSelected : public DrawEvent {
public:
    AddRerollSelected() = default;

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        if (diceGraphics->diceSelected()) {
            bool hasResult = diceGraphics->addRerollSelected();
            notify->sendSelected(false);
            if (hasResult) {
                std::vector<std::vector<uint32_t>> results = diceGraphics->getDiceResults();
                notify->sendResult(diceGraphics->diceName(), diceGraphics->isModifiedRoll(),
                                   results, diceGraphics->getDiceDescriptions());
                return true;
            } else {
                // Return false here because we will enter the drawing loop and then the dice will get
                // redrawn.  If we return true here, then the dice will display, but then there will be
                // a longish pause (about half a second) as the sensors get initialized at the top of the
                // drawing loop.
                return false;
            }
        } else {
            return false;
        }
    }

    evtype type() override { return addRerollSelected; }

    ~AddRerollSelected() override = default;
};

class DeleteSelected : public DrawEvent {
public:
    DeleteSelected() = default;

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        if (diceGraphics->deleteSelected()) {
            std::vector<std::vector<uint32_t>> results = diceGraphics->getDiceResults();
            notify->sendResult(diceGraphics->diceName(), diceGraphics->isModifiedRoll(),
                                 results, diceGraphics->getDiceDescriptions());
            notify->sendSelected(false);
            return true;
        } else {
            return false;
        }
    }

    evtype type() override { return deleteSelected; }

    ~DeleteSelected() override = default;
};

class ResetView : public DrawEvent {
public:
    ResetView() = default;

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        diceGraphics->resetView();
        return true;
    }

    evtype type() override { return resetView; }

    ~ResetView() override = default;
};

class DrawStoppedDiceEvent : public DrawEvent {
    std::string m_name;
    bool m_isModifiedRoll;
    std::vector<std::shared_ptr<DiceDescription>> m_dice;
    std::shared_ptr<TextureAtlas> m_texture;
    std::vector<std::vector<uint32_t>> m_upFaceIndices;
public:
    DrawStoppedDiceEvent(std::string inName,
                std::vector<std::shared_ptr<DiceDescription>> inDice,
                std::shared_ptr<TextureAtlas> inTexture,
                std::vector<std::vector<uint32_t>> inUpFaceIndices,
                bool inIsModifiedRoll)
            : m_name{std::move(inName)},
              m_dice{std::move(inDice)},
              m_texture{std::move(inTexture)},
              m_upFaceIndices{std::move(inUpFaceIndices)},
              m_isModifiedRoll{inIsModifiedRoll}
    {
    }

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        diceGraphics->setTexture(m_texture);
        diceGraphics->setDice(m_name, m_dice, m_isModifiedRoll);
        diceGraphics->initModels();
        diceGraphics->resetToStoppedPositions(m_upFaceIndices);

        // redraw the frame right away and then return false because this event means that we
        // should redraw immediately and not wait for some number of events to come up before we
        // do the redraw.
        diceGraphics->drawFrame();
        notify->sendSelected(false);
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

    bool operator() (std::unique_ptr<RainbowDice> &diceGraphics,
                     std::shared_ptr<Notify> &notify) override {
        bool hasResult = diceGraphics->changeDice(m_diceName, m_dice, m_texture);
        notify->sendSelected(false);
        if (hasResult) {
            // The rolling dice are not animated, they are just moved to stopped positions.
            // Return the results to the GUI.
            std::vector<std::vector<uint32_t>> results = diceGraphics->getDiceResults();
            notify->sendResult(diceGraphics->diceName(), diceGraphics->isModifiedRoll(),
                                 results, diceGraphics->getDiceDescriptions());
            return true;
        } else {
            // We don't have a result for the dice here because these are rolling dice.
            // Return false here because we will enter the drawing loop and then the dice will get
            // redrawn.  If we return true here, then the dice will display, but then there will be
            // a longish pause (about half a second) as the sensors get initialized at the top of the
            // drawing loop.
            return false;
        }
    }

    evtype type() override { return diceChange; }

    ~DiceChangeEvent() override = default;
};

/* Used to communicate between the gui thread and the drawing thread.
 */
class DiceChannel {
private:
    std::mutex m_eventLock;
    std::condition_variable m_eventConditionVariable;

    std::queue<std::shared_ptr<DrawEvent>> m_drawEventQueue;
    bool m_stopDrawing;

public:
    DiceChannel()
            : m_stopDrawing{false}
    {}

    std::shared_ptr<DrawEvent> getEventNoWait();
    std::shared_ptr<DrawEvent> getEvent();

    // blocks on sending event
    void sendEvent(std::shared_ptr<DrawEvent> const &event);
    void sendStopDrawingEvent();
    void clearQueue();
};

DiceChannel &diceChannel();

class DiceWorker {
public:
    bool needsReport(DrawEvent::evtype evtype) {
        return evtype == DrawEvent::diceChange ||
            evtype == DrawEvent::rerollSelected ||
            evtype == DrawEvent::addRerollSelected ||
            evtype == DrawEvent::deleteSelected;
    }

    DiceWorker(std::shared_ptr<WindowType> inSurface,
               std::shared_ptr<Notify> inNotify,
               bool inUseGravity,
               bool inDrawRollingDice,
               bool inUseLegacy,
               bool reverseGravity)
            : m_whichSensors{},
              m_tryVulkan{!inUseLegacy},
              m_diceGraphics{},
              m_notify{std::move(inNotify)}
    {
        std::bitset<3> whichSensors = Sensors::hasWhichSensors();
        if (inDrawRollingDice) {
            if (!inUseGravity) {
                // The user selected to not use gravity.  We need the Linear Acceleration Sensor
                // for this feature.
                if (!whichSensors.test(Sensors::LINEAR_ACCELERATION_SENSOR)) {
                    inUseGravity = true;
                } else {
                    m_whichSensors.set(Sensors::LINEAR_ACCELERATION_SENSOR);
                }
            }

            if (inUseGravity) {
                // prefer using the Linear Acceleration sensor and Gravity sensor.  If these aren't
                // there then use the Accelerometer.  This is because the accelerometer does not
                // sense shaking enough and we end up needing a high pass filter to separate out the
                // shaking and gravity then add the shaking back in at a higher factor.  Might as
                // well let android do this work.
                if (whichSensors.test(Sensors::LINEAR_ACCELERATION_SENSOR) &&
                    whichSensors.test(Sensors::GRAVITY_SENSOR)) {
                    m_whichSensors.set(Sensors::LINEAR_ACCELERATION_SENSOR);
                    m_whichSensors.set(Sensors::GRAVITY_SENSOR);
                } else if (whichSensors.test(Sensors::ACCELEROMETER_SENSOR)) {
                    m_whichSensors.set(Sensors::ACCELEROMETER_SENSOR);
                } else {
                    // give up, we can't draw rolling dice.
                    // TODO: use flick gesture instead?
                    inDrawRollingDice = false;
                }
            }
        }

        initDiceGraphics(std::move(inSurface), inUseGravity, inDrawRollingDice, reverseGravity);
        m_notify->sendGraphicsDescription(m_diceGraphics->graphicsDescription(),
                                          whichSensors.test(Sensors::LINEAR_ACCELERATION_SENSOR),
                                          whichSensors.test(Sensors::GRAVITY_SENSOR),
                                          whichSensors.test(Sensors::ACCELEROMETER_SENSOR));
    }

    void waitingLoop();
private:
    static constexpr uint32_t m_maxEventsBeforeRedraw = 128;

    std::bitset<3> m_whichSensors;
    bool m_tryVulkan;
    std::unique_ptr<RainbowDice> m_diceGraphics;
    std::shared_ptr<Notify> m_notify;

    std::shared_ptr<DrawEvent> drawingLoop(bool reportResult);
    void initDiceGraphics(std::shared_ptr<WindowType> surface,
                          bool inUseGravity, bool inDrawRollingDice, bool reverseGravity);
};

#endif // RAINBOWDICE_DRAWER_HPP
