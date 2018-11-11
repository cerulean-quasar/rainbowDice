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
 * the drawer to stop drawing, cleanup, and exit, it sets this value to true.  The GUI will set
 * this value to false before starting a new drawer.
 */
std::atomic<bool> stopDrawing(false);


/* The following data are per program data and are to be accessed by one thread at a time.
 * Note: there are only two threads in this program; the drawer and the GUI.  Some of the data
 * below are set up by one thread and accessed by another but are guaranteed to not be accessed at
 * the same time because when the GUI thread sets up the data, the drawer has not started yet, or
 * has exited and been joined by the GUI.
 */
// microseconds
int const MAX_EVENT_REPORT_TIME = 20000;

// event identifier for identifying an event that occurs during a poll.  It doesn't matter what this
// value is, it just has to be unique among all the other sensors the program receives events for.
int const EVENT_TYPE_ACCELEROMETER = 462;

ASensorManager *sensorManager = nullptr;
ASensor const *sensor = nullptr;
ASensorEventQueue *eventQueue = nullptr;
ALooper *looper = nullptr;
std::unique_ptr<RainbowDice> diceGraphics;

bool initSensors() {
    sensorManager = ASensorManager_getInstance();
    //sensorManager = ASensorManager_getInstanceForPackage("com.indigo.rainbodice");
    sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    if (sensor == nullptr) {
        // TODO: use a flick gesture instead?
        return false;
    }

    looper = ALooper_forThread();
    if (looper == nullptr) {
        looper = ALooper_prepare(0);
    }

    if (looper == nullptr) {
        return false;
    }

    eventQueue = ASensorManager_createEventQueue(sensorManager, looper, EVENT_TYPE_ACCELEROMETER, nullptr, nullptr);
    return eventQueue != nullptr;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_initDice(
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
    if (!initSensors()) {
        return env->NewStringUTF("Failed to initialize accelerometer");
    }

    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        return env->NewStringUTF("Unable to acquire window from surface.");
    }

    setAssetManager(AAssetManager_fromJava(env, manager));

    // get the texture data
    jbyte *bytes = env->GetByteArrayElements(jbitmap, nullptr);
    size_t bitmapSize = static_cast<size_t>(env->GetArrayLength(jbitmap));
    std::vector<char> bitmap(static_cast<size_t>(bitmapSize));
    memcpy(bitmap.data(), bytes, bitmap.size());
    env->ReleaseByteArrayElements(jbitmap, bytes, JNI_ABORT);

    std::vector<std::string> symbols;
    size_t nbrSymbols = env->GetArrayLength(jSymbols);
    for (int i = 0; i < nbrSymbols; i++) {
        jstring obj = (jstring)env->GetObjectArrayElement(jSymbols, i);
        char const *csymbol = env->GetStringUTFChars(obj, 0);
        std::string symbol(csymbol);
        env->ReleaseStringUTFChars(obj, csymbol);
        symbols.push_back(symbol);
    }

    bool useGl = false;
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
            return env->NewStringUTF(e.what());
        }
    }

    // Load the models.
    jint nbrDiceConfigs = env->GetArrayLength(jDiceConfigs);
    for (int i = 0; i < nbrDiceConfigs; i++) {
        jobject obj = env->GetObjectArrayElement(jDiceConfigs, i);
        jclass diceConfigClass = env->GetObjectClass(obj);

        jmethodID mid = env->GetMethodID(diceConfigClass, "getNumberDiceInRepresentation", "()I");
        if (mid == 0) {
            return env->NewStringUTF("Could not load dice models");
        }
        jint nbrDiceInRepresentation = env->CallIntMethod(obj, mid);

        std::vector<std::vector<std::string>> symbolsDiceVector;
        mid = env->GetMethodID(diceConfigClass, "getSymbolsString", "(I)Ljava/lang/String;");
        if (mid == 0) {
            return env->NewStringUTF("Could not load dice models");
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
            return env->NewStringUTF("Could not load dice models");
        }
        jint nbrDice = env->CallIntMethod(obj, mid);

        for (int j = 0; j < nbrDice; j++) {
            for (auto const &diceSymbols : symbolsDiceVector) {
                diceGraphics->loadObject(diceSymbols);
            }
        }
    }

    try {
        diceGraphics->initPipeline();
    } catch (std::runtime_error &e) {
        diceGraphics.reset();
        ASensorManager_destroyEventQueue(sensorManager, eventQueue);
        if (strlen(e.what()) > 0) {
            return env->NewStringUTF(e.what());
        } else {
            return env->NewStringUTF("Error in initializing graphics.");
        }
    }

    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_draw(
        JNIEnv *env,
        jobject jthis)
{
    if (stopDrawing.load()) {
        // There is nothing to do.  Just return, no results, no error.
        return env->NewStringUTF("");
    }

    try {
        diceGraphics->initThread();
        int rc = ASensorEventQueue_enableSensor(eventQueue, sensor);
        if (rc < 0) {
            return env->NewStringUTF("error: Could not enable sensor");
        }
        int minDelay = ASensor_getMinDelay(sensor);
        minDelay = std::max(minDelay, MAX_EVENT_REPORT_TIME);

        rc = ASensorEventQueue_setEventRate(eventQueue, sensor, minDelay);
        if (rc < 0) {
            ASensorEventQueue_disableSensor(eventQueue, sensor);
            diceGraphics->cleanupThread();
            return env->NewStringUTF("error: Could not set event rate");
        }
        std::vector<std::string> results;
        while (!stopDrawing.load()) {
            rc = ASensorEventQueue_hasEvents(eventQueue);
            if (rc > 0) {
                std::vector<ASensorEvent> events;
                events.resize(100);
                ssize_t nbrEvents = ASensorEventQueue_getEvents(eventQueue, events.data(), events.size());
                if (nbrEvents < 0) {
                    // an error has occurred
                    ASensorEventQueue_disableSensor(eventQueue, sensor);
                    diceGraphics->cleanupThread();
                    return env->NewStringUTF("error: Error on retrieving sensor events.");
                }

                for (int i = 0; i < nbrEvents; i++) {
                    float x = events[i].acceleration.x;
                    float y = events[i].acceleration.y;
                    float z = events[i].acceleration.z;
                    diceGraphics->updateAcceleration(x, y, z);
                }

            }
            bool needsRedraw = diceGraphics->updateUniformBuffer();
            if (needsRedraw) {
                diceGraphics->drawFrame();
            }

            if (diceGraphics->allStopped()) {
                ASensorEventQueue_disableSensor(eventQueue, sensor);
                results = diceGraphics->getDiceResults();
                std::string totalResult;
                for (auto result : results) {
                    totalResult += result + "\n";
                }
                totalResult[totalResult.length() - 1] = '\0';
                ASensorEventQueue_disableSensor(eventQueue, sensor);
                diceGraphics->cleanupThread();
                return env->NewStringUTF(totalResult.c_str());
            }
        }
    } catch (std::runtime_error &e) {
        // no need to draw another frame - we are failing.
        ASensorEventQueue_disableSensor(eventQueue, sensor);
        diceGraphics->cleanupThread();
        return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
    }

    diceGraphics->cleanupThread();
    ASensorEventQueue_disableSensor(eventQueue, sensor);
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_tellDrawerStop(
        JNIEnv *env,
        jobject jthis) {
    stopDrawing.store(true);
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_destroyResources(
        JNIEnv *env,
        jobject jthis) {
    // If this function is being called, it is assumed that the caller already stopped
    // and joined the draw thread.
    if (diceGraphics.get() != nullptr) {
        diceGraphics.reset();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_recreateModels(
        JNIEnv *env,
        jobject jthis) {
    diceGraphics->recreateModels();
    stopDrawing.store(false);
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_recreateSwapChain(
        JNIEnv *env,
        jobject jthis) {
    diceGraphics->recreateSwapChain();
    stopDrawing.store(false);
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_roll(
        JNIEnv *env,
        jobject jthis) {
    stopDrawing.store(false);
    diceGraphics->resetPositions();
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_reRoll(
        JNIEnv *env,
        jobject jthis,
        jintArray jIndices) {
    int count = env->GetArrayLength(jIndices);
    std::set<int> indices;
    jboolean isCopy;
    jint *cIndices = env->GetIntArrayElements(jIndices, &isCopy);
    for (int i = 0; i < count; i++) {
        indices.insert(cIndices[i]);
    }
    env->ReleaseIntArrayElements(jIndices, cIndices, JNI_COMMIT);
    stopDrawing.store(false);
    diceGraphics->initThread();
    diceGraphics->addRollingDiceAtIndices(indices);
    diceGraphics->cleanupThread();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_drawOnce(
        JNIEnv *env,
        jobject jthis,
        jobjectArray jsymbols)
{
    int count = env->GetArrayLength(jsymbols);
    std::vector<std::string> symbols;
    for (int i = 0; i < count; i++) {
        jstring jsymbol = (jstring) env->GetObjectArrayElement(jsymbols, i);
        char const *csymbol = env->GetStringUTFChars(jsymbol, nullptr);
        symbols.push_back(csymbol);
        env->ReleaseStringUTFChars(jsymbol, csymbol);
    }

    try {
        if (diceGraphics.get() == nullptr) {
            return env->NewStringUTF("No dice being rolled.");
        }

        diceGraphics->initThread();
        diceGraphics->resetToStoppedPositions(symbols);
        diceGraphics->drawFrame();
        diceGraphics->cleanupThread();
        return env->NewStringUTF("");
    } catch (std::runtime_error &e) {
        diceGraphics->cleanupThread();
        return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
    }
}