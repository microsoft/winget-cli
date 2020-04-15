// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ValidateCommand
    {
        [SetUp]
        public void Setup()
        {
            // There's a deployment bug that if the last optional package is removed, the main package will also be removed.
            Assert.True(TestCommon.InstallMsix(TestCommon.GetTestFile(Constants.AICLIPackageFile)));
        }

        [Test]
        public void ValidateCommands()
        {
            // Validate a good yaml
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestExeInstaller.yaml"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));

            // Validate invalid yaml
            result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestInvalidManifest.yaml"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Manifest validation failed."));

            // Input file not found
            result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\DoesNotExist"));
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_FILE_NOT_FOUND);
            Assert.True(result.StdOut.Contains("File does not exist"));
        }
    }
}