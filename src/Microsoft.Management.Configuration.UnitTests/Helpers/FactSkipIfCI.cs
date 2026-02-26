// -----------------------------------------------------------------------------
// <copyright file="FactSkipIfCI.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using Xunit;

    /// <summary>
    /// Skip fact tests if running in CI builds.
    /// </summary>
    public class FactSkipIfCI : FactAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FactSkipIfCI"/> class.
        /// </summary>
        public FactSkipIfCI()
        {
            if (Environment.GetEnvironmentVariable("BUILD_BUILDNUMBER") is not null)
            {
                this.Skip = "Skip test for CI builds";
            }
        }
    }
}
