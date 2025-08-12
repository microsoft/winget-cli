// -----------------------------------------------------------------------------
// <copyright file="ShutdownTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using WinGetTestCommon;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for running test on the processor.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    public class ShutdownTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ShutdownTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ShutdownTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// Initiates a shutdown on the process when it is running a synchronous operation.
        /// </summary>
        [Fact]
        public void ShutdownSynchronization_SyncCall()
        {
            this.Fixture.RecreateStatics();

            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitWaits = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = this.ConfigurationUnit();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitWaits, configurationUnitWorks };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnitWaits);

            ManualResetEvent isWaiting = new ManualResetEvent(false);
            ManualResetEvent waitingOn = new ManualResetEvent(false);
            unitProcessor.TestSettingsDelegate = () =>
            {
                isWaiting.Set();
                waitingOn.WaitOne();
                return new TestSettingsResultInstance(configurationUnitWaits) { TestResult = ConfigurationTestResult.Positive };
            };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult? result = null;
            Exception? exception = null;

            ManualResetEvent syncCallDone = new ManualResetEvent(false);
            Thread thread = new Thread(() =>
            {
                try
                {
                    result = processor.TestSet(configurationSet);
                }
                catch (Exception ex)
                {
                    exception = ex;
                }

                syncCallDone.Set();
            });
            thread.Start();

            Assert.True(isWaiting.WaitOne(5000));

            var servers = WinGetServerInstance.GetInstances();
            Assert.Single(servers);

            var server = servers[0];
            Assert.True(server.HasWindow);

            // This is the call pattern from Windows
            this.SendMessageAndLog(server, WindowMessage.QueryEndSession);

            Thread thread2 = new Thread(() =>
            {
                // Release the wait after initiating the shutdown, but before waiting on it
                waitingOn.Set();
            });
            thread2.Start();

            this.SendMessageAndLog(server, WindowMessage.EndSession);
            this.SendMessageAndLog(server, WindowMessage.Close);

            Assert.True(syncCallDone.WaitOne(5000));

            Assert.NotNull(exception);

            Assert.True(server.Process.WaitForExit(5000));
        }

        /// <summary>
        /// Initiates a shutdown on the process when it is running an asynchronous operation.
        /// </summary>
        [Fact]
        public void ShutdownSynchronization_AsyncCall()
        {
            this.Fixture.RecreateStatics();

            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitWaits = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = this.ConfigurationUnit();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitWaits, configurationUnitWorks };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnitWaits);

            ManualResetEvent isWaiting = new ManualResetEvent(false);
            ManualResetEvent waitingOn = new ManualResetEvent(false);
            unitProcessor.TestSettingsDelegate = () =>
            {
                isWaiting.Set();
                waitingOn.WaitOne();
                return new TestSettingsResultInstance(configurationUnitWaits) { TestResult = ConfigurationTestResult.Positive };
            };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            var operation = processor.TestSetAsync(configurationSet);
            Assert.True(isWaiting.WaitOne(5000));

            var servers = WinGetServerInstance.GetInstances();
            Assert.Single(servers);

            var server = servers[0];
            Assert.True(server.HasWindow);

            // This is the call pattern from Windows
            this.SendMessageAndLog(server, WindowMessage.QueryEndSession);

            Thread thread2 = new Thread(() =>
            {
                // Release the wait after initiating the shutdown, but before waiting on it
                waitingOn.Set();
            });
            thread2.Start();

            this.SendMessageAndLog(server, WindowMessage.EndSession);
            this.SendMessageAndLog(server, WindowMessage.Close);

            Assert.ThrowsAny<Exception>(() => operation.GetAwaiter().GetResult());

            Assert.True(server.Process.WaitForExit(5000));
        }

        private void SendMessageAndLog(WinGetServerInstance server, WindowMessage message)
        {
            this.Log.WriteLine($"Sending message {message} to process {server.Process.Id}...");
            try
            {
                if (server.SendMessage(message))
                {
                    this.Log.WriteLine("... succeeded.");
                }
                else
                {
                    this.Log.WriteLine("... failed.");
                }
            }
            catch (Exception e)
            {
                this.Log.WriteLine($"... had exception: {e.Message}");
            }
        }
    }
}
