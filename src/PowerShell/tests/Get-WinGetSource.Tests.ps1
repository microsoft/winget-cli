# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

<#
.Synopsis
   Tests for the Get-WinGetSource cmdlet in the Microsoft.WinGet.Client PowerShell module.
#>

Describe 'Get-WinGetSource' {
    
    It 'Get Test Source' {
        $source = Get-WinGetSource -Name TestSource
        $source.Name | Should -Be 'TestSource'
        $source.Argument | Should -Be 'https://localhost:5001/TestKit/'
        $source.Type | Should -Be 'Microsoft.PreIndexed.Package'
    }
}