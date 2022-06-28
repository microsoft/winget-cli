using Microsoft.Management.Deployment;

namespace WinGetProjection
{
    public class WinGetProjectionFactory
    {

        public WinGetProjectionFactory(IInstanceInitializer instanceInitializer)
        {
            InstanceInitializer = instanceInitializer;
        }

        private IInstanceInitializer InstanceInitializer { get; set; }
        
        public PackageManager CreatePackageManager() => InstanceInitializer.CreateInstance<PackageManager>();
    }
}
