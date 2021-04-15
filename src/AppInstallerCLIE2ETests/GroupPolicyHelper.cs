// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;
using Microsoft.Win32;

namespace AppInstallerCLIE2ETests
{
    /// <summary>
    /// Helper for setting Group Policy settings.
    /// This helper reads the keys and values to use directly from the ADMX file to ensure that the names
    /// used by the source code are correct.
    /// </summary>
    /// <remarks>
    /// This helper modifies the policies for winget configured in the machine.
    /// </remarks>
    public class GroupPolicyHelper
    {
        private const string PoliciesDefinitionFileName = "DesktopAppInstaller.admx";
        private static Lazy<XElement> policyDefinitions = new Lazy<XElement>(() =>
        {
            string filePath = TestCommon.GetTestDataFile(PoliciesDefinitionFileName);
            string fileText = File.ReadAllText(filePath);
            return XElement.Parse(fileText);
        });

        /// <summary>
        /// Names of the XML elements and attributes that make up the definition file.
        /// </summary>
        private static class XmlNames
        {
            private const string Namespace = "http://schemas.microsoft.com/GroupPolicy/2006/07/PolicyDefinitions";

            // Root element
            public static readonly XName PolicyDefinitions = XName.Get("policyDefinitions", Namespace);

            public static readonly XName Policies = XName.Get("policies", Namespace);
            public static readonly XName Policy = XName.Get("policy", Namespace);

            public static readonly XName EnabledValue = XName.Get("enabledValue", Namespace);
            public static readonly XName DisabledValue = XName.Get("disabledValue", Namespace);
            public static readonly XName Elements = XName.Get("elements", Namespace);

            public static readonly XName Decimal = XName.Get("decimal", Namespace);
            public static readonly XName List = XName.Get("list", Namespace);

            public static class Attributes
            {
                public const string Name = "name";
                public const string Value = "value";
                public const string Id = "id";
                public const string Key = "key";
                public const string ValueName = "valueName";
            }
        }

        /// <summary>
        /// Name of the policy. Used to identify it in the file.
        /// </summary>
        private string name;

        /// <summary>
        /// ID of the value element of this policy (if it has one).
        /// This assumes that each policy has a single value element.
        /// </summary>
        private string elementId;

        // Policies available.
        public static GroupPolicyHelper EnableWinget = new GroupPolicyHelper("EnableAppInstaller");
        public static GroupPolicyHelper EnableSettings = new GroupPolicyHelper("EnableSettings");
        public static GroupPolicyHelper EnableExperimentalFeatures = new GroupPolicyHelper("EnableExperimentalFeatures");
        public static GroupPolicyHelper EnableLocalManifests = new GroupPolicyHelper("EnableLocalManifestFiles");
        public static GroupPolicyHelper EnableHashOverride = new GroupPolicyHelper("EnableHashOverride");
        public static GroupPolicyHelper EnableDefaultSource = new GroupPolicyHelper("EnableDefaultSource");
        public static GroupPolicyHelper EnableMicrosoftStoreSource = new GroupPolicyHelper("EnableMicrosoftStoreSource");
        public static GroupPolicyHelper EnableAdditionalSources = new GroupPolicyHelper("EnableAdditionalSources", "AdditionalSources");
        public static GroupPolicyHelper EnableAllowedSources = new GroupPolicyHelper("EnableAllowedSources", "AllowedSources");
        public static GroupPolicyHelper SourceAutoUpdateInterval = new GroupPolicyHelper("SourceAutoUpdateIntervalInMinutes", "SourceAutoUpdateIntervalInMinutes");

        private static GroupPolicyHelper[] AllPolicies = new GroupPolicyHelper[]
        {
            EnableWinget,
            EnableSettings,
            EnableExperimentalFeatures,
            EnableLocalManifests,
            EnableHashOverride,
            EnableDefaultSource,
            EnableMicrosoftStoreSource,
            EnableAdditionalSources,
            EnableAllowedSources,
            SourceAutoUpdateInterval,
        };

        private GroupPolicyHelper(string name)
        {
            this.name = name;
        }

        private GroupPolicyHelper(string name, string elementId)
        {
            this.name = name;
            this.elementId = elementId;
        }

        /// <summary>
        /// Gets the content of the ADMX file as an XML.
        /// </summary>
        private static XElement PolicyDefinitions => policyDefinitions.Value;

        /// <summary>
        /// Gets the XML element that defines this policy.
        /// </summary>
        // The XML structure is like this:
        // <policyDefinitions ...>
        //   ...
        //   <policies>
        //     <policy name="..." ... />
        //   </policies>
        // </policyDefinitions>
        private XElement PolicyElement => PolicyDefinitions
            .Element(XmlNames.Policies)
            .Elements(XmlNames.Policy)
            .First(policy => policy.Attribute(XmlNames.Attributes.Name).Value == this.name);

        /// <summary>
        /// Gets the path to the registry key that backs this policy.
        /// </summary>
        private string KeyPath => this.PolicyElement.Attribute(XmlNames.Attributes.Key).Value;

        /// <summary>
        /// Gets the name of the registry value that backs this policy.
        /// </summary>
        private string ValueName => this.PolicyElement.Attribute(XmlNames.Attributes.ValueName)?.Value;

