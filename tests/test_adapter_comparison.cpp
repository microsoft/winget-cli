#ifdef _MSC_VER
#pragma warning(disable: 4706)
#include <picojson.h>
#pragma warning(default: 4706)
#else
#include <picojson.h>
#endif

#include <gtest/gtest.h>

#include <valijson/adapters/json11_adapter.hpp>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/adapters/picojson_adapter.hpp>
#include <valijson/adapters/rapidjson_adapter.hpp>

#include <valijson/utils/json11_utils.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>
#include <valijson/utils/nlohmann_json_utils.hpp>
#include <valijson/utils/picojson_utils.hpp>
#include <valijson/utils/rapidjson_utils.hpp>

#ifdef VALIJSON_BUILD_BOOST_JSON_ADAPTER
#include <valijson/adapters/boost_json_adapter.hpp>
#include <valijson/utils/boost_json_utils.hpp>
#endif

#ifdef VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER
#include <valijson/adapters/property_tree_adapter.hpp>
#include <valijson/utils/property_tree_utils.hpp>
#endif

#ifdef VALIJSON_BUILD_QT_ADAPTER
#include <valijson/adapters/qtjson_adapter.hpp>
#include <valijson/utils/qtjson_utils.hpp>
#include <utility>
#endif

#ifdef VALIJSON_BUILD_POCO_ADAPTER
#include <valijson/adapters/poco_json_adapter.hpp>
#include <valijson/utils/poco_json_utils.hpp>
#endif

#define TEST_DATA_DIR "../tests/data/documents/"

using valijson::adapters::AdapterTraits;

class TestAdapterComparison : public testing::Test
{
protected:

    struct JsonFile
    {
        JsonFile(std::string path, int strictGroup, int looseGroup)
          : m_path(std::move(path)),
            m_strictGroup(strictGroup),
            m_looseGroup(looseGroup) { }

        std::string m_path;
        int m_strictGroup;
        int m_looseGroup;
    };

    static void SetUpTestCase()
    {
        const std::string testDataDir(TEST_DATA_DIR);

        //
        // Each test is allocated to two groups. The first group is the strict
        // comparison group. All test files that have been assigned to the same
        // group should be equal, when compared using strict types. The second
        // group is the loose comparison group. All tests files in a loose
        // group should be equal, when compared without using strict types.
        //
        // As an example, the first three test files are in the same loose
        // group. This means they are expected to be equal when compared without
        // strict types. However, only the first two files in the same strict
        // group, which means that only they should be equal.
        //
        jsonFiles.emplace_back(testDataDir + "array_doubles_1_2_3.json",        1,  1);
        jsonFiles.emplace_back(testDataDir + "array_integers_1_2_3.json",       1,  1);
        jsonFiles.emplace_back(testDataDir + "array_strings_1_2_3.json",        2,  1);

        jsonFiles.emplace_back(testDataDir + "array_doubles_1_2_3_4.json",      3,  2);
        jsonFiles.emplace_back(testDataDir + "array_integers_1_2_3_4.json",     3,  2);
        jsonFiles.emplace_back(testDataDir + "array_strings_1_2_3_4.json",      4,  2);

        jsonFiles.emplace_back(testDataDir + "array_doubles_10_20_30_40.json",  5,  3);
        jsonFiles.emplace_back(testDataDir + "array_integers_10_20_30_40.json", 5,  3);
        jsonFiles.emplace_back(testDataDir + "array_strings_10_20_30_40.json",  6,  3);
    }

