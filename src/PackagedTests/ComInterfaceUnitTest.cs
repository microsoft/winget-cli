
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
            Assert.IsTrue(catalogRef.Info.Name.Equals("msstore"));
        }
    }
}
