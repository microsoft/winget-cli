set(UT_SOURCES
  src/AssertException.cpp
  src/CompositeTestReporter.cpp
  src/CurrentTest.cpp
  src/DeferredTestReporter.cpp
  src/DeferredTestResult.cpp
  src/GlobalSettings.cpp
  src/MemoryOutStream.cpp
  src/ReportAssert.cpp
  src/Test.cpp
  src/TestDetails.cpp
  src/TestList.cpp
  src/TestReporter.cpp
  src/TestReporterStdout.cpp
  src/TestResults.cpp
  src/TestRunner.cpp
  src/XmlTestReporter.cpp
  )

set(TEST_SOURCES
  src/tests/TestAssertHandler.cpp
  src/tests/TestCheckMacros.cpp
  src/tests/TestChecks.cpp
  src/tests/TestCompositeTestReporter.cpp
  src/tests/TestCurrentTest.cpp
  src/tests/TestDeferredTestReporter.cpp
  src/tests/TestMemoryOutStream.cpp
  src/tests/TestTest.cpp
  src/tests/TestTestList.cpp
  src/tests/TestTestMacros.cpp
  src/tests/TestTestResults.cpp
  src/tests/TestTestRunner.cpp
  src/tests/TestTestSuite.cpp
  src/tests/TestUnitTestPP.cpp
  src/tests/TestXmlTestReporter.cpp
  )

if(UNIX)
  list(APPEND UT_SOURCES
    src/Posix/SignalTranslator.cpp
    src/Posix/TimeHelpers.cpp
    )
elseif(WIN32)
  list(APPEND UT_SOURCES src/Win32/TimeHelpers.cpp)

  add_definitions(-DWIN32 -D_USRDLL -D_CRT_SECURE_NO_DEPRECATE -DUNITTEST_DLL_EXPORT)
endif()

add_library(unittestpp ${UT_SOURCES})
target_link_libraries(unittestpp PUBLIC cpprest)

if(UNIX)
  cpprest_find_boost()
  target_link_libraries(unittestpp PUBLIC cpprestsdk_boost_internal)
endif()
target_link_libraries(unittestpp ${ANDROID_STL_FLAGS})

target_include_directories(unittestpp PRIVATE src)
target_include_directories(unittestpp PUBLIC .)
