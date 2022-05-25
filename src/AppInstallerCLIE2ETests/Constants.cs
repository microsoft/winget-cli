// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    public class Constants
    {
        // Runtime test parameters
        public const string PackagedContextParameter = "PackagedContext";
        public const string AICLIPathParameter = "AICLIPath";
        public const string AICLIPackagePathParameter = "AICLIPackagePath";
        public const string VerboseLoggingParameter = "VerboseLogging";
        public const string LooseFileRegistrationParameter = "LooseFileRegistration";
        public const string InvokeCommandInDesktopPackageParameter = "InvokeCommandInDesktopPackage";
        public const string StaticFileRootPathParameter = "StaticFileRootPath";
        public const string ExeInstallerPathParameter = "ExeTestInstallerPath";
        public const string MsiInstallerPathParameter = "MsiTestInstallerPath";
        public const string MsixInstallerPathParameter = "MsixTestInstallerPath";
        public const string PackageCertificatePathParameter = "PackageCertificatePath";
        public const string AppInstallerTestCert = "AppInstallerTest.cer";
        public const string AppInstallerTestCertThumbprint = "d03e7a688b388b1edde8476a627531c49db88017";

        // Test Sources
        public const string DefaultWingetSourceName = @"winget";
        public const string DefaultWingetSourceUrl = @"https://winget.azureedge.net/cache";
        public const string DefaultMSStoreSourceName = @"msstore";
        public const string DefaultMSStoreSourceUrl = @"https://storeedgefd.dsx.mp.microsoft.com/v9.0";
        public const string TestSourceName = @"TestSource";
        public const string TestSourceUrl = @"https://localhost:5001/TestKit";
        public const string TestSourceIdentifier = @"WingetE2E.Tests_8wekyb3d8bbwe";

        public const string AICLIPackageFamilyName = "WinGetDevCLI_8wekyb3d8bbwe";
        public const string AICLIPackageName = "WinGetDevCLI";
        public const string AICLIAppId = "WinGetDev";

        public const string TestPackage = "TëstPackage.msix";
        public const string ExeInstaller = "AppInstallerTestExeInstaller";
        public const string MsiInstaller = "AppInstallerTestMsiInstaller";
        public const string MsixInstaller = "AppInstallerTestMsixInstaller";
        public const string IndexPackage = "source.msix";
        public const string MakeAppx = "makeappx.exe";
        public const string SignTool = "signtool.exe";
        public const string IndexCreationTool = "IndexCreationTool";
        public const string WinGetUtil = "WinGetUtil";
        public const string E2ETestLogsPath = @"Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\DiagOutputDir";

        // Test installers' package IDs
        public const string ExeInstallerPackageId = "AppInstallerTest.TestExeInstaller";
        public const string MsiInstallerPackageId = "AppInstallerTest.TestMsiInstaller";
        public const string MsixInstallerPackageId = "AppInstallerTest.TestMsixInstaller";

        public const string MsiInstallerProductCode = "{A5D36CF1-1993-4F63-BFB4-3ACD910D36A1}";
        public const string MsixInstallerPackageFamilyName = "6c6338fe-41b7-46ca-8ba6-b5ad5312bb0e_8wekyb3d8bbwe";

        public const string TestExeInstalledFileName = "TestExeInstalled.txt";
        public const string TestExeUninstallerFileName = "UninstallTestExe.bat";

        // Locations
        public const string LocalAppData = "LocalAppData";

        public class ErrorCode
        {
            public const int S_OK = 0;
            public const int ERROR_FILE_NOT_FOUND = unchecked((int)0x80070002);
            public const int ERROR_PATH_NOT_FOUND = unchecked((int)0x80070003);
            public const int ERROR_NO_RANGES_PROCESSED = unchecked((int)0x80070138);
            public const int OPC_E_ZIP_MISSING_END_OF_CENTRAL_DIRECTORY = unchecked((int)0x8051100F);
            public const int ERROR_OLD_WIN_VERSION = unchecked((int)0x8007047E);
            public const int HTTP_E_STATUS_NOT_FOUND = unchecked((int)0x80190194);

            // AICLI custom HRESULTs
            public const int ERROR_INTERNAL_ERROR = unchecked((int)0x8A150001);
            public const int ERROR_INVALID_CL_ARGUMENTS = unchecked((int)0x8A150002);
            public const int ERROR_COMMAND_FAILED = unchecked((int)0x8A150003);
            public const int ERROR_MANIFEST_FAILED = unchecked((int)0x8A150004);
            public const int ERROR_CTRL_SIGNAL_RECEIVED = unchecked((int)0x8A150005);
            public const int ERROR_SHELLEXEC_INSTALL_FAILED = unchecked((int)0x8A150006);
            public const int ERROR_UNSUPPORTED_MANIFESTVERSION = unchecked((int)0x8A150007);
            public const int ERROR_DOWNLOAD_FAILED = unchecked((int)0x8A150008);
            public const int ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX = unchecked((int)0x8A150009);
            public const int ERROR_INDEX_INTEGRITY_COMPROMISED = unchecked((int)0x8A15000A);
            public const int ERROR_SOURCES_INVALID = unchecked((int)0x8A15000B);
            public const int ERROR_SOURCE_NAME_ALREADY_EXISTS = unchecked((int)0x8A15000C);
            public const int ERROR_INVALID_SOURCE_TYPE = unchecked((int)0x8A15000D);
            public const int ERROR_PACKAGE_IS_BUNDLE = unchecked((int)0x8A15000E);
            public const int ERROR_SOURCE_DATA_MISSING = unchecked((int)0x8A15000F);
            public const int ERROR_NO_APPLICABLE_INSTALLER = unchecked((int)0x8A150010);
            public const int ERROR_INSTALLER_HASH_MISMATCH = unchecked((int)0x8A150011);
            public const int ERROR_SOURCE_NAME_DOES_NOT_EXIST = unchecked((int)0x8A150012);
            public const int ERROR_SOURCE_ARG_ALREADY_EXISTS = unchecked((int)0x8A150013);
            public const int ERROR_NO_APPLICATIONS_FOUND = unchecked((int)0x8A150014);
            public const int ERROR_NO_SOURCES_DEFINED = unchecked((int)0x8A150015);
            public const int ERROR_MULTIPLE_APPLICATIONS_FOUND = unchecked((int)0x8A150016);
            public const int ERROR_NO_MANIFEST_FOUND = unchecked((int)0x8A150017);
            public const int ERROR_EXTENSION_PUBLIC_FAILED = unchecked((int)0x8A150018);
            public const int ERROR_COMMAND_REQUIRES_ADMIN = unchecked((int)0x8A150019);
            public const int ERROR_SOURCE_NOT_SECURE = unchecked((int)0x8A15001A);
            public const int ERROR_MSSTORE_BLOCKED_BY_POLICY = unchecked((int)0x8A15001B);
            public const int ERROR_MSSTORE_APP_BLOCKED_BY_POLICY = unchecked((int)0x8A15001C);
            public const int ERROR_EXPERIMENTAL_FEATURE_DISABLED = unchecked((int)0x8A15001D);
            public const int ERROR_MSSTORE_INSTALL_FAILED = unchecked((int)0x8A15001E);
            public const int ERROR_COMPLETE_INPUT_BAD = unchecked((int)0x8A15001F);
            public const int ERROR_YAML_INIT_FAILED = unchecked((int)0x8A150020);
            public const int ERROR_INVALID_MAPPING_KEY = unchecked((int)0x8A150021);
            public const int ERROR_DUPLICATE_MAPPING_KEY = unchecked((int)0x8A150022);
            public const int ERROR_YAML_INVALID_OPERATION = unchecked((int)0x8A150023);
            public const int ERROR_YAML_DOC_BUILD_FAILED = unchecked((int)0x8A150024);
            public const int ERROR_YAML_INVALID_EMITTER_STATE = unchecked((int)0x8A150025);
            public const int ERROR_YAML_INVALID_DATA = unchecked((int)0x8A150026);
            public const int ERROR_LIBYAML_ERROR = unchecked((int)0x8A150027);
            public const int ERROR_MANIFEST_VALIDATION_WARNING = unchecked((int)0x8A150028);
            public const int ERROR_MANIFEST_VALIDATION_FAILURE = unchecked((int)0x8A150029);
            public const int ERROR_INVALID_MANIFEST = unchecked((int)0x8A15002A);
            public const int ERROR_UPDATE_NOT_APPLICABLE = unchecked((int)0x8A15002B);
            public const int ERROR_UPDATE_ALL_HAS_FAILURE = unchecked((int)0x8A15002C);
            public const int ERROR_INSTALLER_SECURITY_CHECK_FAILED = unchecked((int)0x8A15002D);
            public const int ERROR_DOWNLOAD_SIZE_MISMATCH = unchecked((int)0x8A15002E);
            public const int ERROR_NO_UNINSTALL_INFO_FOUND = unchecked((int)0x8A15002F);
            public const int ERROR_EXEC_UNINSTALL_COMMAND_FAILED = unchecked((int)0x8A150030);
            public const int ERROR_ICU_BREAK_ITERATOR_ERROR = unchecked((int)0x8A150031);
            public const int ERROR_ICU_CASEMAP_ERROR = unchecked((int)0x8A150032);
            public const int ERROR_ICU_REGEX_ERROR = unchecked((int)0x8A150033);
            public const int ERROR_IMPORT_INSTALL_FAILED = unchecked((int)0x8A150034);
            public const int ERROR_NOT_ALL_PACKAGES_FOUND = unchecked((int)0x8A150035);
            public const int ERROR_JSON_INVALID_FILE = unchecked((int)0x8A150036);
            public const int ERROR_SOURCE_NOT_REMOTE = unchecked((int)0x8A150037);
            public const int ERROR_UNSUPPORTED_RESTSOURCE = unchecked((int)0x8A150038);
            public const int ERROR_RESTSOURCE_INVALID_DATA = unchecked((int)0x8A150039);
            public const int ERROR_BLOCKED_BY_POLICY = unchecked((int)0x8A15003A);
            public const int ERROR_RESTSOURCE_INTERNAL_ERROR = unchecked((int)0x8A15003B);
            public const int ERROR_RESTSOURCE_INVALID_URL = unchecked((int)0x8A15003C);
            public const int ERROR_RESTSOURCE_UNSUPPORTED_MIME_TYPE = unchecked((int)0x8A15003D);
            public const int ERROR_RESTSOURCE_INVALID_VERSION = unchecked((int)0x8A15003E);
            public const int ERROR_SOURCE_DATA_INTEGRITY_FAILURE = unchecked((int)0x8A15003F);
            public const int ERROR_STREAM_READ_FAILURE = unchecked((int)0x8A150040);
            public const int ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED = unchecked((int)0x8A150041);
            public const int ERROR_PROMPT_INPUT_ERROR = unchecked((int)0x8A150042);
            public const int ERROR_UNSUPPORTED_SOURCE_REQUEST = unchecked((int)0x8A150043);
            public const int ERROR_RESTSOURCE_ENDPOINT_NOT_FOUND = unchecked((int)0x8A150044);
            public const int ERROR_SOURCE_OPEN_FAILED = unchecked((int)0x8A150045);
            public const int ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED = unchecked((int)0x8A150046);
            public const int ERROR_CUSTOMHEADER_EXCEEDS_MAXLENGTH = unchecked((int)0x8A150047);
            public const int ERROR_MISSING_RESOURCE_FILE = unchecked((int)0x8A150048);
            public const int ERROR_MSI_INSTALL_FAILED = unchecked((int)0x8A150049);
            public const int ERROR_INVALID_MSIEXEC_ARGUMENT = unchecked((int)0x8A15004A);
            public const int ERROR_FAILED_TO_OPEN_ALL_SOURCES = unchecked((int)0x8A15004B);
            public const int ERROR_DEPENDENCIES_VALIDATION_FAILED = unchecked((int)0x8A15004C);
            public const int ERROR_MISSING_PACKAGE = unchecked((int)0x8A15004D);
            public const int ERROR_INVALID_TABLE_COLUMN = unchecked((int)0x8A15004E);
            public const int ERROR_UPGRADE_VERSION_NOT_NEWER = unchecked((int)0x8A15004F);
            public const int ERROR_UPGRADE_VERSION_UNKNOWN = unchecked((int)0x8A150050);

            public const int ERROR_INSTALL_PACKAGE_IN_USE = unchecked((int)0x8A150101);
            public const int ERROR_INSTALL_INSTALL_IN_PROGRESS = unchecked((int)0x8A150102);
            public const int ERROR_INSTALL_FILE_IN_USE = unchecked((int)0x8A150103);
            public const int ERROR_INSTALL_MISSING_DEPENDENCY = unchecked((int)0x8A150104);
            public const int ERROR_INSTALL_DISK_FULL = unchecked((int)0x8A150105);
            public const int ERROR_INSTALL_INSUFFICIENT_MEMORY = unchecked((int)0x8A150106);
            public const int ERROR_INSTALL_NO_NETWORK = unchecked((int)0x8A150107);
            public const int ERROR_INSTALL_CONTACT_SUPPORT = unchecked((int)0x8A150108);
            public const int ERROR_INSTALL_REBOOT_REQUIRED_TO_FINISH = unchecked((int)0x8A150109);
            public const int ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL = unchecked((int)0x8A15010A);
            public const int ERROR_INSTALL_REBOOT_INITIATED = unchecked((int)0x8A15010B);
            public const int ERROR_INSTALL_CANCELLED_BY_USER = unchecked((int)0x8A15010C);
            public const int ERROR_INSTALL_ALREADY_INSTALLED = unchecked((int)0x8A15010D);
            public const int ERROR_INSTALL_DOWNGRADE = unchecked((int)0x8A15010E);
            public const int ERROR_INSTALL_BLOCKED_BY_POLICY = unchecked((int)0x8A15010F);
        }
    }
}
