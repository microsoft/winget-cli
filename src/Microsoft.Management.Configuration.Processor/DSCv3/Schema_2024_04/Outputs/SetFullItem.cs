// -----------------------------------------------------------------------------
// <copyright file="SetFullItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// The full form of the set output.
    /// When the retrieved instance is for group resource, adapter resource, or nested inside a group or adapter resource, DSC returns a full set result, which also includes the resource type and instance name.
    /// </summary>
    internal class SetFullItem : FullItemBase<SetSimpleItem, SetFullItem>, IResourceSetItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SetFullItem"/> class.
        /// </summary>
        public SetFullItem()
        {
        }

        /// <inheritdoc />
        public bool RebootRequired => false;
    }
}
