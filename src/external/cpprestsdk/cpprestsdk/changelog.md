cpprestsdk (2.10.18)
* PR#1571 Add ability to parse and emit the NT Epoch 1601-01-01T00:00:00Z
* PR#1571 Update vcpkg submodule
* Update CI configuration
-- cpprestsdk team MON, 1 Feb 2021 20:02:00 -0700

cpprestsdk (2.10.17)
* PR#1550 Fix year calculation for the last day of a leap year
* PR#1523 Fix wrong linking of Apple Frameworks on MacOS
* PR#1520 Define __STDC_FORMAT_MACROS when it hasn't been defined to avoid duplicate define error. 
* PR#1415 Delete apparently broken .vcxprojs and .pfxes.
* Removed defunct email contact information from the readme
-- cpprestsdk team WED, 30 Dec 2020 20:08:00 -0700

cpprestsdk (2.10.16)
* PR#1383 CMake fixes + CMake search for OpenSSL (macOS)
* PR#1392 Update submodule websocketpp to 0.8.2
* PR#1393 Do not report errors (such as EBADF and EINVAL) from setsockopt here, since this is a performance optimization only, and hard errors will be picked up by the following operation
* PR#1379 Fix compilation with GCC 4.8/4.9, which was broken by commit 53fab3a.
* PR#1328 Add support for HTTP redirection in ASIO and WinHTTP-based http_clients
* PR#1332 Fix more http test build fails in certain configurations
* PR#1370 Remove redundant std::move noted by gcc 9.2 (-Wredundant-move)
* PR#1372 Static analyzer (PVS Studio) fixes
* PR#1350 Expose json::value::parse for UTF8 string on Windows
* PR#1344 libcpprestsdk: fix building as a static library
-- cpprestsdk team <askcasablanca@microsoft.com>  FRI, 24 Apr 2020 16:56:00 -0700

cpprestsdk (2.10.15)
* Extremely special thanks to @garethsb-sony for a large number of contributions in this release
* PR#1209 Workarounds for two GCC 4.7.2 bugs with lambda functions
* PR#1220 Fix SxS debug-release builds with Visual Studio
* PR#1219 Fix "Data" to "Date" in the HTTP Server API mapping, and clarify that the indices of these values match the HTTP_HEADER_ID values for HTTP_REQUEST_HEADERS but *not* HTTP_RESPONSE_HEADERS
* PR#1196 Fixing of connections_and_errors::cancel_with_error test which sometimes fires false positive error "There are no pending calls to next_request."
* PR#1233 Trim whitespace and nulls the same way.
* PR#1248 Avoid using permissive- with ZW which breaks VS2019
* PR#1182 Support for WinHTTPAL curl-to-WinHTTP adapter
* PR#1253 http_server_httpsys.cpp requires linking against httpapi.lib, http_client_winhttp.cpp does not.
* PR#1263 Remove trailing slash on websocketpp submodule url, which causes checkout failure on CircleCI with git 2.22.0
* PR#1293 Update vcpkg and remove tests that look for web servers that no longer exist
* PR#1288 Fix test case broken by commit f4c863b
* PR#1276 Added comparison overrides to utility::datetime
* PR#1289 Fix various warnings reported by gcc 9.3, and possibly earlier versions
* PR#1334 Update vcpkg and boost on Android
* PR#1306 Change default installation directory for cmake files to cmake/cpprestsdk
* PR#1330 Use LC_ALL_MASK rather than LC_ALL when calling newlocale
* PR#1310 Add TCP_NODELAY to disable Nagle's algorithm in Boost.ASIO-based http_client
* PR#1335 Turn VS2015 back on now that vcpkg is fixed.
* PR#1322 Enable HTTP compression support on all platforms
* PR#1340 Add Ubuntu 18.04 testing.
* PR#1342 Use C++11 synchronization classes under macOS too
* PR#1339 Fix tcp::resolver data race in the asio backend and be defensive against empty results
-- cpprestsdk team <askcasablanca@microsoft.com>  THR, 22 Feb 2020 08:31:00 -0800

