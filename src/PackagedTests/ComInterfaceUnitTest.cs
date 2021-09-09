
using System;
using System.Collections.Generic;
using System.Diagnostics;
using Microsoft.Management.Deployment;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace PackagedUnitTests
{
    [TestClass]
    public class ComInterfaceUnitTests
    {
        [AssemblyInitialize]
        public static void AssemblyInitialize(TestContext context)
        {
            string path = context.Properties["AICLIPackagePath"].ToString();
            Microsoft.VisualStudio.TestTools.UnitTesting.Logging.Logger.LogMessage("Checking for package");
            Windows.Management.Deployment.PackageManager pm = new Windows.Management.Deployment.PackageManager();
            var packages = pm.FindPackagesForUser("", "WinGetDevCLI_8wekyb3d8bbwe");
            var enumerator = packages.GetEnumerator();
            bool hasMoreItems = enumerator.MoveNext();
            if (hasMoreItems)
            {
                Microsoft.VisualStudio.TestTools.UnitTesting.Logging.Logger.LogMessage("Found package already installed");
            }
            Uri uri = new Uri(path);
            Microsoft.VisualStudio.TestTools.UnitTesting.Logging.Logger.LogMessage("Adding package: " + path);
            var task = pm.AddPackageAsync(uri, null, Windows.Management.Deployment.DeploymentOptions.None).AsTask();
            task.Wait();
            var result = task.Result;
            if (result.ExtendedErrorCode != null)
            {
                Microsoft.VisualStudio.TestTools.UnitTesting.Logging.Logger.LogMessage("AddPackage result: " + result.ExtendedErrorCode + " " + result.ErrorText);
                throw result.ExtendedErrorCode;
            }
        }

        [AssemblyCleanup]
        public static void AssemblyCleanup()
        {
            Windows.Management.Deployment.PackageManager pm = new Windows.Management.Deployment.PackageManager();
            var packages = pm.FindPackagesForUser("", "WinGetDevCLI_8wekyb3d8bbwe");
            var enumerator = packages.GetEnumerator();
            bool hasMoreItems = enumerator.MoveNext();
            if (hasMoreItems)
            {
                string fullName = enumerator.Current.Id.FullName;
                Microsoft.VisualStudio.TestTools.UnitTesting.Logging.Logger.LogMessage("Removing package: " + fullName);
                var task = pm.RemovePackageAsync(fullName).AsTask();
                task.Wait();
                var result = task.Result;
                if (result.ExtendedErrorCode != null)
                {
                    Microsoft.VisualStudio.TestTools.UnitTesting.Logging.Logger.LogMessage("RemovePackage result: " + result.ExtendedErrorCode + " " + result.ErrorText);
                    throw result.ExtendedErrorCode;
                }
            }
        }

        [TestMethod]
        public void OpenPredefinedCatalog()
        {
            PackageManager packageManager = new PackageManager();
            PackageCatalogReference catalogRef = packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.OpenWindowsCatalog);
            Assert.IsTrue(catalogRef.Info.Name.Equals("winget"));
            IReadOnlyList<PackageCatalogReference> catalogs = packageManager.GetPackageCatalogs();
            Assert.IsTrue(catalogs.Count > 0);
            bool foundDefaultCatalog = false;
            for(int i = 0; i < catalogs.Count; i++)
            {
                if (catalogs[i].Info.Name.Equals("winget"))
                {
                    foundDefaultCatalog = true;
                    break;
                }
            }
            Assert.IsTrue(foundDefaultCatalog);
        }

        [TestMethod]
        public void OpenPredefinedStoreCatalog()
        {
            PackageManager packageManager = new PackageManager();
            PackageCatalogReference catalogRef = packageManager.GetPredefinedPackageCatalog(PredefinedPackageCatalog.MicrosoftStore);
            Assert.IsTrue(catalogRef.Info.Name.Equals("storepreview"));
        }
    }
}
