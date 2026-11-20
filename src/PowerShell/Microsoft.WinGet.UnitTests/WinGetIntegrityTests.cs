// -----------------------------------------------------------------------------
// <copyright file="WinGetIntegrityTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.UnitTests
{
    using System;
    using System.IO;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Xunit;

    /// <summary>Tests WinGet executable paths and environment integrity checks.</summary>
    public class WinGetIntegrityTests
    {
        /// <summary>A missing executable path must not select the package alias.</summary>
        [Fact]
        public void MissingExecutablePathDoesNotFallBackToAlias()
        {
            static void CreateWithoutPath() => _ = new WingetCLIWrapper(null!);
            Assert.Throws<ArgumentNullException>(CreateWithoutPath);
        }

        /// <summary>Checks environment health independently of executable launch success.</summary>
        /// <param name="aliasEnabled">Whether the app-execution alias exists.</param>
        /// <param name="pathFormat">The PATH, with placeholders for the WindowsApps directory.</param>
        /// <param name="expected">The expected integrity category.</param>
        [Theory]
        [InlineData(false, "{0}", IntegrityCategory.AppExecutionAliasDisabled)]
        [InlineData(false, null, IntegrityCategory.AppExecutionAliasDisabled)]
        [InlineData(true, null, IntegrityCategory.NotInPath)]
        [InlineData(true, "", IntegrityCategory.NotInPath)]
        [InlineData(true, @".;C:\Other", IntegrityCategory.NotInPath)]
        [InlineData(true, "{0}.other", IntegrityCategory.NotInPath)]
        [InlineData(true, @"{0}\SomePackage", IntegrityCategory.NotInPath)]
        [InlineData(true, @"{0}.other;{0}\SomePackage", IntegrityCategory.NotInPath)]
        [InlineData(true, "{0}.other;{0}", IntegrityCategory.Installed)]
        [InlineData(true, "{0}", IntegrityCategory.Installed)]
        [InlineData(true, @"{0};C:\Other", IntegrityCategory.Installed)]
        [InlineData(true, @"C:\Other;{0};C:\Another", IntegrityCategory.Installed)]
        [InlineData(true, @"C:\Other;{0}", IntegrityCategory.Installed)]
        [InlineData(true, @"{0}\", IntegrityCategory.Installed)]
        [InlineData(true, "{1}", IntegrityCategory.Installed)]
        public void GetEnvironmentIntegrityCategoryChecksAliasAndPath(
            bool aliasEnabled,
            string? pathFormat,
            IntegrityCategory expected)
        {
            string windowsAppsPath = Directory.CreateTempSubdirectory("WinGetIntegrityTests-").FullName;

            try
            {
                if (aliasEnabled)
                {
                    File.WriteAllText(Path.Combine(windowsAppsPath, Constants.WinGetExe), string.Empty);
                }

                string? envPath = pathFormat is null
                    ? null
                    : string.Format(pathFormat, windowsAppsPath, windowsAppsPath.ToUpperInvariant());

                Assert.Equal(expected, WinGetIntegrity.GetEnvironmentIntegrityCategory(windowsAppsPath, envPath));
            }
            finally
            {
                Directory.Delete(windowsAppsPath, true);
            }
        }
    }
}
