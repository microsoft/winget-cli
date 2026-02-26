// -----------------------------------------------------------------------------
// <copyright file="ErrorCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test error command.
    /// </summary>
    public class ErrorCommand
    {
        /// <summary>
        /// Reset settings file to avoid affecting output from error command.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.InitializeWingetSettings();
        }

        /// <summary>
        /// Tests 0.
        /// </summary>
        [Test]
        public void Success()
        {
            var result = TestCommon.RunAICLICommand("error", "0");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x00000000"));
        }

        /// <summary>
        /// Tests 0x8a15c001.
        /// </summary>
        [Test]
        public void HexError()
        {
            var result = TestCommon.RunAICLICommand("error", "0x8a15c001");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x8a15c001"));
            Assert.True(result.StdOut.Contains("WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE"));
        }

        /// <summary>
        /// Tests a number larger than an HRESULT.
        /// </summary>
        [Test]
        public void HexErrorTooBig()
        {
            var result = TestCommon.RunAICLICommand("error", "0x8a15c0014");
            Assert.AreEqual(Constants.ErrorCode.E_INVALIDARG, result.ExitCode);
            Assert.True(result.StdOut.Contains("The given number is too large to be an HRESULT."));
        }

        /// <summary>
        /// Tests 2316681217.
        /// </summary>
        [Test]
        public void DecimalError()
        {
            var result = TestCommon.RunAICLICommand("error", "2316681217");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x8a15c001"));
            Assert.True(result.StdOut.Contains("WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE"));
        }

        /// <summary>
        /// Tests -1978335191.
        /// </summary>
        [Test]
        public void NegativeDecimalError()
        {
            var result = TestCommon.RunAICLICommand("error", "-- -1978335191");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x8a150029"));
            Assert.True(result.StdOut.Contains("APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_FAILURE"));
        }

        /// <summary>
        /// Tests 0x8a15c000.
        /// </summary>
        [Test]
        public void HexErrorNotFound()
        {
            var result = TestCommon.RunAICLICommand("error", "0x8a15c000");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x8a15c000"));
            Assert.True(result.StdOut.Contains("Unknown error code"));
        }

        /// <summary>
        /// Tests 0xA150202.
        /// </summary>
        [Test]
        public void NonError()
        {
            var result = TestCommon.RunAICLICommand("error", "0xA150202");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x0a150202"));
            Assert.True(result.StdOut.Contains("WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE"));
        }

        /// <summary>
        /// Tests WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE.
        /// </summary>
        [Test]
        public void Symbol()
        {
            var result = TestCommon.RunAICLICommand("error", "WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x8a15c001"));
            Assert.True(result.StdOut.Contains("WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE"));
            Assert.AreEqual(2, result.StdOut.Split('\n', System.StringSplitOptions.RemoveEmptyEntries).Length);
        }

        /// <summary>
        /// Tests config.
        /// </summary>
        [Test]
        public void String()
        {
            var result = TestCommon.RunAICLICommand("error", "config");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("0x8a15c001"));
            Assert.True(result.StdOut.Contains("WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE"));
            Assert.True(result.StdOut.Contains("0x8a15c110"));
            Assert.True(result.StdOut.Contains("WINGET_CONFIG_ERROR_UNIT_SETTING_CONFIG_ROOT"));

            // This contains config in it's message.
            Assert.True(result.StdOut.Contains("0x8a150038"));
            Assert.True(result.StdOut.Contains("APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE"));
        }
    }
}
