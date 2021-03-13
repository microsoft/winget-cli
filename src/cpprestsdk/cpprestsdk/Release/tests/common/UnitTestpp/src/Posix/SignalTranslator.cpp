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

#include "SignalTranslator.h"

namespace UnitTest
{
sigjmp_buf* SignalTranslator::s_jumpTarget = 0;

namespace
{
void SignalHandler(int sig) { siglongjmp(*SignalTranslator::s_jumpTarget, sig); }

} // namespace

SignalTranslator::SignalTranslator()
{
    m_oldJumpTarget = s_jumpTarget;
    s_jumpTarget = &m_currentJumpTarget;

    struct sigaction action;
    action.sa_flags = 0;
    action.sa_handler = SignalHandler;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, &m_old_SIGSEGV_action);
    sigaction(SIGFPE, &action, &m_old_SIGFPE_action);
    sigaction(SIGTRAP, &action, &m_old_SIGTRAP_action);
    sigaction(SIGBUS, &action, &m_old_SIGBUS_action);
    sigaction(SIGILL, &action, &m_old_SIGBUS_action);
}

SignalTranslator::~SignalTranslator()
{
    sigaction(SIGILL, &m_old_SIGBUS_action, 0);
    sigaction(SIGBUS, &m_old_SIGBUS_action, 0);
    sigaction(SIGTRAP, &m_old_SIGTRAP_action, 0);
    sigaction(SIGFPE, &m_old_SIGFPE_action, 0);
    sigaction(SIGSEGV, &m_old_SIGSEGV_action, 0);

    s_jumpTarget = m_oldJumpTarget;
}

} // namespace UnitTest
