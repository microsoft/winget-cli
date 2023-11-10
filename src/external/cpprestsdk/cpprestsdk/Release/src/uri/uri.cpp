/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Protocol independent support for URIs.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include <sstream>

using namespace utility::conversions;

namespace web
{
namespace details
{
namespace
{
/// <summary>
/// Unreserved characters are those that are allowed in a URI but do not have a reserved purpose. They include:
/// - A-Z
/// - a-z
/// - 0-9
/// - '-' (hyphen)
/// - '.' (period)
/// - '_' (underscore)
/// - '~' (tilde)
/// </summary>
inline bool is_unreserved(int c)
{
    return ::utility::details::is_alnum((char)c) || c == '-' || c == '.' || c == '_' || c == '~';
}

/// <summary>
/// General delimiters serve as the delimiters between different uri components.
/// General delimiters include:
/// - All of these :/?#[]@
/// </summary>
inline bool is_gen_delim(int c)
{
    return c == ':' || c == '/' || c == '?' || c == '#' || c == '[' || c == ']' || c == '@';
}

/// <summary>
/// Subdelimiters are those characters that may have a defined meaning within component
/// of a uri for a particular scheme. They do not serve as delimiters in any case between
/// uri segments. sub_delimiters include:
/// - All of these !$&'()*+,;=
/// </summary>
inline bool is_sub_delim(int c)
{
    switch (c)
    {
        case '!':
        case '$':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case ';':
        case '=': return true;
        default: return false;
    }
}

/// <summary>
/// Reserved characters includes the general delimiters and sub delimiters. Some characters
/// are neither reserved nor unreserved, and must be percent-encoded.
/// </summary>
inline bool is_reserved(int c) { return is_gen_delim(c) || is_sub_delim(c); }

/// <summary>
/// Legal characters in the scheme portion include:
/// - Any alphanumeric character
/// - '+' (plus)
/// - '-' (hyphen)
/// - '.' (period)
///
/// Note that the scheme must BEGIN with an alpha character.
/// </summary>
inline bool is_scheme_character(int c)
{
    return ::utility::details::is_alnum((char)c) || c == '+' || c == '-' || c == '.';
}

/// <summary>
/// Legal characters in the user information portion include:
/// - Any unreserved character
/// - The percent character ('%'), and thus any percent-endcoded octet
/// - The sub-delimiters
/// - ':' (colon)
/// </summary>
inline bool is_user_info_character(int c) { return is_unreserved(c) || is_sub_delim(c) || c == '%' || c == ':'; }

/// <summary>
/// Legal characters in the authority portion include:
/// - Any unreserved character
/// - The percent character ('%'), and thus any percent-endcoded octet
/// - The sub-delimiters
/// - ':' (colon)
/// - IPv6 requires '[]' allowed for it to be valid URI and passed to underlying platform for IPv6 support
/// </summary>
inline bool is_authority_character(int c)
{
    return is_unreserved(c) || is_sub_delim(c) || c == '%' || c == '@' || c == ':' || c == '[' || c == ']';
}

/// <summary>
/// Legal characters in the path portion include:
/// - Any unreserved character
/// - The percent character ('%'), and thus any percent-endcoded octet
/// - The sub-delimiters
/// - ':' (colon)
/// - '@' (at sign)
/// </summary>
inline bool is_path_character(int c)
{
    return is_unreserved(c) || is_sub_delim(c) || c == '%' || c == '/' || c == ':' || c == '@';
}

/// <summary>
/// Legal characters in the query portion include:
/// - Any path character
/// - '?' (question mark)
/// </summary>
inline bool is_query_character(int c) { return is_path_character(c) || c == '?'; }

/// <summary>
/// Legal characters in the fragment portion include:
/// - Any path character
/// - '?' (question mark)
/// </summary>
inline bool is_fragment_character(int c)
{
    // this is intentional, they have the same set of legal characters
    return is_query_character(c);
}

struct inner_parse_out
{
    const utility::char_t* scheme_begin = nullptr;
    const utility::char_t* scheme_end = nullptr;
    const utility::char_t* uinfo_begin = nullptr;
    const utility::char_t* uinfo_end = nullptr;
    const utility::char_t* host_begin = nullptr;
    const utility::char_t* host_end = nullptr;
    int port = 0;
    const utility::char_t* path_begin = nullptr;
    const utility::char_t* path_end = nullptr;
    const utility::char_t* query_begin = nullptr;
    const utility::char_t* query_end = nullptr;
    const utility::char_t* fragment_begin = nullptr;
    const utility::char_t* fragment_end = nullptr;

