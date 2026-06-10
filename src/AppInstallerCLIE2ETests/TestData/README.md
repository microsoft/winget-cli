## AppInstallerTestMsiInstaller.msi
Due to MSI projects requiring a special extension, we have simply checked in a built version of the test MSI.  To create a new one, which may be needed if the test EXE inside it needs to be updated:

1. Ensure that the extension is installed locally

    a. [Microsoft Visual Studio Installer Projects 2022](https://marketplace.visualstudio.com/items?itemName=VisualStudioClient.MicrosoftVisualStudio2022InstallerProjects)
2. Set configuration to "Release|x86"
3. Build AppInstallerTestMsiInstaller project
4. Check in new MSI over the one in TestData