// -----------------------------------------------------------------------
// <copyright file="ManifestDocumentation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
// -----------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System;
    using System.Collections.Generic;
    using System.Text;

    public class ManifestDocumentation
    {
        /// <summary>
        /// Gets or sets the document label.
        /// </summary>
        public string DocumentLabel { get; set; }

        /// <summary>
        /// Gets or sets the document url.
        /// </summary>
        public string DocumentUrl { get; set; }

    }
}
