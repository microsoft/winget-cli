// -----------------------------------------------------------------------------
// <copyright file="UnitTestFixture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Fixtures
{
    using System;
    using System.IO;
    using System.Management.Automation.Runspaces;
    using System.Reflection;
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Runspaces;
    using Moq;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// Unit test fixture.
    /// </summary>
    public class UnitTestFixture
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UnitTestFixture"/> class.
        /// </summary>
        /// <param name="messageSink">The message sink for the fixture.</param>
        public UnitTestFixture(IMessageSink messageSink)
        {
            this.MessageSink = messageSink;

            string assemblyPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location)
                ?? throw new ArgumentException();

            this.TestModulesPath = Path.Combine(assemblyPath, "TestCollateral", "PowerShellModules");
            if (!Directory.Exists(this.TestModulesPath))
            {
                throw new DirectoryNotFoundException(this.TestModulesPath);
            }

            string? gitSearchPath = Path.GetDirectoryName(assemblyPath);

            while (!string.IsNullOrEmpty(gitSearchPath))
            {
                if (Directory.Exists(Path.Combine(gitSearchPath, ".git")))
                {
                    break;
                }

                gitSearchPath = Path.GetDirectoryName(gitSearchPath);
            }

            this.GitRootPath = gitSearchPath ?? throw new DirectoryNotFoundException("git root path");

            this.ExternalModulesPath = Path.Combine(this.GitRootPath, "src", "PowerShell", "ExternalModules");
            if (!Directory.Exists(this.ExternalModulesPath))
            {
                throw new DirectoryNotFoundException(this.ExternalModulesPath);
            }

            this.ConfigurationStatics = new ConfigurationStaticFunctions();
        }

        /// <summary>
        /// Gets the message sink for the fixture.
        /// </summary>
        public IMessageSink MessageSink { get; private init; }

        /// <summary>
        /// Gets the test module path.
        /// </summary>
        public string TestModulesPath { get; }

        /// <summary>
        /// Gets the git root path.
        /// </summary>
        public string GitRootPath { get; }

        /// <summary>
        /// Gets the external module path.
        /// </summary>
        public string ExternalModulesPath { get; }

        /// <summary>
        /// Gets the configuration statics object to use.
        /// </summary>
        public IConfigurationStatics ConfigurationStatics { get; private init; }

        /// <summary>
        /// Creates a runspace adding the test module path.
        /// </summary>
        /// <param name="validate">Validate runspace.</param>
        /// <returns>PowerShellRunspace.</returns>
        internal IProcessorEnvironment PrepareTestProcessorEnvironment(bool validate = false)
        {
            var processorEnv = new ProcessorEnvironmentFactory(PowerShellConfigurationProcessorType.Hosted).CreateEnvironment(null, PowerShellConfigurationProcessorPolicy.Unrestricted);
            processorEnv.PrependPSModulePath(this.ExternalModulesPath);
            processorEnv.PrependPSModulePath(this.TestModulesPath);

            if (validate)
            {
                processorEnv.ValidateRunspace();
            }

            return processorEnv;
        }
    }
}