cpprestsdk (2.10.14)
* Potential breaking change warning: This release changes the "default" proxy for the WinHTTP backend to go back to WINHTTP_ACCESS_TYPE_DEFAULT_PROXY. See https://github.com/microsoft/cpprestsdk/commit/60e067e71aebebdda5d82955060f5f0821c9df1d for more details. To get automatic WPAD behavior, set the proxy to auto detect.
* macOS with Brew and iOS builds have been disabled and are no longer being tested because our dependency boost for ios project appears to be broken with current releases of XCode as on the Azure Pipelines machines. We are interested in macOS / iOS folks who know what's going on here in contributing a repair to turn this back on.
* PR#1133 Add switches to make apiscan happy.
* PR#1130 json: {"meow"} is not a valid object
* PR#1150 Undefine compress if it is defined by zconf.h
* PR#1156 Fix broken CI Builds
* PR#1155 Use EVP_MAX_MD_SIZE instead of HMAC_MAX_MD_CBLOCK
* PR#1145 Remove the address_configured flag on tcp::resolver::query
* PR#1143 add ping and pong to message handler
* PR#539 Fix reusing ASIO http_client connecting to HTTPS server via proxy
* PR#1175 Fix issue #1171: Order of object destruction
* PR#1183 FIX: SSL proxy tunnel support with basic auth
* PR#1184 Fix profile being set on the compiler instead of the linker.
* PR#1185 Update boost-for-android for Android NDK r20 and disable macOS Homebrew.
* PR#1187 Replace CPPREST_TARGET_XP with version checks, remove ""s, and other cleanup
* PR#1188 Remove proxy settings detection behavior in "default proxy mode."
-- cpprestsdk team <askcasablanca@microsoft.com>  TUE, 16 Jul 2019 09:06:00 +0200

cpprestsdk (2.10.13)
* PR#1120 Fix off by one error in leap years before year 2000, and bad day names
* PR#1117 Parse and emit years from 1900 to 9999, and remove environment variable dependence on Android
* PR#1106 Paranoia for overflow of sprintf buffer in the year 10000
* PR#1101 Update request_timeout_microsecond timeout
* PR#1097 Allow error handling for time out in http_client_asio handle_connect
* PR#1094 Avoid tripping over 32 bit time_t mistakes.
* PR#1093 Don't initialize atomic_flag with 0.
-- cpprestsdk team <askcasablanca@microsoft.com>  WED, 24 Apr 2019 10:57:00 -0800

cpprestsdk (2.10.12)
* PR#1088 Fix data race, GitHub #1085
* PR#1084 Fix oauth nonces containing nulls.
* PR#1082 Workaround data-race on websocketpp's _htonll function
* PR#1080 Fix thread not joined
* PR#1076 Rewrite date formatting and parsing
-- cpprestsdk team <askcasablanca@microsoft.com>  TUE, 26 Mar 2019 11:57:00 -0800

cpprestsdk (2.10.11)
* PR##1073 Move get_jvm_env back into the crossplat namespace
* PR##1049 Add the missing ssl::context callback in websocket_client_config
* PR##1072 Gate stdext::checked_array_iterator usage on _ITERATOR_DEBUG_LEVEL
* PR##1051 Fix http_client_asio "https" with a proxy
* PR##1071 Add --vcpkg-root to repair UWP.
* PR##1041 Update Boost_for_android for Android R19
* PR##1064 Enable testing from root directory
* PR##1057 Returns int64 value in function of seeking to file end on x64 Windows.
* PR##1068 Don't close the output stream when reporting errors reading the body.
* PR##1053 Update vcpkg.
* PR##1032 Fix HTTP/1.0 'Keep-Alive' handling in http_client
* PR##1040 Disable WINHTTP_AUTOPROXY_OPTIONS machinery when using WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY.
-- cpprestsdk team <askcasablanca@microsoft.com>  WED, 20 Mar 2019 02:30:00 -0800

cpprestsdk (2.10.10)
----------------------
* PR#1023 Handle multi-byte unicode characters in json parsing
* PR#1033 Temporary fix for VS2013. Note that VS2013 is still not in support.
-- cpprestsdk team <askcasablanca@microsoft.com>  TUE, 29 Jan 2019 22:38:00 -0800

