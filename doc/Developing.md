# Developer guidance

## Prerequisites

* Windows 10 1809 (17763) or later
* [Developer Mode enabled](https://docs.microsoft.com/windows/uwp/get-started/enable-your-device-for-development)
* [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)
  * Or use WinGet to install it ;) (although you may need to adjust the workloads via Tools->Get Tools and Features...)
* The following workloads:
  * .NET Desktop Development
  * Desktop Development with C++
  * Universal Windows Platform Development
  * Check [.vsconfig file](../.vsconfig) for full components list

* [Windows SDK for Windows 11 (10.0.26100)](https://developer.microsoft.com/en-us/windows/downloads/sdk-archive/)

> [!NOTE]
> You can also get it through `winget install Microsoft.WindowsSDK.10.0.26100` or via Visual Studio > Get Tools and Features > Individual Components > Windows 10 SDK (10.0.26100.0)

* The following extensions:

  * [Microsoft Visual Studio Installer Projects](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022InstallerProjects)

## Building the client

1. Clone the repository
2. Configure your system using the [configuration file](../.config/configuration.winget) in the repository. Run one of the following configurations from the project root so relative paths resolve correctly:
   - For VS Community: `winget configure .config/configuration.winget`
   - For VS Professional: `winget configure .config/configuration.vsProfessional.winget`
   - For VS Enterprise: `winget configure .config/configuration.vsEnterprise.winget`
3. Run `vcpkg integrate install` from the Developer Command Prompt / Developer PowerShell for VS 2022. This is a one-time setup step until the configuration file in step 2 is updated to work with vcpkg setup.

Open `winget-cli\src\AppInstallerCLI.sln` in Visual Studio and build. We currently only build using the solution; command-line methods of building a VS solution should work as well.

## Running and Debugging

After the build finishes, deploy the solution from Build > Deploy Solution. You can then run the client from the command line using `wingetdev`.

To enable step-through debugging, right click on `AppInstallerCLIPackage` in the Solution Explorer, select Properties, and navigate to the Debug tab. In the Debugger type selection, change "Application process" and "Background task process" to "Native Only". This will allow you to add breakpoints and step through the code. The main entry point for the client is in `src/AppInstallerCLI/main.cpp`

The best way to debug the client is to select `Do not launch, but debug my code when it starts` in the `Debug` tab and start the debugging session with <kbd>F5</kbd>. You can then use the `wingetdev` command in a terminal session, or any PowerShell code for COM API interaction, which will get picked up by the debugger.

## Running Unit Tests

The unit tests are located inside the `AppInstallerCLITests` project. When the solution is built, all tests are compiled under `src/<ARCHITECTURE>/<Debug|Release>/AppInstallerCLITests`. An executable `AppInstallerCLITests.exe` is generated in this directory to run the tests. Run `AppInstallerCLITests.exe` from the command line to execute the tests. To see all available options, run `AppInstallerCLITests.exe --help`.

> [!TIP]
> If you just want to run a particular test, you can specify the test name as an argument to the executable. For example, `AppInstallerCLITests.exe EnsureSortedErrorList`.

## Localization

The English resource strings are the source of truth and live in:

- `src/AppInstallerCLIPackage/Shared/Strings/en-us/winget.resw`
- `src/AppInstallerCLIPackage/Shared/Strings/en-us/Resources.resw`

The localized strings under `Localization/Resources/<locale>/` are owned by Microsoft's internal localization team and are fetched automatically. **Do not edit files in `Localization/Resources/`**—any changes will be overwritten.

### Adding or modifying resource strings

When adding a new string or modifying an existing one in the English `.resw` files, always include a `<comment>` element that gives translators enough context to produce a correct translation. This is especially important for:

- **Short or single-word values** (e.g., column headers, labels, status words) where the word has multiple meanings in English. Always clarify which meaning applies and, where relevant, explicitly call out meanings that do **not** apply.
- **Technical jargon** that may have a different colloquial meaning in other languages.
- **Strings with placeholders** — document what each `{0}`, `{1}`, etc. represents.

Example of a well-formed comment for a potentially ambiguous word:

```xml
<data name="SourceListExplicit" xml:space="preserve">
  <value>Explicit</value>
  <comment>Column header meaning the source must be directly named to be used. Do NOT translate as "explicit content" or "adult content".</comment>
</data>
```

Without such comments, translators may rely on the most common meaning of a word, which can produce incorrect results in UI-visible strings.
