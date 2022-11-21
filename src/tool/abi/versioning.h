#pragma once

#include "common.h"

// Roughly corresponds to Windows.Foundation.Metadata.Platform enum
enum class meta_platform : int
{
    windows = 0,
    windows_phone = 1,
};

struct contract_version
{
    std::string_view type_name;
    std::uint32_t version;
};

inline contract_version decode_contract_version(xlang::meta::reader::CustomAttribute const& contractVersionAttr)
{
    using namespace xlang::meta::reader;

    // The ContractVersionAttribute has three constructors, two of which are used for describing contract requirements
    // and the third describing contract definitions. This function is intended only for use with the first two
    auto sig = contractVersionAttr.Value();
    auto& args = sig.FixedArgs();
    if (args.size() != 2)
    {
        XLANG_ASSERT(false);
        return {};
    }

    std::string_view type_name;
    xlang::call(std::get<ElemSig>(args[0].value).value,
        [&](ElemSig::SystemType t)
        {
            type_name = t.name;
        },
        [&](std::string_view name)
        {
            type_name = name;
        },
        [](auto&&)
        {
            XLANG_ASSERT(false);
        });

    return contract_version{ type_name, decode_integer<std::uint32_t>(std::get<ElemSig>(args[1].value).value) };
}

struct platform_version
{
    meta_platform platform;
    std::uint32_t version;
};

inline platform_version decode_platform_version(xlang::meta::reader::CustomAttribute const& versionAttr)
{
    using namespace xlang::meta::reader;

    // There are two constructors for the VersionAttribute: one that takes a single version integer and another that
    // takes a version integer and a platform value
    auto sig = versionAttr.Value();
    auto& args = sig.FixedArgs();
    auto version = decode_integer<std::uint32_t>(std::get<ElemSig>(args[0].value).value);

    auto platform = meta_platform::windows;
    if (args.size() == 2)
    {
        platform = decode_enum<meta_platform>(std::get<ElemSig::EnumValue>(std::get<ElemSig>(args[1].value).value));
    }
    else
    {
        XLANG_ASSERT(args.size() == 1);
    }

    return platform_version{ platform, version };
}

using version = std::variant<contract_version, platform_version>;

inline version version_from_attribute(xlang::meta::reader::CustomAttribute const& attr)
{
    using namespace xlang::meta::reader;

    auto sig = attr.Value();
    auto& args = sig.FixedArgs();
    auto throw_invalid = [&]
    {
        auto [ns, name] = attr.TypeNamespaceAndName();
        xlang::throw_invalid("Attribute \"", ns, ".", name, "\" has no versioning information");
    };
    if (args.empty()) throw_invalid();

    // Whenever an attribute has versioning information, its constructor will come in one of three forms:
    //  1.  attribute(args..., version)
    //  2.  attribute(args..., version, platform)
    //  3.  attribute(args..., version, contract)
    auto& lastElem = std::get<ElemSig>(args.back().value);
    if (std::holds_alternative<std::string_view>(lastElem.value))
    {
        // Scenario '3' from above
        if (args.size() < 2) throw_invalid();

        auto contractName = std::get<std::string_view>(lastElem.value);
        auto version = decode_integer<std::uint32_t>(std::get<ElemSig>(args[args.size() - 2].value).value);
        return contract_version{ contractName, version };
    }
    else if (std::holds_alternative<ElemSig::EnumValue>(lastElem.value))
    {
        // Scenario '2' from above
        if (args.size() < 2) throw_invalid();

        auto platform = decode_enum<meta_platform>(std::get<ElemSig::EnumValue>(lastElem.value));
        auto version = decode_integer<std::uint32_t>(std::get<ElemSig>(args[args.size() - 2].value).value);
        return platform_version{ platform, version };
    }
    else
    {
        // Assume scenario '1' from above. This will throw if not an integer, but that's for the best
        return platform_version{ meta_platform::windows, decode_integer<std::uint32_t>(lastElem.value) };
    }
}

struct previous_contract
{
    std::string_view from_contract;
    std::uint32_t version_introduced;
    std::uint32_t version_removed;
    std::string_view to_contract;
};

struct contract_history
{
    contract_version current_contract;

