// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#ifndef USE_PROD_CLSIDS
#pragma detect_mismatch("USE_PROD_CLSIDS", "Not Defined")
#else
#pragma detect_mismatch("USE_PROD_CLSIDS", "Defined")
#endif

#ifndef AICLI_DISABLE_TEST_HOOKS
#pragma detect_mismatch("AICLI_DISABLE_TEST_HOOKS", "Not Defined")
#else
#pragma detect_mismatch("AICLI_DISABLE_TEST_HOOKS", "Defined")
#endif

#ifndef WINGET_DISABLE_EXPERIMENTAL_FEATURES
#pragma detect_mismatch("WINGET_DISABLE_EXPERIMENTAL_FEATURES", "Not Defined")
#else
#pragma detect_mismatch("WINGET_DISABLE_EXPERIMENTAL_FEATURES", "Defined")
#endif

#ifndef USE_PROD_WINGET_SERVER
#pragma detect_mismatch("USE_PROD_WINGET_SERVER", "Not Defined")
#else
#pragma detect_mismatch("USE_PROD_WINGET_SERVER", "Defined")
#endif

#ifndef WINGET_ENABLE_RELEASE_BUILD
#pragma detect_mismatch("WINGET_ENABLE_RELEASE_BUILD", "Not Defined")
#else
#pragma detect_mismatch("WINGET_ENABLE_RELEASE_BUILD", "Defined")
#endif
