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
#include <looper.h>
#include <native_window.h>
#include <native_window_jni.h>
#include <vector>
#include "android.hpp"
#include "native-lib.hpp"
#include "drawer.hpp"
#include "dice.hpp"

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

    std::vector<std::string> symbols;
    size_t nbrSymbols = static_cast<size_t>(env->GetArrayLength(jSymbols));
    for (int i = 0; i < nbrSymbols; i++) {
        jstring obj = (jstring)env->GetObjectArrayElement(jSymbols, i);
        char const *csymbol = env->GetStringUTFChars(obj, 0);
        std::string symbol(csymbol);
        env->ReleaseStringUTFChars(obj, csymbol);
        symbols.push_back(symbol);
    }

    // get the texture data
    jbyte *bytes = env->GetByteArrayElements(jbitmap, nullptr);
    size_t bitmapSize = static_cast<size_t>(env->GetArrayLength(jbitmap));
    auto bitmap = std::make_unique<unsigned char[]>(bitmapSize);
    memcpy(bitmap.get(), bytes, bitmapSize);
    env->ReleaseByteArrayElements(jbitmap, bytes, JNI_ABORT);

    std::vector<std::pair<float, float>> textureCoordsTopBottom;
    jfloat *jTextureCoordsBottom = env->GetFloatArrayElements(jArrayTextureCoordsBottom, nullptr);
    jfloat *jTextureCoordsTop = env->GetFloatArrayElements(jArrayTextureCoordsTop, nullptr);
    for (int i = 0; i < nbrSymbols; i++) {
        textureCoordsTopBottom.push_back(std::make_pair(jTextureCoordsTop[i], jTextureCoordsBottom[i]));
    }
    env->ReleaseFloatArrayElements(jArrayTextureCoordsTop, jTextureCoordsTop, JNI_ABORT);
    env->ReleaseFloatArrayElements(jArrayTextureCoordsBottom, jTextureCoordsBottom, JNI_ABORT);

    std::vector<std::pair<float, float>> textureCoordsLeftRight;
    jfloat *jTextureCoordsLeft = env->GetFloatArrayElements(jArrayTextureCoordsLeft, nullptr);
    jfloat *jTextureCoordsRight = env->GetFloatArrayElements(jArrayTextureCoordsRight, nullptr);
    for (int i = 0; i < nbrSymbols; i++) {
        textureCoordsLeftRight.push_back(std::make_pair(jTextureCoordsLeft[i], jTextureCoordsRight[i]));
    }
    env->ReleaseFloatArrayElements(jArrayTextureCoordsLeft, jTextureCoordsLeft, JNI_ABORT);
    env->ReleaseFloatArrayElements(jArrayTextureCoordsRight, jTextureCoordsRight, JNI_ABORT);

    auto texture = std::make_shared<TextureAtlas>(symbols, width, height, textureCoordsLeftRight,
            textureCoordsTopBottom, std::move(bitmap), bitmapSize);

    // Load the models.
    jint nbrDiceConfigs = env->GetArrayLength(jDiceConfigs);
    std::vector<std::shared_ptr<DiceDescription>> dice;
    for (int i = 0; i < nbrDiceConfigs; i++) {
        jobject obj = env->GetObjectArrayElement(jDiceConfigs, i);
        jclass diceConfigClass = env->GetObjectClass(obj);

        // number of symbols and values to expect
        jmethodID mid = env->GetMethodID(diceConfigClass, "getNumberOfSides", "()I");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrSides = env->CallIntMethod(obj, mid);
        if (nbrSides < 2) {
            // dice is a constant - ignore this dice.
            continue;
        }

        // symbols
        std::vector<std::string> symbolsDiceVector;
        mid = env->GetMethodID(diceConfigClass, "getSymbolsString", "(I)Ljava/lang/String;");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }

        for (int j = 0; j < nbrSides; j++) {
            jstring jSymbolsDice = (jstring) env->CallObjectMethod(obj, mid, j);
            char const *cSymbolsDice = env->GetStringUTFChars(jSymbolsDice, 0);
            std::string symbolsDice(cSymbolsDice);
            env->ReleaseStringUTFChars(jSymbolsDice, cSymbolsDice);

            symbolsDiceVector.push_back(symbolsDice);
        }

        // Get the values that the symbols correspond to. We will need these to rebuild the dice
        // config when the roll is done so that the java GUI can display the result, save it to the
        // log file, etc.
        std::vector<std::shared_ptr<int32_t>> values;
        mid = env->GetMethodID(diceConfigClass, "getValues", "([Ljava/lang/Integer;)V");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }

        jclass jintegerclass = env->FindClass("java/lang/Integer");
        jobjectArray jvalueArray = env->NewObjectArray(nbrSides, jintegerclass, nullptr);
        env->CallVoidMethod(obj, mid, jvalueArray);

        mid = env->GetMethodID(jintegerclass, "intValue", "()I");
        for (int j = 0; j < nbrSides; j++) {
            jobject jvalue = env->GetObjectArrayElement(jvalueArray, j);
            if (jvalue != nullptr) {
                int32_t value = env->CallIntMethod(jvalue, mid);
                values.push_back(std::make_shared<int32_t>(value));
            } else {
                values.push_back(std::shared_ptr<int32_t>());
            }
        }

        // get the number of dice
        mid = env->GetMethodID(diceConfigClass, "getNumberOfDice", "()I");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrDice = env->CallIntMethod(obj, mid);

        // get reroll indices (into symbols vector
        mid = env->GetMethodID(diceConfigClass, "getNbrIndicesReRollOn", "()I");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jint nbrIndicesRerollOn = env->CallIntMethod(obj, mid);

        std::vector<uint32_t> indicesRerollOn;

        if (nbrIndicesRerollOn > 0) {
            jintArray jarrayindicesRerollOn = env->NewIntArray(nbrIndicesRerollOn);

            mid = env->GetMethodID(diceConfigClass, "getReRollOn", "([I)V");
            if (mid == 0) {
                throw std::runtime_error("Could not load dice models");
            }
            env->CallVoidMethod(obj, mid, jarrayindicesRerollOn);

            jint *jindicesRerollOn = env->GetIntArrayElements(jarrayindicesRerollOn, NULL);
            for (int i = 0; i < nbrIndicesRerollOn; i++) {
                indicesRerollOn.push_back(jindicesRerollOn[i]);
            }

            // we don't want to copy back changes to the array so use JNI_ABORT mode.
            env->ReleaseIntArrayElements(jarrayindicesRerollOn, jindicesRerollOn, JNI_ABORT);
        }

        // color
        mid = env->GetMethodID(diceConfigClass, "isRainbow", "()Z");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }
        jboolean rainbow = env->CallBooleanMethod(obj, mid);

        std::vector<float> color;
        if (!rainbow) {
            mid = env->GetMethodID(diceConfigClass, "getColor", "([F)Z");
            if (mid == 0) {
                throw std::runtime_error("Could not load dice models");
            }

            // the color is always 4 floats long
            jfloatArray jarrayColor = env->NewFloatArray(4);

            jboolean result = env->CallBooleanMethod(obj, mid, jarrayColor);
            if (result) {
                jfloat *jcolor = env->GetFloatArrayElements(jarrayColor, nullptr);
                for (int i = 0; i < 4; i++) {
                    color.push_back(jcolor[i]);
                }

                // we don't want to copy back changes to the array so use JNI_ABORT mode.
                env->ReleaseFloatArrayElements(jarrayColor, jcolor, JNI_ABORT);
            }
        }

        // Is this dice config being added or subtracted?
        mid = env->GetMethodID(diceConfigClass, "isAddOperation", "()Z");
        if (mid == 0) {
            throw std::runtime_error("Could not load dice models");
        }

        bool isAddOperation = env->CallBooleanMethod(obj, mid);

        dice.push_back(std::make_shared<DiceDescription>(static_cast<uint32_t>(nbrDice),
                symbolsDiceVector, values, color, indicesRerollOn, isAddOperation));
    }

    return std::make_pair(dice, texture);
}