cpprestsdk (2.10.9)
----------------------
* PR#973  Address gcc warnings-as-errors in compression code, test improvements
* PR#986  Prevent infinite loop during proxy authentication
* PR#987  Remove use of aligned_union that broke CentOS 7.
* PR#1004 #993, #1002: Add flexibility for iOS building. Adds command line args…
* PR#1009 gcc: Fix compilation with -fno-operator-names
* PR#1019 FIX: crash with std::logic_error when reusing a connection that timed out on the server
* PR#1021 handle null bytes when parsing utf8
* PR#1017 Add in support for adding i386 slice when building for 32-bit targets. Also improve messaging and add means to clean
* PR#1024 http_compression.cpp: fix build with gcc 4.7
* PR#1022 Resolve double free when WinHttpSendRequest fails
-- cpprestsdk team <askcasablanca@microsoft.com>  FRI, 18 Jan 2019 16:58:00 -0800

cpprestsdk (2.10.8)
----------------------
* PR#938 Allow ppltasks.h and pplxtasks.h to co-exist
* PR#951 Fix incorrect const in reinterpret_cast
* PR#955 Fix UWP missing header
* PR#956 Adds support for OpenSSL 1.1.1
* PR#959 Fix Android build issue by remove the crossplat name space before android parameters
* PR#960 Update vcpkg to latest master to fix VS2015 build.
* PR#966 Fix string size for error message generated by windows_category
* PR#958 Add uri_builder::append_path_raw(...) to allow adding elements to path intentionally beginning with '/' ("//" will result in the final path value)
* PR#952 cmake: add code to detect system brotli library
* PR#963 Fix Brotli compress_helper early termination issue
* PR#961 Fixes iOS builds and makes it more future proof
-- cpprestsdk team <askcasablanca@microsoft.com>  WED, 14 Nov 2018 10:24:00 -0800

cpprestsdk (2.10.7)
----------------------
* cpprestsdk now has Azure Pipelines continuous integration.
* Builds for Android and iOS were repaired, now checked in Azure Pipelines to make sure that doesn't bit-rot in the future.
* Several race conditions in the listener were worked around; the listeners remain experimental and are unlikely to productized in their current form; the race conditions are structural, but at least the client tests pass most of the time.
* Incorrect handling of connection pooling bug that caused segfaults on Ubuntu introduced in 2.10.4 has been repaired.
* websocketpp checked in 0.5.1 version has been changed to a submodule and updated to 0.8.1.
* Added an API to set the number of threads in the asio thread pool, see PR#883
* Legacy unmaintained Visual Studio project files have been deleted, please use CMake instead.
* PR#670 Export methods to set/get the ambient scheduler in cpprest dll
* PR#866 Add Transfer-Encoding compression support and extensible compression API
* PR#892 Improve utf8_to_utf16 speed for common path
* PR#897 added URI resolution according to RFC3986
* PR#935 Fix spelling mistakes across the library
* PR#936 Use pplx namespace consistently
* PR#937 Remove _ASYNCRTIMP from ~http_listener() and implement inline
* PR#940 Avoid using identifiers reserved by C++ in header guards
* PR#943 blackjack sample: use vector instead of shared pointer for array
-- cpprestsdk team <askcasablanca@microsoft.com>  MON, 30 Oct 2018 20:32:00 -0800

cpprestsdk (2.10.6)
----------------------
* PR#844 Fix clang build error
-- cpprestsdk team <askcasablanca@microsoft.com>  MON, 30 Aug 2018 16:51:00 -0800

cpprestsdk (2.10.5)
----------------------
* Issue#842 Fix incorrect `cpprest/version.h`
-- cpprestsdk team <askcasablanca@microsoft.com>  FRI, 17 Aug 2018 09:47:00 -0800

cpprestsdk (2.10.4)
----------------------
* Added a `.clang-format` to enable consistent formatting.
* Added support for `Host:` headers changing the checked CNAME field for SSL certificates in WinHTTP and Asio.
* PR#736 passes 0666 to open() for creating files to better match the default behavior for other http clients (wget, etc).
* PR#732 fixes a build issue with clang
* PR#737 taught our cmake to respect the GNUInstallDirs variables
* PR#762 improved handling of dead connections in the connection pool on Asio.
* PR#750 improved error handling in the accept() call in `http_listener`
* PR#776 improved the iOS buildsystem
-- cpprestsdk team <askcasablanca@microsoft.com>  WED, 15 Aug 2018 12:35:00 -0800

cpprestsdk (2.10.3)
----------------------
* Added a root `CMakeLists.txt` to improve support for VS2017 Open Folder.
* PR#809 improves support for `/permissive-` in MSVC
* Issue#804 fixed a regression due to compression support; we no longer fail on unknown Content-Encoding headers if we did not set Accepts-Encoding
* PR#813 fixes build failure with boost 1.63
* PR#779 PR#787 suppress and fix some warnings with new versions of gcc and clang
-- cpprestsdk team <askcasablanca@microsoft.com>  THU, 2 Aug 2018 15:52:00 -0800

