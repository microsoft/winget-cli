/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * compression_tests.cpp
 *
 * Tests cases, including client/server, for the web::http::compression namespace.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "cpprest/asyncrt_utils.h"
#include "cpprest/details/http_helpers.h"
#include "cpprest/version.h"
#include <fstream>

#ifndef __cplusplus_winrt
#include "cpprest/http_listener.h"
#endif

using namespace web;
using namespace utility;
using namespace web::http;
using namespace web::http::client;
using namespace web::http::compression;

using namespace tests::functional::http::utilities;

namespace tests
{
namespace functional
{
namespace http
{
namespace client
{
SUITE(compression_tests)
{
    // A fake "pass-through" compressor/decompressor for testing
    class fake_provider : public compress_provider, public decompress_provider
    {
    public:
        static const utility::string_t FAKE;

        fake_provider(size_t size = static_cast<size_t>(-1)) : _size(size), _so_far(0), _done(false) {}

        virtual const utility::string_t& algorithm() const { return FAKE; }

        virtual size_t decompress(const uint8_t* input,
                                  size_t input_size,
                                  uint8_t* output,
                                  size_t output_size,
                                  operation_hint hint,
                                  size_t& input_bytes_processed,
                                  bool& done)
        {
            size_t bytes;

            if (_done)
            {
                input_bytes_processed = 0;
                done = true;
                return 0;
            }
            if (_size == static_cast<size_t>(-1) || input_size > _size - _so_far)
            {
                std::stringstream ss;
                ss << "Fake decompress - invalid data " << input_size << ", " << output_size << " with " << _so_far
                   << " / " << _size;
                throw std::runtime_error(std::move(ss.str()));
            }
            bytes = (std::min)(input_size, output_size);
            if (bytes)
            {
                memcpy(output, input, bytes);
            }
            _so_far += bytes;
            _done = (_so_far == _size);
            done = _done;
            input_bytes_processed = bytes;
            return input_bytes_processed;
        }

        virtual pplx::task<operation_result> decompress(
            const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint)
        {
            operation_result r;

            try
            {
                r.output_bytes_produced =
                    decompress(input, input_size, output, output_size, hint, r.input_bytes_processed, r.done);
            }
            catch (...)
            {
                pplx::task_completion_event<operation_result> ev;
                ev.set_exception(std::current_exception());
                return pplx::create_task(ev);
            }

            return pplx::task_from_result<operation_result>(r);
        }

        virtual size_t compress(const uint8_t* input,
                                size_t input_size,
                                uint8_t* output,
                                size_t output_size,
                                operation_hint hint,
                                size_t& input_bytes_processed,
                                bool& done)
        {
            size_t bytes;

            if (_done)
            {
                input_bytes_processed = 0;
                done = true;
                return 0;
            }
            if (_size == static_cast<size_t>(-1) || input_size > _size - _so_far)
            {
                std::stringstream ss;
                ss << "Fake compress - invalid data " << input_size << ", " << output_size << " with " << _so_far
                   << " / " << _size;
                throw std::runtime_error(std::move(ss.str()));
            }
            bytes = (std::min)(input_size, output_size);
            if (bytes)
            {
                memcpy(output, input, bytes);
            }
            _so_far += bytes;
            _done = (hint == operation_hint::is_last && _so_far == _size);
            done = _done;
            input_bytes_processed = bytes;
            return input_bytes_processed;
        }

        virtual pplx::task<operation_result> compress(
            const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size, operation_hint hint)
        {
            operation_result r;

            try
            {
                r.output_bytes_produced =
                    compress(input, input_size, output, output_size, hint, r.input_bytes_processed, r.done);
            }
            catch (...)
            {
                pplx::task_completion_event<operation_result> ev;
                ev.set_exception(std::current_exception());
                return pplx::create_task(ev);
            }

            return pplx::task_from_result<operation_result>(r);
        }

        virtual void reset()
        {
            _done = false;
            _so_far = 0;
        }

    private:
        size_t _size;
        size_t _so_far;
        bool _done;
    };

    const utility::string_t fake_provider::FAKE = _XPLATSTR("fake");

    void compress_and_decompress(std::unique_ptr<compress_provider> compressor,
                                 std::unique_ptr<decompress_provider> decompressor,
                                 const size_t buffer_size,
                                 const size_t chunk_size,
                                 bool compressible)
    {
        std::vector<uint8_t> input_buffer;
        size_t i;

        VERIFY_ARE_EQUAL(compressor->algorithm(), decompressor->algorithm());

        input_buffer.reserve(buffer_size);
        for (i = 0; i < buffer_size; ++i)
        {
            uint8_t element;
            if (compressible)
            {
                element = static_cast<uint8_t>('a' + i % 26);
            }
            else
            {
                element = static_cast<uint8_t>(std::rand());
            }

            input_buffer.push_back(element);
        }

        // compress in chunks
        std::vector<size_t> chunk_sizes;
        std::vector<uint8_t> cmp_buffer(buffer_size);
        size_t cmpsize = buffer_size;
        size_t csize = 0;
        operation_result r = {};
        operation_hint hint = operation_hint::has_more;
        for (i = 0; i < buffer_size || csize == cmpsize || !r.done; i += r.input_bytes_processed)
        {
            if (i == buffer_size)
            {
                // the entire input buffer has been consumed by the compressor
                hint = operation_hint::is_last;
            }
            if (csize == cmpsize)
            {
                // extend the output buffer if there may be more compressed bytes to retrieve
                cmpsize += (std::min)(chunk_size, (size_t)200);
                cmp_buffer.resize(cmpsize);
            }
            r = compressor
                    ->compress(input_buffer.data() + i,
                               (std::min)(chunk_size, buffer_size - i),
                               cmp_buffer.data() + csize,
                               (std::min)(chunk_size, cmpsize - csize),
                               hint)
                    .get();
            VERIFY_IS_TRUE(r.input_bytes_processed == (std::min)(chunk_size, buffer_size - i) ||
                           r.output_bytes_produced == (std::min)(chunk_size, cmpsize - csize));
            VERIFY_IS_TRUE(hint == operation_hint::is_last || !r.done);
            chunk_sizes.push_back(r.output_bytes_produced);
            csize += r.output_bytes_produced;
        }
        VERIFY_ARE_EQUAL(r.done, true);

        // once more with no input or output, to assure no error and done
        r = compressor->compress(NULL, 0, NULL, 0, operation_hint::is_last).get();
        VERIFY_ARE_EQUAL(r.input_bytes_processed, 0);
        VERIFY_ARE_EQUAL(r.output_bytes_produced, 0);
        VERIFY_ARE_EQUAL(r.done, true);

        cmp_buffer.resize(csize); // actual

        // decompress in as-compressed chunks
        std::vector<uint8_t> dcmp_buffer(buffer_size);
        size_t dsize = 0;
        size_t nn = 0;
        for (std::vector<size_t>::iterator it = chunk_sizes.begin(); it != chunk_sizes.end(); ++it)
        {
            if (*it)
            {
                auto hint = operation_hint::has_more;
                if (it == chunk_sizes.begin())
                {
                    hint = operation_hint::is_last;
                }

                r = decompressor
                        ->decompress(cmp_buffer.data() + nn,
                                     *it,
                                     dcmp_buffer.data() + dsize,
                                     (std::min)(chunk_size, buffer_size - dsize),
                                     hint)
                        .get();
                nn += *it;
                dsize += r.output_bytes_produced;
            }
        }
        VERIFY_ARE_EQUAL(csize, nn);
        VERIFY_ARE_EQUAL(dsize, buffer_size);
        VERIFY_ARE_EQUAL(input_buffer, dcmp_buffer);
        VERIFY_IS_TRUE(r.done);

        // decompress again in fixed-size chunks
        nn = 0;
        dsize = 0;
        decompressor->reset();
        memset(dcmp_buffer.data(), 0, dcmp_buffer.size());
        do
        {
            size_t n = (std::min)(chunk_size, csize - nn);
            do
            {
                r = decompressor
                        ->decompress(cmp_buffer.data() + nn,
                                     n,
                                     dcmp_buffer.data() + dsize,
                                     (std::min)(chunk_size, buffer_size - dsize),
                                     operation_hint::has_more)
                        .get();
                dsize += r.output_bytes_produced;
                nn += r.input_bytes_processed;
                n -= r.input_bytes_processed;
            } while (n);
        } while (nn < csize || !r.done);
        VERIFY_ARE_EQUAL(csize, nn);
        VERIFY_ARE_EQUAL(dsize, buffer_size);
        VERIFY_ARE_EQUAL(input_buffer, dcmp_buffer);
        VERIFY_IS_TRUE(r.done);

        // once more with no input, to assure no error and done
        r = decompressor->decompress(NULL, 0, NULL, 0, operation_hint::has_more).get();
        VERIFY_ARE_EQUAL(r.input_bytes_processed, 0);
        VERIFY_ARE_EQUAL(r.output_bytes_produced, 0);
        VERIFY_IS_TRUE(r.done);

        // decompress all at once
        decompressor->reset();
        memset(dcmp_buffer.data(), 0, dcmp_buffer.size());
        r = decompressor
                ->decompress(cmp_buffer.data(), csize, dcmp_buffer.data(), dcmp_buffer.size(), operation_hint::is_last)
                .get();
        VERIFY_ARE_EQUAL(r.output_bytes_produced, buffer_size);
        VERIFY_ARE_EQUAL(input_buffer, dcmp_buffer);

        if (decompressor->algorithm() != fake_provider::FAKE)
        {
            // invalid decompress buffer, first and subsequent tries
            cmp_buffer[0] = ~cmp_buffer[1];
            decompressor->reset();
            for (i = 0; i < 2; i++)
            {
                nn = 0;
                try
                {
                    r = decompressor
                            ->decompress(cmp_buffer.data(),
                                         csize,
                                         dcmp_buffer.data(),
                                         dcmp_buffer.size(),
                                         operation_hint::is_last)
                            .get();
                    VERIFY_IS_FALSE(r.done && r.output_bytes_produced == buffer_size);
                }
                catch (std::runtime_error)
                {
                }
            }
        }
    }

    void compress_test(std::shared_ptr<compress_factory> cfactory, std::shared_ptr<decompress_factory> dfactory)
    {
        size_t tuples[][2] = {{3, 1024},
                              {7999, 8192},
                              {8192, 8192},
                              {16001, 8192},
                              {16384, 8192},
                              {140000, 65536},
                              {256 * 1024, 65536},
                              {256 * 1024, 256 * 1024},
                              {263456, 256 * 1024}};

        for (int i = 0; i < sizeof(tuples) / sizeof(tuples[0]); i++)
        {
            for (int j = 0; j < 2; j++)
            {
                if (!cfactory)
                {
                    auto size = tuples[i][0];
                    compress_and_decompress(utility::details::make_unique<fake_provider>(size),
                                            utility::details::make_unique<fake_provider>(size),
                                            size,
                                            tuples[i][1],
                                            !!j);
                }
                else
                {
                    compress_and_decompress(
                        cfactory->make_compressor(), dfactory->make_decompressor(), tuples[i][0], tuples[i][1], !!j);
                }
            }
        }
    }

    TEST_FIXTURE(uri_address, compress_and_decompress_fake)
    {
        compress_test(nullptr, nullptr); // FAKE
    }

    TEST_FIXTURE(uri_address, compress_and_decompress_gzip)
    {
        if (builtin::algorithm::supported(builtin::algorithm::GZIP))
        {
            compress_test(builtin::get_compress_factory(builtin::algorithm::GZIP),
                          builtin::get_decompress_factory(builtin::algorithm::GZIP));
        }
    }

    TEST_FIXTURE(uri_address, compress_and_decompress_deflate)
    {
        if (builtin::algorithm::supported(builtin::algorithm::DEFLATE))
        {
            compress_test(builtin::get_compress_factory(builtin::algorithm::DEFLATE),
                          builtin::get_decompress_factory(builtin::algorithm::DEFLATE));
        }
    }

    TEST_FIXTURE(uri_address, compress_and_decompress_brotli)
    {
        if (builtin::algorithm::supported(builtin::algorithm::BROTLI))
        {
            compress_test(builtin::get_compress_factory(builtin::algorithm::BROTLI),
                          builtin::get_decompress_factory(builtin::algorithm::BROTLI));
        }
    }

    TEST_FIXTURE(uri_address, compress_headers)
    {
        const utility::string_t _NONE = _XPLATSTR("none");

        std::unique_ptr<compress_provider> c;
        std::unique_ptr<decompress_provider> d;

        std::shared_ptr<compress_factory> fcf =
            make_compress_factory(fake_provider::FAKE, []() -> std::unique_ptr<compress_provider> {
                return utility::details::make_unique<fake_provider>();
            });
        std::vector<std::shared_ptr<compress_factory>> fcv;
        fcv.push_back(fcf);
        std::shared_ptr<decompress_factory> fdf =
            make_decompress_factory(fake_provider::FAKE, 800, []() -> std::unique_ptr<decompress_provider> {
                return utility::details::make_unique<fake_provider>();
            });
        std::vector<std::shared_ptr<decompress_factory>> fdv;
        fdv.push_back(fdf);

        std::shared_ptr<compress_factory> ncf =
            make_compress_factory(_NONE, []() -> std::unique_ptr<compress_provider> {
                return utility::details::make_unique<fake_provider>();
            });
        std::vector<std::shared_ptr<compress_factory>> ncv;
        ncv.push_back(ncf);
        std::shared_ptr<decompress_factory> ndf =
            make_decompress_factory(_NONE, 800, []() -> std::unique_ptr<decompress_provider> {
                return utility::details::make_unique<fake_provider>();
            });
        std::vector<std::shared_ptr<decompress_factory>> ndv;
        ndv.push_back(ndf);

        // Supported algorithms
        VERIFY_ARE_EQUAL(builtin::supported(), builtin::algorithm::supported(builtin::algorithm::GZIP));
        VERIFY_ARE_EQUAL(builtin::supported(), builtin::algorithm::supported(builtin::algorithm::DEFLATE));
        if (builtin::algorithm::supported(builtin::algorithm::BROTLI))
        {
            VERIFY_IS_TRUE(builtin::supported());
        }
        VERIFY_IS_FALSE(builtin::algorithm::supported(_XPLATSTR("")));
        VERIFY_IS_FALSE(builtin::algorithm::supported(_XPLATSTR("foo")));

        // Strings that double as both Transfer-Encoding and TE
        std::vector<utility::string_t> encodings = {_XPLATSTR("gzip"),
                                                    _XPLATSTR("gZip  "),
                                                    _XPLATSTR(" GZIP"),
                                                    _XPLATSTR(" gzip "),
                                                    _XPLATSTR("  gzip  ,   chunked  "),
                                                    _XPLATSTR(" gZip , chunked "),
                                                    _XPLATSTR("GZIP,chunked")};

        // Similar, but geared to match a non-built-in algorithm
        std::vector<utility::string_t> fake = {_XPLATSTR("fake"),
                                               _XPLATSTR("faKe  "),
                                               _XPLATSTR(" FAKE"),
                                               _XPLATSTR(" fake "),
                                               _XPLATSTR("  fake  ,   chunked  "),
                                               _XPLATSTR(" faKe , chunked "),
                                               _XPLATSTR("FAKE,chunked")};

        std::vector<utility::string_t> invalid = {_XPLATSTR(","),
                                                  _XPLATSTR(",gzip"),
                                                  _XPLATSTR("gzip,"),
                                                  _XPLATSTR(",gzip, chunked"),
                                                  _XPLATSTR(" ,gzip, chunked"),
                                                  _XPLATSTR("gzip, chunked,"),
                                                  _XPLATSTR("gzip, chunked, "),
                                                  _XPLATSTR("gzip,, chunked"),
                                                  _XPLATSTR("gzip , , chunked"),
                                                  _XPLATSTR("foo")};

        std::vector<utility::string_t> invalid_tes = {
            _XPLATSTR("deflate;q=0.5, gzip;q=2"),
            _XPLATSTR("deflate;q=1.5, gzip;q=1"),
        };

        std::vector<utility::string_t> empty = {_XPLATSTR(""), _XPLATSTR(" ")};

        // Repeat for Transfer-Encoding (which also covers part of TE) and Content-Encoding (which also covers all of
        // Accept-Encoding)
        for (int transfer = 0; transfer < 2; transfer++)
        {
            compression::details::header_types ctype =
                transfer ? compression::details::header_types::te : compression::details::header_types::accept_encoding;
            compression::details::header_types dtype = transfer ? compression::details::header_types::transfer_encoding
                                                                : compression::details::header_types::content_encoding;

            // No compression - Transfer-Encoding
            d = compression::details::get_decompressor_from_header(
                _XPLATSTR(" chunked "), compression::details::header_types::transfer_encoding);
            VERIFY_IS_FALSE((bool)d);

            utility::string_t gzip(builtin::algorithm::GZIP);
            for (auto encoding = encodings.begin(); encoding != encodings.end(); encoding++)
            {
                bool has_comma = false;

                has_comma = encoding->find(_XPLATSTR(",")) != utility::string_t::npos;

                // Built-in only
                c = compression::details::get_compressor_from_header(*encoding, ctype);
                VERIFY_ARE_EQUAL((bool)c, builtin::supported());
                if (c)
                {
                    VERIFY_ARE_EQUAL(c->algorithm(), gzip);
                }

                try
                {
                    d = compression::details::get_decompressor_from_header(*encoding, dtype);
                    VERIFY_ARE_EQUAL((bool)d, builtin::supported());
                    if (d)
                    {
                        VERIFY_ARE_EQUAL(d->algorithm(), gzip);
                    }
                }
                catch (http_exception)
                {
                    VERIFY_IS_TRUE(transfer == !has_comma);
                }
            }

            for (auto encoding = fake.begin(); encoding != fake.end(); encoding++)
            {
                bool has_comma = false;

                has_comma = encoding->find(_XPLATSTR(",")) != utility::string_t::npos;

                // Supplied compressor/decompressor
                c = compression::details::get_compressor_from_header(*encoding, ctype, fcv);
                VERIFY_IS_TRUE((bool)c);
                VERIFY_IS_TRUE(c->algorithm() == fcf->algorithm());

                try
                {
                    d = compression::details::get_decompressor_from_header(*encoding, dtype, fdv);
                    VERIFY_IS_TRUE((bool)d);
                    VERIFY_IS_TRUE(d->algorithm() == fdf->algorithm());
                }
                catch (http_exception)
                {
                    VERIFY_IS_TRUE(transfer == !has_comma);
                }

                // No matching compressor
                c = compression::details::get_compressor_from_header(*encoding, ctype, ncv);
                VERIFY_IS_FALSE((bool)c);

                try
                {
                    d = compression::details::get_decompressor_from_header(*encoding, dtype, ndv);
                    VERIFY_IS_FALSE(true);
                }
                catch (http_exception)
                {
                }
            }

            // Negative tests - invalid headers, no matching algorithm, etc.
            for (auto encoding = invalid.begin(); encoding != invalid.end(); encoding++)
            {
                try
                {
                    c = compression::details::get_compressor_from_header(*encoding, ctype);
                    VERIFY_IS_TRUE(encoding->find(_XPLATSTR(",")) == utility::string_t::npos);
                    VERIFY_IS_FALSE((bool)c);
                }
                catch (http_exception)
                {
                }

                try
                {
                    d = compression::details::get_decompressor_from_header(*encoding, dtype);
                    VERIFY_IS_TRUE(!builtin::supported() && encoding->find(_XPLATSTR(",")) == utility::string_t::npos);
                    VERIFY_IS_FALSE((bool)d);
                }
                catch (http_exception)
                {
                }
            }

            // Negative tests - empty headers
            for (auto encoding = empty.begin(); encoding != empty.end(); encoding++)
            {
                c = compression::details::get_compressor_from_header(*encoding, ctype);
                VERIFY_IS_FALSE((bool)c);

                try
                {
                    d = compression::details::get_decompressor_from_header(*encoding, dtype);
                    VERIFY_IS_FALSE(true);
                }
                catch (http_exception)
                {
                }
            }

            // Negative tests - invalid rankings
            for (auto te = invalid_tes.begin(); te != invalid_tes.end(); te++)
            {
                try
                {
                    c = compression::details::get_compressor_from_header(*te, ctype);
                    VERIFY_IS_FALSE(true);
                }
                catch (http_exception)
                {
                }
            }

            utility::string_t builtin;
            std::vector<std::shared_ptr<decompress_factory>> dv;

            // Builtins
            builtin = compression::details::build_supported_header(ctype);
            if (transfer)
            {
                VERIFY_ARE_EQUAL(!builtin.empty(), builtin::supported());
            }
            else
            {
                VERIFY_IS_FALSE(builtin.empty());
            }

            // Null decompressor - effectively forces no compression algorithms
            dv.push_back(std::shared_ptr<decompress_factory>());
            builtin = compression::details::build_supported_header(ctype, dv);
            VERIFY_ARE_EQUAL(transfer != 0, builtin.empty());
            dv.pop_back();

            if (builtin::supported())
            {
                dv.push_back(builtin::get_decompress_factory(builtin::algorithm::GZIP));
                builtin = compression::details::build_supported_header(ctype, dv); // --> "gzip;q=1.0"
                VERIFY_IS_FALSE(builtin.empty());
            }
            else
            {
                builtin = _XPLATSTR("gzip;q=1.0");
            }

            // TE- and/or Accept-Encoding-specific test cases, regenerated for each pass
            std::vector<utility::string_t> tes = {
                builtin,
                _XPLATSTR("  deflate;q=0.777  ,foo;q=0,gzip;q=0.9,     bar;q=1.0, xxx;q=1  "),
                _XPLATSTR("gzip ; q=1, deflate;q=0.5"),
                _XPLATSTR("gzip;q=1.0, deflate;q=0.5"),
                _XPLATSTR("deflate;q=0.5, gzip;q=1"),
                _XPLATSTR("gzip,deflate;q=0.7"),
                _XPLATSTR("trailers,gzip,deflate;q=0.7")};

            for (int fake = 0; fake < 2; fake++)
            {
                if (fake)
                {
                    // Switch built-in vs. supplied results the second time around
                    for (auto& te : tes)
                    {
                        te.replace(te.find(builtin::algorithm::GZIP), gzip.size(), fake_provider::FAKE);
                        if (te.find(builtin::algorithm::DEFLATE) != utility::string_t::npos)
                        {
                            te.replace(te.find(builtin::algorithm::DEFLATE),
                                       utility::string_t(builtin::algorithm::DEFLATE).size(),
                                       _NONE);
                        }
                    }
                }

                for (auto te = tes.begin(); te != tes.end(); te++)
                {
                    // Built-in only
                    c = compression::details::get_compressor_from_header(*te, ctype);
                    if (c)
                    {
                        VERIFY_IS_TRUE(builtin::supported());
                        VERIFY_IS_FALSE(fake != 0);
                        VERIFY_ARE_EQUAL(c->algorithm(), gzip);
                    }
                    else
                    {
                        VERIFY_IS_TRUE(fake != 0 || !builtin::supported());
                    }

                    // Supplied compressor - both matching and non-matching
                    c = compression::details::get_compressor_from_header(*te, ctype, fcv);
                    VERIFY_ARE_EQUAL(c != 0, fake != 0);
                    if (c)
                    {
                        VERIFY_ARE_EQUAL(c->algorithm(), fake_provider::FAKE);
                    }
                }
            }
        }
    }

    template<typename _CharType>
    class my_rawptr_buffer : public concurrency::streams::rawptr_buffer<_CharType>
    {
    public:
        my_rawptr_buffer(const _CharType* data, size_t size)
            : concurrency::streams::rawptr_buffer<_CharType>(data, size)
        {
        }

        // No acquire(), to force non-acquire compression client codepaths
        virtual bool acquire(_Out_ _CharType*& ptr, _Out_ size_t& count)
        {
            (void)ptr;
            (void)count;
            return false;
        }

        virtual void release(_Out_writes_(count) _CharType* ptr, _In_ size_t count)
        {
            (void)ptr;
            (void)count;
        }

        static concurrency::streams::basic_istream<_CharType> open_istream(const _CharType* data, size_t size)
        {
            return concurrency::streams::basic_istream<_CharType>(
                concurrency::streams::streambuf<_CharType>(std::make_shared<my_rawptr_buffer<_CharType>>(data, size)));
        }
    };

    TEST_FIXTURE(uri_address, compress_client_server)
    {
        bool processed;
        bool skip_transfer_put = false;
        int transfer;

        size_t buffer_sizes[] = {0, 1, 3, 4, 4096, 65536, 100000, 157890};

        std::vector<std::shared_ptr<decompress_factory>> dfactories;
        std::vector<std::shared_ptr<compress_factory>> cfactories;

#if defined(_WIN32) && !defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        // Run a quick test to see if we're dealing with older/broken winhttp for compressed transfer encoding
        {
            test_http_server* p_server = nullptr;
            std::unique_ptr<test_http_server::scoped_server> scoped =
                std::move(utility::details::make_unique<test_http_server::scoped_server>(m_uri));
            scoped->server()->next_request().then([&skip_transfer_put](pplx::task<test_request*> op) {
                try
                {
                    op.get()->reply(static_cast<unsigned short>(status_codes::OK));
                }
                catch (std::runtime_error)
                {
                    // The test server throws if it's destructed with outstanding tasks,
                    // which will happen if winhttp responds 501 without informing us
                    VERIFY_IS_TRUE(skip_transfer_put);
                }
            });

            http_client client(m_uri);
            http_request msg(methods::PUT);
            msg.set_compressor(utility::details::make_unique<fake_provider>(0));
            msg.set_body(concurrency::streams::rawptr_stream<uint8_t>::open_istream((const uint8_t*)nullptr, 0));
            http_response rsp = client.request(msg).get();
            rsp.content_ready().wait();
            if (rsp.status_code() == status_codes::NotImplemented)
            {
                skip_transfer_put = true;
            }
            else
            {
                VERIFY_IS_TRUE(rsp.status_code() == status_codes::OK);
            }
        }
#endif // _WIN32

        auto extra_size = [](size_t bufsz) -> size_t { return (std::max)(static_cast<size_t>(128), bufsz / 1000); };

        // Test decompression both explicitly through the test server and implicitly through the listener;
        // this is the top-level loop in order to avoid thrashing the listeners more than necessary
        for (int real = 0; real < 2; real++)
        {
            web::http::experimental::listener::http_listener listener;
            std::unique_ptr<test_http_server::scoped_server> scoped;
            test_http_server* p_server = nullptr;
            std::vector<uint8_t> v;
            size_t buffer_size;

            // Start the listener, and configure callbacks if necessary
            if (real)
            {
                listener = std::move(web::http::experimental::listener::http_listener(m_uri));
                listener.open().wait();
                listener.support(methods::PUT, [&v, &dfactories, &processed](http_request request) {
                    utility::string_t encoding;
                    http_response rsp;

                    if (request.headers().match(web::http::header_names::transfer_encoding, encoding) ||
                        request.headers().match(web::http::header_names::content_encoding, encoding))
                    {
                        if (encoding.find(fake_provider::FAKE) != utility::string_t::npos)
                        {
                            // This one won't be found by the server in the default set...
                            rsp._get_impl()->set_decompress_factories(dfactories);
                        }
                    }
                    processed = true;
                    rsp.set_status_code(status_codes::OK);
                    request.reply(rsp);
                });
                listener.support(
                    methods::GET,
                    [&v, &buffer_size, &cfactories, &processed, &transfer, &extra_size](http_request request) {
                        utility::string_t encoding;
                        http_response rsp;
                        bool done;

                        if (transfer)
                        {
#if defined(_WIN32) && !defined(__cplusplus_winrt) && !defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
                            // Compression happens in the listener itself
                            done = request.headers().match(web::http::header_names::te, encoding);
                            VERIFY_IS_TRUE(done);
                            if (encoding.find(fake_provider::FAKE) != utility::string_t::npos)
                            {
                                // This one won't be found in the server's default set...
                                rsp._get_impl()->set_compressor(
                                    utility::details::make_unique<fake_provider>(buffer_size));
                            }
#endif // _WIN32
                            rsp.set_body(
                                concurrency::streams::rawptr_stream<uint8_t>::open_istream(v.data(), v.size()));
                        }
                        else
                        {
                            std::unique_ptr<compress_provider> c;
                            std::vector<uint8_t> pre;
                            size_t used;

                            done = request.headers().match(web::http::header_names::accept_encoding, encoding);
                            VERIFY_IS_TRUE(done);
                            pre.resize(v.size() + extra_size(buffer_size));
                            c = compression::details::get_compressor_from_header(
                                encoding, compression::details::header_types::accept_encoding, cfactories);
                            VERIFY_IS_TRUE((bool)c);
                            auto got = c->compress(
                                v.data(), v.size(), pre.data(), pre.size(), operation_hint::is_last, used, done);
                            VERIFY_IS_TRUE(used == v.size());
                            VERIFY_IS_TRUE(done);

                            // Add a single pre-compressed stream, since Content-Encoding requires Content-Length
                            pre.resize(got);
                            rsp.headers().add(header_names::content_encoding, c->algorithm());
                            rsp.set_body(
                                concurrency::streams::container_stream<std::vector<uint8_t>>::open_istream(pre));
                        }
                        processed = true;
                        rsp.set_status_code(status_codes::OK);
                        request.reply(rsp);
                    });
            }
            else
            {
                scoped = std::move(utility::details::make_unique<test_http_server::scoped_server>(m_uri));
                p_server = scoped->server();
            }

            // Test various buffer sizes
            for (int sz = 0; sz < sizeof(buffer_sizes) / sizeof(buffer_sizes[0]); sz++)
            {
                std::vector<utility::string_t> algorithms;
                std::map<utility::string_t, std::shared_ptr<decompress_factory>> dmap;
                std::map<utility::string_t, std::shared_ptr<compress_factory>> cmap;

                buffer_size = buffer_sizes[sz];

                dfactories.clear();
                cfactories.clear();

                // Re-build the sets of compress and decompress factories, to account for the buffer size in our "fake"
                // ones
                if (builtin::algorithm::supported(builtin::algorithm::GZIP))
                {
                    algorithms.push_back(builtin::algorithm::GZIP);
                    dmap[builtin::algorithm::GZIP] = builtin::get_decompress_factory(builtin::algorithm::GZIP);
                    cmap[builtin::algorithm::GZIP] = builtin::get_compress_factory(builtin::algorithm::GZIP);
                    dfactories.push_back(dmap[builtin::algorithm::GZIP]);
                    cfactories.push_back(cmap[builtin::algorithm::GZIP]);
                }
                if (builtin::algorithm::supported(builtin::algorithm::DEFLATE))
                {
                    algorithms.push_back(builtin::algorithm::DEFLATE);
                    dmap[builtin::algorithm::DEFLATE] = builtin::get_decompress_factory(builtin::algorithm::DEFLATE);
                    cmap[builtin::algorithm::DEFLATE] = builtin::get_compress_factory(builtin::algorithm::DEFLATE);
                    dfactories.push_back(dmap[builtin::algorithm::DEFLATE]);
                    cfactories.push_back(cmap[builtin::algorithm::DEFLATE]);
                }
                if (builtin::algorithm::supported(builtin::algorithm::BROTLI))
                {
                    algorithms.push_back(builtin::algorithm::BROTLI);
                    dmap[builtin::algorithm::BROTLI] = builtin::get_decompress_factory(builtin::algorithm::BROTLI);
                    cmap[builtin::algorithm::BROTLI] =
                        make_compress_factory(builtin::algorithm::BROTLI, []() -> std::unique_ptr<compress_provider> {
                            // Use a memory-constrained Brotli instance in some cases for code coverage
                            return builtin::make_brotli_compressor(10, 11, 0, 16, 0, 0);
                        });
                    dfactories.push_back(dmap[builtin::algorithm::BROTLI]);
                    cfactories.push_back(builtin::get_compress_factory(builtin::algorithm::BROTLI));
                }
                algorithms.push_back(fake_provider::FAKE);
                dmap[fake_provider::FAKE] = make_decompress_factory(
                    fake_provider::FAKE, 1000, [buffer_size]() -> std::unique_ptr<decompress_provider> {
                        return utility::details::make_unique<fake_provider>(buffer_size);
                    });
                cmap[fake_provider::FAKE] =
                    make_compress_factory(fake_provider::FAKE, [buffer_size]() -> std::unique_ptr<compress_provider> {
                        return utility::details::make_unique<fake_provider>(buffer_size);
                    });
                dfactories.push_back(dmap[fake_provider::FAKE]);
                cfactories.push_back(cmap[fake_provider::FAKE]);

                v.resize(buffer_size);

                // Test compressible (net shrinking) and non-compressible (net growing) buffers
                for (int compressible = 0; compressible < 2; compressible++)
                {
                    for (size_t x = 0; x < buffer_size; x++)
                    {
                        if (compressible)
                        {
                            v[x] = static_cast<uint8_t>('a' + x % 26);
                        }
                        else
                        {
                            v[x] = static_cast<uint8_t>(std::rand());
                        }
                    }

                    // Test both Transfer-Encoding and Content-Encoding
                    for (transfer = 0; transfer < 2; transfer++)
                    {
                        web::http::client::http_client_config config;
                        config.set_request_compressed_response(!transfer);
                        http_client client(m_uri, config);

                        // Test supported compression algorithms
                        for (auto& algorithm : algorithms)
                        {
                            // Test both GET and PUT
                            for (int put = 0; put < 2; put++)
                            {
                                if (transfer && put && skip_transfer_put)
                                {
                                    continue;
                                }

                                processed = false;

                                if (put)
                                {
                                    std::vector<concurrency::streams::istream> streams;
                                    std::vector<uint8_t> pre;

                                    if (transfer)
                                    {
                                        // Add a pair of non-compressed streams for Transfer-Encoding, one with and one
                                        // without acquire/release support
                                        streams.emplace_back(concurrency::streams::rawptr_stream<uint8_t>::open_istream(
                                            (const uint8_t*)v.data(), v.size()));
                                        streams.emplace_back(
                                            my_rawptr_buffer<uint8_t>::open_istream(v.data(), v.size()));
                                    }
                                    else
                                    {
                                        bool done;
                                        size_t used;
                                        pre.resize(v.size() + extra_size(buffer_size));

                                        auto c = builtin::make_compressor(algorithm);
                                        if (algorithm == fake_provider::FAKE)
                                        {
                                            VERIFY_IS_FALSE((bool)c);
                                            c = utility::details::make_unique<fake_provider>(buffer_size);
                                        }
                                        VERIFY_IS_TRUE((bool)c);
                                        auto got = c->compress(v.data(),
                                                               v.size(),
                                                               pre.data(),
                                                               pre.size(),
                                                               operation_hint::is_last,
                                                               used,
                                                               done);
                                        VERIFY_ARE_EQUAL(used, v.size());
                                        VERIFY_IS_TRUE(done);

                                        // Add a single pre-compressed stream, since Content-Encoding requires
                                        // Content-Length
                                        streams.emplace_back(concurrency::streams::rawptr_stream<uint8_t>::open_istream(
                                            pre.data(), got));
                                    }

                                    for (auto& stream : streams)
                                    {
                                        http_request msg(methods::PUT);

                                        processed = false;

                                        msg.set_body(stream);
                                        if (transfer)
                                        {
                                            if (real)
                                            {
                                                bool boo = msg.set_compressor(algorithm);
                                                VERIFY_ARE_EQUAL(boo, algorithm != fake_provider::FAKE);
                                                if (algorithm == fake_provider::FAKE)
                                                {
                                                    msg.set_compressor(
                                                        utility::details::make_unique<fake_provider>(buffer_size));
                                                }
                                            }
                                            else
                                            {
                                                msg.set_compressor(cmap[algorithm]->make_compressor());
                                            }
                                        }
                                        else
                                        {
                                            msg.headers().add(header_names::content_encoding, algorithm);
                                        }

                                        if (!real)
                                        {
                                            // We implement the decompression path in the server, to prove that valid,
                                            // compressed data is sent
                                            p_server->next_request().then([&](test_request* p_request) {
                                                std::unique_ptr<decompress_provider> d;
                                                std::vector<uint8_t> vv;
                                                utility::string_t header;
                                                size_t used;
                                                size_t got;
                                                bool done;

                                                http_asserts::assert_test_request_equals(
                                                    p_request, methods::PUT, U("/"));

                                                if (transfer)
                                                {
                                                    VERIFY_IS_FALSE(p_request->match_header(
                                                        header_names::content_encoding, header));
                                                    done = p_request->match_header(header_names::transfer_encoding,
                                                                                   header);
                                                    VERIFY_IS_TRUE(done);
                                                    d = compression::details::get_decompressor_from_header(
                                                        header,
                                                        compression::details::header_types::transfer_encoding,
                                                        dfactories);
                                                }
                                                else
                                                {
                                                    done = p_request->match_header(header_names::transfer_encoding,
                                                                                   header);
                                                    if (done)
                                                    {
                                                        VERIFY_IS_TRUE(
                                                            utility::details::str_iequal(_XPLATSTR("chunked"), header));
                                                    }
                                                    done =
                                                        p_request->match_header(header_names::content_encoding, header);
                                                    VERIFY_IS_TRUE(done);
                                                    d = compression::details::get_decompressor_from_header(
                                                        header,
                                                        compression::details::header_types::content_encoding,
                                                        dfactories);
                                                }
#if defined(_WIN32) && !defined(__cplusplus_winrt) && !defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
                                                VERIFY_IS_TRUE((bool)d);
#else  // _WIN32
                                                VERIFY_ARE_NOT_EQUAL((bool)d, !!transfer);
#endif // _WIN32

                                                vv.resize(buffer_size + extra_size(buffer_size));
                                                if (d)
                                                {
                                                    got = d->decompress(p_request->m_body.data(),
                                                                        p_request->m_body.size(),
                                                                        vv.data(),
                                                                        vv.size(),
                                                                        operation_hint::is_last,
                                                                        used,
                                                                        done);
                                                    VERIFY_ARE_EQUAL(used, p_request->m_body.size());
                                                    VERIFY_IS_TRUE(done);
                                                }
                                                else
                                                {
                                                    std::copy(v.begin(), v.end(), vv.begin());
                                                    got = v.size();
                                                }
                                                VERIFY_ARE_EQUAL(buffer_size, got);
                                                vv.resize(buffer_size);
                                                VERIFY_ARE_EQUAL(v, vv);
                                                processed = true;

                                                p_request->reply(static_cast<unsigned short>(status_codes::OK));
                                            });
                                        }

                                        // Send the request
                                        http_response rsp = client.request(msg).get();
                                        VERIFY_ARE_EQUAL(rsp.status_code(), status_codes::OK);
                                        rsp.content_ready().wait();
                                        stream.close().wait();
                                        VERIFY_IS_TRUE(processed);
                                    }
                                }
                                else
                                {
                                    std::vector<uint8_t> vv;
                                    concurrency::streams::ostream stream =
                                        concurrency::streams::rawptr_stream<uint8_t>::open_ostream(vv.data(),
                                                                                                   buffer_size);
                                    http_request msg(methods::GET);

                                    std::vector<std::shared_ptr<decompress_factory>> df = {dmap[algorithm]};
                                    msg.set_decompress_factories(df);

                                    vv.resize(buffer_size + extra_size(buffer_size)); // extra to ensure no overflow

                                    concurrency::streams::rawptr_buffer<uint8_t> buf(
                                        vv.data(), vv.size(), std::ios::out);

                                    if (!real)
                                    {
                                        p_server->next_request().then([&](test_request* p_request) {
                                            std::map<utility::string_t, utility::string_t> headers;
                                            std::unique_ptr<compress_provider> c;
                                            utility::string_t header;
                                            std::vector<uint8_t> cmp;
                                            size_t used;
                                            size_t extra = 0;
                                            size_t skip = 0;
                                            size_t got;
                                            bool done;

                                            std::string ext = ";x=y";
                                            std::string trailer = "a=b\r\nx=y\r\n";

                                            http_asserts::assert_test_request_equals(p_request, methods::GET, U("/"));

                                            if (transfer)
                                            {
                                                // On Windows, someone along the way adds "Accept-Encoding: peerdist",
                                                // so we can't unconditionally assert that Accept-Encoding is not
                                                // present
                                                done = p_request->match_header(header_names::accept_encoding, header);
                                                VERIFY_IS_TRUE(!done ||
                                                               header.find(algorithm) == utility::string_t::npos);
                                                done = p_request->match_header(header_names::te, header);
                                                if (done)
                                                {
                                                    c = compression::details::get_compressor_from_header(
                                                        header, compression::details::header_types::te, cfactories);
                                                }

                                                // Account for space for the chunk header and delimiters, plus a chunk
                                                // extension and a chunked trailer part
                                                extra = 2 * web::http::details::chunked_encoding::
                                                                additional_encoding_space +
                                                        ext.size() + trailer.size();
                                                skip = web::http::details::chunked_encoding::data_offset + ext.size();
                                            }
                                            else
                                            {
                                                VERIFY_IS_FALSE(p_request->match_header(header_names::te, header));
                                                done = p_request->match_header(header_names::accept_encoding, header);
                                                VERIFY_IS_TRUE(done);
                                                c = compression::details::get_compressor_from_header(
                                                    header,
                                                    compression::details::header_types::accept_encoding,
                                                    cfactories);
                                            }
#if !defined __cplusplus_winrt
                                            VERIFY_IS_TRUE((bool)c);
#else  // __cplusplus_winrt
                                            VERIFY_ARE_NOT_EQUAL((bool)c, !!transfer);
#endif // __cplusplus_winrt
                                            cmp.resize(extra + buffer_size + extra_size(buffer_size));
                                            if (c)
                                            {
                                                got = c->compress(v.data(),
                                                                  v.size(),
                                                                  cmp.data() + skip,
                                                                  cmp.size() - extra,
                                                                  operation_hint::is_last,
                                                                  used,
                                                                  done);
                                                VERIFY_ARE_EQUAL(used, v.size());
                                                VERIFY_IS_TRUE(done);
                                            }
                                            else
                                            {
                                                memcpy(cmp.data() + skip, v.data(), v.size());
                                                got = v.size();
                                            }
                                            if (transfer)
                                            {
                                                // Add delimiters for the first (and only) data chunk, plus the final
                                                // 0-length chunk, and hack in a dummy chunk extension and a dummy
                                                // trailer part.  Note that we put *two* "0\r\n" in here in the 0-length
                                                // case... and none of the parsers complain.
                                                size_t total =
                                                    got +
                                                    web::http::details::chunked_encoding::additional_encoding_space +
                                                    ext.size();
                                                _ASSERTE(ext.size() >= 2);
                                                if (got > ext.size() - 1)
                                                {
                                                    cmp[total - 2] =
                                                        cmp[got + web::http::details::chunked_encoding::data_offset];
                                                }
                                                if (got > ext.size() - 2)
                                                {
                                                    cmp[total - 1] =
                                                        cmp[got + web::http::details::chunked_encoding::data_offset +
                                                            1];
                                                }
                                                size_t offset =
                                                    web::http::details::chunked_encoding::add_chunked_delimiters(
                                                        cmp.data(), total, got);
                                                size_t offset2 =
                                                    web::http::details::chunked_encoding::add_chunked_delimiters(
                                                        cmp.data() + total - 7,
                                                        web::http::details::chunked_encoding::additional_encoding_space,
                                                        0);
                                                _ASSERTE(
                                                    offset2 == 7 &&
                                                    web::http::details::chunked_encoding::additional_encoding_space -
                                                            7 ==
                                                        5);
                                                memcpy(cmp.data() + web::http::details::chunked_encoding::data_offset -
                                                           2,
                                                       ext.data(),
                                                       ext.size());
                                                cmp[web::http::details::chunked_encoding::data_offset + ext.size() -
                                                    2] = '\r';
                                                cmp[web::http::details::chunked_encoding::data_offset + ext.size() -
                                                    1] = '\n';
                                                if (got > ext.size() - 1)
                                                {
                                                    cmp[got + web::http::details::chunked_encoding::data_offset] =
                                                        cmp[total - 2];
                                                }
                                                if (got > ext.size() - 2)
                                                {
                                                    cmp[got + web::http::details::chunked_encoding::data_offset + 1] =
                                                        cmp[total - 1];
                                                }
                                                cmp[total - 2] = '\r';
                                                cmp[total - 1] = '\n';
                                                memcpy(cmp.data() + total + 3, trailer.data(), trailer.size());
                                                cmp[total + trailer.size() + 3] = '\r';
                                                cmp[total + trailer.size() + 4] = '\n';
                                                cmp.erase(cmp.begin(), cmp.begin() + offset);
                                                cmp.resize(
                                                    ext.size() + got + trailer.size() +
                                                    web::http::details::chunked_encoding::additional_encoding_space -
                                                    offset + 5);
                                                if (c)
                                                {
                                                    headers[header_names::transfer_encoding] =
                                                        c->algorithm() + _XPLATSTR(", chunked");
                                                }
                                                else
                                                {
                                                    headers[header_names::transfer_encoding] = _XPLATSTR("chunked");
                                                }
                                            }
                                            else
                                            {
                                                cmp.resize(got);
                                                headers[header_names::content_encoding] = c->algorithm();
                                            }
                                            processed = true;

                                            if (cmp.size())
                                            {
                                                p_request->reply(static_cast<unsigned short>(status_codes::OK),
                                                                 utility::string_t(),
                                                                 headers,
                                                                 cmp);
                                            }
                                            else
                                            {
                                                p_request->reply(static_cast<unsigned short>(status_codes::OK),
                                                                 utility::string_t(),
                                                                 headers);
                                            }
                                        });
                                    }

                                    // Common send and response processing code
                                    http_response rsp = client.request(msg).get();
                                    VERIFY_ARE_EQUAL(rsp.status_code(), status_codes::OK);
                                    VERIFY_NO_THROWS(rsp.content_ready().wait());

                                    if (transfer)
                                    {
                                        VERIFY_IS_TRUE(rsp.headers().has(header_names::transfer_encoding));
                                        VERIFY_IS_FALSE(rsp.headers().has(header_names::content_encoding));
                                    }
                                    else
                                    {
                                        utility::string_t header;

                                        VERIFY_IS_TRUE(rsp.headers().has(header_names::content_encoding));
                                        bool boo = rsp.headers().match(header_names::transfer_encoding, header);
                                        if (boo)
                                        {
                                            VERIFY_IS_TRUE(utility::details::str_iequal(_XPLATSTR("chunked"), header));
                                        }
                                    }

                                    size_t offset = 0;
                                    VERIFY_NO_THROWS(offset = rsp.body().read_to_end(buf).get());
                                    VERIFY_ARE_EQUAL(offset, buffer_size);
                                    VERIFY_ARE_EQUAL(offset, static_cast<size_t>(buf.getpos(std::ios::out)));
                                    vv.resize(buffer_size);
                                    VERIFY_ARE_EQUAL(v, vv);
                                    buf.close(std::ios_base::out).wait();
                                    stream.close().wait();
                                }
                                VERIFY_IS_TRUE(processed);
                            }
                        }
                    }
                }
            }
            if (real)
            {
                listener.close().wait();
            }
        }
    }
} // SUITE(request_helper_tests)
} // namespace client
} // namespace http
} // namespace functional
} // namespace tests
