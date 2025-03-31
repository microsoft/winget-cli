// -----------------------------------------------------------------------------
// <copyright file="TestDSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.Helpers;

    /// <summary>
    /// Implements IDSCv3 for tests.
    /// </summary>
    internal class TestDSCv3 : IDSCv3
    {
        /// <summary>
        /// The delegate type for GetResourceByType.
        /// </summary>
        /// <param name="resourceType">The type name of the resource.</param>
        /// <returns>A single resource item.</returns>
        internal delegate IResourceListItem? GetResourceByTypeDelegateType(string resourceType);

        /// <summary>
        /// The delegate type for GetResourceSettings.
        /// </summary>
        /// <param name="unitInternal">The unit to get.</param>
        /// <returns>A get result.</returns>
        internal delegate IResourceGetItem GetResourceSettingsDelegateType(ConfigurationUnitInternal unitInternal);

        /// <summary>
        /// The delegate type for SetResourceSettings.
        /// </summary>
        /// <param name="unitInternal">The unit to set.</param>
        /// <returns>A set result.</returns>
        internal delegate IResourceSetItem SetResourceSettingsDelegateType(ConfigurationUnitInternal unitInternal);

        /// <summary>
        /// The delegate type for TestResource.
        /// </summary>
        /// <param name="unitInternal">The unit to test.</param>
        /// <returns>A test result.</returns>
        internal delegate IResourceTestItem TestResourceDelegateType(ConfigurationUnitInternal unitInternal);

        /// <summary>
        /// The delegate type for TestResource.
        /// </summary>
        /// <param name="unitInternal">The unit to test.</param>
        /// <returns>A test result.</returns>
        internal delegate IList<IResourceExportItem> ExportResourceDelegateType(ConfigurationUnitInternal unitInternal);

        /// <summary>
        /// Gets or sets the GetResourceByType result.
        /// </summary>
        public IResourceListItem? GetResourceByTypeResult { get; set; }

        /// <summary>
        /// Gets or sets the GetResourceByType delegate.
        /// </summary>
        public GetResourceByTypeDelegateType? GetResourceByTypeDelegate { get; set; }

        /// <summary>
        /// Gets or sets the GetResourceSettings result.
        /// </summary>
        public IResourceGetItem? GetResourceSettingsResult { get; set; }

        /// <summary>
        /// Gets or sets the GetResourceSettings delegate.
        /// </summary>
        public GetResourceSettingsDelegateType? GetResourceSettingsDelegate { get; set; }

        /// <summary>
        /// Gets or sets the SetResourceSettings result.
        /// </summary>
        public IResourceSetItem? SetResourceSettingsResult { get; set; }

        /// <summary>
        /// Gets or sets the SetResourceSettings delegate.
        /// </summary>
        public SetResourceSettingsDelegateType? SetResourceSettingsDelegate { get; set; }

        /// <summary>
        /// Gets or sets the TestResource result.
        /// </summary>
        public IResourceTestItem? TestResourceResult { get; set; }

        /// <summary>
        /// Gets or sets the TestResource delegate.
        /// </summary>
        public TestResourceDelegateType? TestResourceDelegate { get; set; }

        /// <summary>
        /// Gets or sets the ExportResource result.
        /// </summary>
        public IList<IResourceExportItem>? ExportResourceResult { get; set; }

        /// <summary>
        /// Gets or sets the ExportResource delegate.
        /// </summary>
        public ExportResourceDelegateType? ExportResourceDelegate { get; set; }

        /// <inheritdoc/>
        public IResourceListItem? GetResourceByType(string resourceType, IDiagnosticsSink? diagnosticsSink = null)
        {
            return this.GetResourceByTypeResult ?? this.GetResourceByTypeDelegate?.Invoke(resourceType);
        }

        /// <inheritdoc/>
        public IResourceGetItem GetResourceSettings(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
        {
            return this.GetResourceSettingsResult ?? this.GetResourceSettingsDelegate?.Invoke(unitInternal) ?? throw new System.NotImplementedException();
        }

        /// <inheritdoc/>
        public IResourceSetItem SetResourceSettings(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
        {
            return this.SetResourceSettingsResult ?? this.SetResourceSettingsDelegate?.Invoke(unitInternal) ?? throw new System.NotImplementedException();
        }

        /// <inheritdoc/>
        public IResourceTestItem TestResource(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
        {
            return this.TestResourceResult ?? this.TestResourceDelegate?.Invoke(unitInternal) ?? throw new System.NotImplementedException();
        }

        /// <inheritdoc/>
        public IList<IResourceExportItem> ExportResource(ConfigurationUnitInternal unitInternal, IDiagnosticsSink? diagnosticsSink = null)
        {
            return this.ExportResourceResult ?? this.ExportResourceDelegate?.Invoke(unitInternal) ?? throw new System.NotImplementedException();
        }
    }
}
