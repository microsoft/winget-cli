// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageCatalogProgress.h"
#include "AppInstallerStrings.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"

using namespace AppInstaller;
using namespace AppInstaller::Repository;

namespace winrt::Microsoft::Management::Deployment
{
    namespace ProgressSinkFactory
    {
        std::shared_ptr<AppInstaller::IProgressSink> CreatePackageCatalogProgressSink(std::string sourceType, std::function<void(double)> progressReporter, bool removeOperation)
        {
            if (sourceType.empty()
                || Utility::CaseInsensitiveEquals( Repository::Microsoft::PredefinedInstalledSourceFactory::Type(), sourceType))
            {
                std::unordered_map<AppInstaller::ProgressType, double> progressWeights;

                // There is no download operation for remove operation, so remove the download progress.
                if (removeOperation)
                {
                    // it is percentage based progress.
                    progressWeights.insert_or_assign(AppInstaller::ProgressType::Bytes, 0);
                }
                else
                {
                    // Add/Update operation has two progress types:
                    // 1. Bytes for downloading index and
                    // 2. Percent for index installation.
                    progressWeights.insert_or_assign(AppInstaller::ProgressType::Bytes, 0.7);
                    progressWeights.insert_or_assign(AppInstaller::ProgressType::Percent, 0.3);
                }

                return std::make_shared<PreIndexedPackageCatalogProgressSink>(progressWeights, progressReporter);
            }
            else
            {
                return std::make_shared<CompletionOnlyProgressSink>(progressReporter);
            }
        }
    }

    CompletionOnlyProgressSink::CompletionOnlyProgressSink(std::function<void(double)> progressReporter) :
        m_progressReporter(progressReporter)
    {
        if (!m_progressReporter)
        {
            THROW_HR(E_INVALIDARG);
        }
    }

    void CompletionOnlyProgressSink::OnProgress(uint64_t current, uint64_t maximum, AppInstaller::ProgressType type)
    {
        UNREFERENCED_PARAMETER(current);
        UNREFERENCED_PARAMETER(maximum);
        UNREFERENCED_PARAMETER(type);
    }

    void CompletionOnlyProgressSink::SetProgressMessage(std::string_view message)
    {
        UNREFERENCED_PARAMETER(message);
    }

    void CompletionOnlyProgressSink::BeginProgress()
    {
        m_progressReporter(0);
    }

    void CompletionOnlyProgressSink::EndProgress(bool hideProgressWhenDone)
    {
        UNREFERENCED_PARAMETER(hideProgressWhenDone);
        m_progressReporter(100);
    }

    PreIndexedPackageCatalogProgressSink::PreIndexedPackageCatalogProgressSink(std::unordered_map<AppInstaller::ProgressType, double> progressWeights, std::function<void(double)> progressReporter) :
        m_progressWeights(progressWeights), m_progressReporter(progressReporter)
    {
        if (!m_progressReporter)
        {
            THROW_HR(E_INVALIDARG);
        }

        // If no weights are provided, default to percent.
        if (m_progressWeights.empty())
        {
            m_progressWeights.insert_or_assign(AppInstaller::ProgressType::Percent, 1.0);
        }

        // Calculate the total weight.
        double totalWeight = 0;
        for (const auto& weight : m_progressWeights)
        {
            if (weight.first != AppInstaller::ProgressType::None)
            {
                totalWeight += weight.second;
            }
        }

        // If the total weight is greater than 1, throw an exception.
        if (totalWeight != 1.0)
        {
            THROW_HR(E_INVALIDARG);
        }
    }

    void PreIndexedPackageCatalogProgressSink::OnProgress(uint64_t current, uint64_t maximum, AppInstaller::ProgressType type)
    {
        if (maximum == 0 || type == AppInstaller::ProgressType::None)
        {
            return;
        }

        double progress = static_cast<double>(current) / maximum;
        m_progressValues[type] = progress;

        double totalProgress = 0.0;

        // Calculate the total progress.
        for (const auto& [progressType, weight] : m_progressWeights)
        {
            // Adjust the total progress value based on the weight.
            totalProgress += m_progressValues[progressType] * weight;
        }

        m_progressReporter(totalProgress * 100);
    }

    void PreIndexedPackageCatalogProgressSink::SetProgressMessage(std::string_view message)
    {
        UNREFERENCED_PARAMETER(message);
    }

    void PreIndexedPackageCatalogProgressSink::BeginProgress()
    {
        OnProgress(0, 1, AppInstaller::ProgressType::None);
    }

    void PreIndexedPackageCatalogProgressSink::EndProgress(bool hideProgressWhenDone)
    {
        UNREFERENCED_PARAMETER(hideProgressWhenDone);
        OnProgress(1, 1, AppInstaller::ProgressType::None);
    }
}
