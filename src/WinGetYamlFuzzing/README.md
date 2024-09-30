---
author: Ryan Fu @ryfu-msft
last updated: 02/07/2024
---

# WinGetYamlFuzzing

The goal of this project is to create a [libFuzzer](http://llvm.org/docs/LibFuzzer.html) based fuzzer for our YAML manifest parsing.

This project only supports the `Fuzzing` configuration in either the `x64` or `x86` platform. The build output directory will be located at `$(ProjectDirectory)\src\$(Platform)\Fuzzing\`

WinGetYamlFuzzer is compiled with `/fsanitize=fuzzer`. This injects the LibFuzzer main function which invokes `LLVMFuzzerTestOneInput`. The LibFuzzer engine code is statically linked into the WinGetYamlFuzzer executable, which is how OneFuzz will interact with the fuzzer by providing the appropriate command-line arguments.

The fuzzer and all libraries that it references need to be compiled with ASan and SanCov (along with various SanCov compiler flags). In order to run the fuzzer, the ASan runtime DLL is required. This file is copied to the output directory as a post-build step from `$(VCToolsInstallDir)\bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll​`.

## Submitting fuzzing artifacts to OneFuzz

The `OneFuzzConfig.json` file contains the information required to submit the fuzzing artifacts. This is where the job dependencies are specified, which includes the fuzzer executable (WinGetYamlFuzzer.exe) and all referenced libraries. This file is copied to the fuzzing build output directory.

The `onefuzz-task@0` task called in our build pipeline yaml file will handle submitting all of the specified fuzzing artifacts to the OneFuzz service which will run the fuzzer and generate ADO bugs to our team if any are encountered. All of the specified job dependencies must be present when submitting to the OneFuzz ADO tas including the OneFuzzConfig.json file.
