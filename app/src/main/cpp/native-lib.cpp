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
#include <jni.h>
#include <string>
#include "string.h"
#include <looper.h>
#include <native_window.h>
#include <native_window_jni.h>
#include <sensor.h>
#include <atomic>
#include "rainbowDice.hpp"
#include "rainbowDiceGL.hpp"
#include "rainbowDiceGlobal.hpp"
#include "android.hpp"

#ifdef CQ_ENABLE_VULKAN
#include "rainbowDiceVulkan.hpp"
#include "TextureAtlasVulkan.h"
#endif

/* Used to communicate between the gui thread and the drawing thread.  When the GUI thread wants
 * the drawer to stop drawing, cleanup, and exit, it sets this value to true.  The drawing thread
 * will set this value to false before starting drawing.
 */
std::atomic<bool> stopDrawing(false);

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

int const Sensors::MAX_EVENT_REPORT_TIME = 20000;
int const Sensors::EVENT_TYPE_ACCELEROMETER = 462;

void Sensors::initSensors() {
    sensorManager = ASensorManager_getInstance();
    sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    if (sensor == nullptr) {
        // TODO: use a flick gesture instead?
        throw std::runtime_error("Accelerometer not present.");
    }

    looper = ALooper_forThread();
    if (looper == nullptr) {
        looper = ALooper_prepare(0);
    }

    if (looper == nullptr) {
        throw std::runtime_error("Could not initialize looper.");
    }

    eventQueue = ASensorManager_createEventQueue(sensorManager, looper, EVENT_TYPE_ACCELEROMETER, nullptr, nullptr);

    int rc = ASensorEventQueue_enableSensor(eventQueue, sensor);
    if (rc < 0) {
        throw std::runtime_error("Could not enable sensor");
    }
    int minDelay = ASensor_getMinDelay(sensor);
    minDelay = std::max(minDelay, MAX_EVENT_REPORT_TIME);

    rc = ASensorEventQueue_setEventRate(eventQueue, sensor, minDelay);
    if (rc < 0) {
        ASensorEventQueue_disableSensor(eventQueue, sensor);
        throw std::runtime_error("Could not set event rate");
    }
}

std::shared_ptr<RainbowDice> initDice(
        JNIEnv *env,
        jobject surface,
        jobject manager,
        jobjectArray jDiceConfigs,
        jobjectArray jSymbols,
        jint width,
        jint height,
        jint imageHeight,
        jint heightBlankSpace,
        jbyteArray jbitmap) {

    // get the texture data
    jbyte *bytes = env->GetByteArrayElements(jbitmap, nullptr);
    size_t bitmapSize = static_cast<size_t>(env->GetArrayLength(jbitmap));
    std::vector<char> bitmap(static_cast<size_t>(bitmapSize));
    memcpy(bitmap.data(), bytes, bitmap.size());
    env->ReleaseByteArrayElements(jbitmap, bytes, JNI_ABORT);

    std::vector<std::string> symbols;
    size_t nbrSymbols = static_cast<size_t>(env->GetArrayLength(jSymbols));
    for (int i = 0; i < nbrSymbols; i++) {
        jstring obj = (jstring)env->GetObjectArrayElement(jSymbols, i);
        char const *csymbol = env->GetStringUTFChars(obj, 0);
        std::string symbol(csymbol);
        env->ReleaseStringUTFChars(obj, csymbol);
        symbols.push_back(symbol);
    }

    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        throw std::runtime_error("Unable to acquire window from surface.");
    }

    setAssetManager(AAssetManager_fromJava(env, manager));

    bool useGl = false;
    std::shared_ptr<RainbowDice> diceGraphics;
#ifdef CQ_ENABLE_VULKAN
    try {
        diceGraphics.reset(new RainbowDiceVulkan(window, symbols, static_cast<uint32_t>(width),
                                                 static_cast<uint32_t>(height), static_cast<uint32_t>(imageHeight),
                                                 static_cast<uint32_t>(heightBlankSpace), bitmap));
    } catch (std::runtime_error &e) {
        useGl = true;
    }
#else
    useGl = true;
