// -----------------------------------------------------------------------------
// <copyright file="TelemetryEvent.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// This class holds the data about a telemetry event detected via the diagnostics side channel.
    /// </summary>
    public class TelemetryEvent
    {
        /// <summary>
        /// The initial indicator that the diagnostics message contains the contents of a telemetry event.
        /// </summary>
        public const string Preamble = "#DebugEventStream";

        /// <summary>
        /// The name of the ConfigUnitRun event.
        /// </summary>
        public const string ConfigUnitRunName = "ConfigUnitRun";

        /// <summary>
        /// The name of the ConfigProcessingSummary event.
        /// </summary>
        public const string ConfigProcessingSummaryName = "ConfigProcessingSummary";

#pragma warning disable SA1600 // Elements should be documented

        // Shared fields
        public const string SetID = "SetID";
        public const string RunIntent = "RunIntent";
        public const string Result = "Result";
        public const string FailurePoint = "FailurePoint";

        // ConfigUnitRun fields
        public const string UnitID = "UnitID";
        public const string UnitName = "UnitName";
        public const string ModuleName = "ModuleName";
        public const string UnitIntent = "UnitIntent";
        public const string Action = "Action";
        public const string SettingsProvided = "SettingsProvided";

        // ConfigProcessingSummary fields
        public const string FromHistory = "FromHistory";
        public const string AssertCount = "AssertCount";
        public const string AssertsRun = "AssertsRun";
        public const string AssertsFailed = "AssertsFailed";
        public const string InformCount = "InformCount";
        public const string InformsRun = "InformsRun";
        public const string InformsFailed = "InformsFailed";
        public const string ApplyCount = "ApplyCount";
        public const string AppliesRun = "AppliesRun";
        public const string AppliesFailed = "AppliesFailed";
#pragma warning restore SA1600 // Elements should be documented

        /// <summary>
        /// Initializes a new instance of the <see cref="TelemetryEvent"/> class.
        /// </summary>
        /// <param name="eventMessage">The message containing the event data.</param>
        public TelemetryEvent(string eventMessage)
        {
            bool preambleSeen = false;

            foreach (string line in eventMessage.Split('\n'))
            {
                if (line == Preamble)
                {
                    preambleSeen = true;
                    continue;
                }

                if (!preambleSeen)
                {
                    // Skip all lines until the preamble is seen
                    continue;
                }

                int splitIndex = line.IndexOf(": ");
                if (splitIndex != -1)
                {
                    this.Properties.Add(line.Substring(0, splitIndex), line.Substring(splitIndex + 2));
                }
            }
        }

        /// <summary>
        /// Gets the properties for this event.
        /// </summary>
        public Dictionary<string, string> Properties { get; private set; } = new Dictionary<string, string>();

        /// <summary>
        /// Gets the name of the event.
        /// </summary>
        public string Name
        {
            get
            {
                return this.Properties["Event"];
            }
        }

        /// <summary>
        /// Gets the activity id.
        /// </summary>
        public Guid ActivityID
        {
            get
            {
                return Guid.Parse(this.Properties["ActivityID"]);
            }
        }

        /// <summary>
        /// Gets the version of the code.
        /// </summary>
        public string CodeVersion
        {
            get
            {
                return this.Properties["CodeVersion"];
            }
        }

        /// <summary>
        /// Gets the caller.
        /// </summary>
        public string Caller
        {
            get
            {
                return this.Properties["Caller"];
            }
        }
    }
}
