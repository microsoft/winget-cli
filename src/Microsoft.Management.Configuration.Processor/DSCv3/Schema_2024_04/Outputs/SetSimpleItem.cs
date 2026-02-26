// -----------------------------------------------------------------------------
// <copyright file="SetSimpleItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;

    /// <summary>
    /// The simple form of the set output.
    /// DSC returns a simple set response when the instance isn't a group resource, adapter resource, or nested inside a group or adapter resource.
    /// </summary>
    internal class SetSimpleItem
    {
        /// <summary>
        /// Gets or sets the state of the resource properties before the attempt to set them.
        /// </summary>
        [JsonRequired]
        public JsonObject? BeforeState { get; set; }

        /// <summary>
        /// Gets or sets the state of the resource properties after the attempt to set them.
        /// </summary>
        [JsonRequired]
        public JsonObject? AfterState { get; set; }

        /// <summary>
        /// Gets or sets the list of properties that changed.
        /// </summary>
        [JsonRequired]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
        public string[] ChangedProperties { get; set; } = [];
    }
}
