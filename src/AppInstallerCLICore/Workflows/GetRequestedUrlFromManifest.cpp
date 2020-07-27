#include "pch.h"
#include "GetRequestedUrlFromManifest.h"

#include "ManifestComparator.h"
#include "ShellExecuteInstallerHandler.h"
#include "WorkflowBase.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    void GetRequestedUrlFromManifest(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        Workflow::ManifestComparator manifestComparator(context.Args);
        const auto selectedLocalization = manifestComparator.GetPreferredLocalization(manifest);
        if (context.Args.Contains(Execution::Args::Type::Homepage)) {
            if (selectedLocalization.Homepage.empty())
            {
                context.Reporter.Info() << Resource::String::UrlNotFound << std::endl;
            }
            else {
                if (!selectedLocalization.Homepage.empty()) {
                    if (Utility::IsUrlRemote(selectedLocalization.Homepage)) {
                        context.Reporter.Info() << Resource::String::ManifestHomepageOpening << ": " << selectedLocalization.Homepage << std::endl;
                        context.Add<Execution::Data::HomepageUrl>(selectedLocalization.Homepage.c_str());
                    }
                    else
                    {
                        context.Reporter.Info() << Resource::String::ManifestHomepageDisplay << ": " << selectedLocalization.Homepage << std::endl;
                    }
                }
            }
        }
        if (context.Args.Contains(Execution::Args::Type::License)) {
            if (selectedLocalization.LicenseUrl.empty())
            {
                context.Reporter.Info() << Resource::String::UrlNotFound << std::endl;
            }
            else {
                if (!selectedLocalization.LicenseUrl.empty()) {
                    if (Utility::IsUrlRemote(selectedLocalization.LicenseUrl)) {
                        context.Reporter.Info() << Resource::String::ManifestLicenseOpening << ": " << selectedLocalization.LicenseUrl << std::endl;
                        context.Add<Execution::Data::LicenseUrl>(selectedLocalization.LicenseUrl.c_str());
                    }
                    else
                    {
                        context.Reporter.Info() << Resource::String::ManifestLicenseDisplay << ": " << selectedLocalization.LicenseUrl << std::endl;
                    }
                }
            }
        }
    }
}
