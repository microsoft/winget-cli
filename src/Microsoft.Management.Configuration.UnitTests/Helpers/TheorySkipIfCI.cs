// -----------------------------------------------------------------------------
// <copyright file="TheorySkipIfCI.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Xunit;
    using Xunit.Sdk;

    /// <summary>
    /// Skip theory test if running in CI builds, and allow the test to skip itself at runtime by
    /// way of <see cref="TestSkip"/> or <see cref="Skip"/>.
    /// </summary>
    [XunitTestCaseDiscoverer("Xunit.Sdk.SkippableTheoryDiscoverer", "Xunit.SkippableFact")]
    public sealed class TheorySkipIfCI : SkippableTheoryAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TheorySkipIfCI"/> class.
        /// </summary>
        public TheorySkipIfCI()
        {
            if (TestSkip.IsCIBuild)
            {
                this.Skip = "Skip test for CI builds";
            }
        }
    }
}
