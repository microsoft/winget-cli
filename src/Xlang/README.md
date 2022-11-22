## Microsoft/Xlang/UndockedRegFreeWinRT

Do not change code under the UndockedRegFreeWinRT directory; it contains Microsoft/Xlang/UndockedRegFreeWinRT source code. 

It is created using git subtree command and points to this specific commit [cfe510d0d2b07484fea2c6d77163de017738c100]:

    git subtree add --prefix=src/Xlang/UndockedRegFreeWinRT https://github.com/microsoft/xlang cfe510d0d2b07484fea2c6d77163de017738c100 --squash

Any future PR updates to this subtree should not be squashed. 

The default project files are used to make the UndockedRegFreeWinRT project compile as part of the WinGet solution:

    "UndockedRegFreeWinRT\UndockedRegFreeWinRT\UndockedRegFreeWinRT.vcxproj" 
    "UndockedRegFreeWinRT\UndockedRegFreeWinRT\UndockedRegFreeWinRT.vcxproj.filters" 

#### Steps used to create the VS project files.

1. Add UndockedRegFreeWinRT project files to WinGet solution.
2. Remove code and files not needed by WinGet project. Basically removing everything except what is contained in UndockedRegFreeWinRT folder.
3. Modify the vcxproj file to follow settings from other project files in the WinGet solution.