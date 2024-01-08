// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationSetGroupProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Runtime.InteropServices.WindowsRuntime;
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.Foundation;

    /// <summary>
    /// A test implementation of IConfigurationGroupProcessor.
    /// </summary>
    internal class TestConfigurationSetGroupProcessor : TestConfigurationSetProcessor, IConfigurationGroupProcessor
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationSetGroupProcessor"/> class.
        /// </summary>
        /// <param name="set">The set that this processor is for.</param>
        internal TestConfigurationSetGroupProcessor(ConfigurationSet? set)
            : base(set)
        {
        }

        /// <summary>
        /// Gets the group that this processor targets.
        /// </summary>
        public object? Group
        {
            get { return this.Set; }
        }

        /// <summary>
        /// Apply settings for the group.
        /// </summary>
        /// <returns>The operation to apply settings.</returns>
        public IAsyncOperationWithProgress<IApplyGroupSettingsResult, IApplyGroupMemberSettingsResult> ApplyGroupSettingsAsync()
        {
            return AsyncInfo.Run((CancellationToken cancellationToken, IProgress<IApplyGroupMemberSettingsResult> progress) => Task.Run<IApplyGroupSettingsResult>(() =>
            {
                ApplyGroupSettingsResultInstance result = new ();

                // TODO: Loop over units, placing them into result

                // TODO: Loop over units, sending progress reports, updating the result object

                return result;
            }));
        }

        /// <summary>
        /// Test settings for the group.
        /// </summary>
        /// <returns>The operation to test settings.</returns>
        public IAsyncOperationWithProgress<ITestGroupSettingsResult, ITestSettingsResult> TestGroupSettingsAsync()
        {
            throw new System.NotImplementedException();
        }
    }
}
