// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#pragma warning( pop )
#include <string>

unsigned char* GetUCharString(const std::string& str);

std::string GetUserSID();

wil::unique_event CreateOrOpenServerStartEvent();
