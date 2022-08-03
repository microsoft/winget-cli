// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

#include <winget/InstallerMetadataCollectionContext.h>

using namespace AppInstaller;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Correlation;
using namespace AppInstaller::Repository::Metadata;
using namespace TestCommon;

namespace
{
    // Indicates to set minimal defaults on TestInput
    struct MinimalDefaults_t {} MinimalDefaults;

    struct TestInput
    {
        TestInput() = default;

        TestInput(MinimalDefaults_t)
        {
            Version = "1.0";
            SupportedMetadataVersion = "1.0";
            SubmissionIdentifier = "1";
            InstallerHash = "ABCD";
            PackageData = std::make_optional<Manifest::Manifest>();
            PackageData->DefaultLocalization.Locale = "en-us";
            PackageData->DefaultLocalization.Add<Manifest::Localization::PackageName>("Name");
            PackageData->DefaultLocalization.Add<Manifest::Localization::Publisher>("Publisher");
        }

        TestInput(MinimalDefaults_t, const std::string& productVersion, const std::string& productCode, Manifest::InstallerTypeEnum installerType) : TestInput(MinimalDefaults)
        {
            CurrentMetadata = std::make_optional<ProductMetadata>();
            CurrentMetadata->SchemaVersion.Assign("1.0");
            CurrentMetadata->ProductVersionMin.Assign(productVersion);
            CurrentMetadata->ProductVersionMax.Assign(productVersion);
            auto& installerMetadata = CurrentMetadata->InstallerMetadataMap[InstallerHash.value()];
            installerMetadata.SubmissionIdentifier = SubmissionIdentifier.value();
            installerMetadata.AppsAndFeaturesEntries.push_back({});
            auto& entry = installerMetadata.AppsAndFeaturesEntries.back();
            entry.DisplayName = PackageData->DefaultLocalization.Get<Manifest::Localization::PackageName>();
            entry.Publisher = PackageData->DefaultLocalization.Get<Manifest::Localization::Publisher>();
            entry.DisplayVersion = productVersion;
            entry.ProductCode = productCode;
            entry.InstallerType = installerType;
        }

        std::optional<std::string> Version;
        std::optional<std::string> SupportedMetadataVersion;
        std::optional<ProductMetadata> CurrentMetadata;
        std::optional<web::json::value> SubmissionData;
        std::optional<std::string> SubmissionIdentifier;
        std::optional<std::string> InstallerHash;
        // Schema 1.0 only cares about DefaultLocale and Locales
        std::optional<Manifest::Manifest> PackageData;

        std::wstring ToJSON()
        {
            web::json::value json;

            if (Version)
            {
                json[L"version"] = JSON::GetStringValue(Version.value());
            }

            if (SupportedMetadataVersion)
            {
                json[L"supportedMetadataVersion"] = JSON::GetStringValue(SupportedMetadataVersion.value());
            }

            if (CurrentMetadata)
            {
                json[L"currentMetadata"] = CurrentMetadata->ToJson(CurrentMetadata->SchemaVersion, 0);
            }

            if (SubmissionData)
            {
                json[L"submissionData"] = SubmissionData.value();
            }
            else if (SubmissionIdentifier)
            {
                web::json::value submissionData;
                submissionData[L"submissionIdentifier"] = JSON::GetStringValue(SubmissionIdentifier.value());
                json[L"submissionData"] = std::move(submissionData);
            }

            if (InstallerHash || PackageData)
            {
                web::json::value packageData;

                if (InstallerHash)
                {
                    packageData[L"installerHash"] = JSON::GetStringValue(InstallerHash.value());
                }

                if (PackageData)
                {
                    packageData[L"DefaultLocale"] = LocaleToJSON(PackageData->DefaultLocalization);

                    // TODO: Implement other locales
                }

                json[L"packageData"] = std::move(packageData);
            }

            return json.serialize();
        }

