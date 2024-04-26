// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentUtil.h"

#include <algorithm>

using namespace SFS;
using namespace SFS::details;

bool contentutil::operator==(const ContentId& lhs, const ContentId& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetNameSpace() == rhs.GetNameSpace() && lhs.GetName() == rhs.GetName() &&
           lhs.GetVersion() == rhs.GetVersion();
}

bool contentutil::operator!=(const ContentId& lhs, const ContentId& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const File& lhs, const File& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetFileId() == rhs.GetFileId() && lhs.GetUrl() == rhs.GetUrl() &&
           lhs.GetSizeInBytes() == rhs.GetSizeInBytes() && lhs.GetHashes() == rhs.GetHashes();
}

bool contentutil::operator!=(const File& lhs, const File& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const ApplicabilityDetails& lhs, const ApplicabilityDetails& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetArchitectures() == rhs.GetArchitectures() &&
           lhs.GetPlatformApplicabilityForPackage() == rhs.GetPlatformApplicabilityForPackage();
}

bool contentutil::operator!=(const ApplicabilityDetails& lhs, const ApplicabilityDetails& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const AppFile& lhs, const AppFile& rhs)
{
    // String characters can be UTF-8 encoded, so we need to compare them in a case-sensitive manner.
    return lhs.GetFileId() == rhs.GetFileId() && lhs.GetUrl() == rhs.GetUrl() &&
           lhs.GetSizeInBytes() == rhs.GetSizeInBytes() && lhs.GetHashes() == rhs.GetHashes() &&
           lhs.GetApplicabilityDetails() == rhs.GetApplicabilityDetails() &&
           lhs.GetFileMoniker() == rhs.GetFileMoniker();
}

bool contentutil::operator!=(const AppFile& lhs, const AppFile& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const Content& lhs, const Content& rhs)
{
    return lhs.GetContentId() == rhs.GetContentId() &&
           (std::is_permutation(lhs.GetFiles().begin(),
                                lhs.GetFiles().end(),
                                rhs.GetFiles().begin(),
                                rhs.GetFiles().end(),
                                [](const File& flhs, const File& frhs) { return flhs == frhs; }));
}

bool contentutil::operator!=(const Content& lhs, const Content& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const AppPrerequisiteContent& lhs, const AppPrerequisiteContent& rhs)
{
    auto areFilesEqual = [&lhs, &rhs]() {
        return std::is_permutation(lhs.GetFiles().begin(),
                                   lhs.GetFiles().end(),
                                   rhs.GetFiles().begin(),
                                   rhs.GetFiles().end(),
                                   [](const AppFile& flhs, const AppFile& frhs) { return flhs == frhs; });
    };
    return lhs.GetContentId() == rhs.GetContentId() && areFilesEqual();
}

bool contentutil::operator!=(const AppPrerequisiteContent& lhs, const AppPrerequisiteContent& rhs)
{
    return !(lhs == rhs);
}

bool contentutil::operator==(const AppContent& lhs, const AppContent& rhs)
{
    auto arePrerequisitesEqual = [&lhs, &rhs]() {
        return std::equal(
            lhs.GetPrerequisites().begin(),
            lhs.GetPrerequisites().end(),
            rhs.GetPrerequisites().begin(),
            rhs.GetPrerequisites().end(),
            [](const AppPrerequisiteContent& clhs, const AppPrerequisiteContent& crhs) { return clhs == crhs; });
    };
    auto areFilesEqual = [&lhs, &rhs]() {
        return std::is_permutation(lhs.GetFiles().begin(),
                                   lhs.GetFiles().end(),
                                   rhs.GetFiles().begin(),
                                   rhs.GetFiles().end(),
                                   [](const AppFile& flhs, const AppFile& frhs) { return flhs == frhs; });
    };
    return lhs.GetContentId() == rhs.GetContentId() && lhs.GetUpdateId() == rhs.GetUpdateId() &&
           arePrerequisitesEqual() && areFilesEqual();
}

bool contentutil::operator!=(const AppContent& lhs, const AppContent& rhs)
{
    return !(lhs == rhs);
}
