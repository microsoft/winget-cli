// -----------------------------------------------------------------------------
// <copyright file="Constants.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    /// <summary>
    /// Constants used by the tests.
    /// </summary>
    public class Constants
    {
        /// <summary>
        /// The assembly name value used by xUnit traits.
        /// </summary>
        public const string AssemblyNameForTraits = "Microsoft.Management.Configuration.UnitTests";

        /// <summary>
        /// The namespace where xUnit traits will be defined.
        /// </summary>
        public const string NamespaceNameForTraits = "Microsoft.Management.Configuration.UnitTests.Helpers";

        /// <summary>
        /// The dynamic runtime factory handler identifier.
        /// </summary>
        public const string DynamicRuntimeHandlerIdentifier = "{73fea39f-6f4a-41c9-ba94-6fd14d633e40}";

        /// <summary>
        /// Test guid for disabling the dynamic factory from setting the 'RunAs' start process verb.
        /// </summary>
        public const string DisableRunAsTestGuid = "1e62d683-2999-44e7-81f7-6f8f35e8d731";
    }
}
