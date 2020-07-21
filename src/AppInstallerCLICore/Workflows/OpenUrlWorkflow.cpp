#include "pch.h"
#include "OpenUrlWorkflow.h"

#include "Command.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // ShellExecutes the given path.
        void InvokeShellExecute(const std::string& uri)
        {
            AICLI_LOG(CLI, Info, << "Opening url. URI: " << uri);

            ShellExecuteA(nullptr, nullptr, static_cast<LPCSTR>(uri.c_str()), nullptr, nullptr, SW_NORMAL);
        }
    }

	void OpenUrlInDefaultBrowser(Execution::Context& context)
    {
		if (context.Args.Contains(Execution::Args::Type::Homepage))
		{
			const std::string homepageUrl = context.Get<Execution::Data::HomepageUrl>();
            InvokeShellExecute(homepageUrl);
		}
        if (context.Args.Contains(Execution::Args::Type::License))
        {
	        const std::string licenseUrl = context.Get<Execution::Data::LicenseUrl>();
            InvokeShellExecute(licenseUrl);
        }

    }
}
