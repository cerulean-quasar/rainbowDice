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
#include <looper.h>
#include <native_window.h>
#include <native_window_jni.h>
#include <sensor.h>
#include "rainbowDice.hpp"
#include "rainbowDiceGlobal.hpp"

// microseconds
int const MAX_EVENT_REPORT_TIME = 100000;

// event identifier for identifying an event that occurs during a poll.
int const EVENT_TYPE_ACCELEROMETER = 462;

bool stopDrawing = false;
bool reported = false;
bool rolling = false;
ASensorManager *sensorManager = nullptr;
ASensor const *sensor = nullptr;
ASensorEventQueue *eventQueue = nullptr;
ALooper *looper = nullptr;

extern "C" JNIEXPORT jstring JNICALL
Java_com_indigo_rainbowdice_MainActivity_initSensors(
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
Java_com_indigo_rainbowdice_MainActivity_initWindow(
        JNIEnv *env,
        jobject jthis,
        jobject surface) {
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    if (window == nullptr) {
        return env->NewStringUTF("Unable to acquire window from surface.");
    }
    int32_t w = ANativeWindow_getWidth(window);
    int32_t h = ANativeWindow_getHeight(window);
    try {
        diceGraphics.initVulkan(diceGraphics.STATE_1, window);
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_indigo_rainbowdice_MainActivity_initPipeline(
        JNIEnv *env,
        jobject jthis) {
    try {
        diceGraphics.initVulkan(diceGraphics.STATE_2, nullptr);
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_indigo_rainbowdice_MainActivity_addSymbol(
        JNIEnv *env,
        jobject jthis,
        jstring jsymbol,
        jint width,
        jint height,
        jbyteArray jbitmap) {
    const char *csymbol = env->GetStringUTFChars(jsymbol, 0);
    std::string symbol(csymbol);
    env->ReleaseStringUTFChars(jsymbol, csymbol);
    jbyte *bytes = env->GetByteArrayElements(jbitmap, nullptr);
    void *bitmap = malloc(static_cast<size_t>(width*height));
    memcpy(bitmap, bytes, static_cast<size_t>(width*height));
    env->ReleaseByteArrayElements(jbitmap, bytes, JNI_ABORT);
    try {
        texAtlas.addSymbol(symbol, static_cast<uint32_t>(width), static_cast<uint32_t>(height), bitmap);
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_indigo_rainbowdice_MainActivity_sendVertexShader(
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
        diceGraphics.addVertexShader(shader);
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_indigo_rainbowdice_MainActivity_sendFragmentShader(
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
        diceGraphics.addFragmentShader(shader);
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
    }
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_indigo_rainbowdice_Draw_draw(
        JNIEnv *env,
        jobject jthis)
{
    try {
        reported = false;
        if (rolling) {
            int rc = ASensorEventQueue_enableSensor(eventQueue, sensor);
            if (rc < 0) {
                return env->NewStringUTF("error: Could not enable sensor");
            }
            int minDelay = ASensor_getMinDelay(sensor);
            minDelay = std::max(minDelay, MAX_EVENT_REPORT_TIME);

            rc = ASensorEventQueue_setEventRate(eventQueue, sensor, minDelay);
            if (rc < 0) {
                return env->NewStringUTF("error: Could not set event rate");
            }
        }
        std::vector<std::string> results;
        while (!stopDrawing) {
            int event = EVENT_TYPE_ACCELEROMETER;
            void *data = nullptr;
            int rc;
            rc = ASensorEventQueue_hasEvents(eventQueue);
            if (rc > 0) {
                std::vector<ASensorEvent> events;
                events.resize(100);
                ssize_t nbrEvents = ASensorEventQueue_getEvents(eventQueue, events.data(), events.size());
                if (nbrEvents < 0) {
                    // an error has occurred
                    return env->NewStringUTF("error: Error on retrieving sensor events.");
                }

                float x = 0;
                float y = 0;
                float z = 0;
                for (int i = 0; i < nbrEvents; i++) {
                    x += events[i].acceleration.x;
                    y += events[i].acceleration.y;
                    z += events[i].acceleration.z;
                }

                diceGraphics.updateAcceleration(x, y, z);
            }
            diceGraphics.updateUniformBuffer();
            diceGraphics.drawFrame();
            if (diceGraphics.allStopped() && !reported) {
                results = diceGraphics.getDiceResults();
                std::string totalResult;
                for (auto result : results) {
                    totalResult += result + "\n";
                }
                totalResult[totalResult.length()-1] = '\0';
                reported = true;
                ASensorEventQueue_disableSensor(eventQueue, sensor);
                return env->NewStringUTF(totalResult.c_str());
            }
        }
    } catch (std::runtime_error &e) {
        // no need to draw another frame - we are failing.
        stopDrawing = true;
        ASensorEventQueue_disableSensor(eventQueue, sensor);
        return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
    }

    ASensorEventQueue_disableSensor(eventQueue, sensor);
    return env->NewStringUTF("");
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_tellDrawerStop(
        JNIEnv *env,
        jobject jthis) {
    stopDrawing = true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_destroyVulkan(
        JNIEnv *env,
        jobject jthis) {
    // If this function is being called, it is assumed that the caller already stopped
    // and joined the draw thread.
    diceGraphics.cleanup();
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_loadModel(
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
    diceGraphics.loadObject(symbols);
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_destroyModels(
        JNIEnv *env,
        jobject jthis) {
    diceGraphics.destroyModels();
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_recreateModels(
        JNIEnv *env,
        jobject jthis) {
    diceGraphics.recreateModels();
    stopDrawing = false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_recreateSwapChain(
        JNIEnv *env,
        jobject jthis) {
    diceGraphics.recreateSwapChain();
    stopDrawing = false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_roll(
        JNIEnv *env,
        jobject jthis) {
    stopDrawing = false;
    rolling = true;
    diceGraphics.resetPositions();
}

extern "C" JNIEXPORT void JNICALL
Java_com_indigo_rainbowdice_MainActivity_reRoll(
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
    stopDrawing = false;
    rolling = true;
    diceGraphics.resetPositions(indices);
}
