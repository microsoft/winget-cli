// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;
    using Microsoft.Management.Deployment;

    public class ComInterfaceTests : BaseCommand
    {
        [Test]
        public void FindDefaultCatalog()
        {
            //int i = 0;
            //while (i < 1000)
            //{
                //try
                //{
                    PackageManager packageManager = new PackageManager();
                    var catalogs = packageManager.GetPackageCatalogs();
                    Assert.True(catalogs.Count > 0);
                    bool foundDefaultCatalog = false;
                    foreach (var catalog in catalogs)
                    {
                        if (catalog.Info.Name.Equals("winget"))
                        {
                            foundDefaultCatalog = true;
                            break;
                        }
                    }
                    Assert.True(foundDefaultCatalog);
                //}
                //catch (System.Exception)
                //{

                //}
                //    i++;
            //}
        }
    }
}