    /// <summary>
    /// Parses the uri, setting the given pointers to locations inside the given buffer.
    /// 'encoded' is expected to point to an encoded zero-terminated string containing a uri
    /// </summary>
    bool parse_from(const utility::char_t* encoded)
    {
        const utility::char_t* p = encoded;

        // IMPORTANT -- A uri may either be an absolute uri, or an relative-reference
        // Absolute: 'http://host.com'
        // Relative-Reference: '//:host.com', '/path1/path2?query', './path1:path2'
        // A Relative-Reference can be disambiguated by parsing for a ':' before the first slash

        bool is_relative_reference = true;
        const utility::char_t* p2 = p;
        for (; *p2 != _XPLATSTR('/') && *p2 != _XPLATSTR('\0'); p2++)
        {
            if (*p2 == _XPLATSTR(':'))
            {
                // found a colon, the first portion is a scheme
                is_relative_reference = false;
                break;
            }
        }

        if (!is_relative_reference)
        {
            // the first character of a scheme must be a letter
            if (!isalpha(*p))
            {
                return false;
            }

            // start parsing the scheme, it's always delimited by a colon (must be present)
            scheme_begin = p++;
            for (; *p != ':'; p++)
            {
                if (!is_scheme_character(*p))
                {
                    return false;
                }
            }
            scheme_end = p;

            // skip over the colon
            p++;
        }

        // if we see two slashes next, then we're going to parse the authority portion
        // later on we'll break up the authority into the port and host
        const utility::char_t* authority_begin = nullptr;
        const utility::char_t* authority_end = nullptr;
        if (*p == _XPLATSTR('/') && p[1] == _XPLATSTR('/'))
        {
            // skip over the slashes
            p += 2;
            authority_begin = p;

            // the authority is delimited by a slash (resource), question-mark (query) or octothorpe (fragment)
            // or by EOS. The authority could be empty ('file:///C:\file_name.txt')
            for (; *p != _XPLATSTR('/') && *p != _XPLATSTR('?') && *p != _XPLATSTR('#') && *p != _XPLATSTR('\0'); p++)
            {
                // We're NOT currently supporting IPvFuture or username/password in authority
                // IPv6 as the host (i.e. http://[:::::::]) is allowed as valid URI and passed to subsystem for support.
                if (!is_authority_character(*p))
                {
                    return false;
                }
            }
            authority_end = p;

            // now lets see if we have a port specified -- by working back from the end
            if (authority_begin != authority_end)
            {
                // the port is made up of all digits
                const utility::char_t* port_begin = authority_end - 1;
                for (; isdigit(*port_begin) && port_begin != authority_begin; port_begin--)
                {
                }

                if (*port_begin == _XPLATSTR(':'))
                {
                    // has a port
                    host_begin = authority_begin;
                    host_end = port_begin;

                    // skip the colon
                    port_begin++;

                    port =
                        utility::conversions::details::scan_string<int>(utility::string_t(port_begin, authority_end));
                }
                else
                {
                    // no port
                    host_begin = authority_begin;
                    host_end = authority_end;
                }

                // look for a user_info component
                const utility::char_t* u_end = host_begin;
                for (; is_user_info_character(*u_end) && u_end != host_end; u_end++)
                {
                }

                if (*u_end == _XPLATSTR('@'))
                {
                    host_begin = u_end + 1;
                    uinfo_begin = authority_begin;
                    uinfo_end = u_end;
                }
            }
        }

        // if we see a path character or a slash, then the
        // if we see a slash, or any other legal path character, parse the path next
        if (*p == _XPLATSTR('/') || is_path_character(*p))
        {
            path_begin = p;

            // the path is delimited by a question-mark (query) or octothorpe (fragment) or by EOS
            for (; *p != _XPLATSTR('?') && *p != _XPLATSTR('#') && *p != _XPLATSTR('\0'); p++)
            {
                if (!is_path_character(*p))
                {
                    return false;
                }
            }
            path_end = p;
        }

        // if we see a ?, then the query is next
        if (*p == _XPLATSTR('?'))
        {
            // skip over the question mark
            p++;
            query_begin = p;

            // the query is delimited by a '#' (fragment) or EOS
            for (; *p != _XPLATSTR('#') && *p != _XPLATSTR('\0'); p++)
            {
                if (!is_query_character(*p))
                {
                    return false;
                }
            }
            query_end = p;
        }

        // if we see a #, then the fragment is next
        if (*p == _XPLATSTR('#'))
        {
            // skip over the hash mark
            p++;
            fragment_begin = p;

            // the fragment is delimited by EOS
            for (; *p != _XPLATSTR('\0'); p++)
            {
                if (!is_fragment_character(*p))
                {
                    return false;
                }
            }
            fragment_end = p;
        }

        return true;
    }

