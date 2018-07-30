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

#ifdef RAINBOWDICE_GLONLY
#include "text.hpp"
#else
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

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_initSensors(
        JNIEnv *env,
        jobject jthis)
{
    sensorManager = ASensorManager_getInstance();
    //sensorManager = ASensorManager_getInstanceForPackage("com.indigo.rainbodice");
    sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    if (sensor == nullptr) {
        // TODO: use a flick gesture instead?
        return env->NewStringUTF("Accelerometer is not present");
    }

    looper = ALooper_forThread();
    if (looper == nullptr) {
        looper = ALooper_prepare(0);
    }

    if (looper == nullptr) {
        return env->NewStringUTF("Could not get looper.");
    }

    eventQueue = ASensorManager_createEventQueue(sensorManager, looper, EVENT_TYPE_ACCELEROMETER, nullptr, nullptr);
    if (eventQueue == nullptr) {
        return env->NewStringUTF("Could not create event queue for sensor");
    }

    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_initWindow(
        JNIEnv *env,
        jobject jthis,
        jboolean useVulkan,
        jobject surface) {
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        return env->NewStringUTF("Unable to acquire window from surface.");
    }

    try {
#ifdef RAINBOWDICE_GLONLY
        diceGraphics.reset(new RainbowDiceGL());
#else
        if (useVulkan) {
            diceGraphics.reset(new RainbowDiceVulkan());
        } else {
            diceGraphics.reset(new RainbowDiceGL());
        }
#endif
        diceGraphics->initWindow(window);
    } catch (std::runtime_error &e) {
        diceGraphics->cleanup();
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_initPipeline(
        JNIEnv *env,
        jobject jthis) {
    try {
        diceGraphics->initPipeline();
    } catch (std::runtime_error &e) {
        diceGraphics->cleanup();
        if (strlen(e.what()) > 0) {
            return env->NewStringUTF(e.what());
        } else {
            return env->NewStringUTF("Error in initializing OpenGL.");
        }
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_addSymbols(
        JNIEnv *env,
        jobject jthis,
        jobjectArray jSymbols,
        jint nbrSymbols,
        jint width,
        jint height,
        jint imageHeight,
        jint bitmapSize,
        jbyteArray jbitmap) {
    jbyte *bytes = env->GetByteArrayElements(jbitmap, nullptr);
    std::vector<char> bitmap(static_cast<size_t>(bitmapSize));
    memcpy(bitmap.data(), bytes, bitmap.size());
    env->ReleaseByteArrayElements(jbitmap, bytes, JNI_ABORT);

    std::vector<std::string> symbols;
    for (int i = 0; i < nbrSymbols; i++) {
        jstring obj = (jstring)env->GetObjectArrayElement(jSymbols, i);
        std::string symbol(env->GetStringUTFChars(obj, 0));
        symbols.push_back(symbol);
    }

    try {
#ifdef RAINBOWDICE_GLONLY
        texAtlas.reset(new TextureAtlas(symbols, static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(imageHeight), bitmap));
#else
        texAtlas.reset(new TextureAtlasVulkan(symbols, static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(imageHeight), bitmap));
#endif
    } catch (std::runtime_error &e) {
        diceGraphics->cleanup();
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_sendVertexShader(
        JNIEnv *env,
        jobject jthis,
        jbyteArray jShader,
        jint length) {
    jbyte *bytes = env->GetByteArrayElements(jShader, nullptr);
    std::vector<char> shader;
    shader.resize(static_cast<size_t>(length));
    memcpy(shader.data(), bytes, shader.size());
    env->ReleaseByteArrayElements(jShader, bytes, JNI_ABORT);

    try {
        diceGraphics->addVertexShader(shader);
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_sendFragmentShader(
        JNIEnv *env,
        jobject jthis,
        jbyteArray jShader,
        jint length) {
    jbyte *bytes = env->GetByteArrayElements(jShader, nullptr);
    std::vector<char> shader;
    shader.resize(static_cast<size_t>(length));
    memcpy(shader.data(), bytes, shader.size());
    env->ReleaseByteArrayElements(jShader, bytes, JNI_ABORT);

    try {
        diceGraphics->addFragmentShader(shader);
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
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
Java_com_quasar_cerulean_rainbowdice_MainActivity_destroyVulkan(
        JNIEnv *env,
        jobject jthis) {
    // If this function is being called, it is assumed that the caller already stopped
    // and joined the draw thread.
    diceGraphics->cleanup();
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_loadModel(
        JNIEnv *env,
        jobject jthis,
        jobjectArray jsymbols) {
    int symbolCount = env->GetArrayLength(jsymbols);
    std::vector<std::string> symbols;
    for (int i=0; i < symbolCount; i ++) {
        jstring jsymbol = (jstring) (env->GetObjectArrayElement(jsymbols, i));
        const char *csymbol = env->GetStringUTFChars(jsymbol, 0);
        std::string symbol = csymbol;
        env->ReleaseStringUTFChars(jsymbol, csymbol);
        symbols.push_back(symbol);
    }
    diceGraphics->loadObject(symbols);
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_MainActivity_destroyModels(
        JNIEnv *env,
        jobject jthis) {
    diceGraphics->destroyModels();
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
    diceGraphics->resetPositions(indices);
}
