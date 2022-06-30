namespace WinGetProjection
{
    using System;
    using System.Diagnostics;
    using Microsoft.Management.Deployment;

    internal class Examples
    {
        public static void RunExample(Action runExample)
        {
            try
            {
                runExample();
                Console.WriteLine("\tSUCCESS");
            }
            catch (Exception ex)
            {
                for (; ex != null; ex = ex.InnerException)
                {
                    Console.WriteLine($"\t{ex.Message}");
                }
                Console.WriteLine("\tFAIL");
            }
        }

        public static void Main()
        {
            IInstanceInitializer initializer;
            WinGetProjectionFactory factory;
            PackageManager packageManager;

            RunExample(() =>
            {
                Console.WriteLine("Activation factory initializer example");
                initializer = new ActivationFactoryInitializer();
                factory = new WinGetProjectionFactory(initializer);
                factory.CreatePackageManager();
            });

            RunExample(() =>
            {
                Console.WriteLine("Application manifest initializer example");
                initializer = new ApplicationManifestInitializer();
                factory = new WinGetProjectionFactory(initializer);
                packageManager = factory.CreatePackageManager();
                Debug.Assert(packageManager != null);
            });

            RunExample(() =>
            {
                Console.WriteLine("Class object initializer example");
                initializer = new ClassObjectInitializer("Microsoft.Management.Deployment.InProc.dll");
                factory = new WinGetProjectionFactory(initializer);
                packageManager = factory.CreatePackageManager();
                Debug.Assert(packageManager != null);
            });

            RunExample(() =>
            {
                Console.WriteLine("Local server initializer example");
                initializer = new LocalServerInitializer();
                factory = new WinGetProjectionFactory(initializer);
                packageManager = factory.CreatePackageManager();
                Debug.Assert(packageManager != null);
            });
        }
    }
}
