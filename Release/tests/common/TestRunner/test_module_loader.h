/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 ***/

#ifndef INCLUDED_TEST_MODULE_LOADER
#define INCLUDED_TEST_MODULE_LOADER

#include "unittestpp.h"
#include <string>

// Exported function from all test dlls.
typedef UnitTest::TestList&(__cdecl* GetTestsFunc)();

// Interface to implement on each platform to be be able to load/unload and call global functions.
class test_module;

// Handles organizing all test binaries and using the correct module loader.
class test_module_loader
{
public:
    test_module_loader();
    ~test_module_loader();

    // Does't complain if module with same name is already loaded.
    unsigned long load(const std::string& dllName);

    // Module must have already been loaded.
    UnitTest::TestList& get_test_list(const std::string& dllName);

private:
    test_module_loader(const test_module_loader&) = delete;
    test_module_loader& operator=(const test_module_loader&) = delete;

    std::map<std::string, test_module*> m_modules;
};

#endif
