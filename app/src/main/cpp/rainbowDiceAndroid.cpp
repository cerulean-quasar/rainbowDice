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
#include "rainbowDice.hpp"
#include "android_native_app_glue.h"

/* read the shader byte code files */
std::vector<char> RainbowDice::readFile(const std::string &filename) {
    if (filename == SHADER_VERT_FILE) {
        return vertexShader;
    } else if (filename == SHADER_FRAG_FILE) {
        return fragmentShader;
    } else {
        throw std::runtime_error(std::string("undefined shader type: ") + filename);
    }
}

/**
 * Choose the resolution of the swap images in the frame buffer.  Just return
 * the current extent (same resolution as the window).
 */
VkExtent2D RainbowDice::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    return capabilities.currentExtent;
}

void RainbowDice::createSurface() {
    VkAndroidSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.window = window;
    if (vkCreateAndroidSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void RainbowDice::destroyWindow() {
    /* release the java window object */
    ANativeWindow_release(window);
}