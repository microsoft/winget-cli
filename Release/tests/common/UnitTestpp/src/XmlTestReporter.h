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

#ifndef UNITTEST_XMLTESTREPORTER_H
#define UNITTEST_XMLTESTREPORTER_H

#include "../config.h"
#ifndef UNITTEST_NO_DEFERRED_REPORTER

#include "DeferredTestReporter.h"
#include <iosfwd>

namespace UnitTest
{
class XmlTestReporter : public DeferredTestReporter
{
public:
    explicit UNITTEST_LINKAGE XmlTestReporter(std::ostream& ostream);

    virtual UNITTEST_LINKAGE void ReportSummary(int totalTestCount,
                                                int failedTestCount,
                                                int failureCount,
                                                float secondsElapsed);

private:
    XmlTestReporter(XmlTestReporter const&);
    XmlTestReporter& operator=(XmlTestReporter const&);

    void AddXmlElement(std::ostream& os, char const* encoding);
    void BeginResults(
        std::ostream& os, int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed);
    void EndResults(std::ostream& os);
    void BeginTest(std::ostream& os, DeferredTestResult const& result);
    void AddFailure(std::ostream& os, DeferredTestResult const& result);
    void EndTest(std::ostream& os, DeferredTestResult const& result);

    std::ostream& m_ostream;
};

} // namespace UnitTest

#endif
#endif
