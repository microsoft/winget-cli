# SFS Client

[![Windows Latest](https://github.com/microsoft/sfs-client/actions/workflows/main-build-windows.yml/badge.svg?branch=main&event=push)](https://github.com/microsoft/sfs-client/actions/workflows/main-build-windows.yml) [![Ubuntu Latest](https://github.com/microsoft/sfs-client/actions/workflows/main-build-ubuntu.yml/badge.svg?branch=main&event=push)](https://github.com/microsoft/sfs-client/actions/workflows/main-build-ubuntu.yml)

## Introduction

This repository holds the Simple File Solution (SFS) Client, a C++ library that simplifies the interface with the SFS service.
Read below to get started on developing and using the library.

## Usage

Follow the [API](API.md) document for tips on how to use the API.

## Getting Started

### Prerequisites

#### Setup script

There are a few dependencies required to work on this project.
To set them up, use the Setup script.

Windows:
```powershell
.\scripts\Setup.ps1
```

Linux:
```bash
source ./scripts/setup.sh
```

The script can be run multiple times as it does not replace what has been installed, and updates dependencies.
It also sets up useful command-line aliases that can be used while developing.

## Consuming the library

This library is distributed as Source Code and meant for consumption in this format. Below we outline how to easily consume us through the vcpkg tool, but feel free to use other methods to incorporate the source.

### vcpkg

The [vcpkg](https://vcpkg.io/) tool is a Microsoft dependency manager for C/C++. It works "for all platforms, buildsystems, and workflows".
In general the dependencies are registered in the central vcpkg registry, where the "portfile" recipes are hosted. These files indicate the way the dependencies should be acquired and built.

One of the features it provides is also a way to find dependencies listed in the local filesystem. See the [overlay-ports](https://learn.microsoft.com/en-us/vcpkg/concepts/overlay-ports) feature to see that.
We are not hosted in the central vcpkg registry, but we provide a template overlay-port for easy consumption of the library. See the [sfs-client-vcpkg-port](./sfs-client-vcpkg-port) folder for the files you need to have in your local repository in order to consume us. A few placeholders have to be filled in on those files.

## Formatting

This project is currently using the clang-format tool to format its source code according to predefined rules.
It is also using cmake-format to format the CMakeLists.txt files.
Both are installed automatically with the Setup script.

### Automatic usage

The project is configured to automatically run formatting upon committing so nobody introduces
unformatted changes to the codebase. This is done through a pre-commit hook.
If you must avoid the hook, you can use `git commit -n` to bypass it.

### Running on command line

Use -h to see the binary help:
```
clang-format -h
cmake-format -h
```

Use -i to edit a file inplace:
```
clang-format -i ./interface.cpp
cmake-format -i ./CMakeLists.txt
```

Wildcards are accepted in clang-format:
```
clang-format -i ./*.h
```

## Building

To build, use the `build` command. It simplifies the CMake build commands and re-generates CMake configurations if needed.

Available build options:

| Switch (PowerShell)  | Switch (Bash)             | Description                                                                              |
|----------------------|---------------------------|------------------------------------------------------------------------------------------|
| -Clean               | --clean                   | Use this to clean the build folder before building.                                      |
| -BuildType           | --build-type              | Use this to define the build type between "Debug" and "Release". The default is "Debug". |
| -EnableTestOverrides | --enable-test-overrides   | Use this to enable test overrides. See [TEST](TEST.md) for more.                         |
| -BuildTests <bool>   | --build-tests {ON, OFF}   | Use this to build tests alongside the library. On by default.                            |
| -BuildSamples <bool> | --build-samples {ON, OFF} | Use this to build samples alongside the library. On by default.                          |

See [below](#building-with-cmake-vscode-extension) for building within VSCode.

## VSCode

[Visual Studio Code](https://code.visualstudio.com) is the recommended editor to work with this project.
But you're free to use other editors and command-line tools.

### Configuring VSCode includes with CMake

VSCode has a great integration with CMake through the CMake Tools extension (ms-vscode.cmake-tools).
It allows you to configure and build the project through the UI.

Open the command pane on VSCode and search for "C/C++: Edit Configurations (JSON)" to create a c_cpp_properties.json file under a .vscode folder in repo root.
The folder is ignored in git by default.
Add the following line to the "configurations" element to make VSCode start using the includes defined in the CMakeLists.txt files.

```json
"configurations": [
  {
    "configurationProvider": "ms-vscode.cmake-tools"
  }
]
```

### Formatting C++ Sources with VSCode

Install the extension https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools and Shift+Alt+F inside a file.
You can also access "Format" options by right clicking on an open file or over a line selection.

If you want it, you can also make VSCode format on each Save operation by adding this to your User JSON settings:
`"editor.formatOnSave": true`

### vcpkg integration

From [the vcpkg docs](https://github.com/Microsoft/vcpkg/#visual-studio-code-with-cmake-tools):

Adding the following to your workspace settings.json will make CMake Tools automatically use vcpkg for libraries:

```json
{
  "cmake.configureSettings": {
    "CMAKE_TOOLCHAIN_FILE": "[vcpkg root]/scripts/buildsystems/vcpkg.cmake"
  }
}
```

### Building with CMake VSCode extension

If you're using the CMake Tools extension on VSCode, you can set the build options through the VSCode settings. Add something like below to either your user or workspace JSON settings to get a default value for an option. You can also later use the command "Edit CMake Cache (UI)" for visual editing.

```json
"cmake.configureArgs": [
    "-DSFS_ENABLE_TEST_OVERRIDES=ON"
]
```

See [SFSOptions.cmake](cmake/SFSOptions.cmake) for the CMake options available for the library.

## Testing

Tests are compiled alongside the library by default, and live in the client/tests subdirectory.
To run the tests, you can use the `test` command. It will run all tests directly and output the result to the console.

If you want to customize the test run, you can make use of the `ctest` tool.

```
ctest --test-dir ./build/client
```

To run specific tests, you can filter the chosen tests through the switch `-R` or `--tests-regex`.
For more test selection switches, use `ctest --help`.

The tests are built using the Catch2 framework. For a more verbose run you can run the executable directly, with -s.

Windows:
```
.\build\tests\bin\<ReleaseConfiguration>\SFSClientTests.exe -s
```

Linux:
```
./build/tests/bin/SFSClientTests -s
```

Follow the [TEST](TEST.md) document for more information regarding testing.

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow Microsoft’s Trademark & Brand Guidelines. Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party’s policies.
