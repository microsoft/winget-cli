Do not change code under yaml-cpp directory. yaml-cpp directory contains yaml-cpp source code from https://github.com/jbeder/yaml-cpp.
It is created using git subtree command.

The YamlCppLib.vcxproj and YamlCppLib.vcxproj.filters are created to make yaml-cpp code compiled as part of the AppInstallerClient solution.

Steps used to generate the VS project files.
1. VS Create project from existing code wizard to pint to yaml-cpp directory. Use static library template.
2. Removed code not needed by AppInstallerClient project. Basically keeping only files under src and include.
3. Modify the created vcxproj file to follow settings from other project files in the AppInstallerClient solution.
4. Configure the additional include path to include the yaml-cpp\include directory.


