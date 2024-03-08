<!-- TEMPLATE
    The purpose of this spec is to describe new APIs, in a way
    that will transfer to docs.microsoft.com (DMC).

    There are two audiences for the spec. The first are people that want to evaluate and
    give feedback on the API, as part of the submission process.
    When it's complete it will be incorporated into the public documentation at
    http://docs.microsoft.com (DMC).
    Hopefully we'll be able to copy it mostly verbatim. So the second audience is
    everyone that reads there to learn how and why to use this API.
    Some of this text also shows up in Visual Studio Intellisense.

    For example, much of the examples and descriptions in the `RadialGradientBrush` API spec
    (https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/RadialGradientBrush/RadialGradientBrush.md)
    were carried over to the public API page on DMC
    (https://docs.microsoft.com/windows/winui/api/microsoft.ui.xaml.media.radialgradientbrush?view=winui-2.5)

    Once the API is on DMC, that becomes the official copy, and this spec becomes an archive.
    For example if the description is updated, that only needs to happen on DMC and needn't
    be duplicated here.

    Examples:
    * New class (RadialGradientBrush):
      https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/RadialGradientBrush/RadialGradientBrush.md
    * New member on an existing class (UIElement.ProtectedCursor):
      https://github.com/microsoft/microsoft-ui-xaml-specs/blob/master/active/UIElement/ElementCursor.md

    Style guide:
    * Use second person; speak to the developer who will be learning/using this API.
    (For example "you use this to..." rather than "the developer uses this to...")
    * Use hard returns to keep the page width within ~100 columns.
    (Otherwise it's more difficult to leave comments in a GitHub PR.)
    * Talk about an API's behavior, not its implementation.
    (Speak to the developer using this API, not to the team implementing it.)
    * A picture is worth a thousand words.
    * An example is worth a million words.
    * Keep examples realistic but simple; don't add unrelated complications.
    (An example that passes a stream needn't show the process of launching the File-Open dialog.)

-->

Microsoft.Management.Configuration API
===

# Background
This API is being added to enable the Developer+ configuration scenarios. It enables interacting with
configuration sets in three contexts:

1. Loading an existing configuration set from a stream
2. Loading previously applied configuration sets from the local history
3. Authoring a new/editing an existing configuration

These configuration sets are composed of configuration units, which describe the individual configurable
items and the values to configure.

Configuration actions consist of:

1. Test :: Determining whether the system state matches the described state
2. Get :: Extracting the current system state with respect to the configuration scope
3. Set :: Applying the described state to the system

This API is also intended to support multiple processes watching for state changes, both for the
configuration set lifetimes and the individual configuration unit states.

# Conceptual pages (How To)

_(Add conceptual documentation that will go to docs.microsoft.com "how to" page if needed)_

# API Pages

<!-- TEMPLATE

  Each of the L2 sections in this "API Pages" section corresponds to a page on DMC.

  It's not necessary to have a section for every class member though:
  * If its purpose and usage is obvious from it's name/type, it's not necessary to
    create a section for it.
  * If its purpose and usage is fully explained by brief description, either
      put it in a table in the "Other [class] members" section
      put it with /// comments in the IDL section

  Create an L2 section here for each API that needs more description or examples.
  For a new class with members, the members should go in their own L2 section.

  Example layout
    ## MyClass
    ## MyClass.Member1
    ## MyClass.Member2
    ## Other MyClass members
    ## MyOtherClass
    ## ...

  Notes:
  * The first line of each of these sections should become that first line on the DMC page,
    which then becomes the description you see in Intellisense.
  * Each page can have description, examples, and remarks.
    Remarks are where the documentation calls out special considerations that the developer
    should be aware of.
  * It can be helpful at the top of an API page (or after the Intellisense text) to add the
    API signature in C#
  * Add a "_Spec note: ..._" to add a note that's useful in this spec but shouldn't go to DMC.
  * Show _examples_, not _samples_; an example is a snippet, a sample is a full working app.

-->

## ConfigurationSetState enumeration

The state of a configuration set in the configuration history.

| Name | Description |
|-|-|
| Unknown | Primarily used for a configuration set that has not been applied. |
| Pending | The configuration set has been recorded into the history, but has not yet begun applying. |
| InProgress | The configuration set has begun being applied to the system. |
| Completed | The configuration set has completed being applied. |

## ConfigurationUnitState enumeration

The state of a configuration unit in the configuration history.

| Name | Description |
|-|-|
| Unknown | Primarily used for a configuration unit that has not been applied. |
| Pending | The configuration unit has been recorded into the history, but has not yet begun applying. |
| InProgress | The configuration unit has begun being applied to the system. |
| Completed | The configuration unit has completed being applied; the result information will contain additional details. |
| Skipped | The configuration unit was skipped; the result information will contain additional details on the reason. |

## ConfigurationUnitDetailLevel enumeration

Defines the level of detail probing that is allowed about a configuration unit.

| Name | Description |
|-|-|
| Local | Only reads details from local data. |
| Catalog | Will query the catalog information for details, but will not download any modules. |
| Download | Will download modules, but not load them. |
| Load | Will download and load modules for details. |

## ConfigurationUnitResultInformation class

Information on a result for a single unit of configuration.

The class is used both in reporting results through the `ConfigurationSet.ConfigurationSetChange` event as they occur
and in viewing past results via the `ConfigurationUnit.ResultInformation` property on a historical record.

## IConfigurationUnitSettingDetails interface

Provides information for a specific configuration unit setting.

The properties on this interface are useful for creating a rich authoring experience.

## IConfigurationUnitSettingDetails.Semantics schema

> _TODO: Define the meaning/schema for this value_

## IConfigurationUnitProcessorDetails interface

Provides information for a specific configuration unit within the runtime.

The properties on this interface are useful for informing the user about the provenance of the code that is
responsible for processing the configuration unit.

## ConfigurationUnit class

A single unit of configuration.

Represents the smallest actionable configuration element.

## ConfigurationUnit constructor

Creates an empty configuration unit for authoring purposes.

```C#
ConfigurationUnit();
```

## ConfigurationUnit properties

| Name | Description |
|-|-|
| UnitName | The name of the unit being configured; not a name for this instance. |
| InstanceIdentifier | An identifier used to uniquely identify the instance of a configuration unit on the system. |
| Identifier | The identifier name of this instance within the set. This value is referenced by other unit's `Dependencies`. |
| Dependencies | The identifiers of the configuration units that this unit depends on. |
| Directives | Contains the values that are for use by the configuration system, related to this unit. |
| Settings | Contains the values that are for use by the configuration unit itself. |
| Details | Contains information on the origin of the configuration unit. You must call `ConfigurationProcessor.Get*DetailsAsync` to populate this value. |
| State | The current state of the configuration unit. |
| ResultInformation | Contains information on the result of the latest attempt to apply the configuration unit. |
| ShouldApply | Allows for control over whether this unit should be applied when the set containing it is applied. |

## ConfigurationUnit.Directives known values

> _TODO: List of well known directives_

## ConfigurationSetChangeEventType enumeration

The change event type that has occurred for a configuration set change.

| Name | Description |
|-|-|
| Unknown | For future use if the caller is not aware of newer change types. |
| SetStateChanged | The state of the configuration set has changed. |
| UnitStateChanged | The state of a configuration unit has changed. |

## ConfigurationSetChangeData class

The change data sent about changes to a specific set.

This class is sent to subscribers of the `ConfigurationSet.ConfigurationSetChange` event, containing information
about the specific change that occurred.

## ConfigurationSet class

A configuration set contains a collection of configuration units and details about the set.

Represents a self contained group of configuration units that are operated on together.

## ConfigurationSet constructors

Creates an empty configuration set for authoring purposes.

```C#
ConfigurationSet();
```

Loads a configuration set from the given stream.

```C#
ConfigurationSet(Windows.Storage.Streams.IInputStream stream);
```

## ConfigurationSet.ConfigurationSetChange event

State changes for this set and it's units are sent to subscribers of this event.

```C#
event Windows.Foundation.TypedEventHandler<ConfigurationSet, ConfigurationSetChangeData> ConfigurationSetChange;
```

## ConfigurationSet.Serialize method

Serializes the configuration set to the given output stream.

```C#
void Serialize(Windows.Storage.Streams.IOutputStream stream);
```

## ConfigurationSet.Remove method

Removes the configuration set from the recorded history, if present.

```C#
void Remove();
```

You can use this method to remove a configuration set that is no longer relevant. For example, it may have been for
a repository that is no longer being used by the user and future conflicts with it's configuration are not important.

## ConfigurationSet properties

| Name | Description |
|-|-|
| Name | The name of the set; if from a file this could be the file name. |
| Origin | The origin of the set; if it came from a repository it could be the remote URL (ex. https://github.com/microsoft/winget-cli.git). |
| InstanceIdentifier | An identifier used to uniquely identify the instance of a configuration set on the system. |
| State | The state that the set is in. |
| InitialIntent | The time that this set was recorded with intent to apply. |
| ApplyBegun | The time that this set was last started to be applied. |
| ApplyEnded | The time that this set was last finished being applied (does not indicate success). |
| ConfigurationUnits | The configuration units that are part of this set. |

## IConfigurationUnitProcessor interface

Provides access to a specific configuration unit within the runtime.

This interface is the primary mechanism used to actually read and write configuration to the system,
but it is not expected that you would use this directly as a consumer of Microsoft.Management.Configuration.

## IConfigurationSetProcessor interface

Contains the lifetime of the processing action for a configuration set.

This interface is used to contain the lifetime of a processing action,
but it is not expected that you would use this directly as a consumer of Microsoft.Management.Configuration.

## IConfigurationProcessorFactory interface

Allows different runtimes to provide specialized handling of configuration processing.

It is not expected that you would use this interface directly, but rather the `ConfigurationProcessor` class.

_Spec note: A separate binary (written by us) will contain the implementation(s) of this interface._

## DiagnosticLevel enumeration

Indicates the importance of diagnostic information.

| Name | Description |
|-|-|
| Verbose | Most useful for debugging scenarios; likely too much for general use. |
| Informational | Details that can be useful for understanding what is happening. |
| Warning | Indicates some abnormal condition, but that is not expected to impact functionality. |
| Error | An error has occurred, but this does not necessarily mean that it will halt the operation. |
| Critical | A serious, fatal condition has been encountered. |

## DiagnosticInformation class

Contains diagnostic information from the configuration system that can be passed along to the user or log files.
This is not intended as primary information, and is thus not localized.

## ConfigurationConflictType enumeration

The type of conflict between configuration sets that was detected.

| Name | Description |
|-|-|
| Unknown | For future use if the caller is not aware of newer conflict types. |
| MatchingOrigin | Indicates that the first configuration set has a matching name and origin to the second, which has already been applied. |
| IdenticalSetApplied | Indicates that the first configuration set is identical to the second, which has already been applied. |
| SettingsConflict | Indicates a conflict between the settings of two configuration units. |

## ConfigurationConflictSetting class

Describes a conflict between a setting of two configuration units.

## ConfigurationConflict class

Describes a conflict between two configuration sets.

## ApplyConfigurationSetFlags enumeration

Flags to control how a configuration set should be applied to the system.

| Name | Description |
|-|-|
| None | The configuration set should be applied in the default manner. |
| DoNotOverwriteMatchingOriginSet | Forces a new configuration set instance to be recorded when the set being applied matches a previous set's origin. The default behavior is to assume that the incoming set is an update to the existing set and overwrite it. |

## ConfigurationChangeEventType enumeration

The configuration set change event type that has occurred.

| Name | Description |
|-|-|
| Unknown | For future use if the caller is not aware of newer change types. |
| SetAdded | A new configuration set was recorded in the history with the intent to be applied. |
| SetStateChanged | A configuration set has changed state. |
| SetRemoved | A configuration set has been removed from the history. |

## ConfigurationChangeEventType class

The change data sent about changes to sets.

## ConfigurationProcessor class

The configuration processor is responsible for the interactions with the system.

You must use this class to do anything beyond reading configuration sets. It is the entrypoint for all actions that
will interact with the actual system configuration.

## ConfigurationProcessor constructor

Creates a configuration processor using the given configuration factory.

```C#
ConfigurationProcessor(IConfigurationProcessorFactory factory);
```

> _TODO: Add details on the mechanics of creating the `IConfigurationProcessorFactory` objects that we provide._

## ConfigurationProcessor.CheckForConflicts(Async) method

Checks for conflicts amongst the configuration sets provided, optionally including the configuration sets already applied to the system.

```C#
Windows.Foundation.Collections.IVectorView<ConfigurationConflict> CheckForConflicts(Windows.Foundation.Collections.IVectorView<ConfigurationSet> configurationSets, Boolean includeConfigurationHistory);

Windows.Foundation.IAsyncOperation< Windows.Foundation.Collections.IVectorView<ConfigurationConflict> > CheckForConflictsAsync(Windows.Foundation.Collections.IVectorView<ConfigurationSet> configurationSets, Boolean includeConfigurationHistory);
```

This method should be used on any configuration set that is opened in order to determine if it would cause a conflict
with previously applied configurations. It should be called *after* setting the `Name` and `Origin` in order to determine
if it is a potential update.

## ConfigurationProcessor.GetSetDetails(Async) method

Gets the details for all configuration units in a set.

```C#
void GetSetDetails(ConfigurationSet configurationSet, ConfigurationUnitDetailLevel detailLevel);

Windows.Foundation.IAsyncAction GetSetDetailsAsync(ConfigurationSet configurationSet, ConfigurationUnitDetailLevel detailLevel);
```

This is a convenience/optimization method that will do the same thing as calling `GetUnitDetails(Async)` on each
configuration unit in the set. See `GetUnitDetails(Async)` for more information on what it will do.

## ConfigurationProcessor.GetUnitDetails(Async) method

Gets the details for all configuration units in a set.

```C#
void GetUnitDetails(ConfigurationUnit unit, ConfigurationUnitDetailLevel detailLevel);

Windows.Foundation.IAsyncAction GetUnitDetailsAsync(ConfigurationUnit unit, ConfigurationUnitDetailLevel detailLevel);
```

This method will get the details about a specific configuration unit and make them available via `ConfigurationUnit.Details`.
The `detailLevel` parameter allows control over how deeply to probe for details. It is an analog for the amount of
trust to place in the configuration unit processor.

## ConfigurationProcessor.ApplySet(Async) method

Applies the configuration set state to the system.

```C#
ApplyConfigurationSetResult ApplySet(ConfigurationSet configurationSet, ApplyConfigurationSetFlags flags);

Windows.Foundation.IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> ApplySetAsync(ConfigurationSet configurationSet, ApplyConfigurationSetFlags flags);
```

Using the async method and it's progress is more efficient than subscribing to the `ConfigurationSetChange` event before calling this method.

## ConfigurationProcessor.TestSet(Async) method

Tests if the system state matches the state described by the configuration set.

```C#
TestConfigurationSetResult TestSet(ConfigurationSet configurationSet);

Windows.Foundation.IAsyncOperationWithProgress<TestConfigurationSetResult, TestConfigurationUnitResult> TestSetAsync(ConfigurationSet configurationSet);
```

## ConfigurationProcessor.GetSettings(Async) method

Gets the current configuration unit settings from the system state.

```C#
GetConfigurationUnitSettingsResult GetSettings(ConfigurationUnit unit);

Windows.Foundation.IAsyncOperation<GetConfigurationUnitSettingsResult> GetSettingsAsync(ConfigurationUnit unit);
```

## ConfigurationProcessor.Diagnostics event

Enables listening to internal diagnostics events for logging purposes.

## ConfigurationProcessor.ConfigurationChange event

Signals changes to the set of configuration sets in the history, as well as changes to the state of configuration sets in the history.

## ConfigurationProcessor.GetConfigurationHistory method

Gets the configuration sets from the recorded history.

```C#
Windows.Foundation.Collections.IVectorView<ConfigurationSet> GetConfigurationHistory();
```

Gets the configuration sets that have already been applied or those recorded with the intent to be applied. This may include in progress sets or those that are waiting to be applied.

# API Details

[Link to the MIDL3 file.](../../src/Microsoft.Management.Configuration/Microsoft.Management.Configuration.idl)

# Appendix

<!-- TEMPLATE
  Anything else that you want to write down about implementation notes and for posterity,
  but that isn't necessary to understand the purpose and usage of the API.

  This or the Background section are a good place to describe alternative designs
  and why they were rejected.
-->

# Sample

This sample illustrates some of the expected usage patterns.

```C#
using Microsoft.Management.Configuration;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.Storage.Streams;

namespace ConfigurationSample
{
    internal static class Helpers
    {
        internal static IConfigurationProcessorFactory CreateIConfigurationProcessorFactory()
        {
            throw new NotImplementedException();
        }

        internal static ConfigurationSet OpenConfigurationSet(string filePath, ConfigurationProcessor processor)
        {
            var fileOperation = FileRandomAccessStream.OpenAsync(filePath, FileAccessMode.Read);
            fileOperation.AsTask().Wait();
            var file = fileOperation.GetResults();
            OpenConfigurationSetResult result = processor.OpenConfigurationSet(file);

            if (result.Set != null)
            {
                return result.Set;
            }

            Console.WriteLine($"Failed opening configuration set: 0x{result.ResultCode:X} at {result.Field}");
            return null;
        }

        internal static void SetWatcher(ConfigurationSet set, ConfigurationSetChangeData data)
        {
            Console.WriteLine($"  - Set: {set.Name} [{set.InstanceIdentifier}]");
            Console.WriteLine($"    Change: {data.Change}");
            Console.WriteLine($"    Set State: {data.SetState}");
            switch (data.Change)
            {
                case ConfigurationSetChangeEventType.UnitStateChanged:
                    Console.WriteLine($"    Unit: {data.Unit.UnitName} [{data.Unit.InstanceIdentifier}]");
                    Console.WriteLine($"    Unit State: {data.UnitState}");
                    if (data.UnitState == ConfigurationUnitState.Completed && data.ResultInformation.ResultCode != null)
                    {
                        Console.WriteLine($"    Failure: {data.ResultInformation.Description} [{data.ResultInformation.ResultCode.HResult}]");
                    }
                    break;
            }
        }
    }

    internal class ApplyProgressWatcher
    {
        private bool isFirstProgress = true;

        internal void Watcher(IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> operation, ConfigurationSetChangeData data)
        {
            if (isFirstProgress)
            {
                isFirstProgress = false;

                // If our first progress callback contains partial results, output them as if they had been called through progress
                ApplyConfigurationSetResult partialResult = operation.GetResults();

                foreach (ApplyConfigurationUnitResult unitResult in partialResult.UnitResults)
                {
                    HandleUnitProgress(unitResult.Unit, unitResult.State, unitResult.ResultInformation);
                }
            }

            switch (data.Change)
            {
                case ConfigurationSetChangeEventType.SetStateChanged:
                    Console.WriteLine($"  - Set State: {data.SetState}");
                    break;
                case ConfigurationSetChangeEventType.UnitStateChanged:
                    HandleUnitProgress(data.Unit, data.UnitState, data.ResultInformation);
                    break;
            }
        }

        private void HandleUnitProgress(ConfigurationUnit unit, ConfigurationUnitState state, ConfigurationUnitResultInformation resultInformation)
        {
            switch (state)
            {
                case ConfigurationUnitState.Pending:
                    break;
                case ConfigurationUnitState.InProgress:
                case ConfigurationUnitState.Completed:
                case ConfigurationUnitState.Skipped:
                    Console.WriteLine($"  - Unit: {unit.UnitName} [{unit.InstanceIdentifier}]");
                    Console.WriteLine($"    Unit State: {state}");
                    if (resultInformation.ResultCode != null)
                    {
                        Console.WriteLine($"    HRESULT: [0x{resultInformation.ResultCode.HResult:X8}]");
                        Console.WriteLine($"    Reason: {resultInformation.Description}");
                    }
                    break;
                case ConfigurationUnitState.Unknown:
                    break;
            }
        }
    }

    internal class Program
    {
        static void LoadAndOutput(string[] args)
        {
            ConfigurationProcessor processor = new ConfigurationProcessor(Helpers.CreateIConfigurationProcessorFactory());

            // Open the given configuration file
            ConfigurationSet configSet = Helpers.OpenConfigurationSet(args[1], processor);
            if (configSet == null)
            {
                return;
            }

            // Output some of the information from the set
            Console.WriteLine($"Configuration Set: {args[1]}");

            foreach (ConfigurationUnit unit in configSet.ConfigurationUnits)
            {
                Console.WriteLine($"  - Configuration Unit: {unit.UnitName}");
                if (!string.IsNullOrEmpty(unit.Identifier))
                {
                    Console.WriteLine($"    Identifier: {unit.Identifier}");
                }
                Console.WriteLine($"    Intent: {unit.Intent}");
                IReadOnlyList<string> dependencies = unit.Dependencies;
                if (dependencies.Count > 0)
                {
                    Console.WriteLine("    Dependencies:");
                    foreach (string dependency in dependencies)
                    {
                        Console.WriteLine($"      {dependency}");
                    }
                }
                ValueSet directives = unit.Directives;
                if (directives.Count > 0)
                {
                    Console.WriteLine("    Directives:");
                    foreach (var directive in unit.Directives)
                    {
                        Console.WriteLine($"      {directive.Key}: {directive.Value}");
                    }
                }
                ValueSet settings = unit.Settings;
                if (settings.Count > 0)
                {
                    Console.WriteLine("    Settings:");
                    foreach (var setting in unit.Settings)
                    {
                        Console.WriteLine($"      {setting.Key}: {setting.Value}");
                    }
                }
            }
        }

        static void LoadAndCheckConflicts(string[] args)
        {
            // Create the factory and processor
            ConfigurationProcessor processor = new ConfigurationProcessor(Helpers.CreateIConfigurationProcessorFactory());

            // Open the given configuration file
            ConfigurationSet configSet = Helpers.OpenConfigurationSet(args[1], processor);
            if (configSet == null)
            {
                return;
            }

            // Set a name and origin for this set so that we can see it in the conflict info
            configSet.Name = Path.GetFileName(args[1]);
            configSet.Origin = args[1];

            // Check for conflicts with existing configurations
            List<ConfigurationSet> configSets = new List<ConfigurationSet>() { configSet };
            IList<ConfigurationConflict> conflicts = processor.CheckForConflicts(configSets, true);

            Console.WriteLine($"Conflicts with Configuration Set: {args[1]}");

            foreach (ConfigurationConflict conflict in conflicts)
            {
                Console.WriteLine($"  - Conflict: {conflict.Conflict}");
                Console.WriteLine($"    First Set: {conflict.FirstSet.Name} [{conflict.FirstSet.Origin}]");
                Console.WriteLine($"    Second Set: {conflict.SecondSet.Name} [{conflict.SecondSet.Origin}]");
                if (conflict.Conflict == ConfigurationConflictType.SettingsConflict)
                {
                    Console.WriteLine($"    First Unit: {conflict.FirstUnit.UnitName} [{conflict.FirstUnit.InstanceIdentifier}]");
                    Console.WriteLine($"    Second Unit: {conflict.SecondUnit.UnitName} [{conflict.SecondUnit.InstanceIdentifier}]");
                    foreach (ConfigurationConflictSetting setting in conflict.Settings)
                    {
                        Console.WriteLine($"    - Setting: {setting.Name}");
                        Console.WriteLine($"      First Value: {setting.FirstValue}");
                        Console.WriteLine($"      Second Value: {setting.SecondValue}");
                    }
                }
            }
        }

        static void LoadAndApply(string[] args)
        {
            // Create the factory and processor
            ConfigurationProcessor processor = new ConfigurationProcessor(Helpers.CreateIConfigurationProcessorFactory());

            // Open the given configuration file
            ConfigurationSet configSet = Helpers.OpenConfigurationSet(args[1], processor);
            if (configSet == null)
            {
                return;
            }

            Console.WriteLine($"Applying Configuration Set: {args[1]}");

            ApplyProgressWatcher watcher = new ApplyProgressWatcher();

            var operation = processor.ApplySetAsync(configSet, ApplyConfigurationSetFlags.None);
            operation.Progress = watcher.Watcher;
            operation.AsTask().Wait();
            ApplyConfigurationSetResult result = operation.GetResults();

            Console.WriteLine($"  - Done: {result.ResultCode.HResult}");
        }

        static void GetHistoryAndWatchEverything(string[] args)
        {
            Console.WriteLine("Watching all configuration [press Enter to stop]:");

            // Create the factory and processor
            ConfigurationProcessor processor = new ConfigurationProcessor(Helpers.CreateIConfigurationProcessorFactory());

            List<ConfigurationSet> list = new List<ConfigurationSet>();

            // Attach to the top level change event
            processor.ConfigurationChange += (ConfigurationSet incomingSet, ConfigurationChangeData data) =>
            {
                int existingSetIndex = -1;

                lock (list)
                {
                    for (int i = 0; i < list.Count; ++i)
                    {
                        if (list[i].InstanceIdentifier == data.InstanceIdentifier)
                        {
                            existingSetIndex = i;
                            break;
                        }
                    }

                    if (data.Change == ConfigurationChangeEventType.SetAdded || data.Change == ConfigurationChangeEventType.SetStateChanged)
                    {
                        if (existingSetIndex == -1)
                        {
                            incomingSet.ConfigurationSetChange += Helpers.SetWatcher;
                            list.Add(incomingSet);
                        }
                    }
                    else // Removed
                    {
                        if (existingSetIndex != -1)
                        {
                            list[existingSetIndex].ConfigurationSetChange -= Helpers.SetWatcher;
                            list.RemoveAt(existingSetIndex);
                        }
                    }
                }

                Console.WriteLine($"  - Set: {data.InstanceIdentifier}");
                Console.WriteLine($"    Change: {data.Change}");
            };

            foreach (ConfigurationSet set in processor.GetConfigurationHistory())
            {
                int existingSetIndex = -1;

                lock (list)
                {
                    for (int i = 0; i < list.Count; ++i)
                    {
                        if (list[i].InstanceIdentifier == set.InstanceIdentifier)
                        {
                            existingSetIndex = i;
                            break;
                        }
                    }

                    if (existingSetIndex == -1)
                    {
                        set.ConfigurationSetChange += Helpers.SetWatcher;
                        list.Add(set);
                    }
                }

                if (existingSetIndex == -1)
                {
                    Console.WriteLine($"  - Set: {set.Name} [{set.InstanceIdentifier}]");
                    Console.WriteLine($"    State: {set.State}");
                }
            }

            // Wait for user to press enter
            Console.ReadLine();
        }

        static void Main(string[] args)
        {
            var method = typeof(Program).GetMethod(args[0], BindingFlags.NonPublic | BindingFlags.Static);

            if (method != null)
            {
                method.Invoke(null, new object[]{ args });
            }
            else
            {
                Console.WriteLine($"{args[0]} is not a sample");
            }
        }
    }
}

```
