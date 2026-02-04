// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/ConfigurableTestSourceFactory.h"

#include <json/json.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // The configuration defined for a source.
        // This can be added to as new scenarios are needed for testing.
        struct TestSourceConfiguration
        {
            TestSourceConfiguration(const std::string& config)
            {
                Json::Value root;
                Json::CharReaderBuilder builder;
                const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

                std::string error;

                if (reader->parse(config.c_str(), config.c_str() + config.size(), &root, &error))
                {
                    // TODO: If this becomes more dynamic, refactor the UserSettings code to make it easier to leverage here
                    ParseHR(root, ".OpenHR", OpenHR);
                    ParseHR(root, ".SearchHR", SearchHR);
                    ParseBool(root, ".ContainsPackage", ContainsPackage);
                }
                else
                {
                    AICLI_LOG(Repo, Error, << "Error parsing test source config: " << error);
                    THROW_HR_MSG(E_INVALIDARG, "%hs", error.c_str());
                }
            }

            // The HR to throw on Factory::Create (if FAILED)
            HRESULT OpenHR = S_OK;

            // The HR to throw on Source::Search (if FAILED)
            HRESULT SearchHR = S_OK;

            // If a result should be returned by search.
            bool ContainsPackage = false;

        private:
            static void ParseHR(const Json::Value& root, const std::string& path, HRESULT& hr)
            {
                const Json::Path jsonPath(path);
                Json::Value node = jsonPath.resolve(root);
                if (!node.isNull())
                {
                    if (node.isIntegral())
                    {
                        hr = node.asInt();
                    }
                    else if (node.isString())
                    {
                        hr = static_cast<HRESULT>(std::strtoll(node.asString().c_str(), nullptr, 0));
                    }
                }
            }

            static void ParseBool(const Json::Value& root, const std::string& path, bool& value)
            {
                const Json::Path jsonPath(path);
                Json::Value node = jsonPath.resolve(root);
                if (!node.isNull())
                {
                    if (node.isBool())
                    {
                        value = node.asBool();
                    }
                }
            }
        };

        // A test package that contains test data.
        struct TestPackage : public std::enable_shared_from_this<TestPackage>, public ICompositePackage, public IPackage, public IPackageVersion
        {
            TestPackage(std::shared_ptr<ISource> source) : m_source(std::move(source)) {}

            Utility::LocIndString GetProperty(PackageProperty property) const override
            {
                switch (property)
                {
                case PackageProperty::Id:
                    return "ConfigurableTestSource Package Identifier"_lis;
                case PackageProperty::Name:
                    return "ConfigurableTestSource Package Name"_lis;
                }

                return {};
            }

            std::shared_ptr<IPackage> GetInstalled() override
            {
                return nullptr;
            }

            std::vector<std::shared_ptr<IPackage>> GetAvailable() override
            {
                return { shared_from_this() };
            }

            std::vector<Utility::LocIndString> GetMultiProperty(PackageMultiProperty) const override
            {
                return {};
            }

            Source GetSource() const override
            {
                return { m_source };
            }

            bool IsSame(const IPackage*) const override
            {
                return false;
            }

            const void* CastTo(IPackageType) const override
            {
                return nullptr;
            }

            std::vector<PackageVersionKey> GetVersionKeys() const override
            {
                return { {} };
            }

            std::shared_ptr<IPackageVersion> GetVersion(const PackageVersionKey&) const override
            {
                return std::static_pointer_cast<IPackageVersion>(NonConstSharedFromThis());
            }

            std::shared_ptr<IPackageVersion> GetLatestVersion() const override
            {
                return std::static_pointer_cast<IPackageVersion>(NonConstSharedFromThis());
            }

            Utility::LocIndString GetProperty(PackageVersionProperty property) const override
            {
                switch (property)
                {
                case PackageVersionProperty::Id:
                    return "ConfigurableTestSource Package Version Identifier"_lis;
                case PackageVersionProperty::Name:
                    return "ConfigurableTestSource Package Version Name"_lis;
                case PackageVersionProperty::SourceIdentifier:
                    return "ConfigurableTestSource Package Version Source Identifier"_lis;
                case PackageVersionProperty::SourceName:
                    return "ConfigurableTestSource Package Version Source Name"_lis;
                case PackageVersionProperty::Version:
                    return "ConfigurableTestSource Package Version Version"_lis;
                case PackageVersionProperty::Channel:
                    return "ConfigurableTestSource Package Version Channel"_lis;
                case PackageVersionProperty::RelativePath:
                    return "ConfigurableTestSource Package Version Relative Path"_lis;
                case PackageVersionProperty::ManifestSHA256Hash:
                    return "ConfigurableTestSource Package Version Manifest SHA 256 Hash"_lis;
                case PackageVersionProperty::Publisher:
                    return "ConfigurableTestSource Package Version Publisher"_lis;
                case PackageVersionProperty::ArpMinVersion:
                    return "ConfigurableTestSource Package Version Arp Min Version"_lis;
                case PackageVersionProperty::ArpMaxVersion:
                    return "ConfigurableTestSource Package Version Arp Max Version"_lis;
                case PackageVersionProperty::Moniker:
                    return "ConfigurableTestSource Package Version Moniker"_lis;
                }

                return {};
            }

            std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty) const override
            {
                return {};
            }

            Manifest::Manifest GetManifest() override
            {
                Manifest::Manifest result;

                result.Id = "ConfigurableTestSource Manifest Identifier";
                result.CurrentLocalization.Add<Manifest::Localization::PackageName>("ConfigurableTestSource Manifest Name");

                return result;
            }

            Metadata GetMetadata() const override
            {
                return {};
            }

        private:
            std::shared_ptr<TestPackage> NonConstSharedFromThis() const
            {
                return const_cast<TestPackage*>(this)->shared_from_this();
            }

            std::shared_ptr<ISource> m_source;
        };

        // The configurable source itself.
        struct ConfigurableTestSource : public std::enable_shared_from_this<ConfigurableTestSource>, public ISource
        {
            static constexpr ISourceType SourceType = ISourceType::ConfigurableTestSource;

            ConfigurableTestSource(const SourceDetails& details, const TestSourceConfiguration& config) :
                m_details(details), m_config(config) {}

            const SourceDetails& GetDetails() const override { return m_details; }

            const std::string& GetIdentifier() const override { return m_details.Identifier; }

            SearchResult Search(const SearchRequest&) const override
            {
                THROW_IF_FAILED(m_config.SearchHR);

                SearchResult result;

                if (m_config.ContainsPackage)
                {
                    std::shared_ptr<TestPackage> package = std::make_shared<TestPackage>(NonConstSharedFromThis());
                    PackageMatchFilter packageFilter{ {}, {} };

                    result.Matches.emplace_back(std::move(package), std::move(packageFilter));
                }

                return result;
            }

            void* CastTo(ISourceType type) override
            {
                if (type == SourceType)
                {
                    return this;
                }

                return nullptr;
            }

        private:
            std::shared_ptr<ConfigurableTestSource> NonConstSharedFromThis() const
            {
                return const_cast<ConfigurableTestSource*>(this)->shared_from_this();
            }

            SourceDetails m_details;
            TestSourceConfiguration m_config;
        };

        struct ConfigurableTestSourceReference : public ISourceReference
        {
            ConfigurableTestSourceReference(const SourceDetails& details) : m_details(details)
            {
                m_details.Identifier = "*ConfigurableTestSource";
            }

            std::string GetIdentifier() override { return m_details.Identifier; }

            SourceDetails& GetDetails() override { return m_details; };

            bool SetCustomHeader(std::optional<std::string>) override { return true; }

            std::shared_ptr<ISource> Open(IProgressCallback&) override
            {
                // enables `source add` with FAILED(OpenHR)
                TestSourceConfiguration config{ m_details.Arg };
                THROW_IF_FAILED(config.OpenHR);
                return std::make_shared<ConfigurableTestSource>(m_details, config);
            }

        private:
            SourceDetails m_details;
        };

        // The actual factory implementation.
        struct ConfigurableTestSourceFactoryImpl : public ISourceFactory
        {
            std::string_view TypeName() const override final
            {
                return ConfigurableTestSourceFactory::Type();
            }

            std::shared_ptr<ISourceReference> Create(const SourceDetails& details) override final
            {
                return std::make_shared<ConfigurableTestSourceReference>(details);
            }

            bool Add(SourceDetails& details, IProgressCallback&) override final
            {
                // Attempt to parse the configuration so that we can fail at the appropriate point
                TestSourceConfiguration config{ details.Arg };
                return true;
            }

            bool Update(const SourceDetails&, IProgressCallback&) override final
            {
                return true;
            }

            bool BackgroundUpdate(const SourceDetails&, IProgressCallback&) override final
            {
                return true;
            }

            bool Remove(const SourceDetails&, IProgressCallback&) override final
            {
                return true;
            }
        };
    }

    std::unique_ptr<ISourceFactory> ConfigurableTestSourceFactory::Create()
    {
        return std::make_unique<ConfigurableTestSourceFactoryImpl>();
    }
}