    private:
        web::json::value LocaleToJSON(const Manifest::ManifestLocalization& localization) const
        {
            web::json::value locale;

            locale[L"PackageLocale"] = JSON::GetStringValue(localization.Locale);

            if (localization.Contains(Manifest::Localization::PackageName))
            {
                locale[L"PackageName"] = JSON::GetStringValue(localization.Get<Manifest::Localization::PackageName>());
            }

            if (localization.Contains(Manifest::Localization::Publisher))
            {
                locale[L"Publisher"] = JSON::GetStringValue(localization.Get<Manifest::Localization::Publisher>());
            }

            // TODO: Implement any other needed fields

            return locale;
        }
    };

    struct TestOutput
    {
        TestOutput(const std::string& json) : OriginalJSON(json)
        {
            web::json::value input = web::json::value::parse(Utility::ConvertToUTF16(json));

            auto versionString = JSON::GetRawStringValueFromJsonNode(input, L"version");
            if (versionString)
            {
                Version = std::move(versionString);
            }

            auto submissionDataValue = JSON::GetJsonValueFromNode(input, L"submissionData");
            if (submissionDataValue)
            {
                SubmissionData = submissionDataValue.value();
            }

            auto installerHashString = JSON::GetRawStringValueFromJsonNode(input, L"installerHash");
            if (installerHashString)
            {
                InstallerHash = std::move(installerHashString);
            }

            auto statusString = JSON::GetRawStringValueFromJsonNode(input, L"status");
            if (statusString)
            {
                Status = std::move(statusString);
            }

            auto metadataValue = JSON::GetJsonValueFromNode(input, L"metadata");
            if (metadataValue && !metadataValue->get().is_null())
            {
                Metadata = std::make_optional<ProductMetadata>();
                Metadata->FromJson(metadataValue->get());
            }

            auto diagnosticsValue = JSON::GetJsonValueFromNode(input, L"diagnostics");
            if (diagnosticsValue)
            {
                auto errorHRNumber = JSON::GetRawIntValueFromJsonNode(diagnosticsValue.value(), L"errorHR");
                if (errorHRNumber)
                {
                    ErrorHR = std::move(errorHRNumber);
                }

                auto errorTextString = JSON::GetRawStringValueFromJsonNode(diagnosticsValue.value(), L"errorText");
                if (errorTextString)
                {
                    ErrorText = std::move(errorTextString);
                }
            }
        }

        std::string OriginalJSON;

        std::optional<std::string> Version;
        std::optional<web::json::value> SubmissionData;
        std::optional<std::string> InstallerHash;
        std::optional<std::string> Status;
        std::optional<ProductMetadata> Metadata;
        std::optional<HRESULT> ErrorHR;
        std::optional<std::string> ErrorText;

        bool IsError() const
        {
            return Status && Status.value() == "Error";
        }

        bool IsSuccess() const
        {
            return Status && Status.value() == "Success";
        }

        bool IsLowConfidence() const
        {
            return Status && Status.value() == "LowConfidence";
        }

        void ValidateFieldPresence() const
        {
            REQUIRE(Version);
            REQUIRE(SubmissionData);
            REQUIRE(InstallerHash);
            REQUIRE(Status);

            REQUIRE(IsSuccess() == Metadata.has_value());

            REQUIRE(IsError() == ErrorHR.has_value());
            REQUIRE(IsError() == ErrorText.has_value());
        }
    };

    struct TestARPCorrelationData : public ARPCorrelationData
    {
        ARPCorrelationResult CorrelateForNewlyInstalled(const Manifest::Manifest&) override
        {
            return CorrelateForNewlyInstalledResult;
        }

        ARPCorrelationResult CorrelateForNewlyInstalledResult;
    };

    InstallerMetadataCollectionContext CreateTestContext(std::unique_ptr<ARPCorrelationData>&& data, TestInput& input)
    {
        return { std::move(data), input.ToJSON() };
    }

    InstallerMetadataCollectionContext CreateTestContext(TestInput& input)
    {
        return CreateTestContext(std::make_unique<TestARPCorrelationData>(), input);
    }

