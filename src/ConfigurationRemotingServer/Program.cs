// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.Management.Configuration.Processor;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
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
        static int Main(string[] args)
        {
            // Help find WindowsPackageManager.dll
            AssemblyLoadContext.Default.ResolvingUnmanagedDll += NativeAssemblyLoadContext.ResolvingUnmanagedHandler;

            string staticsCallback = args[1];

            try
            {
                string completionEventName = args[2];
                uint parentProcessId = uint.Parse(args[3]);

                PowerShellConfigurationSetProcessorFactory factory = new PowerShellConfigurationSetProcessorFactory();

                IObjectReference factoryInterface = MarshalInterface<global::Microsoft.Management.Configuration.IConfigurationSetProcessorFactory>.CreateMarshaler(factory);

                return WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(0, factoryInterface.ThisPtr, staticsCallback, completionEventName, parentProcessId);
            }
            catch(Exception ex)
            {
                WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(ex.HResult, IntPtr.Zero, staticsCallback, null, 0);
                return ex.HResult;
            }
        }

        [DllImport("WindowsPackageManager.dll")]
        private static extern int WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(
            int result,
            IntPtr factory,
            [MarshalAs(UnmanagedType.LPWStr)]string staticsCallback,
            [MarshalAs(UnmanagedType.LPWStr)]string? completionEventName,
            uint parentProcessId);
    }
}
