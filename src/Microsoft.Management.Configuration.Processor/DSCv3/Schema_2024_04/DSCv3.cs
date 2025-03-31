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
        public IResourceListItem? GetResourceByType(string resourceType, IDiagnosticsSink? diagnosticsSink = null)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, ListCommand, resourceType },
            };

            RunSynchronously(processExecution, diagnosticsSink);

            if (processExecution.Output.Count > 1)
            {
                throw new Exceptions.GetDscResourceMultipleMatches(resourceType, null);
            }

            return GetOptionalSingleOutputLineAs<ResourceListItem>(processExecution);
        }

        /// <inheritdoc />
        public IResourceTestItem TestResource(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, TestCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            if (RunSynchronously(processExecution, diagnosticsSink))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Test, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return TestFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Test, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceGetItem GetResourceSettings(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, GetCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            if (RunSynchronously(processExecution, diagnosticsSink))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Get, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return GetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Get, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IResourceSetItem SetResourceSettings(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
        {
            ProcessExecution processExecution = new ProcessExecution()
            {
                ExecutablePath = this.processorSettings.EffectiveDscExecutablePath,
                Arguments = new[] { PlainTextTraces, this.DiagnosticTraceLevel, ResourceCommand, SetCommand, ResourceParameter, unitInternal.QualifiedName, FileParameter, StdInputIdentifier },
                Input = ConvertValueSetToJSON(unitInternal.GetExpandedSettings()),
            };

            if (RunSynchronously(processExecution, diagnosticsSink))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return SetFullItem.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName), GetDefaultJsonOptions());
        }

        /// <inheritdoc />
        public IList<IResourceExportItem> ExportResource(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
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
            };

            if (RunSynchronously(processExecution, diagnosticsSink))
            {
                throw new Exceptions.InvokeDscResourceException(Exceptions.InvokeDscResourceException.Export, unitInternal.QualifiedName, null, processExecution.GetAllErrorLines());
            }

            return ConfigurationDocument.CreateFrom(GetRequiredSingleOutputLineAsJSON(processExecution, Exceptions.InvokeDscResourceException.Set, unitInternal.QualifiedName), GetDefaultJsonOptions()).InterfaceResources;
        }

        /// <summary>
        /// Runs the process, waiting until it completes.
        /// </summary>
        /// <param name="processExecution">The process to run.</param>
        /// <param name="diagnosticsSink">The diagnostics sink.</param>
        /// <returns>True if the exit code was not 0.</returns>
        private static bool RunSynchronously(ProcessExecution processExecution, IDiagnosticsSink? diagnosticsSink)
        {
            diagnosticsSink?.OnDiagnostics(DiagnosticLevel.Verbose, $"Starting process: {processExecution.CommandLine}");

            processExecution.Start().WaitForExit();

            diagnosticsSink?.OnDiagnostics(DiagnosticLevel.Verbose, $"Process exited with code: {processExecution.ExitCode}\n--- Output Stream ---\n{processExecution.GetAllOutputLines()}\n--- Error Stream ---\n{processExecution.GetAllErrorLines()}");

            return processExecution.ExitCode != 0;
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
    }
}
