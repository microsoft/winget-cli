// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>
#include <string_view>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    // The identities of the packages that make up a pre-indexed source, as carried by
    // SourceDetails::Data.
    //
    // Data held exactly one package family name for as long as a source had exactly one package.
    // A delta capable source has two, each published under an identity of its own: the delta, and
    // the baseline that it names. Both have to be known before any network access, because a
    // package is validated against the identity it was asked for.
    //
    // Two syntaxes are therefore accepted:
    //
    //   Microsoft.Winget.Source_8wekyb3d8bbwe
    //   {"fullIdentity":"Microsoft.Winget.Source_8wekyb3d8bbwe"}
    //   {"deltaIdentity":"...","baselineIdentity":"..."}
    //
    // The first is what every client written to date has stored, and remains what is written back
    // for a source with no delta. The two are told apart by the leading brace, which a package
    // family name cannot contain.
    //
    // All three identities can be named at once. Nothing produces that today -- adding a source
    // settles on either the full index or the delta pair -- but a source that can fall back from
    // the delta to the full index has to name all three, and the format is defined so that
    // introducing fallback does not change it again.
    //
    // There is deliberately no migration between the forms. A source whose Data names only a full
    // index identity uses the full index, which is exactly what it did before deltas existed, and
    // a third party source that begins publishing a delta can be re-added.
    struct SourceData
    {
        SourceData() = default;

        // Parses a SourceDetails::Data value in either syntax.
        // Throws if the value is structured but malformed; an unparsable value is not silently
        // treated as a package family name.
        explicit SourceData(std::string_view data);

        // The identity of the full index, which is the only package a source that is not delta
        // capable has.
        const std::string& FullIdentity() const { return m_fullIdentity; }
        void FullIdentity(std::string value) { m_fullIdentity = std::move(value); }

        // The identity of the delta, when the source publishes one.
        const std::string& DeltaIdentity() const { return m_deltaIdentity; }
        void DeltaIdentity(std::string value) { m_deltaIdentity = std::move(value); }

        // The identity of the baseline that the delta names.
        //
        // The baseline is published under an identity of its own rather than sharing the full
        // index's. Sharing made the two indistinguishable to anything that holds packages by
        // identity, so a client could not hold both, and the baseline had to be allowed to move
        // backward over a full index that was merely newer.
        const std::string& BaselineIdentity() const { return m_baselineIdentity; }
        void BaselineIdentity(std::string value) { m_baselineIdentity = std::move(value); }

        bool HasFullIdentity() const { return !m_fullIdentity.empty(); }
        bool HasDeltaIdentity() const { return !m_deltaIdentity.empty(); }
        bool HasBaselineIdentity() const { return !m_baselineIdentity.empty(); }

        // The identity that the source itself is known by, as distinct from that of any one of
        // its packages.
        //
        // The local state directory and the cross process lock are both derived from this, so it
        // has to be settled when the source is added and stay fixed for as long as the source
        // exists. The full index identity is preferred because that is what every source added
        // before deltas existed is already keyed on; a source that has no full index is keyed on
        // its delta, which is the package published at a fixed name and so the one that cannot
        // move out from under it.
        const std::string& SourceIdentity() const { return m_fullIdentity.empty() ? m_deltaIdentity : m_fullIdentity; }

        // Produces the value to store back into SourceDetails::Data.
        //
        // A source with only a full index serializes back to the bare family name it was read
        // from, so that the stored value does not churn for the sources that are not delta
        // capable.
        std::string Serialize() const;

    private:
        std::string m_fullIdentity;
        std::string m_deltaIdentity;
        std::string m_baselineIdentity;
    };

}
