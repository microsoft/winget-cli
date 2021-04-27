# WinGetYamlFuzzing
The goal of this project is to create a [libFuzzer](http://llvm.org/docs/LibFuzzer.html) based fuzzer for our YAML manifest parsing.

## Issues
There is a known issue that is fixed in LLVM 12 for running on Windows, and the only known method to build a successful fuzzer is to manually build a local copy of libFuzzer and link it in to this project.

## Building

First, clone https://github.com/llvm/llvm-project/tree/llvmorg-12.0.0-rc4 (last known working).

From the local clone, run these commands in `cmd` (modifying VS install location as needed):
```
mkdir build
cd build
set verbose=1
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
cmake -DLLVM_ENABLE_PROJECTS="clang;compiler-rt" -DCOMPILER_RT_BUILD_BUILTINS=OFF -DCOMPILER_RT_BUILD_CRT=OFF -DCOMPILER_RT_BUILD_SANITIZERS=OFF -DCOMPILER_RT_BUILD_XRAY=OFF -DCOMPILER_RT_BUILD_PROFILE=OFF -DLLVM_TARGETS_TO_BUILD=X86 -Thost=x64 ../llvm
cmake --build . --target fuzzer --config Release
mkdir ..\install
powershell -Command "$file = 'projects\compiler-rt\lib\fuzzer\cmake_install.cmake'; (Get-Content $file) -replace '\$\(Configuration\)', 'Release' | Out-File -Encoding utf8 $file"
cmake --install . --component fuzzer --config Release -v --prefix ../install
tree /F ..\install
```

Once this has built, update the project's Linker>Input settings to change the placeholder lib to the location of the `clang_rt.fuzzer-x86_64.lib` file under the install directory.

## Running
A script will be added when the issues are resolved and the fuzzer functions out of the box. In order to run it I have been doing the following:
1. Copy the CLITests TestData YAML files to a new corpus directory.
2. Run the following command: `WinGetYamlFuzzing.exe -dict=<full path to dictionary.txt in project> <path to corpus directory>`