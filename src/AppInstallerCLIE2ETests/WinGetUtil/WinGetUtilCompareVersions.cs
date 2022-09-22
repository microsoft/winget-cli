// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using NUnit.Framework;

    public class WinGetUtilCompareVersions
    {
        [Test]
        // V1 = V2
        [TestCase("1.0.0.0", "1.0.0.0", 0)]
        [TestCase("1.0.0", "1.0.0.0", 0)]
        [TestCase("1.0", "1.0.0.0", 0)]
        [TestCase("1", "1.0.0.0", 0)]
        // V1 > V2
        [TestCase("1.0.0.1", "1.0.0.0", 1)]
        [TestCase("1.0.1.0", "1.0.0.0", 1)]
        [TestCase("1.1.0.0", "1.0.0.0", 1)]
        [TestCase("2.0.0.0", "1.0.0.0", 1)]
        // V1 < V2
        [TestCase("1.0.0.0", "1.0.0.1", -1)]
        [TestCase("1.0.0.0", "1.0.1.0", -1)]
        [TestCase("1.0.0.0", "1.1.0.0", -1)]
        [TestCase("1.0.0.0", "2.0.0.0", -1)]
        public void WinGetCompareVersions(string version1, string version2, int expectedResult)
        {
            // Compare versions
            IntPtr hresult = WinGetUtilWrapper.WinGetCompareVersions(version1, version2, out int result);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.AreEqual(expectedResult, result);
        }
    }
}
