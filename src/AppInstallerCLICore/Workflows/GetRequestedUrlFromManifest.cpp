#include "pch.h"
#include "GetRequestedUrlFromManifest.h"

#include "ManifestComparator.h"
#include "ShellExecuteInstallerHandler.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
	void GetRequestedUrlFromManifest(Execution::Context& context)
	{
		if (context.Contains(Execution::Data::SearchResult)) {
			const auto& searchResult = context.Get<Execution::Data::SearchResult>();
			if (searchResult.Matches.size() > 1)
			{
				context.Reporter.Warn() << "More than one result was found please refine your query. Use --Id, --Name --Tag" << std::endl;
			}
			else
			{
				if (context.Contains(Execution::Data::Manifest)) {
					const auto& manifest = context.Get<Execution::Data::Manifest>();
					Workflow::ManifestComparator manifestComparator(context.Args);
					const auto selectedLocalization = manifestComparator.GetPreferredLocalization(manifest);
					if (context.Args.Contains(Execution::Args::Type::Homepage)) {
						if (selectedLocalization.Homepage.empty())
						{
							context.Reporter.Info() << "No Homepage found within manifest" << std::endl;
						}
						else {
							context.Reporter.Info() << "Homepage: " << selectedLocalization.Homepage << std::endl;
							context.Add<Execution::Data::HomepageUrl>(selectedLocalization.Homepage.c_str());
						}
					}
					if (context.Args.Contains(Execution::Args::Type::License)) {
						if (selectedLocalization.LicenseUrl.empty())
						{
							context.Reporter.Info() << "No License found within manifest" << std::endl;
						}
						else {
							context.Reporter.Info() << "License: " << selectedLocalization.LicenseUrl << std::endl;
							context.Add<Execution::Data::LicenseUrl>(selectedLocalization.LicenseUrl.c_str());
						}
					}
				}
				else
				{
					context.Reporter.Info() << "No Homepage found within manifest" << std::endl;
				}
			}
		}
	}
}
