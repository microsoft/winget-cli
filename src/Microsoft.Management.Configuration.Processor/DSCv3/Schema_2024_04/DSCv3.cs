// -----------------------------------------------------------------------------
// <copyright file="DSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.Schema_2024_04.Outputs;

    /// <summary>
    /// An instance of IDSCv3 for interacting with 1.0.
    /// </summary>
    internal class DSCv3 : IDSCv3
    {
        private readonly ProcessorSettings processorSettings;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3"/> class.
        /// </summary>
        /// <param name="processorSettings">The processor settings.</param>
        public DSCv3(ProcessorSettings processorSettings)
        {
            this.processorSettings = processorSettings;
        }

        /// <inheritdoc />
        public IResourceListItem? GetResourceByType(string resourceType)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Command = "resource list",
                Arguments = new[] { resourceType },
            };

            processExecution.Start().WaitForExit();

            return JsonHandling.GetSingleOuputLineAs<ResourceListItem>(processExecution);
        }
    }
}