    TestOutput GetOutput(InstallerMetadataCollectionContext& context)
    {
        std::ostringstream strstr;
        context.Complete(strstr);

        return { strstr.str() };
    }

    TestOutput GetOutput(TestInput& input)
    {
        InstallerMetadataCollectionContext context = CreateTestContext(input);
        return GetOutput(context);
    }

    void BadInputTest(TestInput& input)
    {
        TestOutput output = GetOutput(input);
        REQUIRE(output.IsError());
        output.ValidateFieldPresence();
    }

    ProductMetadata MakeProductMetadata(std::string_view submissionIdentifier = "Submission 1", const std::string& installerHash = "ABCD")
    {
        ProductMetadata result;
        result.SchemaVersion.Assign("1.0");
        result.ProductVersionMin.Assign("1.0");
        result.ProductVersionMax.Assign("1.0");
        auto& installerMetadata = result.InstallerMetadataMap[installerHash];
        installerMetadata.SubmissionIdentifier = submissionIdentifier;
        installerMetadata.AppsAndFeaturesEntries.push_back({});
        auto& entry = installerMetadata.AppsAndFeaturesEntries.back();
        entry.DisplayName = "Name";
        entry.Publisher = "Publisher";
        entry.DisplayVersion = "1.0";
        entry.ProductCode = "{guid}";
        entry.InstallerType = Manifest::InstallerTypeEnum::Msi;
        return result;
    }

    struct TestMerge
    {
        TestMerge() = default;

        TestMerge(MinimalDefaults_t)
        {
            Version = "1.0";
            Metadatas = std::make_optional<std::vector<ProductMetadata>>();
            Metadatas->emplace_back(MakeProductMetadata());
        }

        std::optional<std::string> Version;
        std::optional<std::vector<ProductMetadata>> Metadatas;

        std::wstring ToJSON()
        {
            web::json::value json;

            if (Version)
            {
                json[L"version"] = JSON::GetStringValue(Version.value());
            }

            if (Metadatas)
            {
                web::json::value metadatasArray;

                if (Metadatas->empty())
                {
                    metadatasArray = web::json::value::array();
                }
                else
                {
                    size_t index = 0;
                    for (auto& value : Metadatas.value())
                    {
                        metadatasArray[index++] = value.ToJson(value.SchemaVersion, 0);
                    }
                }

                json[L"metadatas"] = std::move(metadatasArray);
            }

            return json.serialize();
        }
    };
}

TEST_CASE("MetadataCollection_MinimumInput", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    TestOutput output = GetOutput(input);
    REQUIRE(!output.IsError());
    output.ValidateFieldPresence();

    REQUIRE(output.Version.value() == input.Version.value());
    REQUIRE(output.InstallerHash.value() == input.InstallerHash.value());
}

TEST_CASE("MetadataCollection_SubmissionDataCopied", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);

    web::json::value submissionData;
    std::wstring testValueName = L"testValueName";
    std::string testValueValue = "Test value value";
    submissionData[L"submissionIdentifier"] = JSON::GetStringValue("Required identifier");
    submissionData[testValueName] = JSON::GetStringValue(testValueValue);

    input.SubmissionData = submissionData;

    TestOutput output = GetOutput(input);
    REQUIRE(!output.IsError());
    output.ValidateFieldPresence();

    REQUIRE(output.Version.value() == input.Version.value());
    REQUIRE(output.InstallerHash.value() == input.InstallerHash.value());
    auto outputValue = JSON::GetRawStringValueFromJsonNode(output.SubmissionData.value(), testValueName);
    REQUIRE(outputValue);
    REQUIRE(outputValue.value() == testValueValue);
}

TEST_CASE("MetadataCollection_BadInput", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);

#define RESET_FIELD_SECTION(_field_) \
    SECTION("No " #_field_) \
    { \
        input._field_.reset(); \
        BadInputTest(input); \
    }

    RESET_FIELD_SECTION(Version);
    RESET_FIELD_SECTION(SupportedMetadataVersion);
    RESET_FIELD_SECTION(SubmissionIdentifier);
    RESET_FIELD_SECTION(InstallerHash);
    RESET_FIELD_SECTION(PackageData);

#undef RESET_FIELD_SECTION
}

