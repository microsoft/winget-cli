// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Sample demonstrating the ConnectionValidationHandler on PackageCatalogReference.
//
// This sample retrieves a named package catalog, installs a connection validation
// callback that shows certificate information, prompts the user to accept or reject
// the certificate, then reports the Connect() result.
//
// Usage: ConnectionValidationSample <CatalogName>
// Example: ConnectionValidationSample winget
//
// Requirements:
//   - Must run in-process (the ConnectionValidationHandler setter rejects out-of-proc callers).
//   - The Microsoft.Management.Deployment COM server must be registered (deploy the dev package
//     or install WinGet from the Microsoft Store).

using Microsoft.Management.Deployment;
using Windows.Security.Cryptography.Certificates;

if (args.Length == 0)
{
    Console.Error.WriteLine("Usage: ConnectionValidationSample <CatalogName>");
    Console.Error.WriteLine("Example: ConnectionValidationSample winget");
    return 1;
}

string catalogName = args[0];

// Use in-proc activation so that ConnectionValidationHandler can be set.
// CsWinRT activates PackageManager in-proc via DllGetActivationFactory from
// Microsoft.Management.Deployment.dll placed alongside this executable.
var packageManager = new PackageManager();

var catalogRef = packageManager.GetPackageCatalogByName(catalogName);
if (catalogRef is null)
{
    Console.Error.WriteLine($"No catalog named '{catalogName}' found.");
    Console.Error.WriteLine("Use 'winget source list' to see available catalogs.");
    return 1;
}

Console.WriteLine($"Catalog: {catalogRef.Info.Name}  ({catalogRef.Info.Argument})");
Console.WriteLine("Setting connection validation handler...");

catalogRef.ConnectionValidationHandler = (PackageCatalogConnectionValidationEventArgs validationArgs) =>
{
    Certificate cert = validationArgs.ServerCertificate;

    Console.WriteLine();
    Console.WriteLine("Catalog connection validation for: " + catalogRef.Info.Name);
    Console.WriteLine("  at: " + catalogRef.Info.Argument);
    Console.WriteLine();
    Console.WriteLine("=== Server Certificate ===");
    Console.WriteLine($"  Subject:    {cert.Subject}");
    Console.WriteLine($"  Issuer:     {cert.Issuer}");
    Console.WriteLine($"  Valid from: {cert.ValidFrom}");
    Console.WriteLine($"  Valid to:   {cert.ValidTo}");
    Console.WriteLine($"  Serial:     {cert.SerialNumber}");
    Console.WriteLine("==========================");
    Console.WriteLine();
    Console.Write("Accept this certificate? [Y/N]: ");

    while (true)
    {
        string? input = Console.ReadLine()?.Trim().ToUpperInvariant();
        if (input == "Y")
        {
            Console.WriteLine("Certificate accepted.");
            return PackageCatalogConnectionValidationResult.Ok;
        }
        else if (input == "N")
        {
            Console.WriteLine("Certificate rejected.");
            return PackageCatalogConnectionValidationResult.CertificateRejected;
        }
        else
        {
            Console.Write("Please enter Y or N: ");
        }
    }
};

Console.WriteLine("Connecting...");
ConnectResult result = catalogRef.Connect();

Console.WriteLine();
Console.WriteLine($"Connect result: {result.Status}");

if (result.Status == ConnectResultStatus.Ok)
{
    Console.WriteLine($"Successfully connected to '{catalogName}'.");

    // Run a simple search to force a live network call, since Connect() may serve a cached response.
    Console.WriteLine("Searching to verify live connection...");
    var findOptions = new FindPackagesOptions();
    findOptions.Selectors.Add(new PackageMatchFilter
    {
        Field = PackageMatchField.Id,
        Option = PackageFieldMatchOption.StartsWithCaseInsensitive,
        Value = "Microsoft.",
    });
    var searchResult = result.PackageCatalog.FindPackages(findOptions);
    Console.WriteLine($"Search result: {searchResult.Status} ({searchResult.Matches.Count} match(es))");

    return 0;
}
else
{
    Console.Error.WriteLine($"Failed to connect to '{catalogName}'.");
    if (result.ExtendedErrorCode is not null)
    {
        Console.Error.WriteLine($"  Error code: 0x{result.ExtendedErrorCode.HResult:X8}");
    }
    return 1;
}
