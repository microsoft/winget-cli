// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ValidateCommand : BaseCommand
    {
        [Test] 
        public void ValidateManifest()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestValidManifest.yaml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
        }

        [Test]
        public void ValidateManifestWithExtendedCharacter()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\T�stExeInstaller.yaml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
        }

        [Test]
        public void ValidateInvalidManifest()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestInvalidManifest.yaml"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_FAILURE, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation failed."));
        }

        [Test]
        public void ValidateManifestDoesNotExist()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\DoesNotExist"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_PATH_NOT_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Path does not exist"));
        }
    }
}