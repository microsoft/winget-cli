## YamlCppLib

Do not change code under yaml-cpp directory. yaml-cpp directory contains yaml-cpp source code version [0.6.3](https://github.com/jbeder/yaml-cpp/tree/yaml-cpp-0.6.3).  
It is created using git subtree command:

    git subtree add --prefix=src/YamlCppLib/yaml-cpp https://github.com/jbeder/yaml-cpp.git yaml-cpp-0.6.3 --squash

The YamlCppLib.vcxproj and YamlCppLib.vcxproj.filters are created to make yaml-cpp code compiled as part of the AppInstallerClient solution.

#### Steps used to create the VS project files.

1. VS Create project from existing code wizard to pint to yaml-cpp directory. Use static library template.
2. Removed code not needed by AppInstallerClient project. Basically keeping only files under src and include.
3. Modify the created vcxproj file to follow settings from other project files in the AppInstallerClient solution.
4. Configure the additional include path to include the yaml-cpp\include directory.


