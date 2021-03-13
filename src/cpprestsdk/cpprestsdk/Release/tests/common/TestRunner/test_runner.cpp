/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 **/
// TestRunner.cpp : Defines the entry point for the console application.
//

#include <algorithm>
#include <iostream>
#include <map>
#include <regex>
#include <vector>

#ifdef _WIN32
#include <conio.h>

#include <Windows.h>
#else
#include <unistd.h>
#ifdef __APPLE__
#include <dirent.h>
#else
#include <boost/filesystem.hpp>
#endif
#endif

#include "../UnitTestpp/src/GlobalSettings.h"
#include "../UnitTestpp/src/TestReporterStdout.h"
#include "../UnitTestpp/src/TimeHelpers.h"
#include "test_module_loader.h"

static void print_help()
{
    std::cout
        << "Usage: testrunner.exe <test_binaries> [/list] [/listproperties] [/noignore] [/breakonerror] [/detectleaks]"
        << std::endl;
    std::cout << "    [/name:<test_name>] [/select:@key=value] [/loop:<num_times>]" << std::endl;
    std::cout << std::endl;
    std::cout << "    /list              List all the names of the test_binaries and their" << std::endl;
    std::cout << "                       test cases." << std::endl;
    std::cout << std::endl;
    std::cout << "    /listproperties    List all the names of the test binaries, test cases, and" << std::endl;
    std::cout << "                       test properties." << std::endl;
    std::cout << std::endl;
    std::cout << "    /breakonerror      Break into the debugger when a failure is encountered." << std::endl;
    std::cout << "    /detectleaks       Turns CRT leak detection and prints any leaks, Windows only." << std::endl;
    std::cout << std::endl;
    std::cout << "    /name:<test_name>  Run only test cases with matching name. Can contain the" << std::endl;
    std::cout << "                       wildcard '*' character." << std::endl;
    std::cout << std::endl;
    std::cout << "    /noignore          Include tests even if they have the 'Ignore' property set" << std::endl;
    std::cout << std::endl;
    std::cout << "    /select:@key=value Filter by the value of a particular test property." << std::endl;
    std::cout << std::endl;
    std::cout << "    /loop:<num_times>  Run test cases a specified number of times." << std::endl;

    std::cout << std::endl;
    std::cout << "Can also specify general global settings with the following:" << std::endl;
    std::cout << "    /global_key:global_value OR /global_key" << std::endl << std::endl;
}

static std::string to_lower(const std::string& str)
{
    std::string lower;
    for (auto iter = str.begin(); iter != str.end(); ++iter)
    {
        lower.push_back((char)tolower(*iter));
    }
    return lower;
}

static std::vector<std::string> get_files_in_directory()
{
    std::vector<std::string> files;

#ifdef _WIN32

    char exe_directory_buffer[MAX_PATH];
    GetModuleFileNameA(NULL, exe_directory_buffer, MAX_PATH);
    std::string exe_directory = to_lower(exe_directory_buffer);
    auto location = exe_directory.rfind("\\");
    if (location != std::string::npos)
    {
        exe_directory.erase(location + 1);
    }
    else
    {
        std::cout << "Could not determine execution directory" << std::endl;
        exit(-1);
    }

    exe_directory.append("*");
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(exe_directory.c_str(), &findFileData);
    if (hFind != INVALID_HANDLE_VALUE && !(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        files.push_back(findFileData.cFileName);
    }
    while (FindNextFileA(hFind, &findFileData) != 0)
    {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            files.push_back(findFileData.cFileName);
        }
    }
    FindClose(hFind);

#elif defined(__APPLE__)
    auto exe_directory = getcwd(nullptr, 0);

    DIR* dir = opendir(exe_directory);
    free(exe_directory);

    if (dir != nullptr)
    {
        struct dirent* ent = readdir(dir);
        while (ent != nullptr)
        {
            if (ent->d_type == DT_REG)
            {
                files.push_back(ent->d_name);
            }
            ent = readdir(dir);
        }
        closedir(dir);
    }
#else
    using namespace boost::filesystem;

    auto exe_directory = initial_path().string();
    for (auto it = directory_iterator(path(exe_directory)); it != directory_iterator(); ++it)
    {
        if (is_regular_file(*it))
        {
            files.push_back(it->path().filename().string());
        }
    }
#endif

    return files;
}

static std::string replace_wildcard_for_regex(const std::string& str)
{
    std::string result;
    for (auto iter = str.begin(); iter != str.end(); ++iter)
    {
        if (*iter == '*')
        {
            result.push_back('.');
        }
        result.push_back(*iter);
    }
    return result;
}

