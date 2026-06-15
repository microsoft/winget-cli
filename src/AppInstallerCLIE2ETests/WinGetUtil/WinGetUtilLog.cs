// -----------------------------------------------------------------------------
// <copyright file="WinGetUtilLog.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test winget util log.
    /// </summary>
    public class WinGetUtilLog
    {
        /// <summary>
        /// Test logging functions.
        /// </summary>
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
