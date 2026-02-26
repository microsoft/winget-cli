// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ConnectionManager.h"

#include "../ReportingHandler.h"

using namespace SFS::details;

ConnectionManager::ConnectionManager(const ReportingHandler& handler) : m_handler(handler)
{
}

ConnectionManager::~ConnectionManager()
{
}
