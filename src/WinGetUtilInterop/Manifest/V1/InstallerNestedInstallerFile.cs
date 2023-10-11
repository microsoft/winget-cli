using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.WinGetUtil.Models.V1
{
    public class InstallerNestedInstallerFile
    {
        /// <summary>
        /// Gets or sets relative file part.
        /// </summary>
        public string RelativeFilePath { get; set; }

        /// <summary>
        /// Gets or set portable command alias.
        /// </summary>
        public string PortableCommandAlias { get; set; }
    }
}
