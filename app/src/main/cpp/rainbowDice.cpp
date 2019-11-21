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

#include "rainbowDice.hpp"

float constexpr RainbowDice::M_maxZ;

glm::vec3 Filter::acceleration(glm::vec3 const &sensorInputs) {
    float RC = 3.0f;
    auto currentTime = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - highPassAccelerationPrevTime).count();

    highPassAccelerationPrevTime = currentTime;

    unsigned long size = highPassAcceleration.size();
    glm::vec3 acceleration(0.0f, 0.0f, 0.0f);

    if (dt > 1) {
        // the time diff is too long.  This is not good data. Just return the sensor inputs and
        // save the time difference for next call through.
        acceleration = sensorInputs;
    } else if (size == 0) {
        high_pass_samples sample;
        acceleration = sample.output = sample.input = sensorInputs;
        sample.dt = dt;
        highPassAcceleration.push_back(sample);
    } else {
        glm::vec3 nextOut;
        for (unsigned long i=1; i < size; i++) {
            high_pass_samples &sample = highPassAcceleration[i];
            high_pass_samples &prev = highPassAcceleration[i-1];
            float alpha = RC/(RC+sample.dt);
            sample.output = alpha*(prev.output + sample.input - prev.input);

        }
        high_pass_samples sample;
        float alpha = RC/(RC+dt);
        high_pass_samples &prev = highPassAcceleration.back();
        sample.output = alpha*(prev.output + sensorInputs - prev.input);
        sample.input = sensorInputs;
        sample.dt = dt;
        highPassAcceleration.push_back(sample);
        if (size + 1 > highPassAccelerationMaxSize) {
            highPassAcceleration.pop_front();
        }

        if (size < 100) {
            acceleration = sample.output;
        } else {
            acceleration = 20.0f * sample.output + sensorInputs - sample.output;
        }
    }
    return acceleration;
}