    template<typename Adapter1, typename Adapter2>
    static void testComparison()
    {
        std::vector<JsonFile>::const_iterator outerItr, innerItr;

        for(outerItr = jsonFiles.begin(); outerItr != jsonFiles.end() - 1; ++outerItr) {
            for(innerItr = outerItr; innerItr != jsonFiles.end(); ++innerItr) {

                const bool expectedStrict = (outerItr->m_strictGroup == innerItr->m_strictGroup);
                const bool expectedLoose = (outerItr->m_looseGroup == innerItr->m_looseGroup);

                typename AdapterTraits<Adapter1>::DocumentType document1;
                ASSERT_TRUE( valijson::utils::loadDocument(outerItr->m_path, document1) );
                const Adapter1 adapter1(document1);
                const std::string adapter1Name = AdapterTraits<Adapter1>::adapterName();

                typename AdapterTraits<Adapter2>::DocumentType document2;
                ASSERT_TRUE( valijson::utils::loadDocument(innerItr->m_path, document2) );
                const Adapter2 adapter2(document2);
                const std::string adapter2Name = AdapterTraits<Adapter2>::adapterName();

                // If either adapter does not support strict types, then strict
                // comparison should not be used, UNLESS the adapters are of the
                // same type. If they are of the same type, then the internal
                // type degradation should be the same, therefore strict testing
                // of equality makes sense.
                if (adapter1.hasStrictTypes() && adapter2.hasStrictTypes() && adapter1Name == adapter2Name) {
                    EXPECT_EQ(expectedStrict, adapter1.equalTo(adapter2, true))
                        << "Comparing '" << outerItr->m_path << "' to '"
                        << innerItr->m_path << "' "
                        << "with strict comparison enabled";
                    EXPECT_EQ(expectedStrict, adapter2.equalTo(adapter1, true))
                        << "Comparing '" << innerItr->m_path << "' to '"
                        << outerItr->m_path << "' "
                        << "with strict comparison enabled";
                }

                EXPECT_EQ(expectedLoose, adapter1.equalTo(adapter2, false))
                    << "Comparing '" << outerItr->m_path << "' to '"
                    << innerItr->m_path << "' "
                    << "with strict comparison disabled";
                EXPECT_EQ(expectedLoose, adapter2.equalTo(adapter1, false))
                    << "Comparing '" << innerItr->m_path << "' to '"
                    << outerItr->m_path << "' "
                    << "with strict comparison disabled";
            }
        }
    }

    static std::vector<JsonFile> jsonFiles;
};

std::vector<TestAdapterComparison::JsonFile> TestAdapterComparison::jsonFiles;

//
// JsonCppAdapter vs X
// ------------------------------------------------------------------------------------------------

TEST_F(TestAdapterComparison, JsonCppVsJsonCpp)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::JsonCppAdapter>();
}

TEST_F(TestAdapterComparison, JsonCppVsPicoJson)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

#ifdef VALIJSON_BUILD_BOOST_JSON_ADAPTER

TEST_F(TestAdapterComparison, JsonCppVsBoostJson)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::BoostJsonAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_JSON_ADAPTER

#ifdef VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, JsonCppVsPropertyTree)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::PropertyTreeAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, JsonCppVsRapidJson)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, JsonCppVsRapidJsonCrtAlloc)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> > >();
}

#ifdef VALIJSON_BUILD_BOOST_JSON_ADAPTER

//
// BoostJsonAdapter vs X
// ------------------------------------------------------------------------------------------------

