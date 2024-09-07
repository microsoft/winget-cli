// -----------------------------------------------------------------------------
// <copyright file="HashCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;
    using NUnit.Framework.Internal;

    /// <summary>
    /// Test hash command.
    /// </summary>
    public class HashCommand : BaseCommand
    {
        /// <summary>
        /// Test hash file.
        /// </summary>
        [Test]
        public void HashFile()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTestMsiInstaller.msi"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("21d90ee9b3569590c624836ef50bf39791c7184869c227eedc00585e1f39b4de"));
        }

        /// <summary>
        /// Test hash msix.
        /// </summary>
        [Test]
        public void HashMSIX()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile(Constants.TestPackage) + " -m");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("08917b781939a7796746b5e2349e1f1d83b6c15599b60cd3f62816f15e565fc4"));
            Assert.True(result.StdOut.Contains("223b318c4b1154a1fb72b1bc23422810faa5ce899a8e774ba2a02834b2058f00"));
        }

        /// <summary>
        /// Test hash invalid msix.
        /// </summary>
        [Test]
        public void HashInvalidMSIX()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("AppInstallerTestMsiInstaller.msi") + " -m");
            Assert.AreEqual(Constants.ErrorCode.OPC_E_ZIP_MISSING_END_OF_CENTRAL_DIRECTORY, result.ExitCode);
            Assert.True(result.StdOut.Contains("21d90ee9b3569590c624836ef50bf39791c7184869c227eedc00585e1f39b4de"));
            Assert.True(result.StdOut.Contains("Please verify that the input file is a valid, signed MSIX."));
        }

        /// <summary>
        /// Test hash file not found.
        /// </summary>
        [Test]
        public void HashFileNotFound()
        {
            var result = TestCommon.RunAICLICommand("hash", TestCommon.GetTestDataFile("DoesNot.Exist"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_FILE_NOT_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("File does not exist"));
        }
    }
}