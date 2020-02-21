// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/PreIndexedSource.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // Gets the package family name from the details.
        std::string GetPackageFamilyNameFromDetails(const SourceDetails& details)
        {
            THROW_HR_IF(E_UNEXPECTED, details.Data.empty());
            return details.Data;
        }

        // Creates a name for the cross process reader-writer lock given the details.
        std::string CreateNameForCPRWL(const SourceDetails& details)
        {
            using namespace std::string_literals;
            // The only relevant data is the package family name
            return "PreIndexedSourceCPRWL_"s + GetPackageFamilyNameFromDetails(details);
        }

        // *Should only be called when under a CrossProcessReaderWriteLock*
        auto GetPackageFromDetails(const SourceDetails& details)
        {
            std::wstring packageFamilyName = Utility::ConvertToUTF16(GetPackageFamilyNameFromDetails(details));

            auto currentPackage = winrt::Windows::ApplicationModel::Package::Current();
            auto dependencies = currentPackage.Dependencies();
            for (uint32_t i = 0; i < dependencies.Size(); ++i)
            {
                decltype(currentPackage) package = dependencies.GetAt(i);
                if (package.Id().FamilyName() == packageFamilyName)
                {
                    if (package.IsOptional())
                    {
                        return package;
                    }
                    else
                    {
                        AICLI_LOG(Repo, Error, << "Source details references a non-optional package: " << details.Name << " => " << GetPackageFamilyNameFromDetails(details));
                        return nullptr;
                    }
                }
            }
            return nullptr;
        }

        // Source factory for PreIndexedSource
        struct PreIndexedSourceFactory : public ISourceFactory
        {
            bool IsInitialized(const SourceDetails& details) override
            {
                return !details.Data.empty();
            }

            std::unique_ptr<ISource> Create(const SourceDetails& details) override
            {
                return std::make_unique<PreIndexedSource>(details);
            }

            void Update(SourceDetails& details) override
            {
                if (!IsInitialized(details))
                {
                    // If not initialized, we need to open the package and get the family name.
                }

            }

            void Remove(const SourceDetails& details) override
            {
                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForWrite(CreateNameForCPRWL(details));

                // Get the package referenced by the details
                auto optionalPackage = GetPackageFromDetails(details);
                if (!optionalPackage)
                {
                    AICLI_LOG(Repo, Error, << "Source details references an unknown package: " << details.Name << " => " << GetPackageFamilyNameFromDetails(details));
                    return;
                }

                // Mark package as not in use
                optionalPackage.SetInUseAsync(false).get();

                // Remove package
                std::vector<winrt::hstring> packageList;
                packageList.emplace_back(optionalPackage.Id().FamilyName());

                auto currentCatalog = winrt::Windows::ApplicationModel::PackageCatalog::OpenForCurrentPackage();
                auto removeResult = currentCatalog.RemoveOptionalPackagesAsync(std::move(packageList)).get();
                if (FAILED(removeResult.ExtendedError()))
                {
                    AICLI_LOG(Repo, Error, << "Failed to remove package for source: " << details.Name << " => " << GetPackageFamilyNameFromDetails(details) <<
                        " [0x" << std::hex << std::setw(8) << std::setfill('0') << removeResult.ExtendedError() << "]");
                }
            }
        };
    }

    PreIndexedSource::PreIndexedSource(const SourceDetails& details) :
        m_details(details)
    {
        THROW_HR(E_NOTIMPL);
    }

    std::unique_ptr<ISourceFactory> PreIndexedSource::CreateFactory()
    {
        return std::make_unique<PreIndexedSourceFactory>();
    }

    const SourceDetails& PreIndexedSource::GetDetails() const
    {
        return m_details;
    }

    SearchResult PreIndexedSource::Search(const SearchRequest& request) const
    {
        UNREFERENCED_PARAMETER(request);
        THROW_HR(E_NOTIMPL);
    }
}
