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
        public const string DefaultSourceName = @"winget";
        public const string DefaultSourceUrl = @"https://winget.azureedge.net/cache";
        public const string TestSourceName = @"TestSource";
        public const string TestSourceUrl = @"https://localhost:5001/TestKit";

        // Todo: not needed if switch to use prod index for source tests
        public const string IndexPackageRootCert = "IndexPackageIntRoot.cer";
        public const string IndexPackageRootCertThumbprint = "d17697cc206ed26e1a51f5bb96e9356d6d610b74";

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

        public class ErrorCode
        {
            public const int S_OK = 0;
            public const int ERROR_FILE_NOT_FOUND = unchecked((int)0x80070002);
            public const int ERROR_PATH_NOT_FOUND = unchecked((int)0x80070003);
            public const int ERROR_NO_RANGES_PROCESSED = unchecked((int)0x80070138);
            public const int OPC_E_ZIP_MISSING_END_OF_CENTRAL_DIRECTORY = unchecked((int)0x8051100f);
            public const int ERROR_OLD_WIN_VERSION = unchecked((int)0x8007047e);
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
            public const int ERROR_NO_UNINSTALL_INFO_FOUND = unchecked((int)0x8a15002F);
            public const int ERROR_EXEC_UNINSTALL_COMMAND_FAILED = unchecked((int)0x8a150030);
            public const int ERROR_ICU_BREAK_ITERATOR_ERROR = unchecked((int)0x8A150031);
            public const int ERROR_ICU_CASEMAP_ERROR = unchecked((int)0x8A150032);
            public const int ERROR_ICU_REGEX_ERROR = unchecked((int)0x8A150033);
            public const int ERROR_IMPORT_INSTALL_FAILED = unchecked((int)0x8a150034);
            public const int ERROR_NOT_ALL_PACKAGES_FOUND = unchecked((int)0x8a150035);
            public const int ERROR_JSON_INVALID_FILE = unchecked((int)0x8a150036);
            public const int ERROR_SOURCE_NOT_REMOTE = unchecked((int)0x8A150037);
            public const int ERROR_UNSUPPORTED_RESTSOURCE = unchecked((int)0x8A150038);
            public const int ERROR_RESTSOURCE_INVALID_DATA = unchecked((int)0x8A150039);
            public const int ERROR_BLOCKED_BY_POLICY = unchecked((int)0x8a15003A);
        }
    }
}
