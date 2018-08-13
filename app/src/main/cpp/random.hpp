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
#ifndef RAINBOW_DICE_RANDOM_HPP
#define RAINBOW_DICE_RANDOM_HPP
class Random {
private:
    int fd;
    template<typename T> T get();
public:
    Random() : fd(-1) { }
    unsigned int getUInt(unsigned int lowerBound, unsigned int upperBound);
    float getFloat(float lowerBound, float upperBound);
    ~Random();
};
#endif