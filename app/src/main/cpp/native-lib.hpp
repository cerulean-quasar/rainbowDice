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
#ifndef RAINBOWDICE_NATIVELIB_HPP
#define RAINBOWDICE_NATIVELIB_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <jni.h>
#include "diceDescription.hpp"

class Notify {
private:
    JNIEnv *m_env;
    jobject m_notify;
public:
    Notify(JNIEnv *inEnv, jobject inNotify)
        : m_env{inEnv},
          m_notify{inNotify} {}
    void sendResult(
            std::string const &diceName,
            bool isModified,
            std::vector<std::vector<uint32_t>> const &results,
            std::vector<std::shared_ptr<DiceDescription>> const &dice);
    void sendError(std::string const &error);
    void sendError(char const *error);
};
#endif // RAINBOWDICE_NATIVELIB_HPP