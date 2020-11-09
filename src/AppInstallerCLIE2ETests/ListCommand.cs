// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ListCommand : BaseCommand
    {
        [SetUp]
        public void Setup()
        {
            InitializeAllFeatures(false);
            ConfigureFeature("list", true);
        }

        [TearDown]
        public void TearDown()
        {
            InitializeAllFeatures(false);
        }

        [Test]
        public void ListSelf()
        {
            var result = TestCommon.RunAICLICommand("list", Constants.AICLIPackageFamilyName);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.AICLIPackageFamilyName));
        }

        [Test]
        public void ListAfterInstall()
        {
            System.Guid guid = System.Guid.NewGuid();
            string productCode = guid.ToString();
            var installDir = TestCommon.GetRandomTestDir();

            string localAppDataPath = System.Environment.GetEnvironmentVariable(LocalAppData);
            string logFilePath = System.IO.Path.Combine(localAppDataPath, Constants.E2ETestLogsPath);
            logFilePath = System.IO.Path.Combine(logFilePath, "ListAfterInstall-" + System.IO.Path.GetRandomFileName() + ".log");

            var result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);

            result = TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --override \"/InstallDir {installDir} /ProductID {productCode} /LogFile {logFilePath}\"");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains(productCode));
            Assert.True(result.StdOut.Contains("1.0.0.0"));
            // TODO: Uncomment when install starts tracking packages. Until then this won't be true because our install
            //       in this test is using a random guid that won't be in the index.
            //Assert.True(result.StdOut.Contains("2.0.0.0"));
        }
    }
}
