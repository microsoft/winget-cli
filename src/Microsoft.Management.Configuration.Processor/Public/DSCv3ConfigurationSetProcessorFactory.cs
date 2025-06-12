// -----------------------------------------------------------------------------
// <copyright file="DSCv3ConfigurationSetProcessorFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.DSCv3.Set;
    using Microsoft.Management.Configuration.Processor.Factory;

    /// <summary>
    /// IConfigurationSetProcessorFactory implementation using DSC v3.
    /// </summary>
    internal sealed partial class DSCv3ConfigurationSetProcessorFactory : ConfigurationSetProcessorFactoryBase, IConfigurationSetProcessorFactory, IDictionary<string, string>
    {
        private const string DscExecutablePathPropertyName = "DscExecutablePath";
        private const string FoundDscExecutablePathPropertyName = "FoundDscExecutablePath";
        private const string DiagnosticTraceEnabledPropertyName = "DiagnosticTraceEnabled";
        private const string FindDscStateMachinePropertyName = "FindDscStateMachine";

        private ProcessorSettings processorSettings = new ();

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ConfigurationSetProcessorFactory"/> class.
        /// </summary>
        public DSCv3ConfigurationSetProcessorFactory()
        {
            this.processorSettings.DiagnosticsSink = this;
        }

        /// <summary>
        /// Gets or sets the path to the DSC v3 executable.
        /// </summary>
        public string? DscExecutablePath
        {
            get
            {
                return this.processorSettings.DscExecutablePath;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting DscExecutablePath in limit mode is invalid.");
                }

                this.processorSettings.DscExecutablePath = value;
            }
        }

#if !AICLI_DISABLE_TEST_HOOKS
        /// <summary>
        /// Gets the processor settings; for tests only.
        /// </summary>
        public ProcessorSettings Settings
        {
            get
            {
                return this.processorSettings;
            }
        }
#endif

        /// <inheritdoc />
        public ICollection<string> Keys => throw new NotImplementedException();

        /// <inheritdoc />
        public ICollection<string> Values => throw new NotImplementedException();

        /// <inheritdoc />
        public int Count => throw new NotImplementedException();

        /// <inheritdoc />
        public bool IsReadOnly => this.IsLimitMode();

        /// <inheritdoc />
        public string this[string key] { get => this.GetValue(key); set => this.SetValue(key, value); }

        /// <inheritdoc />
        public void Add(string key, string value)
        {
            this.SetValue(key, value);
        }

        /// <inheritdoc />
        public void Add(KeyValuePair<string, string> item)
        {
            this.SetValue(item.Key, item.Value);
        }

        /// <inheritdoc />
        public void Clear()
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public bool Contains(KeyValuePair<string, string> item)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public bool ContainsKey(string key)
        {
            switch (key)
            {
                case DscExecutablePathPropertyName:
                    return this.DscExecutablePath != null;
            }

            return false;
        }

        /// <inheritdoc />
        public void CopyTo(KeyValuePair<string, string>[] array, int arrayIndex)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public IEnumerator<KeyValuePair<string, string>> GetEnumerator()
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public bool Remove(string key)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public bool Remove(KeyValuePair<string, string> item)
        {
            throw new NotImplementedException();
        }

        /// <inheritdoc />
        public bool TryGetValue(string key, [MaybeNullWhen(false)] out string value)
        {
            value = null;

            switch (key)
            {
                case DscExecutablePathPropertyName:
                    value = this.DscExecutablePath!;
                    return true;
                case FoundDscExecutablePathPropertyName:
                    value = this.processorSettings.GetFoundDscExecutablePath() !;
                    return true;
                case DiagnosticTraceEnabledPropertyName:
                    value = this.processorSettings.DiagnosticTraceEnabled.ToString();
                    return true;
                case FindDscStateMachinePropertyName:
                    value = this.processorSettings.PumpFindDscStateMachine().ToString();
                    return true;
            }

            return false;
        }

        /// <inheritdoc />
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        /// <inheritdoc />
        protected override IConfigurationSetProcessor CreateSetProcessorInternal(ConfigurationSet? set, bool isLimitMode)
        {
            ProcessorSettings processorSettingsCopy = this.processorSettings.Clone();
            this.OnDiagnostics(DiagnosticLevel.Verbose, "Creating set processor with settings:\n" + processorSettingsCopy.ToString());
            return new DSCv3ConfigurationSetProcessor(processorSettingsCopy, set, isLimitMode) { SetProcessorFactory = this };
        }

        private string GetValue(string name)
        {
            if (this.TryGetValue(name, out string? result))
            {
                return result;
            }

            throw new ArgumentOutOfRangeException($"Invalid property name: {name}");
        }

        private void SetValue(string name, string value)
        {
            switch (name)
            {
                case DscExecutablePathPropertyName:
                    this.DscExecutablePath = value;
                    break;
                case DiagnosticTraceEnabledPropertyName:
                    this.processorSettings.DiagnosticTraceEnabled = bool.Parse(value);
                    break;
                default:
                    throw new ArgumentOutOfRangeException($"Invalid property name: {name}");
            }
        }
    }
}
