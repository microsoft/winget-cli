namespace WinGetProjection
{
    using System;

    internal class Program
    {
        public static void Main()
        {
            var init = new ClassObjectInitializer("Microsoft.Management.Deployment.dll");
            var factory = new WinGetProjectionFactory(init);
            var pm = factory.CreatePackageManager();
            var pc = pm.GetPackageCatalogs();

            Console.WriteLine("Hello world");
        }
    }
}
