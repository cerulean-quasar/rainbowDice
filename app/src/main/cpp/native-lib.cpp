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
#include <string>
#include <cstring>
#include <vector>
#include <android/looper.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include "android.hpp"
#include "native-lib.hpp"
#include "drawer.hpp"
#include "dice.hpp"

void handleJNIException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        throw std::runtime_error("Java exception occurred");
    }
}

std::pair<std::vector<std::shared_ptr<DiceDescription>>, std::shared_ptr<TextureAtlas>> initDice(
        JNIEnv *env,
        jobjectArray jDiceConfigs,
        jobjectArray jSymbols,
        jint width,
        jint height,
        jfloatArray jArrayTextureCoordsLeft,
        jfloatArray jArrayTextureCoordsRight,
        jfloatArray jArrayTextureCoordsTop,
        jfloatArray jArrayTextureCoordsBottom,
        jbyteArray jbitmap) {

    // delete all JNI local references that get allocated in a loop to avoid using them up.  We don't
    // need to be too careful because the references will be freed at the end of the JNI call and
    // JNI calls calling this function are not long lived.
    auto deleter = [env](jobject localRefRaw) {
        env->DeleteLocalRef(localRefRaw);
    };

    std::vector<std::string> symbols;
    size_t nbrSymbols = static_cast<size_t>(env->GetArrayLength(jSymbols));
    handleJNIException(env);
    for (int i = 0; i < nbrSymbols; i++) {
        std::shared_ptr<_jstring> obj((jstring)env->GetObjectArrayElement(jSymbols, i), deleter);
        handleJNIException(env);
        char const *csymbol = env->GetStringUTFChars(obj.get(), 0);
        handleJNIException(env);
        std::string symbol(csymbol);
        env->ReleaseStringUTFChars(obj.get(), csymbol);
        handleJNIException(env);
        symbols.push_back(symbol);
    }

    // get the texture data
    jbyte *bytes = env->GetByteArrayElements(jbitmap, nullptr);
    handleJNIException(env);
    size_t bitmapSize = static_cast<size_t>(env->GetArrayLength(jbitmap));
    handleJNIException(env);
    auto bitmap = std::make_unique<unsigned char[]>(bitmapSize);
    memcpy(bitmap.get(), bytes, bitmapSize);
    env->ReleaseByteArrayElements(jbitmap, bytes, JNI_ABORT);
    handleJNIException(env);

    std::vector<std::pair<float, float>> textureCoordsTopBottom;
    jfloat *jTextureCoordsBottom = env->GetFloatArrayElements(jArrayTextureCoordsBottom, nullptr);
    handleJNIException(env);
    jfloat *jTextureCoordsTop = env->GetFloatArrayElements(jArrayTextureCoordsTop, nullptr);
    handleJNIException(env);
    for (int i = 0; i < nbrSymbols; i++) {
        textureCoordsTopBottom.push_back(std::make_pair(jTextureCoordsTop[i], jTextureCoordsBottom[i]));
    }
    env->ReleaseFloatArrayElements(jArrayTextureCoordsTop, jTextureCoordsTop, JNI_ABORT);
    handleJNIException(env);
    env->ReleaseFloatArrayElements(jArrayTextureCoordsBottom, jTextureCoordsBottom, JNI_ABORT);
    handleJNIException(env);

    std::vector<std::pair<float, float>> textureCoordsLeftRight;
    jfloat *jTextureCoordsLeft = env->GetFloatArrayElements(jArrayTextureCoordsLeft, nullptr);
    handleJNIException(env);
    jfloat *jTextureCoordsRight = env->GetFloatArrayElements(jArrayTextureCoordsRight, nullptr);
    handleJNIException(env);
    for (int i = 0; i < nbrSymbols; i++) {
        textureCoordsLeftRight.push_back(std::make_pair(jTextureCoordsLeft[i], jTextureCoordsRight[i]));
    }
    env->ReleaseFloatArrayElements(jArrayTextureCoordsLeft, jTextureCoordsLeft, JNI_ABORT);
    handleJNIException(env);
    env->ReleaseFloatArrayElements(jArrayTextureCoordsRight, jTextureCoordsRight, JNI_ABORT);
    handleJNIException(env);

    auto texture = std::make_shared<TextureAtlas>(symbols, width, height, textureCoordsLeftRight,
            textureCoordsTopBottom, std::move(bitmap), bitmapSize);

    // Load the models.
    jclass jintegerclass = env->FindClass("java/lang/Integer");
    handleJNIException(env);
    jint nbrDiceConfigs = env->GetArrayLength(jDiceConfigs);
    handleJNIException(env);
    std::vector<std::shared_ptr<DiceDescription>> dice;
    for (int i = 0; i < nbrDiceConfigs; i++) {
        std::shared_ptr<_jobject> obj(env->GetObjectArrayElement(jDiceConfigs, i), deleter);
        handleJNIException(env);
        std::shared_ptr<_jclass> diceConfigClass(env->GetObjectClass(obj.get()), deleter);
        handleJNIException(env);

        // number of symbols and values to expect
        jmethodID mid = env->GetMethodID(diceConfigClass.get(), "getNumberOfSides", "()I");
        handleJNIException(env);
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrSides = env->CallIntMethod(obj.get(), mid);

        // symbols
        std::vector<std::string> symbolsDiceVector;
        mid = env->GetMethodID(diceConfigClass.get(), "getSymbolsString", "(I)Ljava/lang/String;");
        handleJNIException(env);
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }

        for (int j = 0; j < nbrSides; j++) {
            std::shared_ptr<_jstring> jSymbolsDice((jstring) env->CallObjectMethod(obj.get(), mid, j), deleter);
            handleJNIException(env);
            char const *cSymbolsDice = env->GetStringUTFChars(jSymbolsDice.get(), 0);
            handleJNIException(env);
            std::string symbolsDice(cSymbolsDice);
            env->ReleaseStringUTFChars(jSymbolsDice.get(), cSymbolsDice);
            handleJNIException(env);

            symbolsDiceVector.push_back(symbolsDice);
        }

        // Get the values that the symbols correspond to. We will need these to rebuild the dice
        // config when the roll is done so that the java GUI can display the result, save it to the
        // log file, etc.
        std::vector<std::shared_ptr<int32_t>> values;
        mid = env->GetMethodID(diceConfigClass.get(), "getValues", "([Ljava/lang/Integer;)V");
        handleJNIException(env);
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }

        std::shared_ptr<_jobjectArray> jvalueArray(env->NewObjectArray(nbrSides, jintegerclass, nullptr), deleter);
        handleJNIException(env);
        env->CallVoidMethod(obj.get(), mid, jvalueArray.get());
        handleJNIException(env);

        mid = env->GetMethodID(jintegerclass, "intValue", "()I");
        handleJNIException(env);
        for (int j = 0; j < nbrSides; j++) {
            std::shared_ptr<_jobject> jvalue(env->GetObjectArrayElement(jvalueArray.get(), j), deleter);
            handleJNIException(env);
            if (jvalue != nullptr) {
                int32_t value = env->CallIntMethod(jvalue.get(), mid);
                handleJNIException(env);
                values.push_back(std::make_shared<int32_t>(value));
            } else {
                values.push_back(std::shared_ptr<int32_t>());
            }
        }

        // get the number of dice
        mid = env->GetMethodID(diceConfigClass.get(), "getNumberOfDice", "()I");
        handleJNIException(env);
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrDice = env->CallIntMethod(obj.get(), mid);

        // get reroll indices (into symbols vector
        mid = env->GetMethodID(diceConfigClass.get(), "getNbrIndicesReRollOn", "()I");
        handleJNIException(env);
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrIndicesRerollOn = env->CallIntMethod(obj.get(), mid);
        handleJNIException(env);

        std::vector<uint32_t> indicesRerollOn;

        if (nbrIndicesRerollOn > 0) {
            std::shared_ptr<_jintArray> jarrayindicesRerollOn(env->NewIntArray(nbrIndicesRerollOn), deleter);
            handleJNIException(env);

            mid = env->GetMethodID(diceConfigClass.get(), "getReRollOn", "([I)V");
            handleJNIException(env);
            if (mid == 0) {
                throw std::runtime_error("Could not load dice models");
            }
            env->CallVoidMethod(obj.get(), mid, jarrayindicesRerollOn.get());
            handleJNIException(env);

            jint *jindicesRerollOn = env->GetIntArrayElements(jarrayindicesRerollOn.get(), nullptr);
            handleJNIException(env);
            for (int i = 0; i < nbrIndicesRerollOn; i++) {
                indicesRerollOn.push_back(jindicesRerollOn[i]);
            }

            // we don't want to copy back changes to the array so use JNI_ABORT mode.
            env->ReleaseIntArrayElements(jarrayindicesRerollOn.get(), jindicesRerollOn, JNI_ABORT);
            handleJNIException(env);
        }

        // color
        mid = env->GetMethodID(diceConfigClass.get(), "isRainbow", "()Z");
        handleJNIException(env);
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jboolean rainbow = env->CallBooleanMethod(obj.get(), mid);
        handleJNIException(env);

        std::vector<float> color;
        if (!rainbow) {
            mid = env->GetMethodID(diceConfigClass.get(), "getColor", "([F)Z");
            handleJNIException(env);
            if (mid == 0) {
                throw std::runtime_error("Could not load dice models");
            }

            // the color is always 4 floats long
            std::shared_ptr<_jfloatArray> jarrayColor(env->NewFloatArray(4), deleter);
            handleJNIException(env);

            jboolean result = env->CallBooleanMethod(obj.get(), mid, jarrayColor.get());
            handleJNIException(env);
            if (result) {
                jfloat *jcolor = env->GetFloatArrayElements(jarrayColor.get(), nullptr);
                handleJNIException(env);
                for (int i = 0; i < 4; i++) {
                    color.push_back(jcolor[i]);
                }

                // we don't want to copy back changes to the array so use JNI_ABORT mode.
                env->ReleaseFloatArrayElements(jarrayColor.get(), jcolor, JNI_ABORT);
                handleJNIException(env);
            }
        }

        // Is this dice config being added or subtracted?
        mid = env->GetMethodID(diceConfigClass.get(), "isAddOperation", "()Z");
        handleJNIException(env);
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }

        bool isAddOperation = env->CallBooleanMethod(obj.get(), mid);
        handleJNIException(env);

        dice.push_back(std::make_shared<DiceDescription>(static_cast<uint32_t>(nbrDice),
                symbolsDiceVector, values, color, indicesRerollOn, isAddOperation));
    }

    return std::make_pair(dice, texture);
}

