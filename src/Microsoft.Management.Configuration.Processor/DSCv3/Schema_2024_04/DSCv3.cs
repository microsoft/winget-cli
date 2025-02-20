// -----------------------------------------------------------------------------
// <copyright file="DSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04
{
    using System;
    using System.Linq;
    using System.Security.AccessControl;
    using System.Text.Json;
    using System.Text.Json.Serialization;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.Schema_2024_04.Outputs;
    using Windows.Foundation.Collections;

    /// <summary>
    /// An instance of IDSCv3 for interacting with 1.0.
    /// </summary>
    internal class DSCv3 : IDSCv3
    {
        private const string ResourceParameter = "-r";
        private const string FileParameter = "-f";
        private const string StdInputIdentifier = "-";

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

        /// <inheritdoc />
        public IResourceTestItem TestResource(ConfigurationUnitInternal unitInternal)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Command = "resource test",
                Arguments = new[] { ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            RunSynchronously(processExecution);

            return TestFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceGetItem GetResourceSettings(ConfigurationUnitInternal unitInternal)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Command = "resource get",
                Arguments = new[] { ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            RunSynchronously(processExecution);

            return GetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceSetItem SetResourceSettings(ConfigurationUnitInternal unitInternal)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Command = "resource set",
                Arguments = new[] { ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            RunSynchronously(processExecution);

            return SetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution), GetDefaultJsonOptions());
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

        private static void ThrowOnZeroOutputLines(ProcessExecution processExecution)
        {
            if (processExecution.Output.Count == 0)
            {
                throw new Exception($"DSC process produced no output [{processExecution.CommandLine}]");
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

        private static JsonDocument GetRequiredSingleOutputLineAsJSON(ProcessExecution processExecution)
        {
            ThrowOnMultipleOutputLines(processExecution);
            ThrowOnZeroOutputLines(processExecution);

            return JsonDocument.Parse(processExecution.Output.First());
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

        private static string ConvertValueSetToJSON(ValueSet valueSet)
        {
            return JsonSerializer.Serialize(valueSet.ToHashtable());
        }
    }
}
