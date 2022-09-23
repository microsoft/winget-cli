// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.IO;
    using NUnit.Framework;

    public class WinGetUtilLog
    {
        [Test]
        public void WinGetUtil_Logging()
        {
            IntPtr hresult;
            string filePath = TestCommon.GetRandomTestFile(".log");

            // Init logging
            hresult = WinGetUtilWrapper.WinGetLoggingInit(filePath);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(File.Exists(filePath));

            // Terminate logging
            hresult = WinGetUtilWrapper.WinGetLoggingTerm(filePath);

            Assert.AreEqual(IntPtr.Zero, hresult);
        }
    }
}
