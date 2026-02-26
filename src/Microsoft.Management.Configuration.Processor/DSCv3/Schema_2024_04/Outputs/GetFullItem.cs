// -----------------------------------------------------------------------------
// <copyright file="GetFullItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Windows.Foundation.Collections;

    /// <summary>
    /// The full form of the get output.
    /// When the retrieved instance is for group resource, adapter resource, or nested inside a group or adapter resource, DSC returns a full get result, which also includes the resource type and instance name.
    /// </summary>
    internal class GetFullItem : FullItemBase<GetSimpleItem, GetFullItem>, IResourceGetItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetFullItem"/> class.
        /// </summary>
        public GetFullItem()
        {
        }

        /// <inheritdoc />
        public ValueSet Settings
        {
            get
            {
                if (this.SimpleResult != null)
                {
                    return this.SimpleResult.ActualState?.ToValueSet() ?? throw new System.InvalidOperationException("Get result has not been initialized.");
                }
                else if (this.FullResults != null)
                {
                    throw new System.NotImplementedException("Requires constructing the entire group as the settings.");
                }
                else
                {
                    throw new System.InvalidOperationException("Get result has not been initialized.");
                }
            }
        }
    }
}
