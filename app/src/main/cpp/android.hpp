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
#ifndef RAINBOWDICE_ANDROID_HPP
#define RAINBOWDICE_ANDROID_HPP

#include <android/native_window.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <memory>
#include <streambuf>
#include <bitset>
#include <android/sensor.h>

class Sensors {
public:
    static constexpr uint32_t LINEAR_ACCELERATION_SENSOR = 0;
    static constexpr uint32_t GRAVITY_SENSOR = 1;
    static constexpr uint32_t ACCELEROMETER_SENSOR = 2;

    struct AccelerationEvent {
        float x;
        float y;
        float z;
    };

    static std::bitset<3> hasWhichSensors() {
        std::bitset<3> result{};
        ASensorManager *sensorManager = ASensorManager_getInstance();
        ASensor const *sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_LINEAR_ACCELERATION);
        if (sensor != nullptr) {
            result.set(LINEAR_ACCELERATION_SENSOR);
        }

        sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GRAVITY);
        if (sensor != nullptr) {
            result.set(GRAVITY_SENSOR);
        }

        sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
        if (sensor != nullptr) {
            result.set(ACCELEROMETER_SENSOR);
        }

        return result;
    }

    inline bool hasLinearAccerationEvents() {
        return hasEvents(m_eventQueueLinearAcceleration);
    }

    inline bool hasGravityEvents() {
        return hasEvents(m_eventQueueGravity);
    }

    inline bool hasAccelerometerEvents() {
        return hasEvents(m_eventQueueAccelerometer);
    }

    inline std::vector<AccelerationEvent> getLinearAccelerationEvents() {
        return std::move(getEvents(m_eventQueueLinearAcceleration));
    }

    inline std::vector<AccelerationEvent> getGravityEvents() {
        return std::move(getEvents(m_eventQueueGravity));
    }

    inline std::vector<AccelerationEvent> getAccelerometerEvents() {
        return std::move(getEvents(m_eventQueueAccelerometer));
    }

    ~Sensors() {
        destroyResources();
    }

    explicit Sensors(std::bitset<3> inWhichSensors)
    : m_sensorManager{nullptr},
      m_sensorLinearAcceleration{nullptr},
      m_sensorGravity{nullptr},
      m_sensorAccelerometer{nullptr},
      m_eventQueueLinearAcceleration{nullptr},
      m_eventQueueGravity{nullptr},
      m_eventQueueAccelerometer{nullptr},
      m_looper{nullptr}
    {
        initSensors(inWhichSensors);
    }

private:
    // microseconds
    static constexpr int MAX_EVENT_REPORT_TIME = 20000;

// event identifier for identifying an event that occurs during a poll.  It doesn't matter what this
// value is, it just has to be unique among all the other sensors the program receives events for.
    static constexpr int EVENT_TYPE_LINEAR_ACCELERATION = 460;
    static constexpr int EVENT_TYPE_GRAVITY = 461;
    static constexpr int EVENT_TYPE_ACCELEROMETER = 462;

    ASensorManager *m_sensorManager;
    ASensor const *m_sensorLinearAcceleration;
    ASensor const *m_sensorGravity;
    ASensor const *m_sensorAccelerometer;
    ASensorEventQueue *m_eventQueueLinearAcceleration;
    ASensorEventQueue *m_eventQueueGravity;
    ASensorEventQueue *m_eventQueueAccelerometer;
    ALooper *m_looper;

    void initSensors(std::bitset<3> inWhichSensors);
    ASensorEventQueue *initializeSensor(ASensor const *sensor, int eventType);

    inline bool hasEvents(ASensorEventQueue *eventQueue) {
        return ASensorEventQueue_hasEvents(eventQueue) > 0;
    }

    inline std::vector<AccelerationEvent> getEvents(ASensorEventQueue *eventQueue) {
        std::vector<ASensorEvent> events;
        events.resize(100);
        ssize_t nbrEvents = ASensorEventQueue_getEvents(eventQueue, events.data(), events.size());
        if (nbrEvents < 0) {
            // an error has occurred
            throw std::runtime_error("Error on retrieving sensor events.");
        }

        std::vector<AccelerationEvent> avector;
        for (int i = 0; i < nbrEvents; i++) {
            AccelerationEvent a{
                    events[i].acceleration.x,
                    events[i].acceleration.y,
                    events[i].acceleration.z};
            avector.push_back(a);
        }

        return avector;
    }

    void destroyResources() {
        if (m_sensorLinearAcceleration != nullptr && m_eventQueueLinearAcceleration != nullptr) {
            ASensorEventQueue_disableSensor(m_eventQueueLinearAcceleration, m_sensorLinearAcceleration);
            ASensorManager_destroyEventQueue(m_sensorManager, m_eventQueueLinearAcceleration);
            m_eventQueueLinearAcceleration = nullptr;
            m_sensorLinearAcceleration = nullptr;
        }
        if (m_sensorGravity != nullptr && m_eventQueueGravity != nullptr) {
            ASensorEventQueue_disableSensor(m_eventQueueGravity, m_sensorGravity);
            ASensorManager_destroyEventQueue(m_sensorManager, m_eventQueueGravity);
            m_eventQueueGravity = nullptr;
            m_sensorGravity = nullptr;
        }
        if (m_sensorAccelerometer != nullptr && m_eventQueueAccelerometer) {
            ASensorEventQueue_disableSensor(m_eventQueueAccelerometer, m_sensorAccelerometer);
            ASensorManager_destroyEventQueue(m_sensorManager, m_eventQueueAccelerometer);
            m_eventQueueAccelerometer = nullptr;
            m_sensorAccelerometer = nullptr;
        }
    }
};

typedef ANativeWindow WindowType;

namespace std {
    template<> class default_delete<AAsset> {
    public:
        void operator()(AAsset *toBeDeleted) {
            AAsset_close(toBeDeleted);
        }
    };
}

class AssetStreambuf : public std::streambuf {
private:
    std::unique_ptr<AAsset> asset;
    char buffer[4096];
public:
    explicit AssetStreambuf(std::unique_ptr<AAsset> &&inAsset) : asset(std::move(inAsset)) {}
    int underflow() override;
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) override;
    std::streampos seekpos(std::streampos pos, std::ios_base::openmode which) override;
};

class AssetManagerWrapper {
private:
    AAssetManager *manager;
public:
    explicit AssetManagerWrapper(AAssetManager *inManager) : manager(inManager) {}
    std::unique_ptr<AAsset> getAsset(std::string const &file);
};

extern std::unique_ptr<AssetManagerWrapper> assetWrapper;

std::vector<char> readFile(std::string const &filename);
void setAssetManager(AAssetManager *mgr);

#endif