std::vector<std::vector<uint32_t>> initResults(JNIEnv *env, jobject jDiceResults,
        std::vector<std::shared_ptr<DiceDescription>> const &dice) {

    // delete all JNI local references that get allocated in a loop to avoid using them up.  We don't
    // need to be too careful because the references will be freed at the end of the JNI call and
    // JNI calls calling this function are not long lived.
    auto deleter = [env](jobject localRefRaw) {
        env->DeleteLocalRef(localRefRaw);
    };

    std::vector<std::vector<uint32_t>> diceResults;

    jclass diceResultsClass = env->GetObjectClass(jDiceResults);
    handleJNIException(env);
    jmethodID mid = env->GetMethodID(diceResultsClass, "getNbrResults", "()I");
    handleJNIException(env);
    if (mid == 0) {
        throw std::runtime_error("Could not load dice results");
    }
    uint32_t nbrResults = env->CallIntMethod(jDiceResults, mid);
    handleJNIException(env);

    jmethodID midNbrResultsForDie = env->GetMethodID(diceResultsClass,"geNbrResultsForDie", "(I)I");
    handleJNIException(env);
    jmethodID midResultsForDie = env->GetMethodID(diceResultsClass, "getResultsForDie", "(I[I)V");
    handleJNIException(env);
    if (midNbrResultsForDie == 0 || midResultsForDie == 0) {
        throw std::runtime_error("Could not load dice results");
    }

    int j = 0;
    int k = 1;
    for (uint32_t i = 0; i < nbrResults; i++) {
        if (dice[j]->m_symbols.size() == 1) {
            // we don't want to get the results for constant dice.  Just continue.
            if (k < dice[j]->m_nbrDice) {
                k++;
            } else {
                j++;
                k = 1;
            }
            continue;
        }
        std::vector<uint32_t> resultsForDie;
        uint32_t nbrResultsForDie = env->CallIntMethod(jDiceResults, midNbrResultsForDie, i);
        handleJNIException(env);
        std::shared_ptr<_jintArray> jArrayResultsForDie(env->NewIntArray(nbrResultsForDie), deleter);
        handleJNIException(env);

        env->CallVoidMethod(jDiceResults, midResultsForDie, i, jArrayResultsForDie.get());
        handleJNIException(env);
        jint *jresultsForDie = env->GetIntArrayElements(jArrayResultsForDie.get(), NULL);
        handleJNIException(env);
        for (uint32_t i = 0; i < nbrResultsForDie; i++) {
            resultsForDie.push_back(jresultsForDie[i]);
        }
        // we don't want to copy back changes to the array so use JNI_ABORT mode.
        env->ReleaseIntArrayElements(jArrayResultsForDie.get(), jresultsForDie, JNI_ABORT);
        handleJNIException(env);

        diceResults.push_back(resultsForDie);
        if (k < dice[j]->m_nbrDice) {
            k++;
        } else {
            j++;
            k = 1;
        }
    }

    return std::move(diceResults);
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_rollDice(
        JNIEnv *env,
        jclass jclass1,
        jstring jdiceName,
        jobjectArray jDiceConfigs,
        jobjectArray jSymbols,
        jint width,
        jint height,
        jfloatArray textureCoordLeft,
        jfloatArray textureCoordRight,
        jfloatArray textureCoordTop,
        jfloatArray textureCoordBottom,
        jbyteArray jbitmap) {

    try {
        std::pair<std::vector<std::shared_ptr<DiceDescription>>, std::shared_ptr<TextureAtlas>> dice =
                initDice(env, jDiceConfigs, jSymbols, width, height, textureCoordLeft, textureCoordRight,
                         textureCoordTop, textureCoordBottom, jbitmap);

        const char *cdiceName = env->GetStringUTFChars(jdiceName, nullptr);
        handleJNIException(env);
        std::string diceName(cdiceName);
        env->ReleaseStringUTFChars(jdiceName, cdiceName);
        handleJNIException(env);

        auto event = std::make_shared<DiceChangeEvent>(diceName, std::move(dice.first), dice.second);

        diceChannel().sendEvent(event);
        jstring str = env->NewStringUTF("");
        handleJNIException(env);
        return str;
    } catch (std::runtime_error &e) {
        if (strlen(e.what()) > 0) {
            return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
        } else {
            return env->NewStringUTF("error: Error in initializing graphics.");
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_tellDrawerStop(
        JNIEnv *env,
        jclass jclass1) {
    diceChannel().sendStopDrawingEvent();
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_tellDrawerSurfaceChanged(
        JNIEnv *env,
        jclass jclass1,
        jint width,
        jint height) {
    try {
        auto event = std::make_shared<SurfaceChangedEvent>(width, height);
        diceChannel().sendEvent(event);
        jstring str = env->NewStringUTF("");
        handleJNIException(env);
        return str;
    } catch (std::runtime_error &e) {
        if (strlen(e.what()) > 0) {
            return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
        } else {
            return env->NewStringUTF("error: Error in initializing graphics.");
        }
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_drawStoppedDice(
        JNIEnv *env,
        jclass jclass1,
        jstring jdiceName,
        jobjectArray jDiceConfigs,
        jobject jDiceResults,
        jobjectArray jSymbols,
        jint width,
        jint height,
        jfloatArray textureCoordLeft,
        jfloatArray textureCoordRight,
        jfloatArray textureCoordTop,
        jfloatArray textureCoordBottom,
        jbyteArray jbitmap)
{
    try {
        const char *cdiceName = env->GetStringUTFChars(jdiceName, nullptr);
        handleJNIException(env);
        std::string diceName(cdiceName);
        env->ReleaseStringUTFChars(jdiceName, cdiceName);
        handleJNIException(env);

        jclass diceResultsClass = env->GetObjectClass(jDiceResults);
        handleJNIException(env);
        jmethodID mid = env->GetMethodID(diceResultsClass, "isModifiedRoll", "()Z");
        handleJNIException(env);
        if (mid == 0) {
            return env->NewStringUTF("Could not load dice results");
        }
        bool isModifiedRoll = env->CallBooleanMethod(jDiceResults, mid);
        handleJNIException(env);

        std::pair<std::vector<std::shared_ptr<DiceDescription>>, std::shared_ptr<TextureAtlas>> dice =
                initDice(env, jDiceConfigs,
                         jSymbols, width, height, textureCoordLeft,
                         textureCoordRight, textureCoordTop,
                         textureCoordBottom, jbitmap);
        std::vector<std::vector<uint32_t>> upFaceIndices = std::move(initResults(env, jDiceResults,
                dice.first));
        std::shared_ptr<DrawEvent> event = std::make_shared<DrawStoppedDiceEvent>(std::move(diceName),
                std::move(dice.first), dice.second, std::move(upFaceIndices), isModifiedRoll);
        diceChannel().sendEvent(event);
        return env->NewStringUTF("");
    } catch (std::runtime_error &e) {
        return env->NewStringUTF(e.what());
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_scroll(
        JNIEnv *env,
        jclass jclass1,
        jfloat distanceX,
        jfloat distanceY) {
    auto event = std::make_shared<ScrollEvent>(distanceX, distanceY);
    diceChannel().sendEvent(event);
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_scale(
        JNIEnv *env,
        jclass jclass1,
        jfloat scaleFactor) {
    auto event = std::make_shared<ScaleEvent>(scaleFactor);
    diceChannel().sendEvent(event);
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_tapDice(
        JNIEnv *env,
        jclass jclass1,
        jfloat x,
        jfloat y) {
    auto event = std::make_shared<TapDiceEvent>(x, y);
    diceChannel().sendEvent(event);
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_rerollSelected(
        JNIEnv *env,
        jclass jclass1) {
    diceChannel().sendEvent(std::make_shared<RerollSelected>());
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_addRerollSelected(
        JNIEnv *env,
        jclass jclass1) {
    diceChannel().sendEvent(std::make_shared<AddRerollSelected>());
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_deleteSelected(
        JNIEnv *env,
        jclass jclass1) {
    diceChannel().sendEvent(std::make_shared<DeleteSelected>());
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_Draw_resetView(
        JNIEnv *env,
        jclass jclass1) {
    diceChannel().sendEvent(std::make_shared<ResetView>());
}

extern "C" JNIEXPORT void JNICALL
Java_com_quasar_cerulean_rainbowdice_DiceWorker_startWorker(
        JNIEnv *env,
        jobject jthis,
        jobject jsurface,
        jobject jmanager,
        jobject jnotify,
        jboolean juseGravity,
        jboolean jdrawRollingDice,
        jboolean juseLegacy,
        jboolean jreverseGravity) {

    std::shared_ptr<Notify> notify;
    try {
        notify = std::make_shared<Notify>(env, jnotify);
    } catch (...) {
        return;
    }

    try {
        setAssetManager(AAssetManager_fromJava(env, jmanager));

        ANativeWindow *window = ANativeWindow_fromSurface(env, jsurface);
        if (window == nullptr) {
            notify->sendError("Unable to acquire window from surface.");
            return;
        }

        auto deleter = [](WindowType *windowRaw) {
            /* release the java window object */
            if (windowRaw != nullptr) {
                ANativeWindow_release(windowRaw);
            }
        };
        std::shared_ptr<WindowType> surface(window, deleter);

        //diceChannel().clearQueue();
        DiceWorker worker(surface, notify, juseGravity, jdrawRollingDice, juseLegacy, jreverseGravity);
        surface.reset();
        worker.waitingLoop();
    } catch (std::runtime_error &e) {
        if (strlen(e.what()) > 0) {
            notify->sendError(e.what());
        } else {
            notify->sendError("Error in initializing graphics.");
        }
    } catch (...) {
        notify->sendError("Error in initializing graphics.");
    }
}

void Notify::sendResult(
        std::string const &diceName,
        bool isModified,
        std::vector<std::vector<uint32_t>> const &results,
        std::vector<std::shared_ptr<DiceDescription>> const &dice) {
    /* all the jobject or derived from jobject (jstring, jclass, jintArray, etc) used in this
     * function must be released with DeleteLocalRef before this function returns.  The JVM/JNI
     * normally frees these, but only when the C++ function returns and exits back to java.  Since
     * this function is called potentially many times before the worker thread exits the JNI, not
     * freeing these jobjects is effectively a leak and will eventually cause android to kill the app.
     */
    auto env = m_env;
    auto deleter = [env](jobject localRefRaw) {
        env->DeleteLocalRef(localRefRaw);
    };

    std::string totalResult;
    std::shared_ptr<_jclass> notifyClass(m_env->GetObjectClass(m_notify), deleter);
    handleJNIException(m_env);

    jmethodID midAddMultiResult = m_env->GetMethodID(notifyClass.get(), "addResult", "([I)V");
    handleJNIException(m_env);
    if (midAddMultiResult == nullptr) {
        throw std::runtime_error("Could not send message.");
    }

    jmethodID midAddSingleResult = m_env->GetMethodID(notifyClass.get(), "addResult", "(I)V");
    handleJNIException(m_env);
    if (midAddSingleResult == nullptr) {
        throw std::runtime_error("Could not send message.");
    }

    for (auto const &result : results) {
        if (result.size() == 1) {
            m_env->CallVoidMethod(m_notify, midAddSingleResult, result[0]);
            handleJNIException(m_env);
        } else {
            std::shared_ptr<_jintArray> jresultsArray(m_env->NewIntArray(static_cast<jsize>(result.size())), deleter);
            handleJNIException(m_env);
            jint *jresults = m_env->GetIntArrayElements(jresultsArray.get(), nullptr);
            handleJNIException(m_env);
            int i = 0;
            for (auto const &dieResult : result) {
                jresults[i++] = dieResult;
            }
            m_env->ReleaseIntArrayElements(jresultsArray.get(), jresults, JNI_COMMIT);
            handleJNIException(m_env);
            m_env->CallVoidMethod(m_notify, midAddMultiResult, jresultsArray.get());
            handleJNIException(m_env);
        }
    }

    jmethodID midAddDice = m_env->GetMethodID(notifyClass.get(), "addDice",
                                              "(I[Ljava/lang/String;[Ljava/lang/Integer;[I[FZ)V");
    handleJNIException(m_env);
    if (midAddDice == nullptr) {
        throw std::runtime_error("Could not send message.");
    }
    std::shared_ptr<_jstring> emptyStr(m_env->NewStringUTF(""), deleter);
    handleJNIException(m_env);
    std::shared_ptr<_jclass> stringClass(m_env->FindClass("java/lang/String"), deleter);
    handleJNIException(m_env);
    std::shared_ptr<_jclass> jIntegerClass(m_env->FindClass("java/lang/Integer"), deleter);
    handleJNIException(m_env);
    for (auto const &die : dice) {
        std::shared_ptr<_jobjectArray> jsymbolArray(m_env->NewObjectArray(die->m_symbols.size(),
                                                          stringClass.get(),
                                                          emptyStr.get()), deleter);
        handleJNIException(m_env);
        int i = 0;
        for (auto const &symbol : die->m_symbols) {
            std::shared_ptr<_jstring> str(m_env->NewStringUTF(symbol.c_str()), deleter);
            handleJNIException(m_env);
            m_env->SetObjectArrayElement(jsymbolArray.get(), i++, str.get());
            handleJNIException(m_env);
        }

        std::shared_ptr<_jintArray> jrerollIndicesArray(m_env->NewIntArray(
                static_cast<jsize>(die->m_rerollOnIndices.size())), deleter);
        handleJNIException(m_env);
        jint *jindices = m_env->GetIntArrayElements(jrerollIndicesArray.get(), nullptr);
        handleJNIException(m_env);
        i = 0;
        for (auto const &rerollIndex : die->m_rerollOnIndices) {
            jindices[i++] = rerollIndex;
        }
        m_env->ReleaseIntArrayElements(jrerollIndicesArray.get(), jindices, JNI_COMMIT);
        handleJNIException(m_env);

        std::shared_ptr<_jfloatArray> jcolorArray(
                m_env->NewFloatArray(static_cast<jsize>(die->m_color.size())), deleter);
        handleJNIException(m_env);
        jfloat *jcolor = m_env->GetFloatArrayElements(jcolorArray.get(), nullptr);
        handleJNIException(m_env);
        i = 0;
        for (auto const &colorValue : die->m_color) {
            jcolor[i++] = colorValue;
        }
        m_env->ReleaseFloatArrayElements(jcolorArray.get(), jcolor, JNI_COMMIT);
        handleJNIException(m_env);

        jmethodID midIntegerInit = m_env->GetMethodID(jIntegerClass.get(), "<init>", "(I)V");
        handleJNIException(m_env);
        std::shared_ptr<_jobjectArray> jvalueArray(m_env->NewObjectArray(die->m_values.size(),
                                                         jIntegerClass.get(),
                                                         nullptr), deleter);
        handleJNIException(m_env);
        i = 0;
        for (auto const &value : die->m_values) {
            if (value != nullptr) {
                std::shared_ptr<_jobject> obj(
                        m_env->NewObject(jIntegerClass.get(), midIntegerInit, *value), deleter);
                handleJNIException(m_env);
                m_env->SetObjectArrayElement(jvalueArray.get(), i, obj.get());
                handleJNIException(m_env);
            }
            i++;
        }

        m_env->CallVoidMethod(m_notify, midAddDice, die->m_nbrDice, jsymbolArray.get(), jvalueArray.get(),
                              jrerollIndicesArray.get(), jcolorArray.get(), die->m_isAddOperation);
        handleJNIException(m_env);
    }

    jmethodID midSend = m_env->GetMethodID(notifyClass.get(), "sendResults",
                                           "(Ljava/lang/String;Z)V");
    handleJNIException(m_env);
    if (midSend == nullptr) {
        throw std::runtime_error("Could not send message.");
    }

    std::shared_ptr<_jstring> jdiceName(m_env->NewStringUTF(diceName.c_str()), deleter);
    m_env->CallVoidMethod(m_notify, midSend, jdiceName.get(), isModified);
    handleJNIException(m_env);
}

void Notify::sendError(std::string const &error) {
    sendError(error.c_str());
}

void Notify::sendError(char const *error) {
    auto env = m_env;
    auto deleter = [env](jobject localRefRaw) {
        env->DeleteLocalRef(localRefRaw);
    };

    std::shared_ptr<_jclass> notifyClass(m_env->GetObjectClass(m_notify), deleter);

    jmethodID midSend = m_env->GetMethodID(notifyClass.get(), "sendError", "(Ljava/lang/String;)V");
    if (midSend == nullptr) {
        // do not throw because we could already be in a section catching exceptions
        return;
    }

    std::shared_ptr<_jstring> jerror(m_env->NewStringUTF(error), deleter);
    m_env->CallVoidMethod(m_notify, midSend, jerror.get());
}

void Notify::sendGraphicsDescription(GraphicsDescription const &description,
                                     bool hasLinearAcceleration, bool hasGravity,
                                     bool hasAccelerometer) {
    auto env = m_env;
    auto deleter = [env](jobject localRefRaw) {
        env->DeleteLocalRef(localRefRaw);
    };

    std::shared_ptr<_jclass> notifyClass(m_env->GetObjectClass(m_notify), deleter);

    jmethodID midSend = m_env->GetMethodID(notifyClass.get(), "sendGraphicsDescription",
            "(ZZZZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (midSend == nullptr) {
        throw std::runtime_error("Could not send graphics description message.");
    }

    std::shared_ptr<_jstring> jgraphics(m_env->NewStringUTF(description.m_graphicsName.c_str()), deleter);
    std::shared_ptr<_jstring> jversion(m_env->NewStringUTF(description.m_version.c_str()), deleter);
    std::shared_ptr<_jstring> jdeviceName(m_env->NewStringUTF(description.m_deviceName.c_str()), deleter);
    m_env->CallVoidMethod(m_notify, midSend, hasLinearAcceleration, hasGravity, hasAccelerometer,
            description.m_isVulkan, jgraphics.get(), jversion.get(), jdeviceName.get());
}

void Notify::sendSelected(bool diceSelected) {
    jclass notifyClass = m_env->GetObjectClass(m_notify);

    jmethodID midSend = m_env->GetMethodID(notifyClass, "sendSelected", "(Z)V");
    if (midSend == nullptr) {
        throw std::runtime_error("Could not send dice selected message.");
    }

    m_env->CallVoidMethod(m_notify, midSend, diceSelected);
}