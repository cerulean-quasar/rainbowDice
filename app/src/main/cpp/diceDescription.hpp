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
#ifndef RAINBOWDICE_DICEDESCRIPTION_HPP
#define RAINBOWDICE_DICEDESCRIPTION_HPP

#include <vector>
#include <string>
#include <memory>

// used to describe dice coming from java.  We convert the dice to the C++ representation, but
// need to keep this object around so that we can use it to reconstruct the Java dice object when
// the roll is done.
struct DiceDescription {
    uint32_t m_nbrDice;
    std::vector<std::string> m_symbols;
    std::vector<std::shared_ptr<int32_t>> m_values;
    std::vector<float> m_color;
    std::vector<uint32_t> m_rerollOnIndices;
    bool m_isAddOperation;

    DiceDescription(
            uint32_t inNbrDice,
            std::vector<std::string> inSymbols,
            std::vector<std::shared_ptr<int32_t>> inValues,
            std::vector<float> inColor,
            std::vector<uint32_t> inRerollOnIndices,
            bool inIsAddOperation)
            : m_nbrDice{inNbrDice},
              m_symbols{std::move(inSymbols)},
              m_values{std::move(inValues)},
              m_color{std::move(inColor)},
              m_rerollOnIndices{std::move(inRerollOnIndices)},
              m_isAddOperation{inIsAddOperation} {}
};

struct GraphicsDescription {
    bool m_isVulkan;
    std::string m_graphicsName;
    std::string m_version;
    std::string m_deviceName;
};

#endif // RAINBOWDICE_DICEDESCRIPTION_HPP