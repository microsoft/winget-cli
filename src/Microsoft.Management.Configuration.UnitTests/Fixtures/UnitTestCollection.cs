// -----------------------------------------------------------------------------
// <copyright file="UnitTestCollection.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Fixtures
{
    using Xunit;

    /// <summary>
    /// Unit test collection.
    /// </summary>
    [CollectionDefinition("UnitTestCollection")]
    public class UnitTestCollection : ICollectionFixture<UnitTestFixture>
    {
        // This class has no code, and is never created. Its purpose is simply
        // to be the place to apply [CollectionDefinition] and all the
        // ICollectionFixture<> interfaces.
    }
}
