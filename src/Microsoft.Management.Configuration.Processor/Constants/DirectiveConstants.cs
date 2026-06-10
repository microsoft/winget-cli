// -----------------------------------------------------------------------------
// <copyright file="DirectiveConstants.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Constants
{
    /// <summary>
    /// Directives.
    /// </summary>
    internal static class DirectiveConstants
    {
#pragma warning disable SA1600 // ElementsMustBeDocumented
        public const string MaxVersion = "maxVersion";
        public const string MinVersion = "minVersion";
        public const string Module = "module";
        public const string ModuleGuid = "moduleGuid";
        public const string Repository = "repository";
        public const string Version = "version";
        public const string AllowPrerelease = "allowPrerelease";
#pragma warning restore SA1600 // ElementsMustBeDocumented
    }
}
