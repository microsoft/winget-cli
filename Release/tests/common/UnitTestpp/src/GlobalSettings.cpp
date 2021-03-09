/***
 * This file is based on or incorporates material from the UnitTest++ r30 open source project.
 * Microsoft is not the original author of this code but has modified it and is licensing the code under
 * the MIT License. Microsoft reserves all other rights not expressly granted under the MIT License,
 * whether by implication, estoppel or otherwise.
 *
 * UnitTest++ r30
 *
 * Copyright (c) 2006 Noel Llopis and Charles Nicholson
 * Portions Copyright (c) Microsoft Corporation
 *
 * All Rights Reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ***/

#include "stdafx.h"

#include "GlobalSettings.h"

#include <algorithm>
#include <map>

namespace UnitTest
{
static std::string to_lower(const std::string& str)
{
    std::string retVal;
    retVal.resize(str.size());
    std::transform(str.begin(), str.end(), retVal.begin(), ::tolower);
    return retVal;
}

std::map<std::string, std::string> g_settings;

void GlobalSettings::Add(const std::string& key, const std::string& value) { g_settings[to_lower(key)] = value; }

bool GlobalSettings::Has(const std::string& key) { return g_settings.find(to_lower(key)) != g_settings.end(); }

const std::string& GlobalSettings::Get(const std::string& key)
{
    if (!Has(key))
    {
        throw std::invalid_argument("Error: property is not found");
    }
    return g_settings.find(to_lower(key))->second;
}

} // namespace UnitTest