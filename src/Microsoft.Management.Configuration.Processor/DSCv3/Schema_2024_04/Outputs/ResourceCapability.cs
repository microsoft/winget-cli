// -----------------------------------------------------------------------------
// <copyright file="ResourceCapability.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    /// <summary>
    /// https://learn.microsoft.com/en-us/powershell/dsc/reference/schemas/outputs/resource/list?view=dsc-3.0#capabilities
    /// The capabilities that a resource can have.
    /// </summary>
    internal enum ResourceCapability
    {
        /// <summary>
        /// Can call get on the resource.
        /// Required.
        /// </summary>
        Get,

        /// <summary>
        /// Can call set on the resource.
        /// </summary>
        Set,

        /// <summary>
        /// The resource operates properly in the presence of the `_exist` property.
        /// If not present, DSC will use `delete` when `_exist == false`.
        /// </summary>
        SetHandlesExist,

        /// <summary>
        /// The resource can handle a "what if" query directly.
        /// Otherwise, DSC will handle it synthetically.
        /// </summary>
        WhatIf,

        /// <summary>
        /// The resource can handle a "test" query directly.
        /// Otherwise, DSC will handle it synthetically.
        /// </summary>
        Test,

        /// <summary>
        /// Can call delete on the resource.
        /// </summary>
        Delete,

        /// <summary>
        /// Can call export on the resource.
        /// </summary>
        Export,

        /// <summary>
        /// Can call resolve on the resource.
        /// This can produce new resources, such as importing another configuration document.
        /// </summary>
        Resolve,
    }
}
