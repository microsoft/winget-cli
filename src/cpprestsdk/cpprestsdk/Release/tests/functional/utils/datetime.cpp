/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests for datetime-related utility functions and classes.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <stdint.h>
#include <string>

using namespace utility;

namespace tests
{
namespace functional
{
namespace utils_tests
{
SUITE(datetime)
{
    // This is by no means a comprehensive test suite for the datetime functionality.
    // It's a response to a particular bug and should be amended over time.

    TEST(parsing_dateandtime_basic)
    {
        // ISO 8601
        // RFC 1123

        auto dt1 = utility::datetime::from_string(_XPLATSTR("20130517T00:00:00Z"), utility::datetime::ISO_8601);
        VERIFY_ARE_NOT_EQUAL(0u, dt1.to_interval());

        auto dt2 =
            utility::datetime::from_string(_XPLATSTR("Fri, 17 May 2013 00:00:00 GMT"), utility::datetime::RFC_1123);
        VERIFY_ARE_NOT_EQUAL(0u, dt2.to_interval());

        VERIFY_ARE_EQUAL(dt1.to_interval(), dt2.to_interval());
    }

    TEST(parsing_dateandtime_extended)
    {
        // ISO 8601
        // RFC 1123

        auto dt1 = utility::datetime::from_string(_XPLATSTR("2013-05-17T00:00:00Z"), utility::datetime::ISO_8601);
        VERIFY_ARE_NOT_EQUAL(0u, dt1.to_interval());

        auto dt2 =
            utility::datetime::from_string(_XPLATSTR("Fri, 17 May 2013 00:00:00 GMT"), utility::datetime::RFC_1123);
        VERIFY_ARE_NOT_EQUAL(0u, dt2.to_interval());

        VERIFY_ARE_EQUAL(dt1.to_interval(), dt2.to_interval());
    }

    TEST(parsing_date_basic)
    {
        // ISO 8601
        {
            auto dt = utility::datetime::from_string(_XPLATSTR("20130517"), utility::datetime::ISO_8601);

            VERIFY_ARE_NOT_EQUAL(0u, dt.to_interval());
        }
    }

    TEST(parsing_date_extended)
    {
        // ISO 8601
        {
            auto dt = utility::datetime::from_string(_XPLATSTR("2013-05-17"), utility::datetime::ISO_8601);

            VERIFY_ARE_NOT_EQUAL(0u, dt.to_interval());
        }
    }

    void TestDateTimeRoundtrip(utility::string_t str, utility::string_t strExpected)
    {
        auto dt = utility::datetime::from_string(str, utility::datetime::ISO_8601);
        utility::string_t str2 = dt.to_string(utility::datetime::ISO_8601);
        VERIFY_ARE_EQUAL(str2, strExpected);

        auto dt_me = utility::datetime::from_string_maximum_error(str, utility::datetime::ISO_8601);
        utility::string_t str3 = dt_me.to_string(utility::datetime::ISO_8601);
        VERIFY_ARE_EQUAL(str3, strExpected);
    }

    void TestDateTimeRoundtrip(utility::string_t str) { TestDateTimeRoundtrip(str, str); }

    TEST(parsing_time_roundtrip_datetime1)
    {
        // Preserve all 7 digits after the comma:
        TestDateTimeRoundtrip(_XPLATSTR("2013-11-19T14:30:59.1234567Z"));
    }

    TEST(parsing_time_roundtrip_datetime2)
    {
        // lose the last '000'
        TestDateTimeRoundtrip(_XPLATSTR("2013-11-19T14:30:59.1234567000Z"), _XPLATSTR("2013-11-19T14:30:59.1234567Z"));
        // lose the last '999' without rounding up
        TestDateTimeRoundtrip(_XPLATSTR("2013-11-19T14:30:59.1234567999Z"), _XPLATSTR("2013-11-19T14:30:59.1234567Z"));
    }

    TEST(parsing_time_roundtrip_datetime3)
    {
        // leading 0-s after the comma, tricky to parse correctly
        TestDateTimeRoundtrip(_XPLATSTR("2013-11-19T14:30:59.00123Z"));
    }

    TEST(parsing_time_roundtrip_datetime4)
    {
        // another leading 0 test
        TestDateTimeRoundtrip(_XPLATSTR("2013-11-19T14:30:59.0000001Z"));
    }

    TEST(parsing_time_roundtrip_datetime5)
    {
        // this is going to be truncated
        TestDateTimeRoundtrip(_XPLATSTR("2013-11-19T14:30:59.00000001Z"), _XPLATSTR("2013-11-19T14:30:59Z"));
    }

    TEST(parsing_time_roundtrip_datetime6)
    {
        // Only one digit after the dot
        TestDateTimeRoundtrip(_XPLATSTR("2013-11-19T14:30:59.5Z"));
    }

    TEST(parsing_time_roundtrip_year_1900) { TestDateTimeRoundtrip(_XPLATSTR("1900-01-01T00:00:00Z")); }

    TEST(parsing_time_roundtrip_year_9999) { TestDateTimeRoundtrip(_XPLATSTR("9999-12-31T23:59:59Z")); }

    TEST(parsing_time_roundtrip_year_2016) { TestDateTimeRoundtrip(_XPLATSTR("2016-12-31T20:59:59Z")); }

    TEST(parsing_time_roundtrip_year_2020) { TestDateTimeRoundtrip(_XPLATSTR("2020-12-31T20:59:59Z")); }

    TEST(parsing_time_roundtrip_year_2021) { TestDateTimeRoundtrip(_XPLATSTR("2021-01-01T20:59:59Z")); }

    TEST(parsing_time_roundtrip_year_1601) { TestDateTimeRoundtrip(_XPLATSTR("1601-01-01T00:00:00Z")); }

    TEST(parsing_time_roundtrip_year_1602) { TestDateTimeRoundtrip(_XPLATSTR("1602-01-01T00:00:00Z")); }

    TEST(parsing_time_roundtrip_year_1603) { TestDateTimeRoundtrip(_XPLATSTR("1603-01-01T00:00:00Z")); }

    TEST(parsing_time_roundtrip_year_1604) { TestDateTimeRoundtrip(_XPLATSTR("1604-01-01T00:00:00Z")); }

    TEST(emitting_time_correct_day)
    {
        const auto test = utility::datetime() + UINT64_C(132004507640000000); // 2019-04-22T23:52:44 is a Monday
        const auto actual = test.to_string(utility::datetime::RFC_1123);
        const utility::string_t expected(_XPLATSTR("Mon"));
        VERIFY_ARE_EQUAL(actual.substr(0, 3), expected);
    }

    void TestRfc1123IsTimeT(const utility::char_t* str, uint64_t t)
    {
        datetime dt = datetime::from_string(str, utility::datetime::RFC_1123);
        uint64_t interval = dt.to_interval();
        VERIFY_ARE_EQUAL(0, interval % 10000000);
        interval /= 10000000;
        interval -= 11644473600; // NT epoch adjustment
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(t), interval);
    }

