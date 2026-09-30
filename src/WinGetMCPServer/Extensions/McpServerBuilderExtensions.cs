// -----------------------------------------------------------------------------
// <copyright file="McpServerBuilderExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Extensions
{
    using System.Diagnostics.CodeAnalysis;
    using System.Reflection;
    using Microsoft.Extensions.DependencyInjection;
    using ModelContextProtocol.Protocol;
    using ModelContextProtocol.Server;

    /// <summary>
    /// Extensions for IMcpServerBuilder.
    /// </summary>
    internal static class McpServerBuilderExtensions
    {
        /// <summary>
        /// Registers the tools declared by a type, associating the given icons with each of them.
        /// </summary>
        /// <remarks>
        /// This mirrors the SDK's <c>WithTools&lt;TToolType&gt;</c> because the
        /// <c>McpServerTool</c> attribute can only carry a bare icon source string and cannot express
        /// the mime type or size. Only <c>Icons</c> is supplied here; every other value is left unset
        /// so that the name, title and annotations continue to come from the attributes applied to the
        /// tool methods.
        /// </remarks>
        /// <typeparam name="TToolType">The type declaring the tools.</typeparam>
        /// <param name="builder">The builder to register with.</param>
        /// <param name="icons">The icons to associate with each tool, if any.</param>
        /// <returns>The builder provided in <paramref name="builder"/>.</returns>
        public static IMcpServerBuilder WithToolsAndIcons<[DynamicallyAccessedMembers(
            DynamicallyAccessedMemberTypes.PublicMethods |
            DynamicallyAccessedMemberTypes.NonPublicMethods |
            DynamicallyAccessedMemberTypes.PublicConstructors)] TToolType>(
            this IMcpServerBuilder builder,
            IList<Icon>? icons)
        {
            if (icons == null || icons.Count == 0)
            {
                return builder.WithTools<TToolType>();
            }

            foreach (var toolMethod in typeof(TToolType).GetMethods(
                BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.Instance))
            {
                if (toolMethod.GetCustomAttribute<McpServerToolAttribute>() == null)
                {
                    continue;
                }

                builder.Services.AddSingleton((Func<IServiceProvider, McpServerTool>)(toolMethod.IsStatic ?
                    services => McpServerTool.Create(
                        toolMethod,
                        options: new() { Services = services, Icons = icons }) :
                    services => McpServerTool.Create(
                        toolMethod,
                        requestContext => CreateTarget(requestContext.Services, typeof(TToolType)),
                        new() { Services = services, Icons = icons })));
            }

            return builder;
        }

        private static object CreateTarget(
            IServiceProvider? services,
            [DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.PublicConstructors)] Type type)
        {
            return services != null ? ActivatorUtilities.CreateInstance(services, type) : Activator.CreateInstance(type)!;
        }
    }
}
