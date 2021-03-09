/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests to include headers
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

// Include ATL headers before casablanca headers
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be explicit

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#endif

// These MFC headers are not code analysis clean.
#pragma warning(push)
#pragma warning(disable : 6387)
#include <afx.h>
#include <afxext.h> // MFC extensions
#include <afxwin.h> // MFC core and standard components
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h> // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC support for Windows Common Controls
#endif              // _AFX_NO_AFXCMN_SUPPORT
#pragma warning(pop)

#include <iostream>
// Windows Header Files:
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be explicit

#include "cpprest/http_client.h"
#include "unittestpp.h"
#include <atlbase.h>
#include <atlstr.h>

namespace tests
{
namespace functional
{
namespace misc
{
namespace atl_headers
{
SUITE(header_test1)
{
    TEST(HeaderTest) { web::http::client::http_client client(U("http://www.cnn.com")); }

} // SUITE(header_test1)

} // namespace atl_headers
} // namespace misc
} // namespace functional
} // namespace tests
