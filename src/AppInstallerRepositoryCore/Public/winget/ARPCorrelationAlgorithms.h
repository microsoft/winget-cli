// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <winget/ARPCorrelation.h>
#include <winget/Manifest.h>
#include <winget/NameNormalization.h>
#include <winget/RepositorySearch.h>
#include <winget/RepositorySource.h>

namespace AppInstaller::Repository::Correlation
{
    struct EmptyMatchConfidenceAlgorithm : public IARPMatchConfidenceAlgorithm
    {
        void Init(const AppInstaller::Manifest::Manifest&) override {}
        double ComputeConfidence(const ARPEntry&) const override { return 0; }
    };

    // Algorithm that computes the match confidence by looking at the edit distance between
    // the sequences of words in the name and publisher.
    // The edit distance for this allows only adding or removing words, not editing them,
    // as that would make any two names too similar.
    struct WordsEditDistanceMatchConfidenceAlgorithm : public IARPMatchConfidenceAlgorithm
    {
        using WordSequence = std::vector<std::string>;

        struct NameAndPublisher
        {
            NameAndPublisher(const WordSequence& name, const WordSequence& publisher);
            NameAndPublisher(WordSequence&& name, WordSequence&& publisher);

            WordSequence Name;
            WordSequence Publisher;
            WordSequence NamePublisher;
        };

        void Init(const AppInstaller::Manifest::Manifest& manifest) override;
        double ComputeConfidence(const ARPEntry& arpEntry) const override;

    private:
        WordSequence PrepareString(std::string_view s) const;
        WordSequence NormalizeAndPrepareName(std::string_view name) const;
        WordSequence NormalizeAndPreparePublisher(std::string_view publisher) const;

        AppInstaller::Utility::NameNormalizer m_normalizer{ AppInstaller::Utility::NormalizationVersion::InitialPreserveWhiteSpace };
        std::vector<NameAndPublisher> m_namesAndPublishers;

        // Parameters for the algorithm

        // How much weight to give to the string matching score.
        // The rest is assigned by whether the entry is new.
        const double m_stringMatchingWeight = 0.8;

        // How much weight to give to the matching score of the package name;
        // the rest is assigned to the publisher score.
        const double m_nameMatchingScoreWeight = 2. / 3.;

        // Minimum score needed in the name for us to accept a match.
        // This prevents us from being misled by a long match in the publisher.
        const double m_nameMatchingScoreMinThreshold = 0.2;
    };
}
