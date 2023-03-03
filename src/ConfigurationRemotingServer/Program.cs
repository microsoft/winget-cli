using Microsoft.Management.Configuration.Processor;
using System.Runtime.InteropServices;
using WinRT;

namespace ConfigurationRemotingServer
{
    internal class Program
    {
        private static readonly Guid IID_IConfigurationSetProcessorFactory = new Guid("C73A3D5B-E5DA-5C4A-A257-7B343C354591");

        static int Main(string[] args)
        {
            ulong memoryHandle = ulong.Parse(args[0]);

            try
            {
                ulong initEventHandle = ulong.Parse(args[1]);
                ulong completionEventHandle = ulong.Parse(args[2]);

                ConfigurationSetProcessorFactory factory = new ConfigurationSetProcessorFactory(ConfigurationProcessorType.Hosted, null);
                IObjectReference factoryInterface = ABI.Microsoft.Management.Configuration.Processor.ConfigurationSetProcessorFactory.CreateMarshaler(factory).As(IID_IConfigurationSetProcessorFactory);

                return WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(0, factoryInterface.ThisPtr, memoryHandle, initEventHandle, completionEventHandle);
            }
            catch(Exception ex)
            {
                WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(ex.HResult, IntPtr.Zero, memoryHandle, 0, 0);
                return ex.HResult;
            }
        }

        [DllImport("WindowsPackageManager.dll")]
        private static extern int WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization(int result, IntPtr factory, ulong memoryHandle, ulong initEventHandle, ulong completionMutexHandle);
    }
}