    TEST(parsing_time_rfc1123_accepts_each_day)
    {
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 00:00:00 GMT"), 0);
        TestRfc1123IsTimeT(_XPLATSTR("Fri, 02 Jan 1970 00:00:00 GMT"), 86400 * 1);
        TestRfc1123IsTimeT(_XPLATSTR("Sat, 03 Jan 1970 00:00:00 GMT"), 86400 * 2);
        TestRfc1123IsTimeT(_XPLATSTR("Sun, 04 Jan 1970 00:00:00 GMT"), 86400 * 3);
        TestRfc1123IsTimeT(_XPLATSTR("Mon, 05 Jan 1970 00:00:00 GMT"), 86400 * 4);
        TestRfc1123IsTimeT(_XPLATSTR("Tue, 06 Jan 1970 00:00:00 GMT"), 86400 * 5);
        TestRfc1123IsTimeT(_XPLATSTR("Wed, 07 Jan 1970 00:00:00 GMT"), 86400 * 6);
    }

    TEST(parsing_time_rfc1123_boundary_cases)
    {
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 00:00:00 GMT"), 0);
        TestRfc1123IsTimeT(_XPLATSTR("19 Jan 2038 03:14:06 GMT"), INT_MAX - 1);
        TestRfc1123IsTimeT(_XPLATSTR("19 Jan 2038 03:13:07 -0001"), INT_MAX);
        TestRfc1123IsTimeT(_XPLATSTR("19 Jan 2038 03:14:07 -0000"), INT_MAX);
        TestRfc1123IsTimeT(_XPLATSTR("14 Jan 2019 23:16:21 +0000"), 1547507781);
        TestRfc1123IsTimeT(_XPLATSTR("14 Jan 2019 23:16:21 -0001"), 1547507841);
        TestRfc1123IsTimeT(_XPLATSTR("14 Jan 2019 23:16:21 +0001"), 1547507721);
        TestRfc1123IsTimeT(_XPLATSTR("14 Jan 2019 23:16:21 -0100"), 1547511381);
        TestRfc1123IsTimeT(_XPLATSTR("14 Jan 2019 23:16:21 +0100"), 1547504181);
    }

