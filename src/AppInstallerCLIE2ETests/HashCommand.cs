// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using NUnit.Framework.Internal;

    public class HashCommand : BaseCommand
    {
        [Test]
        public void HashFile()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTest.cer"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("9b4c49ad7e47afd97d2e666e93347745e1647c55f1a7ebba6d31b7dd5f69ee68"));
        }

        [Test]
        public void HashMSIX()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile(Constants.TestPackage) + " -m");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("08917b781939a7796746b5e2349e1f1d83b6c15599b60cd3f62816f15e565fc4"));
            Assert.True(result.StdOut.Contains("223b318c4b1154a1fb72b1bc23422810faa5ce899a8e774ba2a02834b2058f00"));
        }

        [Test]
        public void HashInvalidMSIX()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTest.cer") + " -m");
            Assert.AreEqual(Constants.ErrorCode.OPC_E_ZIP_MISSING_END_OF_CENTRAL_DIRECTORY, result.ExitCode);
            Assert.True(result.StdOut.Contains("9b4c49ad7e47afd97d2e666e93347745e1647c55f1a7ebba6d31b7dd5f69ee68"));
            Assert.True(result.StdOut.Contains("Please verify that the input file is a valid, signed MSIX."));
        }

        [Test]
        public void HashFileNotFound()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("DoesNot.Exist"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_FILE_NOT_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("File does not exist"));
        }
    }
}