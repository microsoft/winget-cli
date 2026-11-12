// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace LocalhostWebServer
{
    using System;
    using System.Net;
    using System.Threading.Tasks;
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.AspNetCore.Http;
    using Microsoft.AspNetCore.StaticFiles;
    using Microsoft.Extensions.Configuration;
    using Microsoft.Extensions.DependencyInjection;
    using Microsoft.Extensions.FileProviders;
    using Microsoft.Extensions.Hosting;

    public class Startup
    {
        public const string StaticFileRequestPath = "/TestKit";

        public static string StaticFileRoot { get; set; }

        public static string CertPath { get; set; }

        public static string CertPassword { get; set; }

        public static string OutCertFile { get; set; }

        public static int Port { get; set; }

        public static string LocalSourceJson { get; set; }

        public static string TestDataPath { get; set; }

        public static bool ExitBeforeRun { get; set; }

        public Startup(IConfiguration configuration)
        {
            Configuration = configuration;
        }

        public void ConfigureServices(IServiceCollection services)
        {
            services.AddControllersWithViews();
        }

        public IConfiguration Configuration { get; }

        public void Configure(IApplicationBuilder app, IWebHostEnvironment env)
        {
            if (env.IsDevelopment())
            {
                app.UseDeveloperExceptionPage();
            }

            app.UseHttpsRedirection();

            //Add .yaml and .msix mappings
            var provider = new FileExtensionContentTypeProvider();
            provider.Mappings[".yml"] = "application/x-yaml";
            provider.Mappings[".yaml"] = "application/x-yaml";
            provider.Mappings[".msix"] = "application/msix";
            provider.Mappings[".exe"] = "application/x-msdownload";
            provider.Mappings[".msi"] = "application/msi";
            provider.Mappings[".appx"] = "application/vns.ms-appx";
            provider.Mappings[".appxbundle"] = "application/vns.ms-appx";
            provider.Mappings[".mszyml"] = "application/x-ms-zip-yaml";
            provider.Mappings[".ttf"] = "application/x-font-ttf";

            //Enable static file serving
            app.UseStaticFiles(new StaticFileOptions
            {
                FileProvider = new PhysicalFileProvider(StaticFileRoot),
                RequestPath = StaticFileRequestPath,
                ContentTypeProvider = provider,
                ServeUnknownFileTypes = true,
                DefaultContentType = "application/octet-stream",
                OnPrepareResponse = ctx =>
                {
                    // REST source information files are extensionless JSON; set the correct content type
                    // and prevent caching so that each Connect() call gets a fresh response and
                    // triggers certificate validation.
                    if (ctx.File.Name == "information")
                    {
                        ctx.Context.Response.ContentType = "application/json";
                        ctx.Context.Response.Headers.CacheControl = "no-store";
                    }
                },
            });

            app.UseDirectoryBrowser(new DirectoryBrowserOptions
            {
                FileProvider = new PhysicalFileProvider(StaticFileRoot),
                RequestPath = StaticFileRequestPath
            });

            app.UseRouting();

            app.UseAuthorization();

            app.UseEndpoints(endpoints =>
            {
                endpoints.MapGet("/DownloadRedirect/{status:int}", context =>
                {
                    string target = context.Request.Query["target"].ToString();
                    if (context.Connection.RemoteIpAddress is null ||
                        !IPAddress.IsLoopback(context.Connection.RemoteIpAddress) ||
                        !int.TryParse(context.Request.RouteValues["status"].ToString(), out int status) ||
                        status is not (301 or 302 or 303 or 307 or 308) ||
                        !Uri.TryCreate(target, UriKind.Absolute, out Uri uri) ||
                        !uri.IsLoopback ||
                        (uri.Scheme != Uri.UriSchemeHttp && uri.Scheme != Uri.UriSchemeHttps))
                    {
                        context.Response.StatusCode = StatusCodes.Status400BadRequest;
                        return Task.CompletedTask;
                    }

                    context.Response.StatusCode = status;
                    context.Response.Headers.Location = target;
                    context.Response.Headers.CacheControl = "no-store";
                    context.Response.ContentLength = 0;
                    return Task.CompletedTask;
                });

                endpoints.MapGet("/DownloadHeaders", context =>
                {
                    if (context.Connection.RemoteIpAddress is null ||
                        !IPAddress.IsLoopback(context.Connection.RemoteIpAddress))
                    {
                        context.Response.StatusCode = StatusCodes.Status403Forbidden;
                        return Task.CompletedTask;
                    }

                    context.Response.ContentType = "text/plain";
                    context.Response.Headers.CacheControl = "no-store";
                    return context.Response.WriteAsync(
                        $"Authorization={context.Request.Headers.ContainsKey("Authorization")};" +
                        $"x-test-auth={context.Request.Headers.ContainsKey("x-test-auth")};" +
                        $"x-test-header={context.Request.Headers.ContainsKey("x-test-header")}");
                });

                endpoints.MapDefaultControllerRoute();
            });
        }
    }
}
