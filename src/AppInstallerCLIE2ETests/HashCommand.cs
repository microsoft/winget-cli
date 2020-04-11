// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class HashCommand
    {
        [Test]
        public void HashCommands()
        {
            // Hash a file
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTestExeInstaller.exe"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("7c616e2fe3853b555ddea4cb50bd307da1dc47898a5fbc3a8646a470107802ab"));

            // Hash msix
            result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTestMsixInstaller.msix" + " -m"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("714c861a52478bfa44228739c500c9429f4e75c7b78d48cac06bfb8f62bad627"));
            Assert.True(result.StdOut.Contains("d8a54b79a9c5956df5b338fdaf37f48e56316465defd182cbe64d8e5d3d53d4e"));

            // The input is not msix but -m is used
            result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTestExeInstaller.exe" + " -m"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.OPC_E_ZIP_MISSING_END_OF_CENTRAL_DIRECTORY);
            Assert.True(result.StdOut.Contains("7c616e2fe3853b555ddea4cb50bd307da1dc47898a5fbc3a8646a470107802ab"));
            Assert.True(result.StdOut.Contains("Please verify that the input file is a valid, signed MSIX."));

            // Input file not found
            result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("DoesNot.Exist"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_FILE_NOT_FOUND);
            Assert.True(result.StdOut.Contains("File does not exist"));
        }
    }
}