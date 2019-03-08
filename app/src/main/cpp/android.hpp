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
#ifndef RAINBOWDICE_ANDROID_HPP
#define RAINBOWDICE_ANDROID_HPP

#include <native_window.h>
#include <asset_manager.h>
#include <asset_manager_jni.h>
#include <memory>
#include <streambuf>
#include <sensor.h>

class Sensors {
public:
    struct AccelerationEvent {
        float x;
        float y;
        float z;
    };

    Sensors() {
        initSensors();
    }

    inline bool hasEvents() {
        return ASensorEventQueue_hasEvents(eventQueue) > 0;
    }

    inline std::vector<AccelerationEvent> getEvents() {
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

    ~Sensors(){
        ASensorEventQueue_disableSensor(eventQueue, sensor);
        ASensorManager_destroyEventQueue(sensorManager, eventQueue);
    }
private:
// microseconds
    static int const MAX_EVENT_REPORT_TIME;

// event identifier for identifying an event that occurs during a poll.  It doesn't matter what this
// value is, it just has to be unique among all the other sensors the program receives events for.
    static int const EVENT_TYPE_ACCELEROMETER;

    ASensorManager *sensorManager = nullptr;
    ASensor const *sensor = nullptr;
    ASensorEventQueue *eventQueue = nullptr;
    ALooper *looper = nullptr;

    void initSensors();
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
    int underflow();
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);
    std::streampos seekpos(std::streampos pos, std::ios_base::openmode which);
};

class AssetManagerWrapper {
private:
    AAssetManager *manager;
public:
    AssetManagerWrapper(AAssetManager *inManager) : manager(inManager) {}
    std::unique_ptr<AAsset> getAsset(std::string const &file);
};

extern std::unique_ptr<AssetManagerWrapper> assetWrapper;

std::vector<char> readFile(std::string const &filename);
void setAssetManager(AAssetManager *mgr);

#endif