    // NOTE: Ordered such that the "earliest" contracts come first. E.g. the first item is the contract where the type
    //       was introduced. If the list is empty, then the above contract/version is the first contract
    std::vector<previous_contract> previous_contracts;
};

template <typename T>
inline std::optional<contract_history> get_contract_history(T const& value)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;

    auto contractAttr = get_attribute(value, metadata_namespace, "ContractVersionAttribute"sv);
    if (!contractAttr)
    {
        return std::nullopt;
    }

    contract_history result;
    result.current_contract = decode_contract_version(contractAttr);

    std::size_t mostRecentContractIndex = std::numeric_limits<std::size_t>::max();
    for_each_attribute(value, metadata_namespace, "PreviousContractVersionAttribute"sv,
        [&](bool /*first*/, auto const& attr)
    {
        auto prevSig = attr.Value();
        auto const& prevArgs = prevSig.FixedArgs();

        // The PreviousContractVersionAttribute has two constructors, both of which start with the same three
        // arguments - the only ones that we care about
        previous_contract prev =
        {
            std::get<std::string_view>(std::get<ElemSig>(prevArgs[0].value).value),
            decode_integer<std::uint32_t>(std::get<ElemSig>(prevArgs[1].value).value),
            decode_integer<std::uint32_t>(std::get<ElemSig>(prevArgs[2].value).value),
        };
        if (prevArgs.size() == 4)
        {
            prev.to_contract = std::get<std::string_view>(std::get<ElemSig>(prevArgs[3].value).value);
            if (prev.to_contract == result.current_contract.type_name)
            {
                XLANG_ASSERT(mostRecentContractIndex == std::numeric_limits<std::size_t>::max());
                mostRecentContractIndex = result.previous_contracts.size();
            }

            result.previous_contracts.push_back(prev);
        }
        else
        {
            // This is the last contract that the type was in before moving to its current contract. Always insert
            // it at the tail
            prev.to_contract = result.current_contract.type_name;
            XLANG_ASSERT(mostRecentContractIndex == std::numeric_limits<std::size_t>::max());
            mostRecentContractIndex = result.previous_contracts.size();
            result.previous_contracts.push_back(prev);
        }
    });

    if (!result.previous_contracts.empty())
    {
        if (mostRecentContractIndex == std::numeric_limits<std::size_t>::max())
        {
            // Not a great error message, however this scenario should not be allowable by mildlrt
            xlang::throw_invalid("Invalid contract history");
        }

        // NOTE: The following loop is N^2, however contract history should be rare and short when present, so this
        // should likely beat out any alternative graph creating algorithm in terms of wall clock execution time
        std::swap(result.previous_contracts[mostRecentContractIndex], result.previous_contracts.back());
        for (std::size_t size = result.previous_contracts.size() - 1; size; --size)
        {
            auto& last = result.previous_contracts[size];
            auto end = result.previous_contracts.begin() + size;
            auto itr = std::find_if(result.previous_contracts.begin(), end, [&](auto const& test)
            {
                return test.to_contract == last.from_contract;
            });
            if (itr == end)
            {
                xlang::throw_invalid("Invalid contract history");
            }
            std::swap(*itr, result.previous_contracts[size - 1]);
        }
    }

    return result;
}

template <typename T>
std::optional<version> match_versioning_scheme(version const& ver, T const& value)
{
    using namespace std::literals;
    using namespace xlang::meta::reader;

    if (std::holds_alternative<contract_version>(ver))
    {
        if (auto history = get_contract_history(value))
        {
            if (history->previous_contracts.empty())
            {
                return history->current_contract;
            }

            auto range = history->previous_contracts.front();
            return contract_version{ range.from_contract, range.version_introduced };
        }

        return std::nullopt;
    }
    else
    {
        auto const& plat = std::get<platform_version>(ver);
        std::optional<version> result;
        for_each_attribute(value, metadata_namespace, "VersionAttribute"sv, [&](bool /*first*/, auto const& attr)
        {
            auto possibleMatch = decode_platform_version(attr);
            if (possibleMatch.platform == plat.platform)
            {
                XLANG_ASSERT(!result);
                result = possibleMatch;
            }
        });

        return result;
    }
}
