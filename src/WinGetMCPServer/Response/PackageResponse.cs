// -----------------------------------------------------------------------------
// <copyright file="PackageResponse.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    using Microsoft.Management.Deployment;
    using ModelContextProtocol.Protocol;
    using WinGetMCPServer.Extensions;

    /// <summary>
    /// Contains reusable responses for package tools.
    /// </summary>
    internal static class PackageResponse
    {
        /// <summary>
        /// Creates a response for a ConnectResult error.
        /// </summary>
        /// <param name="connectResult">The connect result.</param>
        /// <returns>The response.</returns>
        public static CallToolResponse ForConnectError(ConnectResult connectResult)
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"Failed when connecting to the package source with error: {connectResult.ExtendedErrorCode.Message} [0x{connectResult.ExtendedErrorCode.HResult:X8}]" }],
            };
        }

        /// <summary>
        /// Creates a response for a FindPackagesResult error.
        /// </summary>
        /// <param name="findResult">The find packages result.</param>
        /// <returns>The response.</returns>
        public static CallToolResponse ForFindError(FindPackagesResult findResult)
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"Failed when finding packages with reason {findResult.Status} and error: {findResult.ExtendedErrorCode.Message} [0x{findResult.ExtendedErrorCode.HResult:X8}]" }],
            };
        }

        /// <summary>
        /// Creates a response that indicates the operation was cancelled before any changes were made.
        /// </summary>
        /// <returns>The response.</returns>
        public static CallToolResponse ForCancelBeforeSystemChange()
        {
            return new CallToolResponse()
            {
                IsError = true,
                Content = [new Content() { Text = $"The operation was cancelled before any system change was started" }],
            };
        }

        /// <summary>
        /// Creates a response for not finding any packages.
        /// </summary>
        /// <param name="identifer">The identifier used when searching.</param>
        /// <param name="source">The source that was searched.</param>
        /// <returns>The response.</returns>
        public static CallToolResponse ForEmptyFind(string identifer, string? source)
        {
            PackageIdentityErrorResult result = new()
            {
                Message = "Did not find a package with the requested identifier",
                Identifier = identifer,
                Source = source,
            };

            return ToolResponse.FromObject(result);
        }

        /// <summary>
        /// Creates a response for finding multiple packages when only 1 is required.
        /// </summary>
        /// <param name="identifer">The identifier used when searching.</param>
        /// <param name="source">The source that was searched.</param>
        /// <param name="findResult">The result that contains multiple packages.</param>
        /// <returns>The response.</returns>
        public static CallToolResponse ForMultiFind(string identifer, string? source, FindPackagesResult findResult)
        {
            PackageIdentityErrorResult result = new()
            {
                Message = "Found multiple packages matching the requested identifier; provide a more specific identifier and/or source",
                Identifier = identifer,
                Source = source,
            };

            result.Packages.AddPackages(findResult);

            return ToolResponse.FromObject(result);
        }

        /// <summary>
        /// Creates a response for an install operation.
        /// </summary>
        /// <param name="installResult">The install operation result.</param>
        /// <param name="findResult">The post-install package data.</param>
        /// <returns>The response.</returns>
        public static CallToolResponse ForInstallOperation(InstallResult installResult, FindPackagesResult? findResult)
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
                    result.Message = "An error occurred with the source";
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
                result.InstalledPackageInformation = PackageListExtensions.FindPackageResultFromCatalogPackage(findResult.Matches[0].CatalogPackage);
            }

            return ToolResponse.FromObject(result, installResult.Status != InstallResultStatus.Ok);
        }
    }
}
