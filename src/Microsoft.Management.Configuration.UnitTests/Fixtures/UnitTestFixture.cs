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
        /// Creates a runspace adding the test module path.
        /// </summary>
        /// <param name="dscModule">Dsc module.</param>
        /// <returns>PowerShellRunspace.</returns>
        internal IProcessorEnvironment PrepareTestProcessorEnvironment(IDscModule? dscModule = null)
        {
            if (dscModule == null)
            {
                Mock<IDscModule> dscModuleMock = new Mock<IDscModule>();
                dscModuleMock.Setup(
                    (m) => m.ValidateModule(It.IsAny<Runspace>()));
                dscModuleMock.Setup(
                    m => m.ModuleName)
                    .Returns(Modules.PSDesiredStateConfiguration);
                dscModule = dscModuleMock.Object;
            }

            var processorEnv = new HostedEnvironment(dscModule);
            processorEnv.PrependPSModulePath(this.TestModulesPath);
            return processorEnv;
        }
    }
}
