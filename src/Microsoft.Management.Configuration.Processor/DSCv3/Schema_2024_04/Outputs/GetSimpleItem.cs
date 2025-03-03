// -----------------------------------------------------------------------------
// <copyright file="GetSimpleItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;

    /// <summary>
    /// The simple form of the get output.
    /// DSC returns a simple get response when the instance isn't a group resource, adapter resource, or nested inside a group or adapter resource.
    /// </summary>
    internal class GetSimpleItem
    {
        /// <summary>
        /// Gets or sets the state of the resource properties.
        /// </summary>
        [JsonRequired]
        public JsonObject? ActualState { get; set; }
    }
}
