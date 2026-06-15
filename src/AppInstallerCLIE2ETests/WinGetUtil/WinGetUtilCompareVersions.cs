// -----------------------------------------------------------------------------
// <copyright file="WinGetUtilCompareVersions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using NUnit.Framework;

    /// <summary>
    /// Test winget util compare versions.
    /// </summary>
    public class WinGetUtilCompareVersions
    {
        /// <summary>
        /// Test compare versions.
        /// </summary>
        /// <param name="version1">Version 1.</param>
        /// <param name="version2">Version 2.</param>
        /// <param name="expectedResult">Expected result.</param>
        [Test]
        //// V1 = V2
        [TestCase("1.0.0.0", "1.0.0.0", 0)]
        [TestCase("1.0.0", "1.0.0.0", 0)]
        [TestCase("1.0", "1.0.0.0", 0)]
        [TestCase("1", "1.0.0.0", 0)]
        //// V1 > V2
        [TestCase("1.0.0.1", "1.0.0.0", 1)]
        [TestCase("1.0.1.0", "1.0.0.0", 1)]
        [TestCase("1.1.0.0", "1.0.0.0", 1)]
        [TestCase("2.0.0.0", "1.0.0.0", 1)]
        //// V1 < V2
        [TestCase("1.0.0.0", "1.0.0.1", -1)]
        [TestCase("1.0.0.0", "1.0.1.0", -1)]
        [TestCase("1.0.0.0", "1.1.0.0", -1)]
        [TestCase("1.0.0.0", "2.0.0.0", -1)]
        public void WinGetUtil_CompareVersions(string version1, string version2, int expectedResult)
        {
            // Compare versions
            WinGetUtilWrapper.WinGetCompareVersions(version1, version2, out int result);
            Assert.AreEqual(expectedResult, result);
        }
    }
}
