// -----------------------------------------------------------------------------
// <copyright file="TestFullItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// The full form of the test output.
    /// When the retrieved instance is for group resource, adapter resource, or nested inside a group or adapter resource, DSC returns a full test result, which also includes the resource type and instance name.
    /// </summary>
    internal class TestFullItem : FullItemBase<TestSimpleItem, TestFullItem>, IResourceTestItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TestFullItem"/> class.
        /// </summary>
        public TestFullItem()
        {
        }

        /// <inheritdoc />
        public bool InDesiredState
        {
            get
            {
                if (this.SimpleResult != null)
                {
                    return this.SimpleResult.InDesiredState;
                }
                else if (this.FullResults != null)
                {
                    bool result = true;

                    foreach (var item in this.FullResults)
                    {
                        result = result && item.InDesiredState;
                    }

                    return result;
                }
                else
                {
                    throw new System.InvalidOperationException("Test result has not been initialized.");
                }
            }
        }
    }
}
