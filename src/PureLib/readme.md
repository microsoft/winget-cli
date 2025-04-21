## PureLib

Do not change code under the pure directory; it contains pure source code version [1.0.4](https://github.com/ronomon/pure/releases/tag/v1.0.4).  
It is created using git subtree command:

    git subtree add --prefix=src/PureLib/pure https://github.com/ronomon/pure.git v1.0.4 --squash

The PureLib.vcxproj and PureLib.vcxproj.filters are created to make pure code compiled as part of the WinGet solution.

#### Steps used to create the VS project files.

1. VS Create project from existing code wizard to create a Shared Items template.
2. Remove code not needed by WinGet project. Basically keeping only the top level headers.
3. Add the PureLib headers to the shared items.
4.  Modify the created vcxitems file to add the PureLib directory to the include path.

> **When committing the PR, DO NOT squash it.  The two commits are needed as is to allow for future subtree pulls.**<br>