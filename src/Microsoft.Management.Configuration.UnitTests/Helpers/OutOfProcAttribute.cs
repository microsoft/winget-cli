// -----------------------------------------------------------------------------
// <copyright file="OutOfProcAttribute.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using Xunit.Sdk;

    /// <summary>
    /// Trait used to mark a test as being able to run against the out of proc server.
    /// </summary>
    [TraitDiscoverer(OutOfProcDiscoverer.TypeName, Constants.AssemblyNameForTraits)]
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method, AllowMultiple = false)]
    internal class OutOfProcAttribute : Attribute, ITraitAttribute
    {
    }
}
