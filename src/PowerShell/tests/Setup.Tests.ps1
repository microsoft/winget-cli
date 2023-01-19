# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Setup file for all Pester tests related to the Microsoft.WinGet.Client PowerShell module.
#>

BeforeAll {
    # this calls REST API and takes roughly 1 second
    wingetdev source add 'TestSource' 'https://localhost:5001/TestKit/'
}