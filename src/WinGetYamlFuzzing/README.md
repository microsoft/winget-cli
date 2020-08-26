# WinGetYamlFuzzing
The goal of this project is to create a [libFuzzer](http://llvm.org/docs/LibFuzzer.html) based fuzzer for our YAML manifest loading.

## Issues
Currently the fuzzer crashes when exceptions are thrown (built using the VS clang 10 package). This is suspected to be caused by the issue mentioned [here](https://github.com/google/oss-fuzz/issues/2328),
which while fixed, was also regressed. While investigation continues, the fuzzer is of little value.

## Running
A script will be added when the issues are resolved and the fuzzer functions. In order to run it I have been doing the following:
1. Copy the CLITests TestData YAML files to a new corpus directory.
2. Run the following command: `WinGetYamlFuzzing.exe -dict=<full path to dictionary.txt in project> <path to corpus directory>`