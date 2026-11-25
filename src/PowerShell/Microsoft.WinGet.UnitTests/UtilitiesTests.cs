// -----------------------------------------------------------------------------
// <copyright file="UtilitiesTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.UnitTests
{
    using Microsoft.WinGet.Client.Engine.Common;
    using Xunit;

    /// <summary>Tests the PATH update used by Repair.</summary>
    public class UtilitiesTests
    {
        private const string WindowsAppsPath = @"C:\Users\Test\AppData\Local\Microsoft\WindowsApps";

        /// <summary>Repairs missing entries without mistaking similar paths for the WindowsApps directory.</summary>
        /// <param name="pathFormat">The original PATH format.</param>
        /// <param name="expectedFormat">The expected PATH format.</param>
        [Theory]
        [InlineData(null, ";{0}")]
        [InlineData("", ";{0}")]
        [InlineData("{0}.other", "{0}.other;{0}")]
        [InlineData(@"{0}\SomePackage", @"{0}\SomePackage;{0}")]
        [InlineData(@"{0}.other;{0}\SomePackage", @"{0}.other;{0}\SomePackage;{0}")]
        [InlineData("{0}", "{0}")]
        [InlineData(@"{1}\", @"{1}\")]
        [InlineData("{0}.other;{0}", "{0}.other;{0}")]
        public void AddDirectoryToPathRepairsMissingEntry(string? pathFormat, string expectedFormat)
        {
            string? envPath = pathFormat is null
                ? null
                : string.Format(pathFormat, WindowsAppsPath, WindowsAppsPath.ToUpperInvariant());
            string expectedPath = string.Format(expectedFormat, WindowsAppsPath, WindowsAppsPath.ToUpperInvariant());

            string repairedPath = Utilities.AddDirectoryToPath(envPath, WindowsAppsPath);

            Assert.Equal(expectedPath, repairedPath);
            Assert.True(Utilities.PathContainsDirectory(repairedPath, WindowsAppsPath));
            Assert.Equal(repairedPath, Utilities.AddDirectoryToPath(repairedPath, WindowsAppsPath));
        }
    }
}