static std::vector<std::string> get_matching_binaries(const std::string& dllName)
{
    std::vector<std::string> matchingFiles;

    // If starts with .\ remove it.
    std::string expandedDllName(dllName);
    if (expandedDllName.size() > 2 && expandedDllName[0] == '.' && expandedDllName[1] == '\\')
    {
        expandedDllName = expandedDllName.substr(2);
    }

    // Escape any '.'
    size_t oldLocation = 0;
    size_t location = expandedDllName.find(".", oldLocation);
    while (location != std::string::npos)
    {
        expandedDllName.insert(expandedDllName.find(".", oldLocation), "\\");
        oldLocation = location + 2;
        location = expandedDllName.find(".", oldLocation);
    }

    // Replace all '*' in dllName with '.*'
    expandedDllName = replace_wildcard_for_regex(expandedDllName);

    std::vector<std::string> allFiles = get_files_in_directory();

    // Filter out any files that don't match.
    std::regex dllRegex(expandedDllName, std::regex_constants::icase);

    for (auto iter = allFiles.begin(); iter != allFiles.end(); ++iter)
    {
        if (std::regex_match(*iter, dllRegex))
        {
            matchingFiles.push_back(*iter);
        }
    }

    return matchingFiles;
}

static std::multimap<std::string, std::string> g_properties;
static std::vector<std::string> g_test_binaries;
static int g_individual_test_timeout = 60000 * 3;

static int parse_command_line(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        arg = to_lower(arg);

        if (arg.compare("/?") == 0)
        {
            print_help();
            return -1;
        }
        else if (arg.find("/") == 0)
        {
            if (arg.find("/select:@") == 0)
            {
                std::string prop_asgn = std::string(argv[i]).substr(std::string("/select:@").size());
                auto eqsgn = prop_asgn.find('=');
                if (eqsgn < prop_asgn.size())
                {
                    auto key = prop_asgn.substr(0, eqsgn);
                    auto value = prop_asgn.substr(eqsgn + 1);
                    g_properties.insert(std::make_pair(key, value));
                }
                else
                {
                    g_properties.insert(std::make_pair(prop_asgn, "*"));
                }
            }
            else if (arg.find(":") != std::string::npos)
            {
                const size_t index = arg.find(":");
                const std::string key = std::string(argv[i]).substr(1, index - 1);
                const std::string value = std::string(argv[i]).substr(index + 1);
                UnitTest::GlobalSettings::Add(key, value);
            }
            else
            {
                UnitTest::GlobalSettings::Add(arg.substr(1), std::string{});
            }
        }
        else if (arg.find("/debug") == 0)
        {
            printf("Attach debugger now...\n");
            int temp;
            std::cin >> temp;
        }
        else
        {
            g_test_binaries.push_back(arg);
        }
    }

    return 0;
}

static bool matched_properties(const UnitTest::TestProperties& test_props)
{
    // TestRunner can only execute either desktop or winrt tests, but not both.
    // This starts with visual studio versions after VS 2012.
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
#ifdef WINRT_TEST_RUNNER
    UnitTest::GlobalSettings::Add("winrt", std::string{});
#elif defined DESKTOP_TEST_RUNNER
    UnitTest::GlobalSettings::Add("desktop", std::string{});
#endif
#endif

    // The 'Require' property on a test case is special.
    // It requires a certain global setting to be fulfilled to execute.
    if (test_props.Has("Requires"))
    {
        const std::string requires = test_props.Get("Requires");
        std::vector<std::string> requirements;

        // Can be multiple requirements, a semi colon seperated list
        std::string::size_type pos = requires.find_first_of(';');
        std::string::size_type last_pos = 0;
        while (pos != std::string::npos)
        {
            requirements.push_back(requires.substr(last_pos, pos - last_pos));
            last_pos = pos + 1;
            pos = requires.find_first_of(';', last_pos);
        }
        requirements.push_back(requires.substr(last_pos));
        for (auto iter = requirements.begin(); iter != requirements.end(); ++iter)
        {
            if (!UnitTest::GlobalSettings::Has(to_lower(*iter)))
            {
                return false;
            }
        }
    }

    if (g_properties.size() == 0) return true;

    // All the properties specified at the cmd line act as a 'filter'.
    for (auto iter = g_properties.begin(); iter != g_properties.end(); ++iter)
    {
        auto name = iter->first;
        auto value = iter->second;
        if (test_props.Has(name) && (value == "*" || test_props[name] == value))
        {
            return true;
        }
    }
    return false;
}

// Functions to list all the test cases and their properties.
static void handle_list_option(bool listProperties, const UnitTest::TestList& tests, const std::regex& nameRegex)
{
    UnitTest::Test* pTest = tests.GetFirst();
    while (pTest != nullptr)
    {
        std::string fullTestName = pTest->m_details.suiteName;
        fullTestName.append(":");
        fullTestName.append(pTest->m_details.testName);

        if (matched_properties(pTest->m_properties) && std::regex_match(fullTestName, nameRegex))
        {
            std::cout << "    " << fullTestName << std::endl;
            if (listProperties)
            {
                std::for_each(pTest->m_properties.begin(),
                              pTest->m_properties.end(),
                              [&](const std::pair<std::string, std::string> key_value) {
                                  std::cout << "        " << key_value.first << ": " << key_value.second << std::endl;
                              });
            }
        }
        pTest = pTest->m_nextTest;
    }
}

