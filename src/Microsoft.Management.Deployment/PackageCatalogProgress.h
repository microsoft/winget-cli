// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerProgress.h"
#include <unordered_map>
#include <functional>

namespace winrt::Microsoft::Management::Deployment
{
    namespace ProgressSinkFactory
    {
        /// <summary>
        /// Creates a progress sink for package catalog operations based on sourceType.
        /// </summary>
        /// <param name="sourceType">sourceType.</param>
        /// <param name="progressReporter">callback function that reports progress to caller.</param>
        /// <param name="removeOperation"> Default value is false. Identifies if the operation is a PackageCatalog removal and requests the ProgressSink.</param>
        /// <returns>IProgressSink.</returns>
        std::shared_ptr<AppInstaller::IProgressSink> CreatePackageCatalogProgressSink(std::string sourceType, std::function<void(double)> progressReporter, bool removeOperation = false);
    }

    /// <summary>
    /// Progress sink that only reports start and completion to caller.
    /// </summary>
    struct CompletionOnlyProgressSink : AppInstaller::IProgressSink
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="progressReporter">callback that reports progress to caller.</param>
        CompletionOnlyProgressSink(std::function<void(double)> progressReporter);

        void OnProgress(uint64_t current, uint64_t maximum, AppInstaller::ProgressType type) override;
        void SetProgressMessage(std::string_view message) override;
        void BeginProgress() override;
        void EndProgress(bool hideProgressWhenDone) override;

    private:
        std::function<void(double)> m_progressReporter;
    };

    /// <summary>
    /// Progress sink for pre-indexed package catalog operations.
    /// capable of reporting progress for download and installation of index.
    /// Add/update operation has two progress types: Bytes for downloading index and Percent for index installation.
    /// Remove operation has only percentage based progress.
    /// </summary>
    struct PreIndexedPackageCatalogProgressSink : AppInstaller::IProgressSink
    {
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="progressWeights">ProgressType weight map.</param>
        /// <param name="progressReporter">Callback function that reports progress to caller.</param>
        PreIndexedPackageCatalogProgressSink(std::vector<std::pair<AppInstaller::ProgressType, double>> progressWeights, std::function<void(double)> progressReporter);

        /// <summary>
        /// Reports combined progress to caller when configured for multiple progress types.
        /// </summary>
        /// <param name="current">The current progress value.</param>
        /// <param name="maximum">The maximum progress value.</param>
        /// <param name="type">ProgressType for which progress is applicable.</param>
        void OnProgress(uint64_t current, uint64_t maximum, AppInstaller::ProgressType type) override;
        void SetProgressMessage(std::string_view message) override;
        void BeginProgress() override;
        void EndProgress(bool hideProgressWhenDone) override;

    private:
        std::vector<std::pair<AppInstaller::ProgressType, double>> m_progressWeights;
        std::function<void(double)> m_progressReporter;
        std::unordered_map<AppInstaller::ProgressType, double> m_progressValues;
    };
}
