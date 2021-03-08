/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */
#ifdef WIN32
#include <Windows.h>
#else
#include "dlfcn.h"
#include <boost/filesystem.hpp>
#endif

#include "test_module_loader.h"
#include <iostream>

class test_module
{
public:
    test_module(const std::string& dllName) : m_dllName(dllName), m_handle(nullptr) {}

    GetTestsFunc get_test_list()
    {
#if defined(_WIN32)
        return (GetTestsFunc)GetProcAddress(m_handle, "GetTestList");
#else
        auto ptr = dlsym(m_handle, "GetTestList");
        if (ptr == nullptr)
        {
            std::cerr << "couldn't find GetTestList"
                      <<
#ifdef __APPLE__
                " " << dlerror() <<
#endif
                std::endl;
        }
        return (GetTestsFunc)ptr;
#endif
    }

    unsigned long load()
    {
        if (m_handle == nullptr)
        {
#if defined(_WIN32)
            // Make sure ends in .dll
            if (*(m_dllName.end() - 1) != 'l' || *(m_dllName.end() - 2) != 'l' || *(m_dllName.end() - 3) != 'd' ||
                *(m_dllName.end() - 4) != '.')
            {
                return (unsigned long)-1;
            }
            m_handle = LoadLibraryA(m_dllName.c_str());
            if (m_handle == nullptr)
            {
                return GetLastError();
            }
            return 0;
#else
#ifdef __APPLE__
            auto exe_directory = getcwd(nullptr, 0);
            auto path = std::string(exe_directory) + "/" + m_dllName;
            free(exe_directory);
#else
            auto path = boost::filesystem::initial_path().string() + "/" + m_dllName;
#endif

            m_handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
            if (m_handle == nullptr)
            {
                std::cerr << std::string(dlerror()) << std::endl;
                return -1;
            }
            return 0;
#endif
        }
        return 0;
    }

    unsigned long unload()
    {
        if (m_handle != nullptr)
        {
#if defined(_WIN32)
            if (!FreeLibrary(m_handle))
            {
                return GetLastError();
            }
            m_handle = nullptr;
            return 0;
#else
            if (dlclose(m_handle) != 0)
            {
                std::cerr << std::string(dlerror()) << std::endl;
                return -1;
            }
            m_handle = nullptr;
            return 0;
#endif
        }
        return 0;
    }

private:
    const std::string m_dllName;

#if defined(_WIN32)
    HMODULE m_handle;
#else
    void* m_handle;
#endif

    test_module(const test_module&) = delete;
    test_module& operator=(const test_module&) = delete;
};

test_module_loader::test_module_loader() {}

test_module_loader::~test_module_loader()
{
    for (auto iter = m_modules.begin(); iter != m_modules.end(); ++iter)
    {
        iter->second->unload();
        delete iter->second;
    }
}

unsigned long test_module_loader::load(const std::string& dllName)
{
    // Check if the module is already loaded.
    if (m_modules.find(dllName) != m_modules.end())
    {
        return 0;
    }

    test_module* pModule;
    pModule = new test_module(dllName);

    // Load dll.
    const unsigned long error_code = pModule->load();
    if (error_code != 0)
    {
        delete pModule;
        return error_code;
    }
    else
    {
        m_modules[dllName] = pModule;
    }
    return 0;
}

UnitTest::TestList g_list;

UnitTest::TestList& test_module_loader::get_test_list(const std::string& dllName)
{
    GetTestsFunc getTestsFunc = m_modules[dllName]->get_test_list();

    // If there is no GetTestList function then it must be a dll without any tests.
    // Simply return an empty TestList.
    if (getTestsFunc == nullptr)
    {
        return g_list;
    }

    return getTestsFunc();
}