static void ChangeConsoleTextColorToRed()
{
#if defined(__cplusplus_winrt)
#elif defined(_WIN32)
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0004 | 0x0008);
#else
    std::cout << "\033[1;31m";
#endif
}

static void ChangeConsoleTextColorToGreen()
{
#if defined(__cplusplus_winrt)
#elif defined(_WIN32)
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0002 | 0x0008);
#else
    std::cout << "\033[1;32m";
#endif
}

static void ChangeConsoleTextColorToGrey()
{
#if defined(__cplusplus_winrt)
#elif defined(_WIN32)
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
#else
    std::cout << "\033[0m";
#endif
}

bool IsTestIgnored(UnitTest::Test* pTest)
{
    if (pTest->m_properties.Has("Ignore")) return true;
#ifdef _WIN32
    if (pTest->m_properties.Has("Ignore:Windows")) return true;
#elif defined(__APPLE__)
    if (pTest->m_properties.Has("Ignore:Apple")) return true;
#elif (defined(ANDROID) || defined(__ANDROID__))
    if (pTest->m_properties.Has("Ignore:Android")) return true;
#else
    if (pTest->m_properties.Has("Ignore:Linux")) return true;
#endif
    return false;
}

typedef std::map<std::string, UnitTest::TestList> testlist_t;

void list_test_options(testlist_t& testlists)
{
    std::regex nameRegex;

    if (UnitTest::GlobalSettings::Has("name"))
    {
        nameRegex = replace_wildcard_for_regex(UnitTest::GlobalSettings::Get("name"));
    }
    else
    {
        nameRegex = std::regex(".*");
    }

    bool listProperties = UnitTest::GlobalSettings::Has("listproperties");

    for (auto& test_p : testlists)
    {
        std::cout << "=== Showing options for " << test_p.first << " ===" << std::endl;
        handle_list_option(listProperties, test_p.second, nameRegex);
    }
}

testlist_t load_all_tests(test_module_loader& module_loader)
{
    // Remember where each list of tests came from.
    testlist_t testlists;

    // Retrieve the static tests and clear for dll loading.
    testlists.insert({"<static>", UnitTest::GetTestList()});
    UnitTest::GetTestList().Clear();

    // Cycle through all the test binaries and load them
    for (auto& binary_names : g_test_binaries)
    {
        std::vector<std::string> matchingBinaries = get_matching_binaries(binary_names);
        if (matchingBinaries.empty())
        {
            ChangeConsoleTextColorToRed();
            std::cout << "Pattern '" << binary_names << "' not found." << std::endl;
            ChangeConsoleTextColorToGrey();
        }
        for (auto& binary : matchingBinaries)
        {
            unsigned long error_code = module_loader.load(binary);
            if (error_code != 0)
            {
                // Only omit an error if a wildcard wasn't used.
                if (binary_names.find('*') == std::string::npos)
                {
                    ChangeConsoleTextColorToRed();
                    std::cout << "Error loading " << binary << ": " << error_code << std::endl;
                    ChangeConsoleTextColorToGrey();

                    std::exit(error_code);
                }
                else
                {
                    continue;
                }
            }
            std::cout << "Loaded " << binary << "..." << std::endl;

            // Store the loaded binary into the test list map
            testlists.insert({binary, UnitTest::GetTestList()});
            UnitTest::GetTestList().Clear();
        }
    }

    return testlists;
}

void run_all_tests(UnitTest::TestRunner& testRunner, testlist_t& testlists)
{
    int numTimesToRun = 1;
    if (UnitTest::GlobalSettings::Has("loop"))
    {
        std::istringstream strstream(UnitTest::GlobalSettings::Get("loop"));
        strstream >> numTimesToRun;
    }

    const bool include_ignored_tests = UnitTest::GlobalSettings::Has("noignore");

    for (int i = 0; i < numTimesToRun; ++i)
    {
        for (auto& test_p : testlists)
        {
            std::cout << "=== Running tests from: " << test_p.first << " ===" << std::endl;
            UnitTest::TestList& tests = test_p.second;

            std::regex nameRegex(".*");

            if (UnitTest::GlobalSettings::Has("name"))
            {
                nameRegex = replace_wildcard_for_regex(UnitTest::GlobalSettings::Get("name"));
            }
            testRunner.RunTestsIf(tests,
                                  [&](UnitTest::Test* pTest) -> bool {
                                      // Combine suite and test name
                                      std::string fullTestName = pTest->m_details.suiteName;
                                      fullTestName.append(":");
                                      fullTestName.append(pTest->m_details.testName);

                                      if (IsTestIgnored(pTest) && !include_ignored_tests)
                                          return false;
                                      else
                                          return matched_properties(pTest->m_properties) &&
                                                 std::regex_match(fullTestName, nameRegex);
                                  },
                                  g_individual_test_timeout);
        }
    }
}