std::vector<std::vector<uint32_t>> initResults(JNIEnv *env, jobject jDiceResults) {
    std::vector<std::vector<uint32_t>> diceResults;

    jclass diceResultsClass = env->GetObjectClass(jDiceResults);
    jmethodID mid = env->GetMethodID(diceResultsClass, "getNbrResults", "()I");
    if (mid == 0) {
        throw std::runtime_error("Could not load dice results");
    }
    uint32_t nbrResults = env->CallIntMethod(jDiceResults, mid);

    jmethodID midNbrResultsForDie = env->GetMethodID(diceResultsClass,"geNbrResultsForDie", "(I)I");
    jmethodID midResultsForDie = env->GetMethodID(diceResultsClass, "getResultsForDie", "(I[I)V");
    if (midNbrResultsForDie == 0 || midResultsForDie == 0) {
        throw std::runtime_error("Could not load dice results");
    }

    for (uint32_t i = 0; i < nbrResults; i++) {
        std::vector<uint32_t> resultsForDie;
        uint32_t nbrResultsForDie = env->CallIntMethod(jDiceResults, midNbrResultsForDie, i);
        jintArray jArrayResultsForDie = env->NewIntArray(nbrResultsForDie);

        env->CallVoidMethod(jDiceResults, midResultsForDie, i, jArrayResultsForDie);
        jint *jresultsForDie = env->GetIntArrayElements(jArrayResultsForDie, NULL);
        for (uint32_t i = 0; i < nbrResultsForDie; i++) {
            resultsForDie.push_back(jresultsForDie[i]);
        }
        // we don't want to copy back changes to the array so use JNI_ABORT mode.
        env->ReleaseIntArrayElements(jArrayResultsForDie, jresultsForDie, JNI_ABORT);

        diceResults.push_back(resultsForDie);
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
        std::string diceName(cdiceName);
        env->ReleaseStringUTFChars(jdiceName, cdiceName);

        auto event = std::make_shared<DiceChangeEvent>(diceName, std::move(dice.first), dice.second);

        diceChannel().sendEvent(event);
        return env->NewStringUTF("");
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
        return env->NewStringUTF("");
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
    const char *cdiceName = env->GetStringUTFChars(jdiceName, nullptr);
    std::string diceName(cdiceName);
    env->ReleaseStringUTFChars(jdiceName, cdiceName);

    jclass diceResultsClass = env->GetObjectClass(jDiceResults);
    jmethodID mid = env->GetMethodID(diceResultsClass, "isModifiedRoll", "()Z");
    if (mid == 0) {
        return env->NewStringUTF("Could not load dice results");
    }
    bool isModifiedRoll = env->CallBooleanMethod(jDiceResults, mid);

    try {

        std::vector<std::vector<uint32_t>> upFaceIndices = std::move(initResults(env, jDiceResults));
        std::pair<std::vector<std::shared_ptr<DiceDescription>>, std::shared_ptr<TextureAtlas>> dice =
                initDice(env, jDiceConfigs,
                         jSymbols, width, height, textureCoordLeft,
                         textureCoordRight, textureCoordTop,
                         textureCoordBottom, jbitmap);
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

extern "C" JNIEXPORT jstring JNICALL
Java_com_quasar_cerulean_rainbowdice_DiceWorker_startWorker(
        JNIEnv *env,
        jobject jthis,
        jobject jsurface,
        jobject jmanager,
        jobject jnotify) {

    try {
        setAssetManager(AAssetManager_fromJava(env, jmanager));

        ANativeWindow *window = ANativeWindow_fromSurface(env, jsurface);
        if (window == nullptr) {
            return env->NewStringUTF("Unable to acquire window from surface.");
        }

        auto deleter = [](WindowType *windowRaw) {
            /* release the java window object */
            if (windowRaw != nullptr) {
                ANativeWindow_release(windowRaw);
            }
        };

        std::shared_ptr<WindowType> surface(window, deleter);

        std::unique_ptr<Notify> notify(new Notify(env, jnotify));
        DiceWorker worker(surface, notify);
        surface.reset();

        worker.waitingLoop();
        return env->NewStringUTF("");
    } catch (std::runtime_error &e) {
        if (strlen(e.what()) > 0) {
            return env->NewStringUTF((std::string("error: ") + e.what()).c_str());
        } else {
            return env->NewStringUTF("error: Error in initializing graphics.");
        }
    }
}

void Notify::sendResult(
        std::string const &diceName,
        bool isModified,
        std::vector<std::vector<uint32_t>> const &results,
        std::vector<std::shared_ptr<DiceDescription>> const &dice) {
    std::string totalResult;
    jclass notifyClass = m_env->GetObjectClass(m_notify);

    jmethodID midAddMultiResult = m_env->GetMethodID(notifyClass, "addResult", "([I)V");
    if (midAddMultiResult == nullptr) {
        throw std::runtime_error("Could not send message.");
    }

    jmethodID midAddSingleResult = m_env->GetMethodID(notifyClass, "addResult", "(I)V");
    if (midAddSingleResult == nullptr) {
        throw std::runtime_error("Could not send message.");
    }

    for (auto const &result : results) {
        if (result.size() == 1) {
            m_env->CallVoidMethod(m_notify, midAddSingleResult, result[0]);
        } else {
            jintArray jresultsArray = m_env->NewIntArray(static_cast<jsize>(result.size()));
            jint *jresults = m_env->GetIntArrayElements(jresultsArray, nullptr);
            int i = 0;
            for (auto const &dieResult : result) {
                jresults[i++] = dieResult;
            }
            m_env->ReleaseIntArrayElements(jresultsArray, jresults, JNI_COMMIT);
            m_env->CallVoidMethod(m_notify, midAddMultiResult, jresultsArray);
        }
    }

    jmethodID midAddDice = m_env->GetMethodID(notifyClass, "addDice",
            "(I[Ljava/lang/String;[Ljava/lang/Integer;[I[FZ)V");
    if (midAddDice == nullptr) {
        throw std::runtime_error("Could not send message.");
    }
    for (auto const &die : dice) {
        jobjectArray jsymbolArray = m_env->NewObjectArray(die->m_symbols.size(),
                m_env->FindClass("java/lang/String"), m_env->NewStringUTF(""));
        int i = 0;
        for (auto const & symbol : die->m_symbols) {
            m_env->SetObjectArrayElement(jsymbolArray, i++, m_env->NewStringUTF(symbol.c_str()));
        }

        jintArray jrerollIndicesArray = m_env->NewIntArray(static_cast<jsize>(die->m_rerollOnIndices.size()));
        jint *jindices = m_env->GetIntArrayElements(jrerollIndicesArray, nullptr);
        i = 0;
        for (auto const &rerollIndex : die->m_rerollOnIndices) {
            jindices[i++] = rerollIndex;
        }
        m_env->ReleaseIntArrayElements(jrerollIndicesArray, jindices, JNI_COMMIT);

        jfloatArray jcolorArray = m_env->NewFloatArray(static_cast<jsize>(die->m_color.size()));
        jfloat *jcolor = m_env->GetFloatArrayElements(jcolorArray, nullptr);
        i = 0;
        for (auto const &colorValue : die->m_color) {
            jcolor[i++] = colorValue;
        }
        m_env->ReleaseFloatArrayElements(jcolorArray, jcolor, JNI_COMMIT);

        jclass jIntegerClass = m_env->FindClass("java/lang/Integer");
        jmethodID midIntegerInit = m_env->GetMethodID(jIntegerClass, "<init>", "(I)V");
        jobjectArray jvalueArray = m_env->NewObjectArray(die->m_values.size(),
                                                          jIntegerClass,
                                                          nullptr);
        i = 0;
        for (auto const & value : die->m_values) {
            if (value != nullptr) {
                m_env->SetObjectArrayElement(jvalueArray, i,
                                             m_env->NewObject(jIntegerClass, midIntegerInit,
                                                              *value));
            }
            i++;
        }

        m_env->CallVoidMethod(m_notify, midAddDice, die->m_nbrDice, jsymbolArray, jvalueArray,
                jrerollIndicesArray, jcolorArray, die->m_isAddOperation);
    }

    jmethodID midSend = m_env->GetMethodID(notifyClass, "sendResults", "(Ljava/lang/String;Z)V");
    if (midSend == nullptr) {
        throw std::runtime_error("Could not send message.");
    }

    m_env->CallVoidMethod(m_notify, midSend, m_env->NewStringUTF(diceName.c_str()), isModified);
}

void Notify::sendError(std::string const &error) {
    jclass notifyClass = m_env->GetObjectClass(m_notify);

    jmethodID midSend = m_env->GetMethodID(notifyClass, "sendError", "(Ljava/lang/String;)V");
    if (midSend == nullptr) {
        throw std::runtime_error("Could not send error message.");
    }

    jstring jerror = m_env->NewStringUTF(error.c_str());
    m_env->CallVoidMethod(m_notify, midSend, jerror);
}