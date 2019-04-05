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

package com.quasar.cerulean.rainbowdice;

public class Constants {
    // For starting new activities specifying intents, etc
    public static final int DICE_CUSTOMIZATION_ACTIVITY = 2;
    public static final String DICE_FILENAME = "DiceFileName";
    public static final String DICE_CONFIGURATION = "DiceConfiguration";
    public static final String DICE_BEING_EDITED = "DiceBeingEdited";

    public static final int DICE_THEME_SELECTION_ACTIVITY = 3;
    public static final String themeNameConfigValue = "ThemeName";
    public static final String GRAPHICS_API_NAME = "graphics_api_name";
    public static final String GRAPHICS_API_VERSION = "graphics_api_version";
    public static final String GRAPHICS_DEVICE_NAME = "graphics_device_name";
    public static final String GRAPHICS_IS_VULKAN = "graphics_is_vulkan";
    public static final String SENSOR_HAS_LINEAR_ACCELERATION = "sensor_has_linear_acceleration";
    public static final String SENSOR_HAS_GRAVITY = "sensor_has_gravity";
    public static final String SENSOR_HAS_ACCELEROMETER = "sensor_has_accelerometer";

    public static final int DICE_LOG_FILE_ACTIVITY = 4;

    // DiceSidesActivity
    public static final int DICE_SIDES_ACTIVITY = 5;
    public static final String DICE_SIDE_CONFIGURATION = "DiceSideConfiguration";
    public static final String DICE_SIDES_JSON = "DiceSidesJson";
}
