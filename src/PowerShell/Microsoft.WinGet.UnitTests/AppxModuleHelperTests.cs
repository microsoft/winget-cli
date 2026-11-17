// -----------------------------------------------------------------------------
// <copyright file="AppxModuleHelperTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.UnitTests
{
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Windows.ApplicationModel;
    using Xunit;

    /// <summary>
    /// Tests for <see cref="AppxModuleHelper"/>.
    /// </summary>
    public class AppxModuleHelperTests
    {
        /// <summary>
        /// Tests that only packages published by Microsoft and signed by the Store are trusted.
        /// </summary>
        /// <param name="publisherId">The PublisherId of the package.</param>
        /// <param name="signatureKind">The SignatureKind of the package.</param>
        /// <param name="expected">Whether the package is trusted.</param>
        [Theory]
        [InlineData("8wekyb3d8bbwe", PackageSignatureKind.Store, true)]
        [InlineData("8wekyb3d8bbwe", "Store", true)]
        [InlineData("8wekyb3d8bbwe", PackageSignatureKind.None, false)]
        [InlineData("8wekyb3d8bbwe", "Developer", false)]
        [InlineData("8wekyb3d8bbwe", PackageSignatureKind.Enterprise, false)]
        [InlineData("8wekyb3d8bbwe", null, false)]
        [InlineData("0000000000000", PackageSignatureKind.Store, false)]
        [InlineData(null, "Store", false)]
        [InlineData(null, null, false)]
        public void IsStoreSignedMicrosoftPackage(string? publisherId, object? signatureKind, bool expected)
        {
            Assert.Equal(expected, AppxModuleHelper.IsStoreSignedMicrosoftPackage(publisherId, signatureKind));
        }
    }
}