    void write_to(uri_components& components)
    {
        if (scheme_begin)
        {
            components.m_scheme.assign(scheme_begin, scheme_end);
            utility::details::inplace_tolower(components.m_scheme);
        }
        else
        {
            components.m_scheme.clear();
        }

        if (uinfo_begin)
        {
            components.m_user_info.assign(uinfo_begin, uinfo_end);
        }

        if (host_begin)
        {
            components.m_host.assign(host_begin, host_end);
            utility::details::inplace_tolower(components.m_host);
        }
        else
        {
            components.m_host.clear();
        }

        components.m_port = port;

        if (path_begin)
        {
            components.m_path.assign(path_begin, path_end);
        }
        else
        {
            // default path to begin with a slash for easy comparison
            components.m_path = _XPLATSTR("/");
        }

        if (query_begin)
        {
            components.m_query.assign(query_begin, query_end);
        }
        else
        {
            components.m_query.clear();
        }

        if (fragment_begin)
        {
            components.m_fragment.assign(fragment_begin, fragment_end);
        }
        else
        {
            components.m_fragment.clear();
        }
    }
};

// Encodes all characters not in given set determined by given function.
template<class F>
utility::string_t encode_impl(const utf8string& raw, F should_encode)
{
    const utility::char_t* const hex = _XPLATSTR("0123456789ABCDEF");
    utility::string_t encoded;
    for (auto iter = raw.begin(); iter != raw.end(); ++iter)
    {
        // for utf8 encoded string, char ASCII can be greater than 127.
        int ch = static_cast<unsigned char>(*iter);
        // ch should be same under both utf8 and utf16.
        if (should_encode(ch))
        {
            encoded.push_back(_XPLATSTR('%'));
            encoded.push_back(hex[(ch >> 4) & 0xF]);
            encoded.push_back(hex[ch & 0xF]);
        }
        else
        {
            // ASCII don't need to be encoded, which should be same on both utf8 and utf16.
            encoded.push_back((utility::char_t)ch);
        }
    }
    return encoded;
}

// 5.2.3. Merge Paths https://tools.ietf.org/html/rfc3986#section-5.2.3
utility::string_t mergePaths(const utility::string_t& base, const utility::string_t& relative)
{
    const auto lastSlash = base.rfind(_XPLATSTR('/'));
    if (lastSlash == utility::string_t::npos)
    {
        return base + _XPLATSTR('/') + relative;
    }
    else if (lastSlash == base.size() - 1)
    {
        return base + relative;
    }
    // path contains and does not end with '/', we remove segment after last '/'
    return base.substr(0, lastSlash + 1) + relative;
}

// 5.2.4. Remove Dot Segments https://tools.ietf.org/html/rfc3986#section-5.2.4
void removeDotSegments(uri_builder& builder)
{
    const ::utility::string_t dotSegment = _XPLATSTR(".");
    const ::utility::string_t dotDotSegment = _XPLATSTR("..");

    if (builder.path().find(_XPLATSTR('.')) == utility::string_t::npos) return;

    const auto segments = uri::split_path(builder.path());
    std::vector<std::reference_wrapper<const utility::string_t>> result;
    for (auto& segment : segments)
    {
        if (segment == dotSegment)
            continue;
        else if (segment != dotDotSegment)
            result.push_back(segment);
        else if (!result.empty())
            result.pop_back();
    }
    if (result.empty())
    {
        builder.set_path(utility::string_t());
        return;
    }
    utility::string_t path = result.front().get();
    for (size_t i = 1; i != result.size(); ++i)
    {
        path += _XPLATSTR('/');
        path += result[i].get();
    }
    if (segments.back() == dotDotSegment || segments.back() == dotSegment || builder.path().back() == _XPLATSTR('/'))
    {
        path += _XPLATSTR('/');
    }

    builder.set_path(std::move(path));
}
} // namespace

utility::string_t uri_components::join()
{
    // canonicalize components first

    // convert scheme to lowercase
    utility::details::inplace_tolower(m_scheme);
    // convert host to lowercase
    utility::details::inplace_tolower(m_host);

    // canonicalize the path to have a leading slash if it's a full uri
    if (!m_host.empty() && m_path.empty())
    {
        m_path = _XPLATSTR("/");
    }
    else if (!m_host.empty() && m_path[0] != _XPLATSTR('/'))
    {
        m_path.insert(m_path.begin(), 1, _XPLATSTR('/'));
    }

    utility::string_t ret;

    if (!m_scheme.empty())
    {
        ret.append(m_scheme);
        ret.push_back(_XPLATSTR(':'));
    }

    if (!m_host.empty())
    {
        ret.append(_XPLATSTR("//"));

        if (!m_user_info.empty())
        {
            ret.append(m_user_info).append({_XPLATSTR('@')});
        }

        ret.append(m_host);

        if (m_port > 0)
        {
            ret.append({_XPLATSTR(':')}).append(utility::conversions::details::to_string_t(m_port));
        }
    }

    if (!m_path.empty())
    {
        // only add the leading slash when the host is present
        if (!m_host.empty() && m_path.front() != _XPLATSTR('/'))
        {
            ret.push_back(_XPLATSTR('/'));
        }

        ret.append(m_path);
    }

    if (!m_query.empty())
    {
        ret.push_back(_XPLATSTR('?'));
        ret.append(m_query);
    }

    if (!m_fragment.empty())
    {
        ret.push_back(_XPLATSTR('#'));
        ret.append(m_fragment);
    }

    return ret;
}
} // namespace details

uri::uri(const details::uri_components& components) : m_components(components)
{
    m_uri = m_components.join();

    if (!uri::validate(m_uri.c_str()))
    {
        throw uri_exception("provided uri is invalid: " + utility::conversions::to_utf8string(m_uri));
    }
}

uri::uri(const utility::string_t& uri_string) : uri(uri_string.c_str()) {}

uri::uri(const utility::char_t* uri_string)
{
    details::inner_parse_out out;

    if (!out.parse_from(uri_string))
    {
        throw uri_exception("provided uri is invalid: " + utility::conversions::to_utf8string(uri_string));
    }

    out.write_to(m_components);
    m_uri = m_components.join();
}

utility::string_t uri::encode_query_impl(const utf8string& raw)
{
    return details::encode_impl(raw, [](int ch) -> bool {
        switch (ch)
        {
                // Encode '&', ';', and '=' since they are used
                // as delimiters in query component.
            case '&':
            case ';':
            case '=':
            case '%':
            case '+': return true;
            default: return !details::is_query_character(ch);
        }
    });
}

/// </summary>
/// Encodes a string by converting all characters except for RFC 3986 unreserved characters to their
/// hexadecimal representation.
/// </summary>
utility::string_t uri::encode_data_string(const utility::string_t& data)
{
    auto&& raw = utility::conversions::to_utf8string(data);

    return details::encode_impl(raw, [](int ch) -> bool { return !details::is_unreserved(ch); });
}

utility::string_t uri::encode_uri(const utility::string_t& raw, uri::components::component component)
{
    auto&& raw_utf8 = utility::conversions::to_utf8string(raw);

    // Note: we also encode the '+' character because some non-standard implementations
    // encode the space character as a '+' instead of %20. To better interoperate we encode
    // '+' to avoid any confusion and be mistaken as a space.
    switch (component)
    {
        case components::user_info:
            return details::encode_impl(raw_utf8, [](int ch) -> bool {
                return !details::is_user_info_character(ch) || ch == '%' || ch == '+';
            });
        case components::host:
            return details::encode_impl(raw_utf8, [](int ch) -> bool {
                // No encoding of ASCII characters in host name (RFC 3986 3.2.2)
                return ch > 127;
            });
        case components::path:
            return details::encode_impl(
                raw_utf8, [](int ch) -> bool { return !details::is_path_character(ch) || ch == '%' || ch == '+'; });
        case components::query:
            return details::encode_impl(
                raw_utf8, [](int ch) -> bool { return !details::is_query_character(ch) || ch == '%' || ch == '+'; });
        case components::fragment:
            return details::encode_impl(
                raw_utf8, [](int ch) -> bool { return !details::is_fragment_character(ch) || ch == '%' || ch == '+'; });
        case components::full_uri:
        default:
            return details::encode_impl(
                raw_utf8, [](int ch) -> bool { return !details::is_unreserved(ch) && !details::is_reserved(ch); });
    };
}

/// <summary>
/// Helper function to convert a hex character digit to a decimal character value.
/// Throws an exception if not a valid hex digit.
/// </summary>
static int hex_char_digit_to_decimal_char(int hex)
{
    int decimal;
    if (hex >= '0' && hex <= '9')
    {
        decimal = hex - '0';
    }
    else if (hex >= 'A' && hex <= 'F')
    {
        decimal = 10 + (hex - 'A');
    }
    else if (hex >= 'a' && hex <= 'f')
    {
        decimal = 10 + (hex - 'a');
    }
    else
    {
        throw uri_exception("Invalid hexadecimal digit");
    }
    return decimal;
}

template<class String>
static std::string decode_template(const String& encoded)
{
    std::string raw;
    for (auto iter = encoded.begin(); iter != encoded.end(); ++iter)
    {
        if (*iter == '%')
        {
            if (++iter == encoded.end())
            {
                throw uri_exception("Invalid URI string, two hexadecimal digits must follow '%'");
            }
            int decimal_value = hex_char_digit_to_decimal_char(static_cast<int>(*iter)) << 4;
            if (++iter == encoded.end())
            {
                throw uri_exception("Invalid URI string, two hexadecimal digits must follow '%'");
            }
            decimal_value += hex_char_digit_to_decimal_char(static_cast<int>(*iter));

            raw.push_back(static_cast<char>(decimal_value));
        }
        else if (*iter > 127 || *iter < 0)
        {
            throw uri_exception("Invalid encoded URI string, must be entirely ascii");
        }
        else
        {
            // encoded string has to be ASCII.
            raw.push_back(static_cast<char>(*iter));
        }
    }
    return raw;
}

utility::string_t uri::decode(const utility::string_t& encoded) { return to_string_t(decode_template(encoded)); }

std::vector<utility::string_t> uri::split_path(const utility::string_t& path)
{
    std::vector<utility::string_t> results;
    utility::istringstream_t iss(path);
    iss.imbue(std::locale::classic());
    utility::string_t s;

    while (std::getline(iss, s, _XPLATSTR('/')))
    {
        if (!s.empty())
        {
            results.push_back(s);
        }
    }

    return results;
}

std::map<utility::string_t, utility::string_t> uri::split_query(const utility::string_t& query)
{
    std::map<utility::string_t, utility::string_t> results;

    // Split into key value pairs separated by '&'.
    size_t prev_amp_index = 0;
    while (prev_amp_index != utility::string_t::npos)
    {
        size_t amp_index = query.find_first_of(_XPLATSTR('&'), prev_amp_index);
        if (amp_index == utility::string_t::npos) amp_index = query.find_first_of(_XPLATSTR(';'), prev_amp_index);

        utility::string_t key_value_pair = query.substr(
            prev_amp_index,
            amp_index == utility::string_t::npos ? query.size() - prev_amp_index : amp_index - prev_amp_index);
        prev_amp_index = amp_index == utility::string_t::npos ? utility::string_t::npos : amp_index + 1;

        size_t equals_index = key_value_pair.find_first_of(_XPLATSTR('='));
        if (equals_index == utility::string_t::npos)
        {
            continue;
        }
        else if (equals_index == 0)
        {
            utility::string_t value(key_value_pair.begin() + equals_index + 1, key_value_pair.end());
            results[utility::string_t {}] = value;
        }
        else
        {
            utility::string_t key(key_value_pair.begin(), key_value_pair.begin() + equals_index);
            utility::string_t value(key_value_pair.begin() + equals_index + 1, key_value_pair.end());
            results[key] = value;
        }
    }

    return results;
}

bool uri::validate(const utility::string_t& uri_string)
{
    details::inner_parse_out out;
    return out.parse_from(uri_string.c_str());
}

uri uri::authority() const
{
    return uri_builder()
        .set_scheme(this->scheme())
        .set_host(this->host())
        .set_port(this->port())
        .set_user_info(this->user_info())
        .to_uri();
}

uri uri::resource() const
{
    return uri_builder().set_path(this->path()).set_query(this->query()).set_fragment(this->fragment()).to_uri();
}

bool uri::operator==(const uri& other) const
{
    // Each individual URI component must be decoded before performing comparison.
    // TFS # 375865

    if (this->is_empty() && other.is_empty())
    {
        return true;
    }
    else if (this->is_empty() || other.is_empty())
    {
        return false;
    }
    else if (this->scheme() != other.scheme())
    {
        // scheme is canonicalized to lowercase
        return false;
    }
    else if (uri::decode(this->user_info()) != uri::decode(other.user_info()))
    {
        return false;
    }
    else if (uri::decode(this->host()) != uri::decode(other.host()))
    {
        // host is canonicalized to lowercase
        return false;
    }
    else if (this->port() != other.port())
    {
        return false;
    }
    else if (uri::decode(this->path()) != uri::decode(other.path()))
    {
        return false;
    }
    else if (uri::decode(this->query()) != uri::decode(other.query()))
    {
        return false;
    }
    else if (uri::decode(this->fragment()) != uri::decode(other.fragment()))
    {
        return false;
    }

    return true;
}

// resolving URI according to RFC3986, Section 5 https://tools.ietf.org/html/rfc3986#section-5
utility::string_t uri::resolve_uri(const utility::string_t& relativeUri) const
{
    if (relativeUri.empty())
    {
        return to_string();
    }

    if (relativeUri[0] == _XPLATSTR('/')) // starts with '/'
    {
        if (relativeUri.size() >= 2 && relativeUri[1] == _XPLATSTR('/')) // starts with '//'
        {
            return this->scheme() + _XPLATSTR(':') + relativeUri;
        }

        // otherwise relative to root
        auto builder = uri_builder(this->authority());
        builder.append(relativeUri);
        details::removeDotSegments(builder);
        return builder.to_string();
    }

    const auto url = uri(relativeUri);
    if (!url.scheme().empty()) return relativeUri;

    if (!url.authority().is_empty())
    {
        return uri_builder(url).set_scheme(this->scheme()).to_string();
    }

    // relative url
    auto builder = uri_builder(*this);
    if (url.path() == _XPLATSTR("/") || url.path().empty()) // web::uri considers empty path as '/'
    {
        if (!url.query().empty())
        {
            builder.set_query(url.query());
        }
    }
    else if (!this->path().empty())
    {
        builder.set_path(details::mergePaths(this->path(), url.path()));
        details::removeDotSegments(builder);
        builder.set_query(url.query());
    }

    return builder.set_fragment(url.fragment()).to_string();
}

} // namespace web
