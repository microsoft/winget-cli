// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System.IO;
    using System.Linq;
    using NUnit.Framework;

    public class WinGetUtilDownload
    {
        [Test]
        public void WinGetUtil_Download()
        {
            uint hashSize = 32;
            byte[] sha256Hash = new byte[hashSize];
            string installerUrl = @"https://localhost:5001/TestKit/AppInstallerTestExeInstaller/AppInstallerTestExeInstaller.exe";
            string filePath = TestCommon.GetRandomTestFile(".exe");

            // Download
            WinGetUtilWrapper.WinGetDownload(installerUrl, filePath, sha256Hash, hashSize);

            Assert.True(File.Exists(filePath));
            Assert.False(sha256Hash.All(byteVal => byteVal == 0));
        }
    }
}