#endif

    if (useGl) {
        try {
            diceGraphics.reset(new RainbowDiceGL(window, symbols, static_cast<uint32_t>(width),
                                                 static_cast<uint32_t>(height), static_cast<uint32_t>(imageHeight),
                                                 static_cast<uint32_t>(heightBlankSpace), bitmap));
        } catch (std::runtime_error &e) {
            ANativeWindow_release(window);
            throw;
        }
    }

    // Load the models.
    jint nbrDiceConfigs = env->GetArrayLength(jDiceConfigs);
    for (int i = 0; i < nbrDiceConfigs; i++) {
        jobject obj = env->GetObjectArrayElement(jDiceConfigs, i);
        jclass diceConfigClass = env->GetObjectClass(obj);

        jmethodID mid = env->GetMethodID(diceConfigClass, "getNumberOfSidesInt", "()I");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrSides = env->CallIntMethod(obj, mid);
        if (nbrSides < 2) {
            // dice is a constant - ignore this dice.
            continue;
        }

        mid = env->GetMethodID(diceConfigClass, "getNumberDiceInRepresentation", "()I");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrDiceInRepresentation = env->CallIntMethod(obj, mid);

        std::vector<std::vector<std::string>> symbolsDiceVector;
        mid = env->GetMethodID(diceConfigClass, "getSymbolsString", "(I)Ljava/lang/String;");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        for (int j = 0; j < nbrDiceInRepresentation; j++) {
            jstring jSymbolsDice = (jstring) env->CallObjectMethod(obj, mid, j);
            char const *cSymbolsDice = env->GetStringUTFChars(jSymbolsDice, 0);
            std::string symbolsDice(cSymbolsDice);
            env->ReleaseStringUTFChars(jSymbolsDice, cSymbolsDice);

            std::size_t epos = symbolsDice.find("\n");
            std::size_t bpos = 0;
            std::vector<std::string> symbolsDieVector;
            while (epos != std::string::npos) {
                std::string symbolDice = symbolsDice.substr(bpos, epos-bpos);
                symbolsDieVector.push_back(symbolDice);
                bpos = epos+1;
                epos = symbolsDice.find("\n", bpos);
            }
            symbolsDiceVector.push_back(symbolsDieVector);
        }

        mid = env->GetMethodID(diceConfigClass, "getNumberOfDiceInt", "()I");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrDice = env->CallIntMethod(obj, mid);

        mid = env->GetMethodID(diceConfigClass, "getReRollOnInt", "()I");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint reroll = env->CallIntMethod(obj, mid);

        std::string rerollSymbol{std::to_string(reroll)};

        for (int j = 0; j < nbrDice; j++) {
            for (auto const &diceSymbols : symbolsDiceVector) {
                diceGraphics->loadObject(diceSymbols, rerollSymbol);
            }
        }
    }

    diceGraphics->initModels();

    return diceGraphics;
}

std::string drawRollingDice(std::shared_ptr<RainbowDice> const &diceGraphics)
{
    Sensors sensor;

    while (!stopDrawing.load()) {
        if (sensor.hasEvents()) {
            std::vector<Sensors::AccelerationEvent> events = sensor.getEvents();
            for (auto const &event : events) {
                diceGraphics->updateAcceleration(event.x, event.y, event.z);
            }

        }
        bool needsRedraw = diceGraphics->updateUniformBuffer();
        if (needsRedraw) {
            diceGraphics->drawFrame();
        }

        if (diceGraphics->allStopped()) {
            if (diceGraphics->needsReroll()) {
                diceGraphics->addRollingDice();
                continue;
            }
            std::vector<std::vector<std::string>> results = diceGraphics->getDiceResults();
            std::string totalResult;
            for (auto result : results) {
                for (auto dieResult : result) {
                    totalResult += dieResult + "\t";
                }
                totalResult[totalResult.length() - 1] = '\n';
            }
            totalResult[totalResult.length() - 1] = '\0';
            return totalResult;
        }
    }

    return std::string();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_rollDice(
        JNIEnv *env,
        jobject jthis,
        jobject surface,
        jobject manager,
        jobjectArray jDiceConfigs,
        jobjectArray jSymbols,
        jint width,
        jint height,
        jint imageHeight,
        jint heightBlankSpace,
        jbyteArray jbitmap) {

    stopDrawing.store(false);

    try {
        std::shared_ptr<RainbowDice> diceGraphics = initDice(env, surface, manager, jDiceConfigs,
                jSymbols, width, height, imageHeight, heightBlankSpace, jbitmap);
        diceGraphics->resetPositions();
        std::string results = drawRollingDice(diceGraphics);
        return env->NewStringUTF(results.c_str());
    } catch (std::runtime_error &e) {
        if (strlen(e.what()) > 0) {
            return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
        } else {
            return env->NewStringUTF("error: Error in initializing graphics.");
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_tellDrawerStop(
        JNIEnv *env,
        jobject jthis) {
    stopDrawing.store(true);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_drawStoppedDice(
        JNIEnv *env,
        jobject jthis,
        jobjectArray jSymbolsUp,
        jobject surface,
        jobject manager,
        jobjectArray jDiceConfigs,
        jobjectArray jSymbols,
        jint width,
        jint height,
        jint imageHeight,
        jint heightBlankSpace,
        jbyteArray jbitmap)
{
    int count = env->GetArrayLength(jSymbolsUp);
    std::vector<std::string> symbols;
    for (int i = 0; i < count; i++) {
        jstring jsymbol = (jstring) env->GetObjectArrayElement(jSymbolsUp, i);
        char const *csymbol = env->GetStringUTFChars(jsymbol, nullptr);
        symbols.push_back(csymbol);
        env->ReleaseStringUTFChars(jsymbol, csymbol);
    }

    try {
        std::shared_ptr<RainbowDice> diceGraphics = initDice(env, surface, manager, jDiceConfigs,
                                                             jSymbols, width, height, imageHeight,
                                                             heightBlankSpace, jbitmap);
        diceGraphics->resetToStoppedPositions(symbols);
        diceGraphics->drawFrame();
        return env->NewStringUTF("");
    } catch (std::runtime_error &e) {
        return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
    }
}