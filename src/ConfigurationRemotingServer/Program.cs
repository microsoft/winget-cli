// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.Management.Configuration.Processor;
using System.Reflection;
using System.Runtime.InteropServices;
using WinRT;

namespace ConfigurationRemotingServer
{
    internal class Program
    {
        static int Main(string[] args)
        {
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
