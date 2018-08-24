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
#include <memory>
#include "rainbowDiceGlobal.hpp"
#include "text.hpp"

const std::string SHADER_VERT_FILE = "vertexShader";
const std::string SHADER_FRAG_FILE = "fragmentShader";

std::unique_ptr<TextureAtlas> texAtlas;

float const screenWidth = 1.2f;
float const screenHeight = 1.6f;

glm::vec3 const viewPoint = {0.0f, 0.0f, 3.0f};
