// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    public class Constants
    {
        public const string PackagedContextParameter = "PackagedContext";
        public const string AICLIPathParameter = "AICLIPath";
        public const string VerboseLoggingParameter = "VerboseLogging";

        public const string AppInstallerTestCert = "AppInstallerTest.cer";
        public const string AppInstallerTestCertThumbprint = "d03e7a688b388b1edde8476a627531c49db88017";
        // Todo: not needed if switch to use prod index
        public const string IndexPackageCert = "IndexPackageInt.cer";
        public const string IndexPackageCertThumbprint = "1da968e670a0257f61628aad20c60c64fdecd41a";

        public const string AICLIPackageFile = "AppInstallerCLIPackage.appxbundle";
        public const string AICLIPackageName = "AppInstallerCLI";

        // Todo: switch to use prod index when available
        public const string TestSourceUrl = @"https://pkgmgr-int.azureedge.net/cache";
        public const string TestSourceName = @"TestSource";

        public class ErrorCode
        {
            public const int S_OK = 0;
            public const int ERROR_FILE_NOT_FOUND = unchecked((int)0x80070002);
            public const int ERROR_NO_RANGES_PROCESSED = unchecked((int)0x80070138);
            public const int OPC_E_ZIP_MISSING_END_OF_CENTRAL_DIRECTORY = unchecked((int)0x8051100f);

            // AICLI custom HResults
            public const int ERROR_INTERNAL_ERROR = unchecked((int)0x8A150001);
            public const int ERROR_INVALID_CL_ARGUMENTS = unchecked((int)0x8A150002);
            public const int ERROR_COMMAND_FAILED = unchecked((int)0x8A150003);
            public const int ERROR_MANIFEST_FAILED = unchecked((int)0x8A150004);
            //public const int ERROR_WORKFLOW_FAILED = unchecked((int)0x8A150005) // Unused, can be repurposed
            public const int ERROR_SHELLEXEC_INSTALL_FAILED = unchecked((int)0x8A150006);
            //public const int ERROR_RUNTIME_ERROR = unchecked((int)0x8A150007) // Unused, can be repurposed
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
        }
    }
}
