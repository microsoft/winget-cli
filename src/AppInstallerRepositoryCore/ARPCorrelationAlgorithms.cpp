// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ARPCorrelationAlgorithms.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Correlation
{
    using WordSequence = WordsEditDistanceMatchConfidenceAlgorithm::WordSequence;

    namespace
    {
        // A simple matrix class to hold score tables without having to allocate multiple arrays.
        struct Matrix
        {
            Matrix(size_t rows, size_t columns) : m_rows(rows), m_columns(columns), m_data(rows* columns) {}

            double& At(size_t i, size_t j)
            {
                return m_data[i * m_columns + j];
            }

        private:
            size_t m_rows;
            size_t m_columns;
            std::vector<double> m_data;
        };

        double EditDistanceScore(const std::vector<std::string>& s1, const std::vector<std::string>& s2)
        {
            // Naive implementation of edit distance (scaled over the sequence size).
            // This considers only the operations of adding and removing elements.

            if (s1.empty() || s2.empty())
            {
                return 0;
            }

            // distance[i, j] = distance between s1[0:i] and s2[0:j]
            // We don't need to hold more than two rows at a time, but it's simpler to keep the whole table.
            Matrix distance(s1.size() + 1, s2.size() + 1);

            for (size_t i = 0; i < s1.size(); ++i)
            {
                for (size_t j = 0; j < s2.size(); ++j)
                {
                    double& d = distance.At(i, j);
                    if (s1[i] == s2[j])
                    {
                        // If the two elements are equal, the distance is the same as from one element before.
                        // In case we are on the first element of one of the two sequences, the distance is
                        // equal to the cost of adding all the previous elements in the other
                        if (i == 0)
                        {
                            d = static_cast<double>(j);
                        }
                        else if (j == 0)
                        {
                            d = static_cast<double>(i);
                        }
                        else
                        {
                            d = distance.At(i - 1, j - 1);
                        }
                    }
                    else
                    {
                        // If the two elements are distinct, the score is the cost of removing the last element
                        // in one sequence plus the cost of editing the remainder of both.
                        if (i > 0 && j > 0)
                        {
                            d = 1 + std::min(distance.At(i - 1, j), distance.At(i, j - 1));
                        }
                        else if (i > 0)
                        {
                            d = 1 + distance.At(i - 1, j);
                        }
                        else if (j > 0)
                        {
                            d = 1 + distance.At(i, j - 1);
                        }
                        else
                        {
                            // Remove one and add the other
                            d = 2;
                        }
                    }
                }
            }

            // Maximum distance is equal to the sum of both lengths (removing all elements from one and adding all the elements from the other).
            // We use that to scale to [0,1].
            // A smaller distance represents a higher match, so we subtract from 1 for the final score
            double editDistance = distance.At(s1.size() - 1, s2.size() - 1);
            return 1 - editDistance / (s1.size() + s2.size());
        }
    }

    WordsEditDistanceMatchConfidenceAlgorithm::NameAndPublisher::NameAndPublisher(const WordSequence& name, const WordSequence& publisher) : Name(name), Publisher(publisher)
    {
        NamePublisher.insert(NamePublisher.end(), publisher.begin(), publisher.end());
        NamePublisher.insert(NamePublisher.end(), name.begin(), name.end());
    }

    WordsEditDistanceMatchConfidenceAlgorithm::NameAndPublisher::NameAndPublisher(WordSequence&& name, WordSequence&& publisher) : Name(std::move(name)), Publisher(std::move(publisher))
    {
        NamePublisher.insert(NamePublisher.end(), publisher.begin(), publisher.end());
        NamePublisher.insert(NamePublisher.end(), name.begin(), name.end());

    }

    void WordsEditDistanceMatchConfidenceAlgorithm::Init(const AppInstaller::Manifest::Manifest& manifest)
    {
        // We will use the name and publisher from each localization.
        m_namesAndPublishers.clear();

        WordSequence defaultPublisher;
        if (manifest.DefaultLocalization.Contains(Manifest::Localization::Publisher))
        {
            defaultPublisher = NormalizeAndPreparePublisher(manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
        }

        if (manifest.DefaultLocalization.Contains(Manifest::Localization::PackageName))
        {
            WordSequence defaultName = NormalizeAndPrepareName(manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
            m_namesAndPublishers.emplace_back(defaultName, defaultPublisher);

            for (const auto& loc : manifest.Localizations)
            {
                if (loc.Contains(Manifest::Localization::PackageName) || loc.Contains(Manifest::Localization::Publisher))
                {
                    auto name = loc.Contains(Manifest::Localization::PackageName) ? NormalizeAndPrepareName(loc.Get<Manifest::Localization::PackageName>()) : defaultName;
                    auto publisher = loc.Contains(Manifest::Localization::Publisher) ? NormalizeAndPreparePublisher(loc.Get<Manifest::Localization::Publisher>()) : defaultPublisher;

                    m_namesAndPublishers.emplace_back(std::move(name), std::move(publisher));
                }
            }
        }
    }

    double WordsEditDistanceMatchConfidenceAlgorithm::ComputeConfidence(const ARPEntry& arpEntry) const
    {
        // Name and Publisher are available as multi properties, but for ARP entries there will only be 0 or 1 values.
        NameAndPublisher arpNameAndPublisher(
            NormalizeAndPrepareName(arpEntry.Entry->GetInstalledVersion()->GetProperty(PackageVersionProperty::Name).get()),
            NormalizeAndPreparePublisher(arpEntry.Entry->GetInstalledVersion()->GetProperty(PackageVersionProperty::Publisher).get()));

        // Get the best score across all localizations
        double bestMatchingScore = 0;
        for (const auto& manifestNameAndPublisher : m_namesAndPublishers)
        {
            // Sometimes the publisher may be included in the name, for example Microsoft PowerToys as opposed to simply PowerToys.
            // This may happen both in the ARP entry and the manifest. We try adding it in case it is in one but not in both.
            auto nameScore = EditDistanceScore(manifestNameAndPublisher.Name, arpNameAndPublisher.Name);

            // Ignore cases where the name is not at all similar to avoid matching due to publisher only
            if (nameScore < m_nameMatchingScoreMinThreshold)
            {
                continue;
            }

            auto publisherScore = EditDistanceScore(manifestNameAndPublisher.Publisher, arpNameAndPublisher.Publisher);
            auto namePublisherScore = std::max(
                EditDistanceScore(manifestNameAndPublisher.NamePublisher, arpNameAndPublisher.Name),
                EditDistanceScore(manifestNameAndPublisher.Name, arpNameAndPublisher.NamePublisher));

            // Use the best between considering name and publisher as a single string or separately.
            auto score = std::max(
                nameScore * m_nameMatchingScoreWeight + publisherScore * (1 - m_nameMatchingScoreWeight),
                namePublisherScore);
            bestMatchingScore = std::max(bestMatchingScore, score);
        }

        // Factor in whether this entry is new
        auto result = bestMatchingScore * m_stringMatchingWeight + (arpEntry.IsNewOrUpdated ? 1 : 0) * (1 - m_stringMatchingWeight);

        return result;
    }

    WordSequence WordsEditDistanceMatchConfidenceAlgorithm::PrepareString(std::string_view s) const
    {
        return Utility::SplitIntoWords(Utility::FoldCase(s));
    }

    WordSequence WordsEditDistanceMatchConfidenceAlgorithm::NormalizeAndPrepareName(std::string_view name) const
    {
        return PrepareString(m_normalizer.NormalizeName(name).Name());
    }

    WordSequence WordsEditDistanceMatchConfidenceAlgorithm::NormalizeAndPreparePublisher(std::string_view publisher) const
    {
        return PrepareString(m_normalizer.NormalizePublisher(publisher));
    }
}
