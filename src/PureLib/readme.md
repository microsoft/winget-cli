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

## zlib
Original source: https://github.com/madler/zlib
Current commit: [Commit 51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf](https://github.com/madler/zlib/commit/51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf)

Pure utilizes `zlib` for scanning zip files. To update this library, run the following command, then update the above commit for reference.  'develop' can be replaced with the appropriate commit spec as desired.

```
    git subtree pull -P src/PureLib/pure/zlib https://github.com/madler/zlib develop --squash
```
> **When committing the PR, DO NOT squash it.  The two commits are needed as is to allow for future subtree pulls.**<br>