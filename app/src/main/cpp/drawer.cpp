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
#include "rainbowDiceGL.hpp"
#include "rainbowDiceGlobal.hpp"
#include "android.hpp"
#include "drawer.hpp"
#include "native-lib.hpp"

#ifdef CQ_ENABLE_VULKAN
#include "rainbowDiceVulkan.hpp"
#include "TextureAtlasVulkan.h"
#endif

std::shared_ptr<DrawEvent> DiceChannel::getEventNoWait() {
    // critical section
    std::unique_lock<std::mutex> lock(m_eventLock);

    if (m_stopDrawing) {
        m_stopDrawing = false;
        return std::make_shared<StopDrawingEvent>(StopDrawingEvent{});
    }

    if (m_drawEventQueue.empty()) {
        return std::shared_ptr<DrawEvent>();
    }

    std::shared_ptr<DrawEvent> event = m_drawEventQueue.front();
    m_drawEventQueue.pop();
    return event;
}

// blocks waiting for the next event.
std::shared_ptr<DrawEvent> DiceChannel::getEvent() {
    // critical section
    std::unique_lock<std::mutex> lock(m_eventLock);

    if (m_stopDrawing) {
        m_stopDrawing = false;
        return std::make_shared<StopDrawingEvent>(StopDrawingEvent{});
    }

    if (m_drawEventQueue.empty()) {
        m_eventConditionVariable.wait(lock);
    }

    if (m_stopDrawing) {
        m_stopDrawing = false;
        return std::make_shared<StopDrawingEvent>(StopDrawingEvent{});
    }

    std::shared_ptr<DrawEvent> event = m_drawEventQueue.front();
    m_drawEventQueue.pop();
    return event;
}

void DiceChannel::sendEvent(std::shared_ptr<DrawEvent> const &event) {
    std::unique_lock<std::mutex> lock(m_eventLock);

    m_drawEventQueue.push(event);
    m_eventConditionVariable.notify_one();
}

void DiceChannel::sendStopDrawingEvent() {
    std::unique_lock<std::mutex> lock(m_eventLock);

    m_stopDrawing = true;
    m_eventConditionVariable.notify_one();
}

DiceChannel &diceChannel() {
    static DiceChannel g_diceChannel{};

    return g_diceChannel;
}

void DiceWorker::initDiceGraphics(std::shared_ptr<WindowType> &surface) {
#ifdef CQ_ENABLE_VULKAN
    if (m_tryVulkan) {
        try {
            m_diceGraphics.reset(new RainbowDiceVulkan(surface));
        } catch (std::runtime_error &e) {
            m_tryVulkan = false;
        }
    }
#endif

    if (!m_tryVulkan) {
        m_diceGraphics.reset(new RainbowDiceGL(surface));
    }
}

void DiceWorker::waitingLoop() {
    while (true) {
        uint32_t nbrRequireRedraw = 0;
        std::shared_ptr<DrawEvent> event = diceChannel().getEvent();
        if (event->type() == DrawEvent::stopDrawing) {
            return;
        } else if ((*event)(m_diceGraphics, m_notify)) {
            nbrRequireRedraw++;
        }

        while (nbrRequireRedraw < m_maxEventsBeforeRedraw) {
            auto event = diceChannel().getEventNoWait();
            if (event == nullptr) {
                break;
            } else  if (event->type() == DrawEvent::stopDrawing) {
                return;
            } else if ((*event)(m_diceGraphics, m_notify)) {
                nbrRequireRedraw++;
            }
        }

        if (nbrRequireRedraw > 0) {
            m_diceGraphics->drawFrame();
        }

        while (!m_diceGraphics->allStopped()) {
            std::shared_ptr<DrawEvent> eventDrawing = drawingLoop();
            if (eventDrawing != nullptr) {
                if (eventDrawing->type() == DrawEvent::stopDrawing) {
                    return;
                } else {
                    (*eventDrawing)(m_diceGraphics, m_notify);
                }
            }
        }
    }
}

std::shared_ptr<DrawEvent> DiceWorker::drawingLoop() {
    Sensors sensor;

    while (true) {
        if (sensor.hasEvents()) {
            std::vector<Sensors::AccelerationEvent> events = sensor.getEvents();
            for (auto const &event : events) {
                m_diceGraphics->updateAcceleration(event.x, event.y, event.z);
            }

        }

        // events are a lot cheaper than a redraw, so process several of them.
        uint32_t nbrRequireRedraw = 0;
        while (nbrRequireRedraw < m_maxEventsBeforeRedraw) {
            auto event = diceChannel().getEventNoWait();
            if (event != nullptr) {
                switch (event->type()) {
                    case DrawEvent::stopDrawing:
                    case DrawEvent::diceChange:
                    case DrawEvent::drawStoppedDice:
                        // let the waiting loop handle these.  Just return the event - this signals
                        // that the waiting loop needs to handle these events.
                        return event;
                    case DrawEvent::surfaceChanged:
                    case DrawEvent::scrollSurface:
                    case DrawEvent::scaleSurface:
                    case DrawEvent::resetView:
                        // process the event.
                        if ((*event)(m_diceGraphics, m_notify)) {
                            nbrRequireRedraw++;
                        }
                        break;
                    case DrawEvent::tapDice:
                    case DrawEvent::rerollSelected:
                    case DrawEvent::addRerollSelected:
                    case DrawEvent::deleteSelected:
                        // ignore these events while drawing
                        break;
                }
            } else {
                break;
            }
        }

        bool needsRedraw = m_diceGraphics->updateUniformBuffer();
        if (needsRedraw || nbrRequireRedraw > 0) {
            m_diceGraphics->drawFrame();
        }

        if (m_diceGraphics->allStopped()) {
            if (m_diceGraphics->needsReroll()) {
                m_diceGraphics->addRollingDice();
                continue;
            }
            std::vector<std::vector<uint32_t>> results = m_diceGraphics->getDiceResults();
            m_notify->sendResult(m_diceGraphics->diceName(), m_diceGraphics->isModifiedRoll(),
                    results, m_diceGraphics->getDiceDescriptions());
            return std::shared_ptr<DrawEvent>();
        }
    }
}
