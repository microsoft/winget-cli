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
                std::vector<std::pair<AppInstaller::ProgressType, double>> progressWeights;

                // There is no download operation for remove operation, so use only percentage based progress to account for uninstall.
                if (removeOperation)
                {
                    // it is percentage based progress.
                    progressWeights.push_back(std::make_pair(AppInstaller::ProgressType::Percent, 1.0));
                }
                else
                {
                    // Add/Update operation has two progress types:
                    // 1. Bytes for downloading index and
                    // 2. Percent for index installation.
                    progressWeights.push_back(std::make_pair(AppInstaller::ProgressType::Bytes, 0.7));
                    progressWeights.push_back(std::make_pair(AppInstaller::ProgressType::Percent, 0.3));
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

    void CompletionOnlyProgressSink::OnProgress(uint64_t /*current*/, uint64_t /*maximum*/, AppInstaller::ProgressType /*type*/)
    {
    }

    void CompletionOnlyProgressSink::SetProgressMessage(std::string_view /*message*/)
    {
    }

    void CompletionOnlyProgressSink::BeginProgress()
    {
        m_progressReporter(0);
    }

    void CompletionOnlyProgressSink::EndProgress(bool /*hideProgressWhenDone*/)
    {
        m_progressReporter(100);
    }

    PreIndexedPackageCatalogProgressSink::PreIndexedPackageCatalogProgressSink(std::vector<std::pair<AppInstaller::ProgressType, double>> progressWeights, std::function<void(double)> progressReporter) :
        m_progressWeights(progressWeights), m_progressReporter(progressReporter)
    {
        if (!m_progressReporter)
        {
            THROW_HR(E_INVALIDARG);
        }

        // If no weights are provided, default to percent.
        if (m_progressWeights.empty())
        {
            m_progressWeights.push_back(std::make_pair(AppInstaller::ProgressType::Percent, 1.0));
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
        double totalWeight = 0.0;

        // Calculate the total progress.
        for (const auto& [progressType, weight] : m_progressWeights)
        {
            double progressValue = m_progressValues[progressType];

            // [NOTE:] Sequential execution assumption & Handling incomplete progress reports :
            // This progress calculation assumes that each operation is executed sequentially, meaning the download must be complete before
            // the installation begins.If the download fails, the installation will not proceed.However, there may be cases where the previous
            // operation completes successfully, but its onprogress callback does not report 100% completion(e.g., the last progress report for
            // the download was at 90%, but the download is complete, and the installation has started).This can result in the total progress not
            // reaching 100% after the last operation completes due to the gap in the previous operation's progress report.To handle this, consider
            // the progress for the last operation as complete by assigning its full weight while computing progress for the following operation.
            // For example, while computing progress for the installation, consider the download operation complete even if it did not report progress
            // exactly at 100%.
            if (progressValue != 0)
            {
                totalProgress = totalWeight;
            }

            // Adjust the total progress value based on the weight.
            totalWeight += weight;
            totalProgress += progressValue * weight;
        }

        m_progressReporter(totalProgress * 100);
    }

    void PreIndexedPackageCatalogProgressSink::SetProgressMessage(std::string_view /*message*/)
    {
    }

    void PreIndexedPackageCatalogProgressSink::BeginProgress()
    {
        m_progressReporter(0);
    }

    void PreIndexedPackageCatalogProgressSink::EndProgress(bool /*hideProgressWhenDone*/)
    {
        m_progressReporter(100);
    }
}
