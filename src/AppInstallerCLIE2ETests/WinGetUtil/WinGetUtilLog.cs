// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System.IO;
    using NUnit.Framework;

    public class WinGetUtilLog
    {
        [Test]
        public void WinGetUtil_Logging()
        {
            string filePath = TestCommon.GetRandomTestFile(".log");

            // Init logging
            WinGetUtilWrapper.WinGetLoggingInit(filePath);
            Assert.True(File.Exists(filePath));

            // Terminate logging
            WinGetUtilWrapper.WinGetLoggingTerm(filePath);
        }
    }
}
