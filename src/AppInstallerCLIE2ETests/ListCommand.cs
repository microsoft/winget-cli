// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ListCommand
    {
        private const string ListTestSourceName = @"ListTestSource";
        private const string ListTestSourceUrl = @"https://localhost:5001/TestKit";

        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{ListTestSourceName} {ListTestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", ListTestSourceName);
            TestCommon.WaitForDeploymentFinish();
        }

        //[Test]
        public void ListTest()
        {
            // Example List Command Test
            var installDir = TestCommon.GetRandomTestDir();
            TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --silent -l {installDir}");
            var result = TestCommon.RunAICLICommand("list", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"));
        }
    }
}