TEST_CASE("MetadataCollection_LowConfidence", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    // The default test correlation object won't have a package correlation set
    TestOutput output = GetOutput(input);
    REQUIRE(output.IsLowConfidence());
    REQUIRE(!output.Metadata);
    output.ValidateFieldPresence();
}

TEST_CASE("MetadataCollection_NewPackage", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Test Package Name");
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>("Test Publisher");
    manifest.Version = "1.2.3";
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = "{guid}";

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Msi);
    metadata[PackageVersionMetadata::InstalledScope] = Manifest::ScopeToString(Manifest::ScopeEnum::User);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == output.Metadata->ProductVersionMax.ToString());
    REQUIRE(output.Metadata->ProductVersionMin.ToString() == manifest.Version);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
    REQUIRE(entry.Scope.empty());
    REQUIRE(entry.AppsAndFeaturesEntries.size() == 1);
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayVersion == manifest.Version);
    REQUIRE(entry.AppsAndFeaturesEntries[0].ProductCode == manifest.Installers[0].ProductCode);
    REQUIRE(entry.AppsAndFeaturesEntries[0].InstallerType == Manifest::InstallerTypeEnum::Msi);
    REQUIRE(output.Metadata->HistoricalMetadataList.empty());
}

TEST_CASE("MetadataCollection_SameSubmission_SameInstaller", "[metadata_collection]")
{
    std::string version = "1.3.5";
    std::string productCode = "{guid}";
    Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Msi;

    TestInput input(MinimalDefaults, version, productCode, installerType);
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Different Language Name");
    // Same publisher
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].Publisher);
    manifest.Version = version;
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = productCode;

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(installerType);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == output.Metadata->ProductVersionMax.ToString());
    REQUIRE(output.Metadata->ProductVersionMin.ToString() == manifest.Version);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
    REQUIRE(entry.AppsAndFeaturesEntries.size() == 2);

    // One should have all values, and the other should have just a different name
    // Base which one is which off of whether Publisher is set
    for (const auto& featureEntry : entry.AppsAndFeaturesEntries)
    {
        if (featureEntry.Publisher.empty())
        {
            REQUIRE(featureEntry.DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
            REQUIRE(featureEntry.DisplayVersion.empty());
            REQUIRE(featureEntry.ProductCode.empty());
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Unknown);
        }
        else
        {
            REQUIRE(featureEntry.DisplayName == input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].DisplayName);
            REQUIRE(featureEntry.Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
            REQUIRE(featureEntry.DisplayVersion == manifest.Version);
            REQUIRE(featureEntry.ProductCode == manifest.Installers[0].ProductCode);
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Msi);
        }
    }
    REQUIRE(output.Metadata->HistoricalMetadataList.empty());
}

TEST_CASE("MetadataCollection_SameSubmission_NewInstaller", "[metadata_collection]")
{
    std::string versionPresent = "1.3.5";
    std::string versionIncoming = "1.3.5.1";
    std::string productCodePresent = "{guid}";
    std::string productCodeIncoming = "{guid_different}";
    Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Msi;

    TestInput input(MinimalDefaults, versionPresent, productCodePresent, installerType);
    // Change the incoming hash to be new
    input.InstallerHash = input.InstallerHash.value() + "_DIFFERENT";
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Name (but different architecture)");
    // Same publisher
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].Publisher);
    manifest.Version = versionIncoming;
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = productCodeIncoming;

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(installerType);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == versionPresent);
    REQUIRE(output.Metadata->ProductVersionMax.ToString() == versionIncoming);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 2);

    for (const auto& installerMetadata : output.Metadata->InstallerMetadataMap)
    {
        const auto& entry = installerMetadata.second;

        REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
        REQUIRE(entry.AppsAndFeaturesEntries.size() == 1);

        const auto& featureEntry = entry.AppsAndFeaturesEntries.front();
        REQUIRE(featureEntry.Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());

        if (featureEntry.ProductCode == productCodePresent)
        {
            REQUIRE(featureEntry.DisplayName == input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].DisplayName);
            REQUIRE(featureEntry.DisplayVersion == versionPresent);
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Msi);
        }
        else
        {
            REQUIRE(featureEntry.DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
            REQUIRE(featureEntry.DisplayVersion == versionIncoming);
            REQUIRE(featureEntry.InstallerType == Manifest::InstallerTypeEnum::Msi);
            REQUIRE(featureEntry.ProductCode == productCodeIncoming);
        }
    }

    REQUIRE(output.Metadata->HistoricalMetadataList.empty());
}

