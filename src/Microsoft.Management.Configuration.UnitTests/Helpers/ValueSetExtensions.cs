// -----------------------------------------------------------------------------
// <copyright file="ValueSetExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Text;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Extensions for ValueSet.
    /// </summary>
    internal static class ValueSetExtensions
    {
        /// <summary>
        /// Converts the value set to YAML like output.
        /// </summary>
        /// <param name="set">The set to output.</param>
        /// <returns>The string.</returns>
        public static string ToYaml(this ValueSet set)
        {
            StringBuilder sb = new StringBuilder();
            ToYaml(set, sb);
            return sb.ToString();
        }

        private static void ToYaml(ValueSet set, StringBuilder sb, int indentation = 0)
        {
            foreach (var keyValuePair in set)
            {
                bool addLine = true;

                sb.Append(' ', indentation);
                sb.Append(keyValuePair.Key);
                sb.Append(": ");

                if (keyValuePair.Value == null)
                {
                    sb.Append("null");
                }
                else
                {
                    switch (keyValuePair.Value)
                    {
                        case int i:
                            sb.Append(i);
                            break;
                        case string s:
                            sb.Append(s);
                            break;
                        case bool b:
                            sb.Append(b);
                            break;
                        case ValueSet v:
                            sb.AppendLine();
                            ToYaml(v, sb, indentation + 2);
                            addLine = false;
                            break;
                        default:
                            throw new NotImplementedException($"Add ToYaml type `{keyValuePair.Value.GetType().Name}`");
                    }
                }

                if (addLine)
                {
                    sb.AppendLine();
                }
            }
        }
    }
}
