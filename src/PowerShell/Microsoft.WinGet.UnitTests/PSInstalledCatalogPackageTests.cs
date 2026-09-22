// -----------------------------------------------------------------------------
// <copyright file="PSInstalledCatalogPackageTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.UnitTests
{
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Xunit;

    /// <summary>
    /// Tests for <see cref="PSInstalledCatalogPackage"/>.
    /// </summary>
    public class PSInstalledCatalogPackageTests
    {
        /// <summary>
        /// Tests that IsPinned is computed lazily and cached after the first access.
        /// </summary>
        [Fact]
        public void IsPinned_ComputesOnceAndCachesResult()
        {
            var calls = 0;
            CatalogPackage catalogPackage = new (objRef: null!);
            PSInstalledCatalogPackage package = new (
                catalogPackage,
                _ =>
                {
                    calls++;
                    return true;
                });

            Assert.Equal(0, calls);

            Assert.True(package.IsPinned);
            Assert.Equal(1, calls);

            Assert.True(package.IsPinned);
            Assert.Equal(1, calls);
        }
    }
}
