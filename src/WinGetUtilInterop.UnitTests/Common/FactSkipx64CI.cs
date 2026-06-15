// -----------------------------------------------------------------------------
// <copyright file="FactSkipx64CI.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetUtilInterop.UnitTests.Common
{
    using System;
    using Xunit;

    /// <summary>
    /// Skip fact tests if running in CI x64 builds.
    /// </summary>
    public class FactSkipx64CI : FactAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FactSkipx64CI"/> class.
        /// </summary>
        public FactSkipx64CI()
        {
            if (Environment.Is64BitProcess && Environment.GetEnvironmentVariable("BUILD_BUILDNUMBER") is not null)
            {
                this.Skip = "Skip test for x64 CI builds";
            }
        }
    }
}
