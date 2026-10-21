// -----------------------------------------------------------------------------
// <copyright file="FactSkipIfCI.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Xunit;
    using Xunit.Sdk;

    /// <summary>
    /// Skip fact tests if running in CI builds, and allow the test to skip itself at runtime by
    /// way of <see cref="TestSkip"/> or <see cref="Skip"/>.
    /// </summary>
    [XunitTestCaseDiscoverer("Xunit.Sdk.SkippableFactDiscoverer", "Xunit.SkippableFact")]
    public class FactSkipIfCI : SkippableFactAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FactSkipIfCI"/> class.
        /// </summary>
        public FactSkipIfCI()
        {
            if (TestSkip.IsCIBuild)
            {
                this.Skip = "Skip test for CI builds";
            }
        }
    }
}