#if defined(__cplusplus_winrt)
#include "ROApi.h"
#endif

int main(int argc, char* argv[])
{
#if defined(__cplusplus_winrt)
    Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
#elif defined(_WIN32)
    // Add standard error as output as well.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

    // The test runner built with WinRT support might be used on a pre Win8 machine.
    // Obviously in that case WinRT test cases can't run, but non WinRT ones should be
    // fine. So dynamically try to call RoInitialize/RoUninitialize.
    HMODULE hComBase = LoadLibrary(L"combase.dll");
    if (hComBase != nullptr)
    {
        typedef HRESULT(WINAPI * RoInit)(int);
        RoInit roInitFunc = (RoInit)GetProcAddress(hComBase, "RoInitialize");
        if (roInitFunc != nullptr)
        {
            roInitFunc(1); // RO_INIT_MULTITHREADED
        }
    }

    struct console_restorer
    {
        CONSOLE_SCREEN_BUFFER_INFO m_originalConsoleInfo;
        console_restorer() { GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &m_originalConsoleInfo); }
        ~console_restorer()
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), m_originalConsoleInfo.wAttributes);
        }
    } local;
#endif

    if (parse_command_line(argc, argv) != 0)
    {
        return -1;
    }

    if (g_test_binaries.empty())
    {
        std::cout << "Warning: no test binaries were specified" << std::endl;
    }

    int totalTestCount = 0, failedTestCount = 0;
    std::vector<std::string> failedTests;
    UnitTest::TestReporterStdout testReporter;

    bool breakOnError = false;
    if (UnitTest::GlobalSettings::Has("breakonerror"))
    {
        breakOnError = true;
    }

    // The list_test_options() function determines if list or listProperties.
    bool listOption = false;
    if (UnitTest::GlobalSettings::Has("list"))
    {
        listOption = true;
    }
    if (UnitTest::GlobalSettings::Has("listproperties"))
    {
        listOption = true;
    }
#ifdef _WIN32
    if (UnitTest::GlobalSettings::Has("detectleaks"))
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
#endif

    // Start timer.
    UnitTest::Timer timer;
    timer.Start();

    test_module_loader module_loader;

    testlist_t testlists = load_all_tests(module_loader);

    if (listOption)
    {
        list_test_options(testlists);
        return 0;
    }

    // Run test cases
    UnitTest::TestRunner testRunner(testReporter, breakOnError);

    run_all_tests(testRunner, testlists);

    totalTestCount += testRunner.GetTestResults()->GetTotalTestCount();
    failedTestCount += testRunner.GetTestResults()->GetFailedTestCount();
    if (totalTestCount == 0)
    {
        std::cout << "No tests were run. Check the command line syntax (try 'TestRunner.exe /help')" << std::endl;
    }
    else
    {
        if (testRunner.GetTestResults()->GetFailedTestCount() > 0)
        {
            ChangeConsoleTextColorToRed();
            const std::vector<std::string>& failed = testRunner.GetTestResults()->GetFailedTests();
            std::for_each(failed.begin(), failed.end(), [](const std::string& failedTest) {
                std::cout << "**** " << failedTest << " FAILED ****" << std::endl << std::endl;
                std::fflush(stdout);
            });
            ChangeConsoleTextColorToGrey();
        }
        else
        {
            ChangeConsoleTextColorToGreen();
            std::cout << "All test cases PASSED" << std::endl << std::endl;
            ChangeConsoleTextColorToGrey();
        }
        const std::vector<std::string>& newFailedTests = testRunner.GetTestResults()->GetFailedTests();
        failedTests.insert(failedTests.end(), newFailedTests.begin(), newFailedTests.end());

        const double elapsedTime = timer.GetTimeInMs();
        std::cout << "Finished running all " << totalTestCount << " tests." << std::endl
                  << "Took " << elapsedTime << "ms" << std::endl;
    }

#if defined(__cplusplus_winrt)
#elif defined(_WIN32)
    if (hComBase != nullptr)
    {
        typedef void(WINAPI * RoUnInit)();
        RoUnInit roUnInitFunc = (RoUnInit)GetProcAddress(hComBase, "RoUninitialize");
        if (roUnInitFunc != nullptr)
        {
            roUnInitFunc();
        }
        FreeLibrary(hComBase);
    }
#endif

    return failedTestCount;
}
