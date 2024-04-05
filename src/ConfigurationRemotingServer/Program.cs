// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Management.Configuration;
using Microsoft.Management.Configuration.Processor;
using Windows.Storage.Streams;
using WinRT;

namespace ConfigurationRemotingServer
{
    internal class Program
    {
        private const string CommandLineSectionSeparator = "~~~~~~";
        private const string ExternalModulesName = "ExternalModules";

        static int Main(string[] args)
        {
            ulong memoryHandle = ulong.Parse(args[0]);

            try
            {
                ulong initEventHandle = ulong.Parse(args[1]);
                ulong completionEventHandle = ulong.Parse(args[2]);
                ulong parentProcessHandle = ulong.Parse(args[3]);

                PowerShellConfigurationSetProcessorFactory factory = new PowerShellConfigurationSetProcessorFactory();

                // Set default properties.
                var externalModulesPath = GetExternalModulesPath();
                if (string.IsNullOrWhiteSpace(externalModulesPath))
                {
                    throw new DirectoryNotFoundException("Failed to get ExternalModules.");
                }

                // Set as implicit module paths so it will be always included in AdditionalModulePaths
                factory.ImplicitModulePaths = new List<string>() { externalModulesPath };
                factory.ProcessorType = PowerShellConfigurationProcessorType.Hosted;

                // Parse limitation set if applicable.
                // The format will be:
                //     <Common args for initialization> ~~~~~~ <Metadata json> ~~~~~~ <Limitation Set in yaml>
                // Metadata json format:
                //     {
                //         "path": "C:\full\file\path.yaml"
                //     }
                // If a limitation set is provided, the processor will be limited
                // to only work on units defined inside the limitation set.
                var commandPtr = GetCommandLineW();
                var commandStr = Marshal.PtrToStringUni(commandPtr) ?? string.Empty;

                // In case the limitation set content contains the separator, we'll not use Split method.
                var firstSeparatorIndex = commandStr.IndexOf(CommandLineSectionSeparator);
                if (firstSeparatorIndex > 0)
                {
                    var secondSeparatorIndex = commandStr.IndexOf(CommandLineSectionSeparator, firstSeparatorIndex + CommandLineSectionSeparator.Length);
                    if (secondSeparatorIndex <= 0)
                    {
                        throw new ArgumentException("The input command contains only one separator string.");
                    }

                    // Parse limitation set.
                    byte[] limitationSetBytes = Encoding.UTF8.GetBytes(commandStr.Substring(secondSeparatorIndex + CommandLineSectionSeparator.Length));
                    InMemoryRandomAccessStream limitationSetStream = new InMemoryRandomAccessStream();
                    DataWriter streamWriter = new DataWriter(limitationSetStream);
                    streamWriter.WriteBytes(limitationSetBytes);
                    streamWriter.StoreAsync().GetAwaiter().GetResult();
                    streamWriter.DetachStream();
                    limitationSetStream.Seek(0);
                    ConfigurationProcessor processor = new ConfigurationProcessor(factory);
                    var limitationSetResult = processor.OpenConfigurationSet(limitationSetStream);
                    if (limitationSetResult.ResultCode != null)
                    {
                        throw limitationSetResult.ResultCode;
                    }

                    var limitationSet = limitationSetResult.Set;
                    if (limitationSet == null)
                    {
                        throw new ArgumentException("The limitation set cannot be parsed.");
                    }

                    // Now parse metadata json and update the limitation set
                    var metadataJson = JsonSerializer.Deserialize<LimitationSetMetadata>(commandStr.Substring(
                        firstSeparatorIndex + CommandLineSectionSeparator.Length,
                        secondSeparatorIndex - firstSeparatorIndex - CommandLineSectionSeparator.Length));

                    if (metadataJson != null)
                    {
                        limitationSet.Path = metadataJson.Path;
                    }

                    // Set the limitation set in factory.
                    factory.LimitationSet = limitationSet;
                }

                IObjectReference factoryInterface = MarshalInterface<global::Microsoft.Management.Configuration.IConfigurationSetProcessorFactory>.CreateMarshaler(factory);

                return WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(0, factoryInterface.ThisPtr, memoryHandle, initEventHandle, completionEventHandle, parentProcessHandle);
            }
            catch(Exception ex)
            {
                WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(ex.HResult, IntPtr.Zero, memoryHandle, 0, 0, 0);
                return ex.HResult;
            }
        }

        private class LimitationSetMetadata
        {
            [JsonPropertyName("path")]
            public string Path { get; set; } = string.Empty;
        }

        private static string GetExternalModulesPath()
        {
            var currentAssemblyDirectoryPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            if (currentAssemblyDirectoryPath != null)
            {
                var packageRootPath = Directory.GetParent(currentAssemblyDirectoryPath)?.FullName;
                if (packageRootPath != null)
                {
                    var externalModulesPath = Path.Combine(packageRootPath, ExternalModulesName);
                    if (Directory.Exists(externalModulesPath))
                    {
                        return externalModulesPath;
                    }
                }
            }

            return string.Empty;
        }

        [DllImport("WindowsPackageManager.dll")]
        private static extern int WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(int result, IntPtr factory, ulong memoryHandle, ulong initEventHandle, ulong completionMutexHandle, ulong parentProcessHandle);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr GetCommandLineW();
    }
}
