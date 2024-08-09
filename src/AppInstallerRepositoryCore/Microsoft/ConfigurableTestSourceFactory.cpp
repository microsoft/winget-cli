// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/ConfigurableTestSourceFactory.h"

#include <json/json.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

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
        };

        // The configurable source itself.
        struct ConfigurableTestSource : public ISource
        {
            static constexpr ISourceType SourceType = ISourceType::ConfigurableTestSource;

            ConfigurableTestSource(const SourceDetails& details, const TestSourceConfiguration& config) :
                m_details(details), m_config(config) {}

            const SourceDetails& GetDetails() const override { return m_details; }

            const std::string& GetIdentifier() const override { return m_details.Identifier; }

            SearchResult Search(const SearchRequest&) const override
            {
                THROW_IF_FAILED(m_config.SearchHR);
                return {};
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