TEST_CASE("MetadataCollection_NewSubmission", "[metadata_collection]")
{
    std::string versionPresent = "1.3.5";
    std::string versionIncoming = "1.4.0";
    std::string productCodePresent = "{guid}";
    std::string productCodeIncoming = "{guid_different}";
    Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Msi;

    TestInput input(MinimalDefaults, versionPresent, productCodePresent, installerType);
    input.SubmissionIdentifier = input.SubmissionIdentifier.value() + "_NEW";
    input.InstallerHash = input.InstallerHash.value() + "_DIFFERENT";
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].DisplayName);
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].Publisher);
    manifest.Version = versionIncoming;
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = productCodeIncoming;

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(installerType);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == output.Metadata->ProductVersionMax.ToString());
    REQUIRE(output.Metadata->ProductVersionMin.ToString() == manifest.Version);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
    REQUIRE(entry.AppsAndFeaturesEntries.size() == 1);
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayVersion == manifest.Version);
    REQUIRE(entry.AppsAndFeaturesEntries[0].ProductCode == manifest.Installers[0].ProductCode);
    REQUIRE(entry.AppsAndFeaturesEntries[0].InstallerType == Manifest::InstallerTypeEnum::Msi);

    REQUIRE(output.Metadata->HistoricalMetadataList.size() == 1);
    const auto& historicalEntry = output.Metadata->HistoricalMetadataList[0];
    REQUIRE(historicalEntry.ProductVersionMin.ToString() == input.CurrentMetadata->ProductVersionMin.ToString());
    REQUIRE(historicalEntry.ProductVersionMax.ToString() == input.CurrentMetadata->ProductVersionMax.ToString());
    const auto& appsAndFeaturesEntry = input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries.front();
    REQUIRE(historicalEntry.Names.size() == 1);
    REQUIRE(*historicalEntry.Names.begin() == appsAndFeaturesEntry.DisplayName);
    REQUIRE(historicalEntry.ProductCodes.size() == 1);
    REQUIRE(*historicalEntry.ProductCodes.begin() == appsAndFeaturesEntry.ProductCode);
    REQUIRE(historicalEntry.Publishers.size() == 1);
    REQUIRE(*historicalEntry.Publishers.begin() == appsAndFeaturesEntry.Publisher);
}

TEST_CASE("MetadataCollection_Merge_Empty", "[metadata_collection]")
{
    TestMerge mergeData{ MinimalDefaults };
    mergeData.Metadatas->clear();

    REQUIRE_THROWS_HR(InstallerMetadataCollectionContext::Merge(mergeData.ToJSON(), 0, {}), E_NOT_SET);
}

TEST_CASE("MetadataCollection_Merge_SubmissionMismatch", "[metadata_collection]")
{
    TestMerge mergeData{ MinimalDefaults };
    mergeData.Metadatas->emplace_back(MakeProductMetadata("Submission 2"));

    REQUIRE_THROWS_HR(InstallerMetadataCollectionContext::Merge(mergeData.ToJSON(), 0, {}), E_NOT_VALID_STATE);
}

