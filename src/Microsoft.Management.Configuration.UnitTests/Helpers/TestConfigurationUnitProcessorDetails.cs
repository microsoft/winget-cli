// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationUnitProcessorDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;
    using Windows.Security.Cryptography.Certificates;

    /// <summary>
    /// A test implementation of IConfigurationProcessorFactory.
    /// </summary>
    internal class TestConfigurationUnitProcessorDetails : IConfigurationUnitProcessorDetails
    {
        public string Author => throw new NotImplementedException();

        public bool IsLocal => throw new NotImplementedException();

        public string ModuleDescription => throw new NotImplementedException();

        public Uri ModuleDocumentationUri => throw new NotImplementedException();

        public string ModuleName => throw new NotImplementedException();

        public string ModuleSource => throw new NotImplementedException();

        public string ModuleType => throw new NotImplementedException();

        public DateTimeOffset PublishedDate => throw new NotImplementedException();

        public Uri PublishedModuleUri => throw new NotImplementedException();

        public string Publisher => throw new NotImplementedException();

        public IReadOnlyList<IConfigurationUnitSettingDetails> Settings => throw new NotImplementedException();

        public CertificateChain SigningCertificateChain => throw new NotImplementedException();

        public string UnitDescription => throw new NotImplementedException();

        public Uri UnitDocumentationUri => throw new NotImplementedException();

        public Uri UnitIconUri => throw new NotImplementedException();

        public string UnitName => throw new NotImplementedException();

        public string Version => throw new NotImplementedException();
    }
}
