// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class UpdateCommand
    {
        private const string UpdateTestSourceName = @"UpdateTestSource";
        private const string UpdateTestSourceUrl = @"https://localhost:5001/TestKit";

        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{UpdateTestSourceName} {UpdateTestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", UpdateTestSourceName);
            TestCommon.WaitForDeploymentFinish();
        }

        //[Test]
        public void UpdateTest()
        {
            // Example Update Command Test
            // TODO: Modify test once final behavior of Update Command is established
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"AppInstallerTest.OutdatedTestExeInstaller --silent -l {installDir}");
            var result = TestCommon.RunAICLICommand("update", "AppInstallerTest.TestExeInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller updated"));
            result = TestCommon.RunAICLICommand("list", "");
            Assert.True(result.StdOut.Contains("Version: 1.2.3.4"));
        }
    }
}
