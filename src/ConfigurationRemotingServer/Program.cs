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
            ulong memoryHandle = ulong.Parse(args[0]);

            try
            {
                ulong initEventHandle = ulong.Parse(args[1]);
                ulong completionEventHandle = ulong.Parse(args[2]);
                ulong parentProcessHandle = ulong.Parse(args[3]);

                PowerShellConfigurationSetProcessorFactory factory = new PowerShellConfigurationSetProcessorFactory();

                IObjectReference factoryInterface = MarshalInterface<global::Microsoft.Management.Configuration.IConfigurationSetProcessorFactory>.CreateMarshaler(factory);

                return WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(0, factoryInterface.ThisPtr, memoryHandle, initEventHandle, completionEventHandle, parentProcessHandle);
            }
            catch(Exception ex)
            {
                WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(ex.HResult, IntPtr.Zero, memoryHandle, 0, 0, 0);
                return ex.HResult;
            }
        }

        [DllImport("WindowsPackageManager.dll")]
        private static extern int WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(int result, IntPtr factory, ulong memoryHandle, ulong initEventHandle, ulong completionMutexHandle, ulong parentProcessHandle);
    }
}