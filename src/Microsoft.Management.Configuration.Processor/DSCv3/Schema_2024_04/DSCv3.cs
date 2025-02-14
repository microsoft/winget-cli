// -----------------------------------------------------------------------------
// <copyright file="DSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04
{
    using System;
    using System.Linq;
    using System.Text.Json;
    using System.Text.Json.Serialization;
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

            RunSynchronously(processExecution);

            return GetOptionalSingleOutputLineAs<ResourceListItem>(processExecution);
        }

        private static void RunSynchronously(ProcessExecution processExecution)
        {
            processExecution.Start().WaitForExit();

            if (processExecution.ExitCode != 0)
            {
                // TODO: Improve error handling. This applies to this entire file. It likely means completely owning all exceptions that leave this class.
                throw new Exception($"DSC process exited with code `{processExecution.ExitCode}` [{processExecution.CommandLine}]");
            }
        }

        private static void ThrowOnMultipleOutputLines(ProcessExecution processExecution)
        {
            if (processExecution.Output.Count > 1)
            {
                throw new Exception($"DSC process produced {processExecution.Output.Count} lines [{processExecution.CommandLine}]");
            }
        }

        private static T? GetOptionalSingleOutputLineAs<T>(ProcessExecution processExecution)
        {
            ThrowOnMultipleOutputLines(processExecution);

            if (processExecution.Output.Count == 0)
            {
                return default;
            }

            return JsonSerializer.Deserialize<T>(processExecution.Output.First(), GetDefaultJsonOptions());
        }

        private static JsonSerializerOptions GetDefaultJsonOptions()
        {
            return new JsonSerializerOptions()
            {
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
                Converters =
                {
                    new JsonStringEnumConverter(),
                },
            };
        }
    }
}
