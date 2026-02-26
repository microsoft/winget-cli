// -----------------------------------------------------------------------------
// <copyright file="TestSimpleItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;

    /// <summary>
    /// The simple form of the test output.
    /// DSC returns a simple test response when the instance isn't a group resource, adapter resource, or nested inside a group or adapter resource.
    /// </summary>
    internal class TestSimpleItem
    {
        /// <summary>
        /// Gets or sets the desired state of the resource properties.
        /// </summary>
        [JsonRequired]
        public JsonObject? DesiredState { get; set; }

        /// <summary>
        /// Gets or sets the actual state of the resource properties.
        /// </summary>
        [JsonRequired]
        public JsonObject? ActualState { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the resource is in the desired state.
        /// </summary>
        [JsonRequired]
        public bool InDesiredState { get; set; }

        /// <summary>
        /// Gets or sets the list of properties that are not in the desired state.
        /// </summary>
        [JsonRequired]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
        public string[] DifferingProperties { get; set; } = [];
    }
}
