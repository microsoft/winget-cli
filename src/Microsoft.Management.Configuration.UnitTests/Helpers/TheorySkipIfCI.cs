// -----------------------------------------------------------------------------
// <copyright file="TheorySkipIfCI.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using Xunit;

    /// <summary>
    /// Skip theory test if running in CI builds.
    /// </summary>
    public sealed class TheorySkipIfCI : TheoryAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TheorySkipIfCI"/> class.
        /// </summary>
        public TheorySkipIfCI()
        {
            if (Environment.GetEnvironmentVariable("BUILD_BUILDNUMBER") is not null)
            {
                this.Skip = "Skip test for CI builds";
            }
        }
    }
}
