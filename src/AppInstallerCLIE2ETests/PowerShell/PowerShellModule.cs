// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class PowerShellModule
    {
        // TODO: change this to either be an input or dynamically generated path.
        private const string moduleManifestPath = @"C:\Users\ryfu\git\ryfu-msft\winget-cli\src\x64\Debug\PowerShell\Microsoft.WinGet.Client.psd1";

        [SetUp]
        public void Setup()
        {
        }

        // Repeat e2e tests for all commandlets for a basic sanity check.

        [Test]
        public void FindWinGetPackage()
        {
            var result = TestCommon.RunCommandWithResult("pwsh", $"-Command ipmo {moduleManifestPath}; Find-WinGetPackage -Id Microsoft.WingetCreate");
            Assert.IsTrue(result.ExitCode == 0, "Powershell commandlet should succeed in admin mode.");
        }
    }
}