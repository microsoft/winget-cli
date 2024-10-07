// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Management.Configuration;
using Microsoft.Management.Configuration.Processor;
using WinRT;

namespace ConfigurationRemotingServer
{
    /// <summary>
    /// Custom assembly load context.
    /// </summary>
    internal class NativeAssemblyLoadContext : AssemblyLoadContext
    {
        private static readonly string PackageRootPath;

        private static readonly NativeAssemblyLoadContext NativeALC = new();

        static NativeAssemblyLoadContext()
        {
            var self = typeof(NativeAssemblyLoadContext).Assembly;
            PackageRootPath = Path.Combine(
                Path.GetDirectoryName(self.Location)!,
                "..");
        }

        private NativeAssemblyLoadContext()
            : base("NativeAssemblyLoadContext", isCollectible: false)
        {
        }

        /// <summary>
        /// Handler to resolve unmanaged assemblies.
        /// </summary>
        /// <param name="context">Assembly load context.</param>
        /// <param name="name">Assembly name.</param>
        /// <returns>The assembly, null if not in our assembly location.</returns>
        internal static IntPtr ResolvingUnmanagedHandler(Assembly context, string name)
        {
            if (name.Equals("WindowsPackageManager.dll", StringComparison.OrdinalIgnoreCase))
            {
                return NativeALC.LoadUnmanagedDll(name);
            }

            return IntPtr.Zero;
        }

        /// <inheritdoc/>
        protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
        {
            string path = Path.Combine(PackageRootPath, unmanagedDllName);
            if (File.Exists(path))
            {
                return this.LoadUnmanagedDllFromPath(path);
            }

            return IntPtr.Zero;
        }
    }

    internal class Program
    {
        private const string CommandLineSectionSeparator = "~~~~~~";
        private const string ExternalModulesName = "ExternalModules";

        static int Main(string[] args)
        {
            // Remove any attached console to prevent modules (or their actions) from writing to our console.
            FreeConsole();

            // Help find WindowsPackageManager.dll
            AssemblyLoadContext.Default.ResolvingUnmanagedDll += NativeAssemblyLoadContext.ResolvingUnmanagedHandler;

            string staticsCallback = args[1];

            try
            {
                string completionEventName = args[2];
                uint parentProcessId = uint.Parse(args[3]);

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
                    MemoryStream memoryStream = new MemoryStream();
                    memoryStream.Write(limitationSetBytes);
                    memoryStream.Flush();
                    memoryStream.Seek(0, SeekOrigin.Begin);
                    ConfigurationProcessor processor = new ConfigurationProcessor(factory);
                    var limitationSetResult = processor.OpenConfigurationSet(memoryStream.AsInputStream());
                    memoryStream.Close();

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

                        if (metadataJson.ModulePath != null)
                        {
                            PowerShellConfigurationProcessorLocation parsedLocation = PowerShellConfigurationProcessorLocation.Default;
                            if (Enum.TryParse<PowerShellConfigurationProcessorLocation>(metadataJson.ModulePath, out parsedLocation))
                            {
                                factory.Location = parsedLocation;
                            }
                            else
                            {
                                factory.Location = PowerShellConfigurationProcessorLocation.Custom;
                                factory.CustomLocation = metadataJson.ModulePath;
                            }
                        }
                    }

                    // Set the limitation set in factory.
                    factory.LimitationSet = limitationSet;
                }

                IObjectReference factoryInterface = MarshalInterface<global::Microsoft.Management.Configuration.IConfigurationSetProcessorFactory>.CreateMarshaler(factory);

                return WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(0, factoryInterface.ThisPtr, staticsCallback, completionEventName, parentProcessId);
            }
            catch(Exception ex)
            {
                WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(ex.HResult, IntPtr.Zero, staticsCallback, null, 0);
                return ex.HResult;
            }
        }

        private class LimitationSetMetadata
        {
            [JsonPropertyName("path")]
            public string Path { get; set; } = string.Empty;

            [JsonPropertyName("modulePath")]
            public string? ModulePath { get; set; } = null;
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
        private static extern int WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(
            int result,
            IntPtr factory,
            [MarshalAs(UnmanagedType.LPWStr)]string staticsCallback,
            [MarshalAs(UnmanagedType.LPWStr)]string? completionEventName,
            uint parentProcessId);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr GetCommandLineW();

        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool FreeConsole();
    }
}
