namespace WinGetProjection
{
    using System;

    internal class Program
    {
        public static void Main()
        {
            var init = new ActivationFactoryInitializer();
            var factory = new WinGetProjectionFactory(init);
            var pm = factory.CreatePackageManager();
            var pc = pm.GetPackageCatalogs();

            Console.WriteLine("Hello world");
        }
    }
}
