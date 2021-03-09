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

#ifndef UNITTEST_TEST_PROPERTIES_H
#define UNITTEST_TEST_PROPERTIES_H

#include <map>
#include <stdexcept>
#include <string>

namespace UnitTest
{
// Simple key value pairs.
class TestProperties
{
public:
    TestProperties() {}

    void Add(const std::string& key, const std::string& value)
    {
        if (!Has(key))
        {
            m_properties[key] = value;
        }
        else
        {
            m_properties[key] += ";";
            m_properties[key] += value;
        }
    }

    bool Has(const std::string& key) const { return m_properties.find(key) != m_properties.end(); }

    const std::string& Get(const std::string& key) const
    {
        if (!Has(key))
        {
            throw std::invalid_argument("Error: property is not found");
        }
        return m_properties.find(key)->second;
    }

    const std::string& operator[](const std::string& key) const { return Get(key); }

    std::map<std::string, std::string>::const_iterator begin() const { return m_properties.begin(); }

    std::map<std::string, std::string>::const_iterator end() const { return m_properties.end(); }

private:
    std::map<std::string, std::string> m_properties;
    TestProperties(const TestProperties&);
    TestProperties& operator=(const TestProperties&);
};

} // namespace UnitTest

#endif
