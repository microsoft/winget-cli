# xlang

The xlang project contains the tools and components to support the cross-language and cross-platform runtime by Microsoft.

## Building xlang

xlang uses CMake instead of vcxproj in order to support cross platform builds.

### Prerequisites

Building xlang for Windows requires the following to be installed:
* [Visual Studio 2017](https://developer.microsoft.com/windows/downloads), version 15.8.7 or later.
  * Visual Studio's Desktop Development with C++ workload installation is required.
* [CMake](https://cmake.org/), version 3.9 or later
  * Visual Studio's Desktop Development with C++ workload includes CMake 3.11
* [Ninja](https://ninja-build.org/), version 1.8 or later
  * Visual Studio's Desktop Development with C++ workload includes Ninja 1.8.2
* [Windows 10 SDK](https://developer.microsoft.com/en-US/windows/downloads/windows-10-sdk), Version 1903 or later (Build Number 10.0.18362.0)
  * Visual Studio does not install this by default with version 15.8.7 or 15.8.8. It is an optional component in Visual Studio 15.8
* [Visual Studio SDK](https://docs.microsoft.com/en-us/visualstudio/extensibility/installing-the-visual-studio-sdk?view=vs-2017)
  * The Visual Studio SDK is an optional component installed through Visual Studio setup

Building xlang for Linux requires the following to be installed. The [ubuntu/configure.sh](/src/scripts/ubuntu/configure.sh) script will automatically install these depenencies via [apt](https://en.wikipedia.org/wiki/APT_(Debian)).

* [Clang](http://clang.llvm.org/), version 6.0 or later.
* LLVM [libc++](http://libcxx.llvm.org/) and [libc++ ABI](http://libcxxabi.llvm.org/), release 60 or later.
* [CMake](https://cmake.org/), version 3.9 or later
* [Ninja](https://ninja-build.org/), version 1.8 or later

> Note, xlang has very few platform-specific dependencies, so it should be easy to port to most systems. However, it has only been tested with Ubuntu 18.04.

### Building for Windows with Visual Studio 2017

Visual Studio 2017 has built in CMake support. For best results, use Visual Studio's latest update.

1. Select File -> Open -> CMake... from the main menu, navigate to the root of your cloned xlang and select the CMakeLists.txt file.
2. Wait a bit for VS to parse the CMake files.
    * the src/CMakeSettings.json file is configured to generate the build files into the _build folder of the repo. Under the _build folder, build files and artifacts are generated under the *Platform*/*Architecture*/*Configuration* subfolder (for example, _build/Windows/x86/Debug)
3. To build, you can do any of the following
    * Select CMake -> Build All to build everything
    * Select CMake -> Build Only -> *Build Target* to build a specific target and its dependencies.
    * Select a target executable from the Startup Item dropdown (with the green arrow) and use Visual Studio's build and debug hotkeys (Ctrl-Shift-B to build, F5 to build and debug, Shift-F5 to build and run)

### Building for Windows on the Command Line

Building xlang (or any CMake based project) from the command line is a two step process - First the build files are generated using [CMake](http://cmake.org/) and then build is execute via [Ninja](http://ninja-build.org). Both of these tools are installed as part of the Visual C++ tools for CMake component.

The [windows/build.cmd](/src/scripts/windows/build.cmd) build script automates the process of running CMake and Ninja. It generates build files and artifacts into the same folder structure Visual Studio uses (described above). You must run build.cmd from a VS 2017 Developer command prompt. Open a Developer Command Prompt, navigate to the root of your cloned xlang repo and execute the following:

``` shell
.\src\scripts\windows\build.cmd
```

build.cmd supports the following command line parameters:

| | | |
|-|-|-|
| -h, --help| shows help method |
| -v, --verbose | shows detailed build output |
| -f, --force-cmake | forces re-run of CMake |
| -b, --build-type **value** | specify build type (Debug, Release, RelWithDebInfo, MinSizeRel). Defaults to Debug. |
| **build target** | specify build target. Defaults to build all |

### Building for Linux on the Command Line

> Note, xlang is a young project. While we have aspirations to support mutiple platforms (detailed in [XDN01](/design_notes/XDN01%20-%20A%20Strategy%20for%20Language%20Interoperability.md)), we are a long way from realizing that dream. In particular, while both the C++ and Python projection tools in the [/tool](/src/tool) directory can be compiled for Windows and Linux, today they both generate Windows specific code. As the xlang [Platform Adaptation Layer (aka PAL)](/src/platform) comes online, we will be updating all of our code to work with it, enabling us to target platforms beyond just Windows.

Building for Linux on the command line is similar to building for Windows on the command line. The [ubuntu/build.sh](/src/scripts/ubuntu/build.sh) script automates the process of running CMake and Ninja to produce a build. Similar to building on Windows, build.sh generates build artifacts into the _build/*Platform*/*Architecture*/*Configuration* folder.

> Note, only x86_64 builds are supported on Linux at this time, so build.sh hard codes this segment of the build path.

Command line arguments for ubuntu/build.sh are  identical to the command line arguments for windows/build.cmd described above.

### Known Issues

The following build failures indicate using outdated tools:

> src\platform\string_base.h(74): error C2327: 'xlang::impl::string_storage_base::alternate_form': is not a type name, static, or enumerator

    Solution: upgrade to Visual Studio 15.8.6 or later

> C:\Program Files (x86)\Windows Kits\10\include\10.0.17134.0\cppwinrt\winrt\base.h(2185): error C3861: 'from_abi': identifier not found

    Solution: upgrade to Windows SDK 17663 or later

## Folder Structure

### /Library

The **/library** folder contains the C++ header libraries provided by xlang for parsing and generating various formats.

* **cmd_reader.h** parses command line arguments.

* **meta_reader.h** parses [ECMA-335 metadata](http://www.ecma-international.org/publications/standards/Ecma-335.htm), similar to [Windows's metadata APIs](http://docs.microsoft.com/en-us/windows/desktop/api/rometadataapi/) but working directly against the metadata tables and written almost entirely in standard C++.

* **task_group.h** executes tasks in parallel for release builds but one at a time in debug builds.

* **text_writer.h** writes formatted text output.

* **xlang.meta.natvis** provides Visual Studio debug visualizations of xlang::meta types.  Recommended install technique is to create a symlink:
  > for /f "tokens=2*" %i in ('reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders" /v Personal ^| findstr Personal') do @for /f "tokens=2" %k in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest ^| findstr catalog_productLineVersion') do @echo %j\Visual Studio %k\Visualizers| for /f "delims=" %l in ('more') do @md "%l" 2>nul & mklink "%l\xlang.meta.natvis" "c:\git\xlang\src\library\xlang.meta.natvis" 2>nul

  See also [Deploying .natvis files](https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects?view=vs-2015#BKMK_natvis_location).

### /Platform

The **/platform** folder contains the declaration and implementations of the common and minimal C API (and platform-specific implementations) used to support xlang on different platforms (otherwise known as the PAL or Platform Adaptation Layer).

Eventually, source in the PAL will be split into separate folders based on underlying platform (Windows, Linux, Android, etc). For now, the PAL only has a Windows implementation.

* **published/pal.h** contains the declaration of the PAL surface area

### /Tool

The **/tool** folder contains the tools provided in support of xlang development. This includes tools for working with idl and winmd files as well as for generating language projections.

* **/tool/cppwinrt** implements the tool that generates the C++ 17 language projection. Currently, the generated projection is Windows only, but the tool will be updated to generate cross-platform compatible code.

* **/tool/natviz** implements a [Natvis visualization](http://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects) of C++/WinRT components for the Visual Studio debugger 2017. Over time, this tool will be updated to support visualization of xlang components.

* **/tool/python** implements the tool that generates the Python language projection. Currently, this tool is in pre-alpha state and builds on C++/WinRT projection from the Windows SDK rather than xlang. The tool will be updated to generate cross-platform compatible code.

### /Test

The **/test** folder contains the unit tests for testing the libraries, platforms, and projections.

### /Scripts

The **/script** folder contains the scripts and tools used to build and bootstrap the various projects and language projections.

* **windows/build.cmd** builds xlang for Windows. Note, this must be run from a VS 2017 command prompt.

* **ubuntu/build.sh** bash script to build xlang for Ubuntu 18.04. Works on both native Ubuntu and in Windows Subsystem for Linux.

* **ubuntu/configure.sh** bash script to configure Ubuntu 18.04 with the tools needed for xlang.

* **scorch.cmake** cmake function to delete build artifacts not automatically removed by the clean target.

* **genStringLiteralFiles.cmake** cmake function to generate C++ string literals from files.

### /Package

The **/package** folder contains projects for packaging reusable xlang artifacts.  Some of these, such as the C++/WinRT NuGet package and Visual Studio extension, are regularly built, signed, and published by Microsoft.
