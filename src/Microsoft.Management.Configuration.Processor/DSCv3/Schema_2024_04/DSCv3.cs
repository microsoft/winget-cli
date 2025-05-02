// -----------------------------------------------------------------------------
// <copyright file="DSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04
{
    using System;
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
        private const string ExportCommand = "export";
        private const string ResourceParameter = "-r";
        private const string FileParameter = "-f";
        private const string StdInputIdentifier = "-";

        private const string PathEnvironmentVariable = "PATH";
        private const string DSCResourcePathEnvironmentVariable = "DSC_RESOURCE_PATH";

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
                return this.processorSettings.DiagnosticTraceEnabled ? DiagnosticTraceLevelArguments : string.Empty;
            }
        }

        /// <inheritdoc />
        public IResourceListItem? GetResourceByType(string resourceType, ProcessorRunSettings? runSettings)
        {
            ResourceListItem? result = this.GetResourceByTypeInternal(resourceType, null, runSettings);
            if (result != null)
            {
                return result;
            }

            // Check for this resource within adapters
            List<ResourceListItem> results = new List<ResourceListItem>();

            foreach (ResourceListItem resource in this.GetAllResources(runSettings))
            {
                if (resource.Kind == Definitions.ResourceKind.Adapter)
                {
                    result = this.GetResourceByTypeInternal(resourceType, resource.Type, runSettings);
                    if (result != null)
                    {
                        results.Add(result);
                    }
                }
            }

            return this.GetResourceByLatestVersion(results);
        }

        /// <inheritdoc />
        public IResourceTestItem TestResource(ConfigurationUnitInternal unitInternal, ProcessorRunSettings? runSettings)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, TestCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
                EnvironmentVariables = CreateEnvironmentVariablesFromProcessorRunSettings(runSettings),
            };

            if (this.RunSynchronously(processExecution))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Test, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return TestFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Test, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceGetItem GetResourceSettings(ConfigurationUnitInternal unitInternal, ProcessorRunSettings? runSettings)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, GetCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
                EnvironmentVariables = CreateEnvironmentVariablesFromProcessorRunSettings(runSettings),
            };

            if (this.RunSynchronously(processExecution))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Get, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return GetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Get, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceSetItem SetResourceSettings(ConfigurationUnitInternal unitInternal, ProcessorRunSettings? runSettings)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, SetCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
                EnvironmentVariables = CreateEnvironmentVariablesFromProcessorRunSettings(runSettings),
            };

            if (this.RunSynchronously(processExecution))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return SetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IList<IResourceExportItem> ExportResource(ConfigurationUnitInternal unitInternal, ProcessorRunSettings? runSettings)
        {
            // 3.0 can't handle input to export; 3.1 will fix that.
            ValueSet expandedSettings = unitInternal.GetExpandedSettings();
            if (expandedSettings.Count != 0)
            {
                throw new NotImplementedException("Must use DSC v3.1.* to provide input to export.");
            }

            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, ExportCommand, ResourceParameter, unitInternal.QualifiedName },
                EnvironmentVariables = CreateEnvironmentVariablesFromProcessorRunSettings(runSettings),
            };

            if (this.RunSynchronously(processExecution))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Export, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return ConfigurationDocument.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName), GetDefaultJsonOptions()).InterfaceResources;
        }

        /// <inheritdoc />
        public List<IResourceListItem> GetAllResources(ProcessorRunSettings? runSettings)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, ListCommand },
                EnvironmentVariables = CreateEnvironmentVariablesFromProcessorRunSettings(runSettings),
            };

            this.RunSynchronously(processExecution);

            return GetOutputLinesAs<ResourceListItem>(processExecution).ToList<IResourceListItem>();
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

        private static List<ProcessExecutionEnvironmentVariable> CreateEnvironmentVariablesFromProcessorRunSettings(ProcessorRunSettings? runSettings)
        {
            List<ProcessExecutionEnvironmentVariable> result = new List<ProcessExecutionEnvironmentVariable>();
            if (runSettings is not null && !string.IsNullOrEmpty(runSettings.ResourceSearchPaths))
            {
                // For exclusive search paths, adding to PATH is still needed as anything referenced in the manifest is still searched from PATH.
                result.Add(new ProcessExecutionEnvironmentVariable { Name = PathEnvironmentVariable, Value = runSettings.ResourceSearchPaths, ValueType = ProcessExecutionEnvironmentVariableValueType.Prepend });

                if (runSettings.ResourceSearchPathsExclusive)
                {
                    result.Add(new ProcessExecutionEnvironmentVariable { Name = DSCResourcePathEnvironmentVariable, Value = runSettings.ResourceSearchPaths, ValueType = ProcessExecutionEnvironmentVariableValueType.Override });
                }
            }

            return result;
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

        private ResourceListItem? GetResourceByTypeInternal(string resourceType, string? adapter, ProcessorRunSettings? runSettings)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, ListCommand, adapter != null ? $"-a {adapter}" : string.Empty, resourceType },
                EnvironmentVariables = CreateEnvironmentVariablesFromProcessorRunSettings(runSettings),
            };

            this.RunSynchronously(processExecution);

            List<ResourceListItem> results = GetOutputLinesAs<ResourceListItem>(processExecution);

            return this.GetResourceByLatestVersion(results);
        }

        private ResourceListItem? GetResourceByLatestVersion(List<ResourceListItem> resources)
        {
            // There may be different versions of same resource on the system. We check if all
            // resource types match, we return the first one.
            // TODO: May want to pick the latest one from the list. But since we are not using
            // the version in our commands, picking any one is good for now.
            ResourceListItem? candidate = null;
            string candidateType = string.Empty;

            foreach (ResourceListItem resource in resources)
            {
                if (candidate == null)
                {
                    candidate = resource;
                    candidateType = resource.Type;
                }
                else if (!candidateType.Equals(resource.Type, StringComparison.OrdinalIgnoreCase))
                {
                    throw new Exceptions.GetDscResourceMultipleMatches(candidateType, null);
                }
            }

            return candidate;
        }
    }
}
