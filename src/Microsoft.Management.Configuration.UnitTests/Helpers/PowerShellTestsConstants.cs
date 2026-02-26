// -----------------------------------------------------------------------------
// <copyright file="PowerShellTestsConstants.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    /// <summary>
    /// PowerShell related constants.
    /// </summary>
    internal static class PowerShellTestsConstants
    {
#pragma warning disable SA1600 // ElementsMustBeDocumented
        public static class TestModule
        {
            public const string SimpleTestResourceModuleName = "xSimpleTestResource";
            public const string SimpleFileResourceName = "SimpleFileResource";
            public const string SimpleTestResourceName = "SimpleTestResource";
            public const string SimpleTestResourceThrowsName = "SimpleTestResourceThrows";
            public const string SimpleTestResourceErrorName = "SimpleTestResourceError";
            public const string SimpleTestResourceManifestFileName = "xSimpleTestResource.psd1";
            public const string SimpleTestResourceVersion = "0.0.0.1";
        }
#pragma warning restore SA1600 // ElementsMustBeDocumented
    }
}
