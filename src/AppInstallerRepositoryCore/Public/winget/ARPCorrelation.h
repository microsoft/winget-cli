// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller
{
    namespace Manifest
    {
        struct Manifest;
    }

    namespace Repository
    {
        struct IPackage;
    }
}

namespace AppInstaller::Repository::Correlation
{

    // An algorithm for correlating a package with an ARP entry. 
    struct ARPCorrelationAlgorithmBase
    {
        virtual ~ARPCorrelationAlgorithmBase() = default;

        // Computes a matching score between a package manifest and a manifest entry.
        // A higher score indicates a more certain match.
        // The possible range of values is determined by the algorithm.
        virtual double GetMatchingScore(
            const Manifest::Manifest& manifest,
            std::shared_ptr<IPackage> arpEntry) const = 0;

        // Gets the minimum score needed by this algorithm for something to be considered a match.
        virtual double GetMatchingThreshold() const = 0;
    };

#define DEFINE_CORRELATION_ALGORITHM(_name_) \
    struct _name_ : public ARPCorrelationAlgorithmBase \
    { \
        double GetMatchingScore(const Manifest::Manifest& manifest, std::shared_ptr<IPackage> arpEntry) const override; \
        double GetMatchingThreshold() const override; \
    }

    // We define multiple algorithms to compare how good their results are,
    // but we will only use one in the product.

    // No correlation between packages and ARP entries
    DEFINE_CORRELATION_ALGORITHM(NoMatch);

    // This is the algorithm we will actually use.
    using ARPCorrelationAlgorithm = NoMatch;
}