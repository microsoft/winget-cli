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

                // Assume that the additional modules path is a sibling directory to the one containing this binary
                string assemblyDirectory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) ?? throw new InvalidDataException();
                string rootDirectory = Path.GetDirectoryName(assemblyDirectory) ?? throw new InvalidDataException(); ;
                string modulesPath = Path.Combine(rootDirectory, "ExternalModules");

                PowerShellConfigurationSetProcessorFactory factory = new PowerShellConfigurationSetProcessorFactory();

                IPowerShellConfigurationProcessorFactoryProperties properties = factory.As<IPowerShellConfigurationProcessorFactoryProperties>();
                properties.AdditionalModulePaths = new List<string>() { modulesPath };
                // This can be RemoteSigned eventually or keep it Unrestricted for dev builds.
                properties.Policy = PowerShellConfigurationProcessorPolicy.Unrestricted;
                properties.ProcessorType = PowerShellConfigurationProcessorType.Hosted;

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