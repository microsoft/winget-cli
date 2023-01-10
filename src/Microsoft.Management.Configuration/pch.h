// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>

// TODO: Is this needed?

#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 4467 Allow use of uuid attribute for com object creation.
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
#pragma warning( pop )

#include <mutex>