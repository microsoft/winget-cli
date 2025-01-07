// -----------------------------------------------------------------------------
// <copyright file="OutOfProcAttribute.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using Xunit.Sdk;

    /// <summary>
    /// Trait used to mark a test as being able to run against the out of proc server.
    /// </summary>
    [TraitDiscoverer(OutOfProcDiscoverer.TypeName, Constants.AssemblyNameForTraits)]
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method, AllowMultiple = false)]
    public class OutOfProcAttribute : Attribute, ITraitAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OutOfProcAttribute"/> class.
        /// </summary>
        public OutOfProcAttribute()
        {
            // To run the tests OOP, you need to replace Microsoft.Management.Configuration.dll with Microsoft.Management.Configuration.OutOfProc.dll (renamed to remove the OutOfProc).
            // You will also need to copy over Microsoft.Management.Configuration.winmd as it is needed by COM.
            //
            // You can use the script to do this:
            //  <git root>\src\Microsoft.Management.Configuration.OutOfProc\Prepare-ConfigurationOOPTests.ps1 -BuildOutputPath <git root>\src\x64\Debug
            //
            // It can be easier to run the tests on the command line because any changes needing a recompile will overwrite the DLL update above.
            // The test runner is located somewhere like this:
            //  C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\Extensions\TestPlatform
            // and the command line from there is:
            //  .\vstest.console.exe "<location of your repo>\src\x64\Debug\Microsoft.Management.Configuration.UnitTests\net8.0-windows10.0.22000.0\Microsoft.Management.Configuration.UnitTests.dll" --TestCaseFilter:Category=OutOfProc
        }
    }
}
