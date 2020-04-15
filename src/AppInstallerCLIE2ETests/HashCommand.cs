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
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTest.cer"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("9b4c49ad7e47afd97d2e666e93347745e1647c55f1a7ebba6d31b7dd5f69ee68"));

            // Hash msix
            result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("Microsoft.VCLibs.x86.14.00.Desktop.appx" + " -m"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("e1c21e8bd053ec28881dff284f611221c1adb139daba39aaa719d76e53ccdf8d"));
            Assert.True(result.StdOut.Contains("779a33f6ea5a39ac57bd6ba4ef805a8b751b63bd63d1b0df91f8ebee20e2395a"));

            // The input is not msix but -m is used
            result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTest.cer" + " -m"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.OPC_E_ZIP_MISSING_END_OF_CENTRAL_DIRECTORY);
            Assert.True(result.StdOut.Contains("9b4c49ad7e47afd97d2e666e93347745e1647c55f1a7ebba6d31b7dd5f69ee68"));
            Assert.True(result.StdOut.Contains("Please verify that the input file is a valid, signed MSIX."));

            // Input file not found
            result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("DoesNot.Exist"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_FILE_NOT_FOUND);
            Assert.True(result.StdOut.Contains("File does not exist"));
        }
    }
}