## src/PureLib/pure/zlib

Do not change code under the zlib directory; it contains madler/zlib source code. 

It is created using git subtree command and points to this specific commit [04f42ceca40f73e2978b50e93806c2a18c1281fc]:

    git subtree add --prefix=src/PureLib/pure/zlib https://github.com/madler/zlib.git 04f42ceca40f73e2978b50e93806c2a18c1281fc --squash

PureLib already has existing project files that compiles as part of the WinGet solution. Since PureLib has a dependency on the zlib library, the git subtree command is needed to keep the zlib library updated. Current zlib version: [1.2.13]