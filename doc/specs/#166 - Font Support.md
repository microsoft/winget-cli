---
author: Ryan Fu @ryfu-msft
created on: 2024-9-18
last updated: 2024-9-18
issue id: 166
---

# Support for installation of fonts

For [#66](https://github.com/microsoft/winget-cli/issues/166)

## Abstract
This spec outlines the design for supporting the installation of fonts.

## Context

### Font File Types

This feature will support font files with the following file types:
- .ttf (True Type File) - Most common format
- .ttc (True Type Collection)
- .otf (Open Type Font)

The following font types to my knowledge are not supported:
- .woff & .woff2 (developed by Google for the modern browser)
- .eot (Embedded OpenType font file) Not used because of security issues.

### Installing Fonts
Typically, a user would download the font file and drag it to the `C:\Windows\Fonts` directory in the explorer view, but there are a couple things happening behind the scenes.

Depending on the scope of the user, the font will be stored in two locations:
- `C:\Windows\Fonts` (Machine )
- `%LOCALAPPDATA%\Microsoft\Windows\Fonts` (User Mode)

In addition, a new registry entry is created in the following registry paths:
The name of the registry key typically matches the title of the font file, and the value is set to the path of the font file.

#### Local Machine Registry
---

`Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts`

| Name    | Type | Value |
| -------- | ------- | ------- |
| Calibri Bold Italic (TrueType)  | REG_SZ    | calibriz.ttf |

> Note that for the LOCAL_MACHINE registry entry, only the file name is specified, not the full path.

#### Current User Registry
---

 `Computer\HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts`

| Name    | Type | Value |
| -------- | ------- | ------- |
| Roboto Bold Italic (TrueType)  | REG_SZ    | C:\Users\ryfu\AppData\Local\Microsoft\Windows\Fonts\Roboto-BoldItalic.ttf |


### Uninstalling Fonts
To remove a font, the inverse operation of the steps above need to be completed. 
1. Remove the registry key entry for that specific font file
2. Remove the file from the font directory


## Manifest Changes:
- Addition of `font` to `installerType` and `nestedInstallerType`
- Addition of `FontFaceName`. If `font` is specified for either the `installerType` and `nestedInstallerType`, then a `fontFaceName` is required. Each nested font must have a `fontFaceName` specified.

Manifest Example:
```yaml
PackageIdentifier: Microsoft.CascadiaCode
PackageName: Cascadia
PackageVersion: 2404.23
Installers:
- InstallerType: zip
  InstallerUrl: https://github.com/microsoft/cascadia-code/releases/download/v2404.23/CascadiaCode-2404.23.zip
  InstallerSha256: a911410626c0e09d03fa3fdda827188fda96607df50fecc3c5fee5906e33251b
  NestedInstallerFiles:
  - RelativeFilePath: ttf/CascadiaCodeItalic.ttf
  - RelativeFilePath: ttf/CascadiaMonoNFItalic.ttf
ManifestType: installer
ManifestVersion: 1.9.0
```

A new `PredefinedSource` will be created for `Fonts`.

`PredefinedInstalledFontFactory` will be defined that creates a SQLite index from the installed font information.

The table below shows how a font will be recorded in the index and highlights some of the similaries to an MSIX package.

| Property | Package | Font |
| -------- | ------- | ---- |
| Id    |  MSIX\NotepadPlusPlus_1.0.0.0_neutral__7njy0v32s6xk6    |  Machine\Arial\Bold  |
| Version    |  1.0.0.0  |  7.01  |
| Name    |  NotepadPlusPlus   |  Arial Bold  |
| PackageFamilyName    |  NotepadPlusPlus_1.0.0.0_neutral__7njy0v32s6xk6    |  Arial  |
| InstalledLocation    |   N/A    | %LOCALAPPDATA%/Microsoft/Windows/Fonts/Arial.ttf |
| InstalledScope    |       |   User |



> The unique identifer for each font will be a combination of the scope, font font family name, and font face name.

> The version of the font only exists at the font face level and not at the family level

## CLI Behavior / Implementation Details

New Command: `winget fonts`
---
Fonts should be completely separate from the existing package management experience. Fonts aren't as complex as installing packages so having new subcommands can help simplify the necessary arguments for proper functionality. To support this, a new command called `font` will be added. The following subcommands will be available.
- `winget fonts list`
- `winget fonts install`
- `winget fonts uninstall`
- `winget fonts upgrade`
- `winget fonts show`

`winget fonts list`
---
The default behavior of `winget fonts list` is to display all installed font families and the number of faces that belong to each font family.

| Family Name | Face Count |
| -------- | ------- |
| Arial    |   6    |
| Bahnschrift | 1   |
| Cascadia Code | 30 |

The code snippet below shows how to enumerate the list of installed font family names.
```c++
    wchar_t localeNameBuffer[LOCALE_NAME_MAX_LENGTH];
    const auto localeName = GetUserDefaultLocaleName(localeNameBuffer, LOCALE_NAME_MAX_LENGTH) ? localeNameBuffer : L"en-US";

    wil::com_ptr<IDWriteFactory7> factory;
    THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(factory), factory.put_unknown()));

    wil::com_ptr<IDWriteFontCollection> collection;
    THROW_IF_FAILED(factory->GetSystemFontCollection(collection.addressof(), FALSE));

    const auto familyCount = collection->GetFontFamilyCount();

    for (UINT32 familyIndex = 0; familyIndex < familyCount; familyIndex++)
    {
        // Get font family from font collection by index.
        wil::com_ptr<IDWriteFontFamily> family;
        THROW_IF_FAILED(collection->GetFontFamily(familyIndex, family.addressof()));

        // Get font family name.
        wil::com_ptr<IDWriteLocalizedStrings> names;
        THROW_IF_FAILED(family->GetFamilyNames(names.addressof()));

        UINT32 index;
        BOOL exists;
        if (FAILED(names->FindLocaleName(localeName, &index, &exists)) || !exists)
        {
            index = 0;
        }

        UINT32 nameLength;
        THROW_IF_FAILED(names->GetStringLength(index, &nameLength));
        nameLength += 1; // Account for the trailing null terminator during allocation.

        wchar_t name[512];
        const auto fontCount = family->GetFontCount();
        THROW_HR_IF(E_OUTOFMEMORY, nameLength > ARRAYSIZE(name));
        THROW_IF_FAILED(names->GetString(0, &name[0], nameLength));
    }
```

If a user specifies a specific name such as : 

`winget fonts list --name 'Arial'`

Then the tool will display all of the font faces related to that font family name along with the corresponding font file path.

| Face Name | Face Version | Font Family | Font File Path |
| -------- | ------- | ------- | ------- |
| Regular    | 7.01 |  Arial    | C:\Windows\Fonts\arial.ttf |
| Narrow | 7.01 | Arial   | C:\Windows\Fonts\ARIALN.TTF |
| Italic | 7.01 | Arial | C:\Windows\Fonts\ariali.ttf |
| Narrow Italic | 7.01 | Arial | C:\Windows\Fonts\ARIALNI.TTF |
| Narrow Bold | 7.01 | Arial | C:\Windows\Fonts\ARIALNB.TTF |
| Narrow Bold Oblique | 7.01 | Arial | C:\Windows\Fonts\ARIALNB.TTF |

> Note that some font faces share the same font file path. This will need to be handled properly during uninstall.

Sample Code:

```c++
        // Create a single font face from a family.
        wil::com_ptr<IDWriteFontFace> fontFace;
        THROW_IF_FAILED(font->CreateFontFace(fontFace.addressof()));

        // https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritefontface-getfiles
        UINT32 fileCount;
        THROW_IF_FAILED(fontFace->GetFiles(&fileCount, nullptr)); // Use null to only retrieve the number of files.

        wil::com_ptr<IDWriteFontFile> fontFiles[8];
        THROW_HR_IF(E_OUTOFMEMORY, fileCount > ARRAYSIZE(fontFiles));
        THROW_IF_FAILED(fontFace->GetFiles(&fileCount, fontFiles[0].addressof()));

        // Iterate through all the files corresponding to that font face name.
        for (UINT32 i = 0; i < fileCount; ++i) {
            wil::com_ptr<IDWriteFontFileLoader> loader;
            THROW_IF_FAILED(fontFiles[i]->GetLoader(loader.addressof()));

            const void* fontFileReferenceKey;
            UINT32 fontFileReferenceKeySize;
            THROW_IF_FAILED(fontFiles[i]->GetReferenceKey(&fontFileReferenceKey, &fontFileReferenceKeySize));

            // Retrieve and print the full font file path.
            if (const auto localLoader = loader.try_query<IDWriteLocalFontFileLoader>()) {
                UINT32 pathLength;
                THROW_IF_FAILED(localLoader->GetFilePathLengthFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &pathLength));
                pathLength += 1; // Account for the trailing null terminator during allocation.

                wchar_t path[512];
                THROW_HR_IF(E_OUTOFMEMORY, pathLength > ARRAYSIZE(path));
                THROW_IF_FAILED(localLoader->GetFilePathFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &path[0], pathLength));

                // Normalize path for better readability
                // Use backup semantics so we can access protected files.
                const auto h = CreateFileW(path, 0, 0, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                GetFinalPathNameByHandleW(h, path, ARRAYSIZE(path), 0);
                CloseHandle(h);

                wprintf(L"%s\n", &path[0]); 
            }
        }
```

`winget fonts install`
---

#### 1. Initial Validation
Winget will check that `effectiveInstallerType == font` before kicking off the font installation flow.

The flow will start by validating the font file using the [IDWriteFontFile::Analyze](https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritefontfile-analyze) method. The `isValid` boolean value will be checked first to determine if we can support installing this font file. If it is not valid, the workflow will terminate and an error message will be displayed to the user.

```C++
    BOOL isValid;
    DWRITE_FONT_FILE_TYPE fileType;
    DWRITE_FONT_FACE_TYPE faceType;
    UINT32 numOfFaces;
    THROW_IF_FAILED(localFontFile->Analyze(&isValid, &fileType, &faceType, &numOfFaces));
```

#### 2. Determining desired state
Based on the `--scope` argument, this will be used to determine where the font file will be copied to, and where we will create a new registry key.

User mode:

Font path: `%LOCALAPPDATA%/Microsoft/Windows/Fonts/fontName.ttf`

Registry Entry: `[Font Name (True Type)] : [Full font path]`

> Nearly all of the font types are .ttf and append the `(True Type)` font type to the end of the font name. We will follow the same pattern if the font type is True Type.

Machine scope:
Font path: Use the known folder id for the fonts directory: `CSIDL_FONTS`. The typical path is `C:\Windows\Fonts`.

Registry Entry: `[Font Name (True Type)] : [font file name]`

For all the entries above, we will check whether those files/entries already exist and return an error to the user. The user can bypass these checks by including the `--force argument` which will overwrite these files/entries.

#### 3. Applying desired state
Once all checks have been verified, we will apply the desired state so that the font file will exist in the appropriate location and a new registry entry will be created.

`winget fonts uninstall`
---

1. Determine which font file(s) matches the font family/face name

Utilizing the enumeration code from above, we will determine which file(s) corresponds to the specified font family. During enumeration, it is possible to have duplicate entries for the same font family name if the font is installed in both user and system scope. The user will need to provide the `--scope` argument to filter down to exactly one font family. 

`std::vector<std::filesystem::path> GetFontFilePathsByName(familyName, faceName (optional))`


2. Check the registry values for matching font path

We will check the corresponding registry for that file path. To improve performance, we will create a heuristic to determine the key name of the font by combining the font name and appending `(True Type)` if the file is indeed a true type font file. Most fonts are registered in this format so this is aims to optimize for the most common scenario.

Our fallback method will be to iterate through all font registry keys to create a map of all registered fonts and font files and determine if there is key value that matches each font path. 

There is no guarantee that the key name must match the intended name of the font. The registry key name has no impact on how the font is registered by the OS. Because of this, the only guaranteed solution is to scan through all font registry entries and look for a single match.

3. Remove the file and the registry entry. 
Once we have determined the matching font file and registry key entry, both of these items will be removed from the system.


### Uninstall Scenarios:
---

1. **Uninstalling a font family with a single font face:**

`winget fonts uninstall --name 'Baskerville Old Face'`

Winget will uninstall a font family with a single font face without any warnings.

2. **Uninstalling a font family with multiple font faces:**

`winget fonts uninstall --name 'Bahnschrift' --remove-multiple-fonts`

If a font family contains multiple font faces, the user must include the `--remove-multiple-fonts`. Otherwise, a blocking warning will be shown to the user.

3. **Uninstalling a specific font face:**

`winget fonts uninstall --name 'Bahnschrift' --font-face 'Semibold'`

4. **Uninstalling a font face that shares the same font file as another font face:**

`winget fonts uninstall --name 'Arial' --font-face 'Bold'`

If a specific font face that is being removed shares a font file with another font face, an error will be shown to the user and the workflow will be terminated.

The user must include the `--force` argument to remove both files. This means that the registry entry from the other font face must be removed as well. The client should notify the user, which font faces are removed.

`winget fonts upgrade`
---
Identifying the version only applies at the  font face level.

Retrieving the version of the font face:
```c++
       wil::com_ptr<IDWriteLocalizedStrings> versionString;
       BOOL versionStringExists;
       THROW_IF_FAILED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_VERSION_STRINGS, &versionString, &versionStringExists));
       UINT32 versionStringIndex;
       if (FAILED(versionString->FindLocaleName(localeName, &versionStringIndex, &versionStringExists)) || !versionStringExists) {
           versionStringIndex = 0;
       }

       UINT32 versionStringLength = 0;
       THROW_IF_FAILED(versionString->GetStringLength(versionStringIndex, &versionStringLength));
       versionStringLength += 1; // Account for the trailing null terminator during allocation.

       wchar_t result[512];
       THROW_HR_IF(E_OUTOFMEMORY, versionStringLength > ARRAYSIZE(result));
       THROW_IF_FAILED(versionString->GetString(versionStringIndex, &result[0], versionStringLength));

       wprintf(L"%s\n", &result[0]);
```

