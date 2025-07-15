// -----------------------------------------------------------------------------
// <copyright file="PackageResponse.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    using Microsoft.Management.Deployment;
    using ModelContextProtocol.Protocol;
    using System.Text.Json;

    /// <summary>
    /// Contains reusable responses for package tools.
    /// </summary>
    internal static class PackageResponse
    {
        public static CallToolResponse ResponseForConnectError(ConnectResult connectResult)
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"Failed when connecting to the WinGet package catalog with error: {connectResult.ExtendedErrorCode.Message} [0x{connectResult.ExtendedErrorCode.HResult:X8}]" }],
            };
        }

        public static CallToolResponse ResponseForFindError(FindPackagesResult findResult)
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"Failed when finding packages with reason {findResult.Status} and error: {findResult.ExtendedErrorCode.Message} [0x{findResult.ExtendedErrorCode.HResult:X8}]" }],
            };
        }

        public static CallToolResponse ResponseForCancelBeforeSystemChange()
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"The operation was cancelled before any system change was started" }],
            };
        }

        public static CallToolResponse ResponseForEmptyFind(string identifer, string? catalog)
        {
            PackageIdentityErrorResult result = new()
            {
                Message = "Did not find a package with the requested identifier",
                Identifier = identifer,
                Catalog = catalog,
            };

            return ResponseFromObject(result);
        }

        public static CallToolResponse ResponseForMultiFind(string identifer, string? catalog, FindPackagesResult findResult)
        {
            PackageIdentityErrorResult result = new()
            {
                Message = "Found multiple packages matching the requested identifier; provide a more specific identifier and/or catalog",
                Identifier = identifer,
                Catalog = catalog,
            };

            AddFoundPackagesToList(result.Packages, findResult);

            return ResponseFromObject(result);
        }

        public static CallToolResponse ResponseForInstallOperation(InstallResult installResult, FindPackagesResult? findResult)
        {
            InstallOperationResult result = new InstallOperationResult();

            switch (installResult.Status)
            {
                case InstallResultStatus.Ok:
                    result.Message = "Install completed successfully";
                    break;
                case InstallResultStatus.BlockedByPolicy:
                    result.Message = "Installation was blocked by policy";
                    break;
                case InstallResultStatus.CatalogError:
                    result.Message = "An error occurred with the catalog";
                    break;
                case InstallResultStatus.InternalError:
                    result.Message = "An internal WinGet error occurred";
                    break;
                case InstallResultStatus.InvalidOptions:
                    result.Message = "The install options were invalid";
                    break;
                case InstallResultStatus.DownloadError:
                    result.Message = "An error occurred while downloading the package installer";
                    break;
                case InstallResultStatus.InstallError:
                    result.Message = "The package installer failed during installation";
                    break;
                case InstallResultStatus.ManifestError:
                    result.Message = "The package manifest was invalid";
                    break;
                case InstallResultStatus.NoApplicableInstallers:
                    result.Message = "No applicable package installers were available for this system";
                    break;
                case InstallResultStatus.NoApplicableUpgrade:
                    result.Message = "No applicable upgrade was available for this system";
                    break;
                case InstallResultStatus.PackageAgreementsNotAccepted:
                    result.Message = "The package requires accepting agreements; please install manually";
                    break;
                default:
                    result.Message = "Unknown install status";
                    break;
            }

            if (installResult.RebootRequired)
            {
                result.RebootRequired = true;
            }

            result.ErrorCode = installResult.ExtendedErrorCode?.HResult;

            if (installResult.Status == InstallResultStatus.InstallError)
            {
                result.InstallerErrorCode = installResult.InstallerErrorCode;
            }

            if (findResult != null && findResult.Status == FindPackagesResultStatus.Ok && findResult.Matches?.Count == 1)
            {
                result.InstalledPackageInformation = FindPackageResultFromCatalogPackage(findResult.Matches[0].CatalogPackage);
            }

            return ResponseFromObject(result, installResult.Status != InstallResultStatus.Ok);
        }
    }
}
