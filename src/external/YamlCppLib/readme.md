## YamlCppLib

Do not change code under the libyaml directory; it contains libyaml source code version [0.2.5](https://github.com/yaml/libyaml/releases/tag/0.2.5).  
It is created using git subtree command:

    git subtree add --prefix=src/YamlCppLib/libyaml https://github.com/yaml/libyaml.git release/0.2.5 --squash

The YamlCppLib.vcxproj and YamlCppLib.vcxproj.filters are created to make libyaml code compiled as part of the WinGet solution.

#### Steps used to create the VS project files.

1. VS Create project from existing code wizard to point to libyaml directory. Use static library template.
2. Removed code not needed by WinGet project. Basically keeping only files under src and include.
3. Modify the created vcxproj file to follow settings from other project files in the WinGet solution.
4. Configure the additional include path to include the libyaml\include directory.
5. Copy libyaml\cmake\config.h.in and process it as cmake would have.
6. Add definitions "/D YAML_DECLARE_STATIC /D HAVE_CONFIG_H".