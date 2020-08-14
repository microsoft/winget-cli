// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ValidateCommand
    {
        [Test] 
        public void ValidManifest()
        {
            // Validate a good yaml
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestExeInstaller.yaml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
        }

        [Test]
        public void ValidManifestWithExtendedCharacter()
        {
            // Validate a good yaml with extended character
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TëstExeInstaller.yaml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
        }

        [Test]
        public void InvalidManifest()
        {
            // Validate invalid yaml
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestInvalidManifest.yaml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation failed."));
        }

        [Test]
        public void ManifestDoesNotExist()
        {
            // Input file not found
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\DoesNotExist"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_FILE_NOT_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("File does not exist"));
        }
    }
}