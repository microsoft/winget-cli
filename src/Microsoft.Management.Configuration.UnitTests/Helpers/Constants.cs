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
        /// Test guid for enabling test mode for the dynamic runtime factory. Forces factory to exclude 'runas' verb and sets current IL to medium.
        /// </summary>
        public const string EnableDynamicFactoryTestMode = "1e62d683-2999-44e7-81f7-6f8f35e8d731";

        /// <summary>
        /// Test guid for allowing the restricted integrity level to be supported.
        /// </summary>
        public const string EnableRestrictedIntegrityLevelTestGuid = "5cae3226-185f-4289-815c-3c089d238dc6";

        /// <summary>
        /// Test guid for forcing units to have a high integrity level during the final routing of unit processor creation.
        /// </summary>
        public const string ForceHighIntegrityLevelUnitsTestGuid = "f698d20f-3584-4f28-bc75-28037e08e651";
    }
}
