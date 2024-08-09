## src/PureLib/pure/zlib

Do not change code under the zlib directory; it contains madler/zlib source code. 

It is created using git subtree command and points to this specific commit [51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf]:

    git subtree add --prefix=src/PureLib/pure/zlib https://github.com/madler/zlib.git 51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf --squash

PureLib already has existing project files that compiles as part of the WinGet solution. Since PureLib has a dependency on the zlib library, the git subtree command is needed to keep the zlib library updated. Current zlib version: [1.3.1]