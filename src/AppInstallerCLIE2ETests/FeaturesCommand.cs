// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class FeaturesCommand
    {
        [Test]
        public void DisplayFeatures()
        {
            var result = TestCommon.RunAICLICommand("features", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Command Sample"));
            Assert.True(result.StdOut.Contains("Argument Sample"));
            Assert.True(result.StdOut.Contains("Microsoft Store Support"));
            Assert.False(result.StdOut.Contains("Enabled"));
        }
    }
}
