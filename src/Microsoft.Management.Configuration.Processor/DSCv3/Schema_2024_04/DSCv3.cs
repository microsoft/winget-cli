// -----------------------------------------------------------------------------
// <copyright file="DSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.Json;
    using System.Text.Json.Serialization;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Windows.Foundation.Collections;

    /// <summary>
    /// An instance of IDSCv3 for interacting with 1.0.
    /// </summary>
    internal class DSCv3 : IDSCv3
    {
        private const string PlainTextTraces = "-t plaintext";
        private const string DiagnosticTraceLevelArguments = "-l trace";
        private const string ResourceCommand = "resource";
        private const string ListCommand = "list";
        private const string TestCommand = "test";
        private const string GetCommand = "get";
        private const string SetCommand = "set";
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

        private string DiagnosticTraceLevel
        {
            get
            {
                return this.processorSettings.DiagnosticTraceLevel ? DiagnosticTraceLevelArguments : string.Empty;
            }
        }

        /// <inheritdoc />
        public IResourceListItem? GetResourceByType(string resourceType)
        {
            ResourceListItem? result = this.GetResourceByType(resourceType, null);
            if (result != null)
            {
                return result;
            }

            // Check for this resource within adapters
            List<ResourceListItem> results = new List<ResourceListItem>();

            foreach (ResourceListItem resource in this.GetAllResources())
            {
                if (resource.Kind == Definitions.ResourceKind.Adapter)
                {
                    result = this.GetResourceByType(resourceType, resource.Type);
                    if (result != null)
                    {
                        results.Add(result);
                    }
                }
            }

            if (results.Count > 1)
            {
                throw new Exceptions.GetDscResourceMultipleMatches(resourceType, null);
            }

            return results.FirstOrDefault();
        }

        /// <inheritdoc />
        public IResourceTestItem TestResource(ConfigurationUnitInternal unitInternal)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, TestCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            if (this.RunSynchronously(processExecution))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Test, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return TestFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Test, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceGetItem GetResourceSettings(ConfigurationUnitInternal unitInternal)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, GetCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            if (this.RunSynchronously(processExecution))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Get, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return GetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Get, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceSetItem SetResourceSettings(ConfigurationUnitInternal unitInternal)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, SetCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            if (this.RunSynchronously(processExecution))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return SetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        private static void ThrowOnMultipleOutputLines(ProcessExecution processExecution, string method, string resourceName)
        {
            if (processExecution.Output.Count > 1)
            {
                throw new Exceptions.InvokeDscResourceException(method, resourceName, processExecution.GetAllOutputLines());
            }
        }

        private static void ThrowOnZeroOutputLines(ProcessExecution processExecution, string method, string resourceName)
        {
            if (processExecution.Output.Count == 0)
            {
                throw new Exceptions.InvokeDscResourceException(method, resourceName);
            }
        }

        private static T? GetOptionalSingleOutputLineAs<T>(ProcessExecution processExecution)
        {
            if (processExecution.Output.Count == 0)
            {
                return default;
            }

            return JsonSerializer.Deserialize<T>(processExecution.Output.First(), GetDefaultJsonOptions());
        }

        private static List<T> GetOutputLinesAs<T>(ProcessExecution processExecution)
        {
            List<T> result = new List<T>();
            var options = GetDefaultJsonOptions();

            foreach (string line in processExecution.Output)
            {
                T? lineObject = JsonSerializer.Deserialize<T>(line, options);
                if (lineObject != null)
                {
                    result.Add(lineObject);
                }
            }

            return result;
        }

        private static JsonDocument GetRequiredSingleOutputLineAsJSON(ProcessExecution processExecution, string method, string resourceName)
        {
            ThrowOnMultipleOutputLines(processExecution, method, resourceName);
            ThrowOnZeroOutputLines(processExecution, method, resourceName);

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

        /// <summary>
        /// Runs the process, waiting until it completes.
        /// </summary>
        /// <param name="processExecution">The process to run.</param>
        /// <returns>True if the exit code was not 0.</returns>
        private bool RunSynchronously(ProcessExecution processExecution)
        {
            this.processorSettings.DiagnosticsSink?.OnDiagnostics(DiagnosticLevel.Verbose, $"Starting process: {processExecution.CommandLine}");

            processExecution.Start().WaitForExit();

            this.processorSettings.DiagnosticsSink?.OnDiagnostics(DiagnosticLevel.Verbose, $"Process exited with code: {processExecution.ExitCode}\n--- Output Stream ---\n{processExecution.GetAllOutputLines()}\n--- Error Stream ---\n{processExecution.GetAllErrorLines()}");

            return processExecution.ExitCode != 0;
        }

        private ResourceListItem? GetResourceByType(string resourceType, string? adapter)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, ListCommand, adapter != null ? $"-a {adapter}" : string.Empty, resourceType },
            };

            this.RunSynchronously(processExecution);

            if (processExecution.Output.Count > 1)
            {
                throw new Exceptions.GetDscResourceMultipleMatches(resourceType, null);
            }

            return GetOptionalSingleOutputLineAs<ResourceListItem>(processExecution);
        }

        private List<ResourceListItem> GetAllResources()
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, ListCommand },
            };

            this.RunSynchronously(processExecution);

            return GetOutputLinesAs<ResourceListItem>(processExecution);
        }
    }
}