    TEST(parsing_time_rfc1123_uses_each_field)
    {
        TestRfc1123IsTimeT(_XPLATSTR("02 Jan 1970 00:00:00 GMT"), 86400);
        TestRfc1123IsTimeT(_XPLATSTR("12 Jan 1970 00:00:00 GMT"), 950400);
        TestRfc1123IsTimeT(_XPLATSTR("01 Feb 1970 00:00:00 GMT"), 2678400);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 2000 00:00:00 GMT"), 946684800);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 2100 00:00:00 GMT"), 4102444800);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1990 00:00:00 GMT"), 631152000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1971 00:00:00 GMT"), 31536000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 10:00:00 GMT"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 01:00:00 GMT"), 3600);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 00:10:00 GMT"), 600);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 00:01:00 GMT"), 60);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 00:00:10 GMT"), 10);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 00:00:01 GMT"), 1);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 10:00:00 GMT"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 02:00:00 PST"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 03:00:00 PDT"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 03:00:00 MST"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 04:00:00 MDT"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 04:00:00 CST"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 05:00:00 CDT"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 05:00:00 EST"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 06:00:00 EDT"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 06:00:00 -0400"), 36000);
        TestRfc1123IsTimeT(_XPLATSTR("01 Jan 1970 05:59:00 -0401"), 36000);
    }

    TEST(parsing_time_rfc1123_max_days)
    {
        TestRfc1123IsTimeT(_XPLATSTR("31 Jan 1970 00:00:00 GMT"), 2592000);
        TestRfc1123IsTimeT(_XPLATSTR("28 Feb 2019 00:00:00 GMT"), 1551312000); // non leap year allows feb 28
        TestRfc1123IsTimeT(_XPLATSTR("29 Feb 2020 00:00:00 GMT"), 1582934400); // leap year allows feb 29
        TestRfc1123IsTimeT(_XPLATSTR("31 Mar 1970 00:00:00 GMT"), 7689600);
        TestRfc1123IsTimeT(_XPLATSTR("30 Apr 1970 00:00:00 GMT"), 10281600);
        TestRfc1123IsTimeT(_XPLATSTR("31 May 1970 00:00:00 GMT"), 12960000);
        TestRfc1123IsTimeT(_XPLATSTR("30 Jun 1970 00:00:00 GMT"), 15552000);
        TestRfc1123IsTimeT(_XPLATSTR("31 Jul 1970 00:00:00 GMT"), 18230400);
        TestRfc1123IsTimeT(_XPLATSTR("31 Aug 1970 00:00:00 GMT"), 20908800);
        TestRfc1123IsTimeT(_XPLATSTR("30 Sep 1970 00:00:00 GMT"), 23500800);
        TestRfc1123IsTimeT(_XPLATSTR("31 Oct 1970 00:00:00 GMT"), 26179200);
        TestRfc1123IsTimeT(_XPLATSTR("30 Nov 1970 00:00:00 GMT"), 28771200);
        TestRfc1123IsTimeT(_XPLATSTR("31 Dec 1970 00:00:00 GMT"), 31449600);
    }

    TEST(parsing_time_rfc1123_invalid_cases)
    {
        const utility::string_t bad_strings[] = {
            _XPLATSTR("Ahu, 01 Jan 1970 00:00:00 GMT"), // bad letters in each place
            _XPLATSTR("TAu, 01 Jan 1970 00:00:00 GMT"),
            _XPLATSTR("ThA, 01 Jan 1970 00:00:00 GMT"),
            _XPLATSTR("ThuA 01 Jan 1970 00:00:00 GMT"),
            _XPLATSTR("Thu,A01 Jan 1970 00:00:00 GMT"),
            _XPLATSTR("Thu, A1 Jan 1970 00:00:00 GMT"),
            _XPLATSTR("Thu, 0A Jan 1970 00:00:00 GMT"),
            _XPLATSTR("Thu, 01AJan 1970 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 Aan 1970 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 JAn 1970 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 JaA 1970 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 JanA1970 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan A970 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1A70 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 19A0 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 197A 00:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970A00:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 A0:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 0A:00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00A00:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:A0:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:0A:00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00A00 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:A0 GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:0A GMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00AGMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00 AMT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00 GAT"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00 GMA"),
            _XPLATSTR(""), // truncation
            _XPLATSTR("T"),
            _XPLATSTR("Th"),
            _XPLATSTR("Thu"),
            _XPLATSTR("Thu,"),
            _XPLATSTR("Thu, "),
            _XPLATSTR("Thu, 0"),
            _XPLATSTR("Thu, 01"),
            _XPLATSTR("Thu, 01 "),
            _XPLATSTR("Thu, 01 J"),
            _XPLATSTR("Thu, 01 Ja"),
            _XPLATSTR("Thu, 01 Jan"),
            _XPLATSTR("Thu, 01 Jan "),
            _XPLATSTR("Thu, 01 Jan 1"),
            _XPLATSTR("Thu, 01 Jan 19"),
            _XPLATSTR("Thu, 01 Jan 197"),
            _XPLATSTR("Thu, 01 Jan 1970"),
            _XPLATSTR("Thu, 01 Jan 1970 "),
            _XPLATSTR("Thu, 01 Jan 1970 0"),
            _XPLATSTR("Thu, 01 Jan 1970 00"),
            _XPLATSTR("Thu, 01 Jan 1970 00:"),
            _XPLATSTR("Thu, 01 Jan 1970 00:0"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:0"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00 "),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00 G"),
            _XPLATSTR("Thu, 01 Jan 1970 00:00:00 GM"),
            _XPLATSTR("Fri, 01 Jan 1970 00:00:00 GMT"), // wrong day
            _XPLATSTR("01 Jan 1600 00:00:00 GMT"),      // year too small
            _XPLATSTR("01 Xxx 1971 00:00:00 GMT"),      // month bad
            _XPLATSTR("00 Jan 1971 00:00:00 GMT"),      // day too small
            _XPLATSTR("32 Jan 1971 00:00:00 GMT"),      // day too big
            _XPLATSTR("30 Feb 1971 00:00:00 GMT"),      // day too big for feb
            _XPLATSTR("30 Feb 1971 00:00:00 GMT"),      // day too big for feb (non-leap year)
            _XPLATSTR("32 Mar 1971 00:00:00 GMT"),      // other months
            _XPLATSTR("31 Apr 1971 00:00:00 GMT"),
            _XPLATSTR("32 May 1971 00:00:00 GMT"),
            _XPLATSTR("31 Jun 1971 00:00:00 GMT"),
            _XPLATSTR("32 Jul 1971 00:00:00 GMT"),
            _XPLATSTR("32 Aug 1971 00:00:00 GMT"),
            _XPLATSTR("31 Sep 1971 00:00:00 GMT"),
            _XPLATSTR("32 Oct 1971 00:00:00 GMT"),
            _XPLATSTR("31 Nov 1971 00:00:00 GMT"),
            _XPLATSTR("32 Dec 1971 00:00:00 GMT"),
            _XPLATSTR("01 Jan 1971 70:00:00 GMT"), // hour too big
            _XPLATSTR("01 Jan 1971 24:00:00 GMT"),
            _XPLATSTR("01 Jan 1971 00:60:00 GMT"), // minute too big
            _XPLATSTR("01 Jan 1971 00:00:70 GMT"), // second too big
            _XPLATSTR("01 Jan 1971 00:00:61 GMT"),
            _XPLATSTR("01 Jan 1600 00:00:00 GMT"),   // underflow
            _XPLATSTR("01 Jan 1969 00:00:00 CEST"),  // bad tz
            _XPLATSTR("14 Jan 2019 23:16:21 G0100"), // bad tzoffsets
            _XPLATSTR("01 Jan 1970 00:00:00 +2400"),
            _XPLATSTR("01 Jan 1970 00:00:00 -3000"),
            _XPLATSTR("01 Jan 1970 00:00:00 +2160"),
            _XPLATSTR("01 Jan 1970 00:00:00 -2400"),
            _XPLATSTR("01 Jan 1970 00:00:00 -2160"),
            _XPLATSTR("00 Jan 1971 00:00:00 GMT"), // zero month day
        };

        for (const auto& str : bad_strings)
        {
            auto dt = utility::datetime::from_string(str, utility::datetime::RFC_1123);
            VERIFY_ARE_EQUAL(0, dt.to_interval());
            auto dt_me = utility::datetime::from_string_maximum_error(str, utility::datetime::RFC_1123);
            VERIFY_ARE_EQUAL(utility::datetime::maximum(), dt_me);
        }
    }

    TEST(parsing_time_iso8601_boundary_cases)
    {
        // boundary cases:
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T00:00:00Z"));                                         // epoch
        TestDateTimeRoundtrip(_XPLATSTR("2038-01-19T03:14:06+00:00"), _XPLATSTR("2038-01-19T03:14:06Z")); // INT_MAX - 1
        TestDateTimeRoundtrip(_XPLATSTR("2038-01-19T03:13:07-00:01"),
                              _XPLATSTR("2038-01-19T03:14:07Z")); // INT_MAX after subtacting 1
        TestDateTimeRoundtrip(_XPLATSTR("2038-01-19T03:14:07-00:00"), _XPLATSTR("2038-01-19T03:14:07Z"));
    }

    TEST(parsing_time_iso8601_uses_each_timezone_digit)
    {
        TestDateTimeRoundtrip(_XPLATSTR("2019-01-14T23:16:21+00:00"), _XPLATSTR("2019-01-14T23:16:21Z"));
        TestDateTimeRoundtrip(_XPLATSTR("2019-01-14T23:16:21-00:01"), _XPLATSTR("2019-01-14T23:17:21Z"));
        TestDateTimeRoundtrip(_XPLATSTR("2019-01-14T23:16:21+00:01"), _XPLATSTR("2019-01-14T23:15:21Z"));
        TestDateTimeRoundtrip(_XPLATSTR("2019-01-14T23:16:21-01:00"), _XPLATSTR("2019-01-15T00:16:21Z"));
        TestDateTimeRoundtrip(_XPLATSTR("2019-01-14T23:16:21+01:00"), _XPLATSTR("2019-01-14T22:16:21Z"));
    }

    TEST(parsing_time_iso8601_uses_each_digit)
    {
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T00:00:01Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T00:01:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T01:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-02T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-02-01T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1971-01-01T00:00:00Z"));

        TestDateTimeRoundtrip(_XPLATSTR("1999-01-01T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-12-01T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-09-01T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-30T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-31T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T23:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T19:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T00:59:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T00:00:59Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T00:00:60Z"), _XPLATSTR("1970-01-01T00:01:00Z")); // leap seconds
    }

    TEST(parsing_time_iso8601_accepts_month_max_days)
    {
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-31T00:00:00Z")); // jan
        TestDateTimeRoundtrip(_XPLATSTR("2019-02-28T00:00:00Z")); // non leap year allows feb 28
        TestDateTimeRoundtrip(_XPLATSTR("2020-02-29T00:00:00Z")); // leap year allows feb 29
        TestDateTimeRoundtrip(_XPLATSTR("1970-03-31T00:00:00Z")); // mar
        TestDateTimeRoundtrip(_XPLATSTR("1970-04-30T00:00:00Z")); // apr
        TestDateTimeRoundtrip(_XPLATSTR("1970-05-31T00:00:00Z")); // may
        TestDateTimeRoundtrip(_XPLATSTR("1970-06-30T00:00:00Z")); // jun
        TestDateTimeRoundtrip(_XPLATSTR("1970-07-31T00:00:00Z")); // jul
        TestDateTimeRoundtrip(_XPLATSTR("1970-08-31T00:00:00Z")); // aug
        TestDateTimeRoundtrip(_XPLATSTR("1970-09-30T00:00:00Z")); // sep
        TestDateTimeRoundtrip(_XPLATSTR("1970-10-31T00:00:00Z")); // oct
        TestDateTimeRoundtrip(_XPLATSTR("1970-11-30T00:00:00Z")); // nov
        TestDateTimeRoundtrip(_XPLATSTR("1970-12-31T00:00:00Z")); // dec
    }

    TEST(parsing_time_iso8601_accepts_lowercase_t_z)
    {
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01t00:00:00Z"), _XPLATSTR("1970-01-01T00:00:00Z"));
        TestDateTimeRoundtrip(_XPLATSTR("1970-01-01T00:00:00z"), _XPLATSTR("1970-01-01T00:00:00Z"));
    }

    TEST(parsing_time_roundtrip_datetime_accepts_invalid_no_trailing_timezone)
    {
        // No digits after the dot, or non-digits. This is not a valid input, but we should not choke on it,
        // Simply ignore the bad fraction
        const utility::string_t bad_strings[] = {_XPLATSTR("2013-11-19T14:30:59.Z"),
                                                 _XPLATSTR("2013-11-19T14:30:59.a12Z")};
        utility::string_t str_corrected = _XPLATSTR("2013-11-19T14:30:59Z");

        for (const auto& str : bad_strings)
        {
            auto dt = utility::datetime::from_string(str, utility::datetime::ISO_8601);
            utility::string_t str2 = dt.to_string(utility::datetime::ISO_8601);
            VERIFY_ARE_EQUAL(str2, str_corrected);
        }
    }

    TEST(parsing_time_roundtrip_datetime_invalid2)
    {
        // Various unsupported cases. In all cases, we have produce an empty date time
        const utility::string_t bad_strings[] = {
            _XPLATSTR(""),                     // empty
            _XPLATSTR(".Z"),                   // too short
            _XPLATSTR(".Zx"),                  // no trailing Z
            _XPLATSTR("3.14Z")                 // not a valid date
            _XPLATSTR("a971-01-01T00:00:00Z"), // any non digits or valid separators
            _XPLATSTR("1a71-01-01T00:00:00Z"),
            _XPLATSTR("19a1-01-01T00:00:00Z"),
            _XPLATSTR("197a-01-01T00:00:00Z"),
            _XPLATSTR("1971a01-01T00:00:00Z"),
            _XPLATSTR("1971-a1-01T00:00:00Z"),
            _XPLATSTR("1971-0a-01T00:00:00Z"),
            _XPLATSTR("1971-01a01T00:00:00Z"),
            _XPLATSTR("1971-01-a1T00:00:00Z"),
            _XPLATSTR("1971-01-0aT00:00:00Z"),
            // _XPLATSTR("1971-01-01a00:00:00Z"), parsed as complete date
            _XPLATSTR("1971-01-01Ta0:00:00Z"),
            _XPLATSTR("1971-01-01T0a:00:00Z"),
            _XPLATSTR("1971-01-01T00a00:00Z"),
            _XPLATSTR("1971-01-01T00:a0:00Z"),
            _XPLATSTR("1971-01-01T00:0a:00Z"),
            _XPLATSTR("1971-01-01T00:00a00Z"),
            _XPLATSTR("1971-01-01T00:00:a0Z"),
            _XPLATSTR("1971-01-01T00:00:0aZ"),
            // "1971-01-01T00:00:00a", accepted as per invalid_no_trailing_timezone above
            _XPLATSTR("1"), // truncation
            _XPLATSTR("19"),
            _XPLATSTR("197"),
            _XPLATSTR("1970"),
            _XPLATSTR("1970-"),
            _XPLATSTR("1970-0"),
            _XPLATSTR("1970-01"),
            _XPLATSTR("1970-01-"),
            _XPLATSTR("1970-01-0"),
            // _XPLATSTR("1970-01-01"), complete date
            _XPLATSTR("1970-01-01T"),
            _XPLATSTR("1970-01-01T0"),
            _XPLATSTR("1970-01-01T00"),
            _XPLATSTR("1970-01-01T00:"),
            _XPLATSTR("1970-01-01T00:0"),
            _XPLATSTR("1970-01-01T00:00"),
            _XPLATSTR("1970-01-01T00:00:"),
            _XPLATSTR("1970-01-01T00:00:0"),
            // _XPLATSTR("1970-01-01T00:00:00"), // accepted as invalid timezone above
            _XPLATSTR("1600-01-01T00:00:00Z"), // year too small
            _XPLATSTR("1971-00-01T00:00:00Z"), // month too small
            _XPLATSTR("1971-20-01T00:00:00Z"), // month too big
            _XPLATSTR("1971-13-01T00:00:00Z"),
            _XPLATSTR("1971-01-00T00:00:00Z"), // day too small
            _XPLATSTR("1971-01-32T00:00:00Z"), // day too big
            _XPLATSTR("1971-02-30T00:00:00Z"), // day too big for feb
            _XPLATSTR("1971-02-30T00:00:00Z"), // day too big for feb (non-leap year)
            _XPLATSTR("1971-03-32T00:00:00Z"), // other months
            _XPLATSTR("1971-04-31T00:00:00Z"),
            _XPLATSTR("1971-05-32T00:00:00Z"),
            _XPLATSTR("1971-06-31T00:00:00Z"),
            _XPLATSTR("1971-07-32T00:00:00Z"),
            _XPLATSTR("1971-08-32T00:00:00Z"),
            _XPLATSTR("1971-09-31T00:00:00Z"),
            _XPLATSTR("1971-10-32T00:00:00Z"),
            _XPLATSTR("1971-11-31T00:00:00Z"),
            _XPLATSTR("1971-12-32T00:00:00Z"),
            _XPLATSTR("1971-01-01T70:00:00Z"), // hour too big
            _XPLATSTR("1971-01-01T24:00:00Z"),
            _XPLATSTR("1971-01-01T00:60:00Z"), // minute too big
            _XPLATSTR("1971-01-01T00:00:70Z"), // second too big
            _XPLATSTR("1971-01-01T00:00:61Z"),
            _XPLATSTR("1600-01-01T00:00:00Z"),      // underflow
            _XPLATSTR("1601-01-01T00:00:00+00:01"), // time zone underflow
            // _XPLATSTR("1970-01-01T00:00:00.Z"), // accepted as invalid timezone above
            _XPLATSTR("1970-01-01T00:00:00+24:00"), // bad tzoffsets
            _XPLATSTR("1970-01-01T00:00:00-30:00"),
            _XPLATSTR("1970-01-01T00:00:00+21:60"),
            _XPLATSTR("1970-01-01T00:00:00-24:00"),
            _XPLATSTR("1970-01-01T00:00:00-21:60"),
            _XPLATSTR("1971-01-00"), // zero month day
        };

        for (const auto& str : bad_strings)
        {
            auto dt = utility::datetime::from_string(str, utility::datetime::ISO_8601);
            VERIFY_ARE_EQUAL(dt.to_interval(), 0);
            auto dt_me = utility::datetime::from_string_maximum_error(str, utility::datetime::ISO_8601);
            VERIFY_ARE_EQUAL(dt_me, utility::datetime::maximum());
        }
    }

    TEST(can_emit_nt_epoch_zero_rfc_1123)
    {
        auto result = utility::datetime {}.to_string(utility::datetime::RFC_1123);
        VERIFY_ARE_EQUAL(_XPLATSTR("Mon, 01 Jan 1601 00:00:00 GMT"), result);
    }

    TEST(can_emit_nt_epoch_zero_iso_8601)
    {
        auto result = utility::datetime {}.to_string(utility::datetime::ISO_8601);
        VERIFY_ARE_EQUAL(_XPLATSTR("1601-01-01T00:00:00Z"), result);
    }

    TEST(can_emit_year_9999_rfc_1123)
    {
        auto result =
            utility::datetime::from_interval(INT64_C(0x24C85A5ED1C018F0)).to_string(utility::datetime::RFC_1123);
        VERIFY_ARE_EQUAL(_XPLATSTR("Fri, 31 Dec 9999 23:59:59 GMT"), result);
    }

    TEST(can_emit_year_9999_iso_8601)
    {
        auto result =
            utility::datetime::from_interval(INT64_C(0x24C85A5ED1C018F0)).to_string(utility::datetime::ISO_8601);
        VERIFY_ARE_EQUAL(_XPLATSTR("9999-12-31T23:59:59.999Z"), result);
    }

    TEST(can_parse_nt_epoch_zero_rfc_1123)
    {
        auto dt =
            utility::datetime::from_string(_XPLATSTR("Mon, 01 Jan 1601 00:00:00 GMT"), utility::datetime::RFC_1123);
        VERIFY_ARE_EQUAL(0U, dt.to_interval());
        auto dt_me = utility::datetime::from_string_maximum_error(_XPLATSTR("Mon, 01 Jan 1601 00:00:00 GMT"),
                                                                  utility::datetime::RFC_1123);
        VERIFY_ARE_EQUAL(0U, dt_me.to_interval());
    }

    TEST(can_parse_nt_epoch_zero_iso_8601)
    {
        auto dt = utility::datetime::from_string(_XPLATSTR("1601-01-01T00:00:00Z"), utility::datetime::ISO_8601);
        VERIFY_ARE_EQUAL(0U, dt.to_interval());
        auto dt_me = utility::datetime::from_string_maximum_error(_XPLATSTR("1601-01-01T00:00:00Z"),
                                                                  utility::datetime::ISO_8601);
        VERIFY_ARE_EQUAL(0U, dt_me.to_interval());
    }
} // SUITE(datetime)

} // namespace utils_tests
} // namespace functional
} // namespace tests