TEST_CASE("MetadataCollection_Merge_DifferentInstallers", "[metadata_collection]")
{
    TestMerge mergeData{ MinimalDefaults };
    mergeData.Metadatas->emplace_back(MakeProductMetadata(mergeData.Metadatas->at(0).InstallerMetadataMap.begin()->second.SubmissionIdentifier, "EFGH"));

    std::wstring mergeResult = InstallerMetadataCollectionContext::Merge(mergeData.ToJSON(), 0, {});
    REQUIRE(!mergeResult.empty());

    ProductMetadata mergeMetadata;
    mergeMetadata.FromJson(web::json::value::parse(mergeResult));

    REQUIRE(mergeMetadata.InstallerMetadataMap.size() == 2);
    for (const auto& item : mergeMetadata.InstallerMetadataMap)
    {
        REQUIRE(item.second.AppsAndFeaturesEntries.size() == 1);
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].DisplayName.empty());
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].Publisher.empty());
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].DisplayVersion.empty());
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].ProductCode.empty());
    }
}

TEST_CASE("MetadataCollection_Merge_SameInstaller", "[metadata_collection]")
{
    TestMerge mergeData{ MinimalDefaults };
    mergeData.Metadatas->emplace_back(MakeProductMetadata());

    std::wstring mergeResult = InstallerMetadataCollectionContext::Merge(mergeData.ToJSON(), 0, {});
    REQUIRE(!mergeResult.empty());

    ProductMetadata mergeMetadata;
    mergeMetadata.FromJson(web::json::value::parse(mergeResult));

    REQUIRE(mergeMetadata.InstallerMetadataMap.size() == 1);
    for (const auto& item : mergeMetadata.InstallerMetadataMap)
    {
        REQUIRE(item.second.AppsAndFeaturesEntries.size() == 1);
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].DisplayName.empty());
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].Publisher.empty());
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].DisplayVersion.empty());
        REQUIRE(!item.second.AppsAndFeaturesEntries[0].ProductCode.empty());
    }
}

TEST_CASE("MetadataCollection_NewPackage_1_1", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    input.SupportedMetadataVersion = "1.1";
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Test Package Name");
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>("Test Publisher");
    manifest.Version = "1.2.3";
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = "{guid}";

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Msi);
    metadata[PackageVersionMetadata::InstalledScope] = Manifest::ScopeToString(Manifest::ScopeEnum::User);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->ProductVersionMin.ToString() == output.Metadata->ProductVersionMax.ToString());
    REQUIRE(output.Metadata->ProductVersionMin.ToString() == manifest.Version);
    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.SubmissionIdentifier == input.SubmissionIdentifier.value());
    REQUIRE(entry.Scope == metadata[PackageVersionMetadata::InstalledScope]);
    REQUIRE(entry.AppsAndFeaturesEntries.size() == 1);
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayName == manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].Publisher == manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
    REQUIRE(entry.AppsAndFeaturesEntries[0].DisplayVersion == manifest.Version);
    REQUIRE(entry.AppsAndFeaturesEntries[0].ProductCode == manifest.Installers[0].ProductCode);
    REQUIRE(entry.AppsAndFeaturesEntries[0].InstallerType == Manifest::InstallerTypeEnum::Msi);
    REQUIRE(output.Metadata->HistoricalMetadataList.empty());
}

TEST_CASE("MetadataCollection_NewPackage_NoScope", "[metadata_collection]")
{
    TestInput input(MinimalDefaults);
    input.SupportedMetadataVersion = "1.1";
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Test Package Name");
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>("Test Publisher");
    manifest.Version = "1.2.3";
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = "{guid}";

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Msi);

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];
    REQUIRE(entry.Scope.empty());
}

