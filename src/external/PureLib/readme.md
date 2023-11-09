## PureLib

Do not change code under the pure directory; it contains pure source code version [1.0.4](https://github.com/ronomon/pure/releases/tag/v1.0.4).  
It is created using git subtree command:

    git subtree add --prefix=src/PureLib/pure https://github.com/ronomon/pure.git v1.0.4 --squash

The PureLib.vcxproj and PureLib.vcxproj.filters are created to make pure code compiled as part of the WinGet solution.

#### Steps used to create the VS project files.

1. VS Create project from existing code wizard to point to PureLib directory. Use static library template.
2. Removed code not needed by WinGet project. Basically keeping only files under src and include.
3. Modify the created vcxproj file to follow settings from other project files in the WinGet solution.
4. Configure the additional include path to include the pure directory.