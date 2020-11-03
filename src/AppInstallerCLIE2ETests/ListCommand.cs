﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ListCommand : BaseCommand
    {
        [Test]
        public void ListSelf()
        {
            var result = TestCommon.RunAICLICommand("list", "WinGetDevCLI_8wekyb3d8bbwe");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("WinGet Dev CLI"));
            Assert.True(result.StdOut.Contains("WinGetDevCLI_8wekyb3d8bbwe"));
        }

        [Test]
        public void ListAfterInstall()
        {
            System.Guid guid = System.Guid.NewGuid();
            string productCode = guid.ToString();
            var installDir = TestCommon.GetRandomTestDir();

            var result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.False(result.StdOut.Contains(productCode));

            TestCommon.RunAICLICommand("install", $"AppInstallerTest.TestExeInstaller --override \"/InstallDir {installDir} /ProductID {productCode}\"");

            result = TestCommon.RunAICLICommand("list", productCode);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains(productCode));
            Assert.True(result.StdOut.Contains("1.0.0.0"));
            Assert.True(result.StdOut.Contains("2.0.0.0"));
        }
    }
}
