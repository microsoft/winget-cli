// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorTelemetryTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Threading;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Microsoft.VisualStudio.TestPlatform.ObjectModel;
    using Xunit;
    using Xunit.Abstractions;
    using static System.Collections.Specialized.BitVector32;

    /// <summary>
    /// Unit tests for running test on the processor.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class ConfigurationProcessorTelemetryTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorTelemetryTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorTelemetryTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// No event is generated if the unit succeeds.
        /// </summary>
        [Fact]
        public void Telemetry_NoUnitEventOnSuccess()
        {
            TelemetryTestObjects testObjects = new TelemetryTestObjects(getFails: false);
            testObjects.Processor = this.CreateConfigurationProcessorWithDiagnostics(testObjects.Factory);
            testObjects.CreateDetails();

            GetConfigurationUnitSettingsResult result = testObjects.Processor.GetUnitSettings(testObjects.Unit);

            Assert.Single(this.EventSink.Events);
            Assert.Equal(TelemetryEvent.ConfigUnitRunName, this.EventSink.Events[0].Name);
        }

        /// <summary>
        /// No event is generated if the details have not been retrieved.
        /// </summary>
        [Fact]
        public void Telemetry_NoUnitEventIfNoDetails()
        {
            TelemetryTestObjects testObjects = new TelemetryTestObjects();
            testObjects.Processor = this.CreateConfigurationProcessorWithDiagnostics(testObjects.Factory);

            GetConfigurationUnitSettingsResult result = testObjects.Processor.GetUnitSettings(testObjects.Unit);

            Assert.Empty(this.EventSink.Events);
        }

        /// <summary>
        /// No event is generated if the module is not public.
        /// </summary>
        [Fact]
        public void Telemetry_NoUnitEventIfNotPublic()
        {
            TelemetryTestObjects testObjects = new TelemetryTestObjects();
            testObjects.Processor = this.CreateConfigurationProcessorWithDiagnostics(testObjects.Factory);
            testObjects.CreateDetails(isPublic: false);

            GetConfigurationUnitSettingsResult result = testObjects.Processor.GetUnitSettings(testObjects.Unit);

            Assert.Empty(this.EventSink.Events);
        }

        /// <summary>
        /// The activity set by the caller is the value in the event.
        /// </summary>
        [Fact]
        public void Telemetry_ActivityID()
        {
            TelemetryTestObjects testObjects = new TelemetryTestObjects();
            testObjects.Processor = this.CreateConfigurationProcessorWithDiagnostics(testObjects.Factory);
            testObjects.CreateDetails();

            Guid activity = Guid.NewGuid();
            testObjects.Processor.ActivityIdentifier = activity;

            GetConfigurationUnitSettingsResult result = testObjects.Processor.GetUnitSettings(testObjects.Unit);

            Assert.Single(this.EventSink.Events);
            Assert.Equal(TelemetryEvent.ConfigUnitRunName, this.EventSink.Events[0].Name);
            Assert.Equal(activity, this.EventSink.Events[0].ActivityID);
        }

        /// <summary>
        /// Disabling telemetry causes no event to be produced.
        /// </summary>
        /// <param name="state">The state of telemetry.</param>
        [Theory]
        [InlineData(true)]
        [InlineData(false)]
        public void Telemetry_EnableState(bool state)
        {
            TelemetryTestObjects testObjects = new TelemetryTestObjects();
            testObjects.Processor = this.CreateConfigurationProcessorWithDiagnostics(testObjects.Factory);
            testObjects.CreateDetails();

            testObjects.Processor.GenerateTelemetryEvents = state;

            GetConfigurationUnitSettingsResult result = testObjects.Processor.GetUnitSettings(testObjects.Unit);

            if (state)
            {
                Assert.Single(this.EventSink.Events);
                Assert.Equal(TelemetryEvent.ConfigUnitRunName, this.EventSink.Events[0].Name);
            }
            else
            {
                Assert.Empty(this.EventSink.Events);
            }
        }

        /// <summary>
        /// The caller set by the caller is the value in the event.
        /// </summary>
        [Fact]
        public void Telemetry_Caller()
        {
            TelemetryTestObjects testObjects = new TelemetryTestObjects();
            testObjects.Processor = this.CreateConfigurationProcessorWithDiagnostics(testObjects.Factory);
            testObjects.CreateDetails();

            string caller = "TheTests";
            testObjects.Processor.Caller = caller;

            GetConfigurationUnitSettingsResult result = testObjects.Processor.GetUnitSettings(testObjects.Unit);

            Assert.Single(this.EventSink.Events);
            Assert.Equal(TelemetryEvent.ConfigUnitRunName, this.EventSink.Events[0].Name);
            Assert.Equal(caller, this.EventSink.Events[0].Caller);
        }

        /// <summary>
        /// Verifies all of the telemetry fields that come from executing a specific unit.
        /// </summary>
        [Fact]
        public void Telemetry_UnitFields()
        {
#pragma warning disable CS8602 // Dereference of a possibly null reference.
            TelemetryTestObjects testObjects = new TelemetryTestObjects();
            testObjects.Processor = this.CreateConfigurationProcessorWithDiagnostics(testObjects.Factory);
            testObjects.CreateDetails();

            testObjects.Unit.Type = "TestUnitName";
            testObjects.UnitDetails.ModuleName = "TestModuleName";

            string setting1 = "setting1";
            string setting2 = "setting2";

            testObjects.Unit.Settings.Add(setting1, 0);
            testObjects.Unit.Settings.Add(setting2, 0);

            GetConfigurationUnitSettingsResult result = testObjects.Processor.GetUnitSettings(testObjects.Unit);

            Assert.Single(this.EventSink.Events);
            TelemetryEvent runEvent = this.EventSink.Events[0];
            Assert.Equal(TelemetryEvent.ConfigUnitRunName, runEvent.Name);
            Assert.NotEqual(string.Empty, runEvent.CodeVersion);
            Assert.NotEqual(Guid.Empty, runEvent.ActivityID);
            Assert.Equal(string.Empty, runEvent.Caller);
            Assert.Equal(Guid.Empty, Guid.Parse(runEvent.Properties[TelemetryEvent.SetID]));
            Assert.NotEqual(Guid.Empty, Guid.Parse(runEvent.Properties[TelemetryEvent.UnitID]));
            Assert.Equal(testObjects.Unit.Type, runEvent.Properties[TelemetryEvent.UnitName]);
            Assert.Equal(testObjects.UnitDetails.ModuleName, runEvent.Properties[TelemetryEvent.ModuleName]);
            Assert.Equal(((int)testObjects.Unit.Intent).ToString(), runEvent.Properties[TelemetryEvent.UnitIntent]);
            Assert.Equal(((int)ConfigurationUnitIntent.Inform).ToString(), runEvent.Properties[TelemetryEvent.RunIntent]);
            Assert.NotEqual(string.Empty, runEvent.Properties[TelemetryEvent.Action]);
            Assert.Equal(testObjects.GetResult.ResultInformation.ResultCode.HResult.ToString(), runEvent.Properties[TelemetryEvent.Result]);
            Assert.Equal(((int)testObjects.GetResult.ResultInformation.ResultSource).ToString(), runEvent.Properties[TelemetryEvent.FailurePoint]);
            Assert.Equal(setting1 + "|" + setting2, runEvent.Properties[TelemetryEvent.SettingsProvided]);
#pragma warning restore CS8602 // Dereference of a possibly null reference.
        }

        private class TelemetryTestObjects
        {
            public TelemetryTestObjects(bool getFails = true)
            {
                this.Unit = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Apply };

                this.Factory = new TestConfigurationProcessorFactory();
                this.Factory.NullProcessor = new TestConfigurationSetProcessor(null);

                this.UnitProcessor = this.Factory.NullProcessor.CreateTestProcessor(this.Unit);

                if (getFails)
                {
                    var getResult = new GetSettingsResultInstance(this.Unit);
                    getResult.InternalResult.ResultCode = new NullReferenceException();
                    getResult.InternalResult.ResultSource = ConfigurationUnitResultSource.UnitProcessing;
                    this.GetResult = getResult;
                    this.UnitProcessor.GetSettingsDelegate = () => this.GetResult;
                }
            }

            public ConfigurationUnit Unit { get; set; }

            public TestConfigurationProcessorFactory Factory { get; set; }

            public TestConfigurationUnitProcessor UnitProcessor { get; set; }

            public IGetSettingsResult? GetResult { get; set; }

            public TestConfigurationUnitProcessorDetails? UnitDetails { get; set; }

            public ConfigurationProcessor? Processor { get; set; }

            public void CreateDetails(bool isPublic = true)
            {
#pragma warning disable CS8602 // Dereference of a possibly null reference.
                this.UnitDetails = this.Factory.NullProcessor.CreateUnitDetails(this.Unit, ConfigurationUnitDetailFlags.ReadOnly);
#pragma warning restore CS8602 // Dereference of a possibly null reference.
                this.UnitDetails.IsPublic = isPublic;

                this.Processor?.GetUnitDetails(this.Unit, ConfigurationUnitDetailFlags.ReadOnly);
            }
        }
    }
}