TEST_F(TestAdapterComparison, BoostJsonVsPicoJson)
{
    testComparison<
        valijson::adapters::BoostJsonAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

TEST_F(TestAdapterComparison, BoostJsonVsBoostJson)
{
    testComparison<
        valijson::adapters::BoostJsonAdapter,
        valijson::adapters::BoostJsonAdapter>();
}

TEST_F(TestAdapterComparison, BoostJsonVsRapidJson)
{
    testComparison<
        valijson::adapters::BoostJsonAdapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, BoostJsonVsRapidJsonCrtAlloc)
{
    testComparison<
        valijson::adapters::BoostJsonAdapter,
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> > >();
}

#endif // VALIJSON_BUILD_BOOST_JSON_ADAPTER

#ifdef VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER
//
// PropertyTreeAdapter vs X
// ------------------------------------------------------------------------------------------------

TEST_F(TestAdapterComparison, PropertyTreeVsPicoJson)
{
    testComparison<
        valijson::adapters::PropertyTreeAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

TEST_F(TestAdapterComparison, PropertyTreeVsPropertyTree)
{
    testComparison<
        valijson::adapters::PropertyTreeAdapter,
        valijson::adapters::PropertyTreeAdapter>();
}

TEST_F(TestAdapterComparison, PropertyTreeVsRapidJson)
{
    testComparison<
        valijson::adapters::PropertyTreeAdapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, PropertyTreeVsRapidJsonCrtAlloc)
{
    testComparison<
        valijson::adapters::PropertyTreeAdapter,
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> > >();
}

#endif // VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

//
// RapidJson vs X
// ------------------------------------------------------------------------------------------------

TEST_F(TestAdapterComparison, RapidJsonVsRapidJson)
{
    testComparison<
        valijson::adapters::RapidJsonAdapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, RapidJsonVsRapidJsonCrtAlloc)
{
    testComparison<
        valijson::adapters::RapidJsonAdapter,
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> > >();
}

TEST_F(TestAdapterComparison, RapidJsonVsPicoJson)
{
    testComparison<
        valijson::adapters::RapidJsonAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

//
// PicoJsonAdapter vs X
// ------------------------------------------------------------------------------------------------

TEST_F(TestAdapterComparison, PicoJsonVsPicoJson)
{
    testComparison<
        valijson::adapters::PicoJsonAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

TEST_F(TestAdapterComparison, PicoJsonVsRapidJsonCrtAlloc)
{
    testComparison<
        valijson::adapters::PicoJsonAdapter,
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> > >();
}

TEST_F(TestAdapterComparison, RapidJsonCrtAllocVsRapidJsonCrtAlloc)
{
    testComparison<
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> >,
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> > >();
}

//
// Json11Adapter vs X
// ------------------------------------------------------------------------------------------------

TEST_F(TestAdapterComparison, Json11VsJson11)
{
    testComparison<
        valijson::adapters::Json11Adapter,
        valijson::adapters::Json11Adapter>();
}

TEST_F(TestAdapterComparison, Json11VsJsonCpp)
{
    testComparison<
        valijson::adapters::Json11Adapter,
        valijson::adapters::JsonCppAdapter>();
}


TEST_F(TestAdapterComparison, Json11VsRapidJson)
{
    testComparison<
        valijson::adapters::Json11Adapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, Json11VsRapidJsonCrtAlloc)
{
    testComparison<
        valijson::adapters::Json11Adapter,
        valijson::adapters::GenericRapidJsonAdapter<
            rapidjson::GenericValue<rapidjson::UTF8<>,
            rapidjson::CrtAllocator> > >();
}

TEST_F(TestAdapterComparison, Json11VsPicoJson)
{
    testComparison<
        valijson::adapters::Json11Adapter,
        valijson::adapters::PicoJsonAdapter>();
}

#ifdef VALIJSON_BUILD_BOOST_JSON_ADAPTER

TEST_F(TestAdapterComparison, Json11VsBoostJson)
{
    testComparison<
        valijson::adapters::Json11Adapter,
        valijson::adapters::BoostJsonAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_JSON_ADAPTER

#ifdef VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, Json11VsPropertyTree)
{
    testComparison<
        valijson::adapters::Json11Adapter,
        valijson::adapters::PropertyTreeAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

//
// NlohmannJsonAdapter vs X
// ------------------------------------------------------------------------------------------------

TEST_F(TestAdapterComparison, NlohmannJsonVsNlohmannJson) {
    testComparison<
        valijson::adapters::NlohmannJsonAdapter,
        valijson::adapters::NlohmannJsonAdapter>();
}

TEST_F(TestAdapterComparison, NlohmannJsonVsJson11)
{
    testComparison<
            valijson::adapters::NlohmannJsonAdapter,
            valijson::adapters::Json11Adapter>();
}

TEST_F(TestAdapterComparison, NlohmannJsonVsJsonCpp)
{
    testComparison<
            valijson::adapters::NlohmannJsonAdapter,
            valijson::adapters::JsonCppAdapter>();
}


TEST_F(TestAdapterComparison, NlohmannJsonVsRapidJson)
{
    testComparison<
            valijson::adapters::NlohmannJsonAdapter,
            valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, NlohmannJsonVsRapidJsonCrtAlloc)
{
    testComparison<
            valijson::adapters::NlohmannJsonAdapter,
            valijson::adapters::GenericRapidJsonAdapter<
                    rapidjson::GenericValue<rapidjson::UTF8<>,
                            rapidjson::CrtAllocator> > >();
}

TEST_F(TestAdapterComparison, NlohmannJsonVsPicoJson)
{
    testComparison<
            valijson::adapters::NlohmannJsonAdapter,
            valijson::adapters::PicoJsonAdapter>();
}

#ifdef VALIJSON_BUILD_BOOST_JSON_ADAPTER

TEST_F(TestAdapterComparison, NlohmannJsonVsBoostJson)
{
    testComparison<
            valijson::adapters::NlohmannJsonAdapter,
            valijson::adapters::BoostJsonAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_JSON_ADAPTER

#ifdef VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, NlohmannJsonVsPropertyTree)
{
    testComparison<
            valijson::adapters::NlohmannJsonAdapter,
            valijson::adapters::PropertyTreeAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

//
// QtJsonAdapter vs X
// ------------------------------------------------------------------------------------------------

#ifdef VALIJSON_BUILD_QT_ADAPTER

TEST_F(TestAdapterComparison, QtJsonVsQtJson) {
    testComparison<
        valijson::adapters::QtJsonAdapter,
        valijson::adapters::QtJsonAdapter>();
}

TEST_F(TestAdapterComparison, QtJsonVsJsonCpp)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::JsonCppAdapter>();
}

TEST_F(TestAdapterComparison, QtJsonVsRapidJson)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, QtJsonVsRapidJsonCrtAlloc)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::GenericRapidJsonAdapter<
                    rapidjson::GenericValue<rapidjson::UTF8<>,
                            rapidjson::CrtAllocator> > >();
}

TEST_F(TestAdapterComparison, QtJsonVsPicoJson)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::PicoJsonAdapter>();
}

#ifdef VALIJSON_BUILD_BOOST_JSON_ADAPTER

TEST_F(TestAdapterComparison, QtJsonVsBoostJson)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::BoostJsonAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_JSON_ADAPTER

#ifdef VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, QtJsonVsPropertyTree)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::PropertyTreeAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, QtJsonVsJson11)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::Json11Adapter>();
}

TEST_F(TestAdapterComparison, QtJsonVsNlohmannJson)
{
    testComparison<
            valijson::adapters::QtJsonAdapter,
            valijson::adapters::NlohmannJsonAdapter>();
}

#endif // VALIJSON_BUILD_QT_ADAPTER

//
// PocoJsonAdapter vs X
// ------------------------------------------------------------------------------------------------

#ifdef VALIJSON_BUILD_POCO_ADAPTER

TEST_F(TestAdapterComparison, PocoJsonVsPocoJson)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::PocoJsonAdapter>();
}

TEST_F(TestAdapterComparison, PocoJsonVsJsonCpp)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::JsonCppAdapter>();
}

TEST_F(TestAdapterComparison, PocoJsonVsRapidJson)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, PocoJsonVsRapidJsonCrtAlloc)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::GenericRapidJsonAdapter<
                    rapidjson::GenericValue<rapidjson::UTF8<>,
                            rapidjson::CrtAllocator> > >();
}

TEST_F(TestAdapterComparison, PocoJsonVsPicoJson)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::PicoJsonAdapter>();
}

#ifdef VALIJSON_BUILD_BOOST_JSON_ADAPTER

TEST_F(TestAdapterComparison, PocoJsonVsBoostJson)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::BoostJsonAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_JSON_ADAPTER

#ifdef VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, PocoJsonVsPropertyTree)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::PropertyTreeAdapter>();
}

#endif // VALIJSON_BUILD_BOOST_PROPERTY_TREE_ADAPTER

TEST_F(TestAdapterComparison, PocoJsonVsJson11)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::Json11Adapter>();
}

TEST_F(TestAdapterComparison, PocoJsonVsNlohmannJsonAdapter)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::NlohmannJsonAdapter>();
}

#ifdef VALIJSON_BUILD_QT_ADAPTER

TEST_F(TestAdapterComparison, PocoJsonVsQtJson)
{
    testComparison<
            valijson::adapters::PocoJsonAdapter,
            valijson::adapters::QtJsonAdapter>();
}

#endif // VALIJSON_BUILD_QT_ADAPTER

#endif // VALIJSON_BUILD_POCO_ADAPTER
