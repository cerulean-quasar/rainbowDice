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
#ifndef RAINBOWDICE_GLOBAL_HPP
#define RAINBOWDICE_GLOBAL_HPP

#include <string>
#include <native_window.h>

typedef ANativeWindow WindowType;

extern const std::string SHADER_VERT_FILE;
extern const std::string SHADER_FRAG_FILE;

#include "rainbowDice.hpp"
#include "text.hpp"
#include <memory>
#include <GLES2/gl2.h>

extern std::unique_ptr<TextureAtlas> texAtlas;

#endif
