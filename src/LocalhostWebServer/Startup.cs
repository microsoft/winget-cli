// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace LocalhostWebServer
{
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
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

            //Enable static file serving
            app.UseStaticFiles(new StaticFileOptions
            {
                FileProvider = new PhysicalFileProvider(StaticFileRoot),
                RequestPath = StaticFileRequestPath,
                ContentTypeProvider = provider,
                ServeUnknownFileTypes = true,
                DefaultContentType = "application/octet-stream"
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
                endpoints.MapDefaultControllerRoute();
            });
        }
    }
}