cpprestsdk (2.10.0)
----------------------
* Removed VS2013 MSBuild files. Use CMake with the "Visual Studio 12 2013" generator.
* Added VS2017 MSBuild files for convenience. It is highly recommended to use vcpkg or CMake instead to build the product library.
* Added UWP versions of the Windows Store samples for VS2017.
* Updated minimum required cmake version to 3.0.
* Added CMake config-file support to installation. This should be consumed by doing:
```cmake
find_package(cpprestsdk REQUIRED)
target_link_libraries(my_executable PRIVATE cpprestsdk::cpprest)
```
* Fixed several race conditions and memory leaks in the ASIO `http_client`.
* Fixed process termination bug around certain exceptional cases in all `http_client`s.
* Improved handling of `/Zcwchar_t-` on MSVC. That doesn't make it a good idea.
* Fixed use-after-free in the Windows Desktop `http_client` exposed by VS2017.
* Totally overhaul the CMake buildsystem for much better support of Windows and more shared code between platforms.
* PR#550 adds all remaining official HTTP status codes to `http::status_codes`.
* PR#563 wraps SSL errors on Windows Desktop in `http_exception`s, with more readable descriptions.
* PR#562 and PR#307 fixes building with LibreSSL.
* PR#551 adds convenience wrappers `json::value::has_T_field(T)` for inspecting object values.
* PR#549 fixes a race condition in the ASIO client during header parsing.
* PR#495 fixes a memory leak during proxy autodetection on Windows Desktop.
* PR#496 and PR#500 expand proxy autodetection to also consider Internet Explorer settings on Windows Desktop.
* PR#498 fixes error when handling responses of type NoContent, NotModified, or from 100 to 199.
* PR#398 enables specifying the User Agent used in OAuth2 requests.
* PR#494 improves the BingRequest sample's handling of proxies.
* PR#516 enables certificate revocation checks on Windows Desktop.
* PR#502 improves compatibility with glibc 2.26.
* PR#507 adds `http_request::get_remote_address()` to expose the client's IP address for `http_listener`.
* PR#521 enables use of empty passwords on Windows in `web::credentials`.
* PR#526 and PR#285 improve compatibility with openssl 1.1.0.
* PR#527 fixes a bug in the ASIO `http_client` where the proxy is passed the same credentials as the target host.
* PR#504 makes `uri_builder::to_string()` and `uri_builder::to_uri()` `const`.
* PR#446 adds handling for the host wildchar `+` to the ASIO `http_listener`.
* PR#465 improves compatibility with clang on Linux.
* PR#454 improves compatibility with icc 17.0.
* PR#487 fixes static library builds of `test_runner` on non-Windows platforms.
* PR#415 handles malformed URL requests to the ASIO `http_listener` instead of crashing.
* PR#393 fixes a race condition in the websocketpp `websocket_client`.
* PR#259 fixes several races in the ASIO `http_listener` which result in memory leaks or use after free of the connection objects.
* PR#376 adds `http_client_config::set_nativesessionhandle_options()` which enables customization of the session handle on Windows Desktop.
* PR#365 updates our convenience OpenSSL build scripts for Android to use openssl 1.0.2k.
* PR#336 makes the ASIO `http_client` more consistent with the Windows clients by not appending the port when it is default. This improves compatibility with AWS S3.
* PR#251 dramatically improves UTF8/16 conversions from 6s per 1MB to 3s per 1GB (2000x improvement).
* PR#246 enables TLS 1.1 and 1.2 on Windows 7 and Windows 8.
* PR#308 enables limited IPv6 support to `http_client` and `http_server`, depending on the underlying platform.
* PR#309 fixes a bug in base64 encoding that previously read beyond the input array, causing segfaults/AVs.
* PR#233 adds compression support (deflate and gzip) for Windows Desktop and ASIO `http_client`s based on Zlib.
* PR#218 fixes a memory leak in the UWP `http_client` when processing headers.
* PR#260 fixes inappropriate handling of certain connections errors in the ASIO `http_listener`.

-- cpprestsdk team <askcasablanca@microsoft.com>  SAT, 21 Oct 2017 00:52:00 -0800