TEST_CASE("MetadataCollection_SameSubmission_SameInstaller_Scopes", "[metadata_collection]")
{
    std::string version = "1.3.5";
    std::string productCode = "{guid}";
    Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Msi;
    std::string currentScope = GENERATE(std::string{},
        Manifest::ScopeToString(Manifest::ScopeEnum::Unknown),
        Manifest::ScopeToString(Manifest::ScopeEnum::User),
        Manifest::ScopeToString(Manifest::ScopeEnum::Machine));
    std::string newScope{ Manifest::ScopeToString(Manifest::ScopeEnum::User) };

    INFO(currentScope);

    TestInput input(MinimalDefaults, version, productCode, installerType);
    input.SupportedMetadataVersion = "1.1";
    input.CurrentMetadata->SchemaVersion = { "1.1" };
    input.CurrentMetadata->InstallerMetadataMap.begin()->second.Scope = currentScope;
    auto correlationData = std::make_unique<TestARPCorrelationData>();

    Manifest::Manifest manifest;
    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Different Language Name");
    // Same publisher
    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(input.CurrentMetadata->InstallerMetadataMap.begin()->second.AppsAndFeaturesEntries[0].Publisher);
    manifest.Version = version;
    manifest.Installers.push_back({});
    manifest.Installers[0].ProductCode = productCode;

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = Manifest::InstallerTypeToString(installerType);
    metadata[PackageVersionMetadata::InstalledScope] = newScope;

    correlationData->CorrelateForNewlyInstalledResult.Package = std::make_shared<TestPackageVersion>(manifest, metadata);

    InstallerMetadataCollectionContext context = CreateTestContext(std::move(correlationData), input);
    TestOutput output = GetOutput(context);

    REQUIRE(output.IsSuccess());
    output.ValidateFieldPresence();

    REQUIRE(output.Metadata->InstallerMetadataMap.size() == 1);
    REQUIRE(output.Metadata->InstallerMetadataMap.count(input.InstallerHash.value()) == 1);
    const auto& entry = output.Metadata->InstallerMetadataMap[input.InstallerHash.value()];

    if (currentScope.empty())
    {
        REQUIRE(entry.Scope == newScope);
    }
    else if (currentScope != newScope)
    {
        // If Unknown, should stay Unknown
        // If different, should become Unknown
        REQUIRE(entry.Scope == Manifest::ScopeToString(Manifest::ScopeEnum::Unknown));
    }
    else
    {
        // If same, should not change
        REQUIRE(entry.Scope == currentScope);
    }
}

TEST_CASE("MetadataCollection_Merge_SameInstaller_Scopes", "[metadata_collection]")
{
    TestMerge mergeData{ MinimalDefaults };
    mergeData.Metadatas->emplace_back(MakeProductMetadata());

    std::string currentScope = GENERATE(std::string{},
        Manifest::ScopeToString(Manifest::ScopeEnum::Unknown),
        Manifest::ScopeToString(Manifest::ScopeEnum::User),
        Manifest::ScopeToString(Manifest::ScopeEnum::Machine));
    std::string newScope{ Manifest::ScopeToString(Manifest::ScopeEnum::Machine) };

    INFO(currentScope);

    mergeData.Metadatas->at(0).SchemaVersion = { "1.1" };
    mergeData.Metadatas->at(0).InstallerMetadataMap.begin()->second.Scope = currentScope;
    mergeData.Metadatas->at(1).SchemaVersion = { "1.1" };
    mergeData.Metadatas->at(1).InstallerMetadataMap.begin()->second.Scope = newScope;

    std::wstring mergeResult = InstallerMetadataCollectionContext::Merge(mergeData.ToJSON(), 0, {});
    REQUIRE(!mergeResult.empty());

    ProductMetadata mergeMetadata;
    mergeMetadata.FromJson(web::json::value::parse(mergeResult));

    REQUIRE(mergeMetadata.InstallerMetadataMap.size() == 1);
    for (const auto& item : mergeMetadata.InstallerMetadataMap)
    {
        if (currentScope.empty())
        {
            REQUIRE(item.second.Scope == newScope);
        }
        else if (currentScope != newScope)
        {
            // If Unknown, should stay Unknown
            // If different, should become Unknown
            REQUIRE(item.second.Scope == Manifest::ScopeToString(Manifest::ScopeEnum::Unknown));
        }
        else
        {
            // If same, should not change
            REQUIRE(item.second.Scope == currentScope);
        }
    }
}