        /// <summary>
        /// Gets the XElement that defines the single value element of this policy.
        /// This only works if the policy has a single element for its value.
        /// </summary>
        // Looks for something like this:
        // <policy>
        //   <elements>
        //     <something id="..." />
        //   </elements>
        // </policy>
        // We use only list and decimal elements.
        private XElement ValueElement => this.PolicyElement
            .Element(XmlNames.Elements)
            .Elements()
            .First(element => element.Attribute(XmlNames.Attributes.Id).Value == this.elementId);

        /// <summary>
        /// Deletes all of the existing policies from the registry.
        /// </summary>
        public static void DeleteExistingPolicies()
        {
            foreach (var policy in AllPolicies)
            {
                policy.SetNotConfigured();
            }
        }

        /// <summary>
        /// Sets the policy to the Enabled state.
        /// This will fail if the policy's EnabledValue does not exist or is not exactly as expected.
        /// </summary>
        public void Enable()
        {
            // The expected format is like this:
            // <enabledValue>
            //   <decimal value="1" />
            // </enabledValue>
            // We expect the value to always be 1, but still parse it to catch errors in the ADMX.
            int enabledValue = GetDecimalValue(this.PolicyElement.Element(XmlNames.EnabledValue));
            using (RegistryKey key = this.GetKey())
            {
                key.SetValue(this.ValueName, enabledValue);
            }
        }

        /// <summary>
        /// Sets the policy to the Disabled state.
        /// This will fail if the policy's DisabledValue does not exist or is not exactly as expected.
        /// </summary>
        public void Disable()
        {
            // The expected format is like this:
            // <enabledValue>
            //   <decimal value="0" />
            // </enabledValue>
            // We expect the value to always be 0, but still parse it to catch errors in the ADMX.
            int disabledValue = GetDecimalValue(this.PolicyElement.Element(XmlNames.DisabledValue));
            using (RegistryKey key = this.GetKey())
            {
                key.SetValue(this.ValueName, disabledValue);
            }
        }

        /// <summary>
        /// Sets the policy to the Not Configured state.
        /// This deletes the value associated with the policy, including its list if it has one.
        /// </summary>
        public void SetNotConfigured()
        {
            // Delete the enabled/disabled value
            if (this.ValueName != null)
            {
                using (RegistryKey key = this.GetKey())
                {
                    key.DeleteValue(this.ValueName, throwOnMissingValue: false);
                }
            }

            // Delete the value element
            if (this.elementId != null)
            {
                if (this.ValueElement.Name == XmlNames.List)
                {
                    // Lists are stored in separate keys.
                    Registry.LocalMachine.DeleteSubKeyTree(this.ValueElement.Attribute(XmlNames.Attributes.Key).Value, throwOnMissingSubKey: false);
                }
                else if (this.ValueElement.Name == XmlNames.Decimal)
                {
                    // Decimals are stored in single values
                    using (RegistryKey key = this.GetKey())
                    {
                        key.DeleteValue(this.ValueElement.Attribute(XmlNames.Attributes.ValueName).Value, throwOnMissingValue: false);
                    }
                }
            }
        }

        /// <summary>
        /// Sets the value of the policy when enabled.
        /// This uses only the "elements" of the policy, not the "enabledValue".
        /// The type used in the registry is chosen automatically.
        /// </summary>
        /// <param name="value">Value of the policy.</param>
        public void SetEnabledValue(object value)
        {
            using (RegistryKey key = this.GetKey())
            {
                key.SetValue(
                    this.ValueElement.Attribute(XmlNames.Attributes.ValueName).Value,
                    value);
            }
        }

        /// <summary>
        /// Sets the list value of the policy when enabled.
        /// This sets from the "elements" and also sets the enabled value as lists are also gated by a toggle.
        /// This will fail if the value of the policy is not a list.
        /// </summary>
        /// <param name="values">Values to set in the list.</param>
        public void SetEnabledList(IEnumerable<string> values)
        {
            this.Enable();

            // Delete the existing list
            string listKeyPath = this.ValueElement.Attribute(XmlNames.Attributes.Key).Value;
            Registry.LocalMachine.DeleteSubKeyTree(listKeyPath, throwOnMissingSubKey: false);

            // Create and fill the key.
            // This assumes that the values don't need to have special names or prefixes.
            var listKey = Registry.LocalMachine.CreateSubKey(listKeyPath);
            int index = 0;
            foreach (string value in values)
            {
                listKey.SetValue(index++.ToString(), value);
            }

            listKey.Close();
        }

        /// <summary>
        /// Gets the value from a "decimal" child element.
        /// </summary>
        /// <param name="element">Element containing the decimal.</param>
        /// <returns>Value in the element.</returns>
        private static int GetDecimalValue(XElement element)
        {
            // Reads a child that looks like this:
            // <decimal value="X" />
            return int.Parse(element.Element(XmlNames.Decimal).Attribute(XmlNames.Attributes.Value).Value);
        }

        /// <summary>
        /// Gets the registry key backing this policy.
        /// </summary>
        /// <remarks>
        /// This assumes that all the policies are machine-wide.
        /// If this changes, we will need to parse the class="machine|user" attribute.
        /// </remarks>
        private RegistryKey GetKey()
        {
            return Registry.LocalMachine.CreateSubKey(this.KeyPath);
        }
    }
}
