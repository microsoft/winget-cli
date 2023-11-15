filter Assert-WhiteSpaceIsNull {
    IF ([string]::IsNullOrWhiteSpace($_)){$null}
    ELSE {$_}
}

class WinGetSource
{
    [string] $Name
    [string] $Argument
    [string] $Data
    [string] $Identifier
    [string] $Type

    WinGetSource ()
    {  }

    WinGetSource ([string]$a, [string]$b, [string]$c, [string]$d, [string]$e)
    {
        $this.Name       = $a.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Argument   = $b.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Data       = $c.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Identifier = $d.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Type       = $e.TrimEnd() | Assert-WhiteSpaceIsNull
    }

    WinGetSource ([string[]]$a)
    {
        $this.name       = $a[0].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Argument   = $a[1].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Data       = $a[2].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Identifier = $a[3].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Type       = $a[4].TrimEnd() | Assert-WhiteSpaceIsNull
    }
    
    WinGetSource ([WinGetSource]$a)
    {
        $this.Name       = $a.Name.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Argument   = $a.Argument.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Data       = $a.Data.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Identifier = $a.Identifier.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Type       = $a.Type.TrimEnd() | Assert-WhiteSpaceIsNull

    }
    
    [WinGetSource[]] Add ([WinGetSource]$a)
    {
        $FirstValue  = [WinGetSource]::New($this)
        $SecondValue = [WinGetSource]::New($a)
        
        [WinGetSource[]] $Combined = @([WinGetSource]::New($FirstValue), [WinGetSource]::New($SecondValue))

        Return $Combined
    }

    [WinGetSource[]] Add ([String[]]$a)
    {
        $FirstValue  = [WinGetSource]::New($this)
        $SecondValue = [WinGetSource]::New($a)
        
        [WinGetSource[]] $Combined = @([WinGetSource]::New($FirstValue), [WinGetSource]::New($SecondValue))

        Return $Combined
    }
}

class WinGetPackage
{
    [string]$Name
    [string]$Id
    [string]$Version
    [string]$Available
    [string]$Source
    [string]$Match

    WinGetPackage ([string] $a, [string]$b, [string]$c, [string]$d, [string]$e)
    {
        $this.Name    = $a.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Id      = $b.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Version = $c.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Available = $d.TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Source  = $e.TrimEnd() | Assert-WhiteSpaceIsNull
    }
    
    WinGetPackage ([WinGetPackage] $a) {
        $this.Name    = $a.Name | Assert-WhiteSpaceIsNull
        $this.Id      = $a.Id | Assert-WhiteSpaceIsNull
        $this.Version = $a.Version | Assert-WhiteSpaceIsNull
        $this.Available = $a.Available | Assert-WhiteSpaceIsNull
        $this.Source  = $a.Source | Assert-WhiteSpaceIsNull

    }
    WinGetPackage ([psobject] $a) {
        $this.Name      = $a.Name | Assert-WhiteSpaceIsNull
        $this.Id        = $a.Id | Assert-WhiteSpaceIsNull
        $this.Version   = $a.Version | Assert-WhiteSpaceIsNull
        $this.Available = $a.Available | Assert-WhiteSpaceIsNull
        $this.Source    = $a.Source | Assert-WhiteSpaceIsNull
    }
    
    WinGetSource ([string[]]$a)
    {
        $this.name      = $a[0].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Id        = $a[1].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Version   = $a[2].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Available = $a[3].TrimEnd() | Assert-WhiteSpaceIsNull
        $this.Source    = $a[4].TrimEnd() | Assert-WhiteSpaceIsNull
    }

    
    [WinGetPackage[]] Add ([WinGetPackage] $a)
    {
        $FirstValue  = [WinGetPackage]::New($this)
        $SecondValue = [WinGetPackage]::New($a)

        [WinGetPackage[]]$Result = @([WinGetPackage]::New($FirstValue), [WinGetPackage]::New($SecondValue))

        Return $Result
    }

    [WinGetPackage[]] Add ([String[]]$a)
    {
        $FirstValue  = [WinGetPackage]::New($this)
        $SecondValue = [WinGetPackage]::New($a)
        
        [WinGetPackage[]] $Combined = @([WinGetPackage]::New($FirstValue), [WinGetPackage]::New($SecondValue))

        Return $Combined
    }
}
Function Invoke-WinGetCommand
{
    PARAM(
        [Parameter(Position=0, Mandatory=$true)] [string[]]$WinGetArgs,
        [Parameter(Position=0, Mandatory=$true)] [string[]]$IndexTitles,
        [Parameter()]                            [switch] $JSON
    )
    BEGIN
    {
        $Index  = @()
        $Result = @()
        $i      = 0
        $IndexTitlesCount = $IndexTitles.Count
        $Offset = 0
        $Found = $false
        
        ## Remove two characters from the string length and add "..." to the end (only if there is the three below characters present).
        [string[]]$WinGetSourceListRaw = & "WinGet" $WingetArgs | out-string -stream | foreach-object{$_ -replace ("$([char]915)$([char]199)$([char]170)", "$([char]199)")}
    }
    PROCESS
    {
        if($JSON){
            ## If expecting JSON content, return the object
            return $WinGetSourceListRaw | ConvertFrom-Json
        }

        ## Gets the indexing of each title
        $regex = $IndexTitles -join "|"
        for ($Offset=0; $Offset -lt $WinGetSourceListRaw.Length; $Offset++) {
            if($WinGetSourceListRaw[$Offset].Split(" ")[0].Trim() -match $regex) {
                $Found = $true
                break
            }
        }
        if(!$Found) {
            Write-Error -Message "No results were found." -TargetObject $WinGetSourceListRaw
            return
        }
        
        foreach ($IndexTitle in $IndexTitles) {
            ## Creates an array of titles and their string location
            $IndexStart = $WinGetSourceListRaw[$Offset].IndexOf($IndexTitle)
            $IndexEnds  = ""

            IF($IndexStart -ne "-1") {
                $Index += [pscustomobject]@{
                    Title = $IndexTitle
                    Start = $IndexStart
                    Ends = $IndexEnds
                    }
            }
        }

        ## Orders the Object based on Index value
        $Index = $Index | Sort-Object Start

        ## Sets the end of string value
        while ($i -lt $IndexTitlesCount) {
            $i ++

            ## Sets the End of string value (if not null)
            if($Index[$i].Start) {
                $Index[$i-1].Ends = ($Index[$i].Start -1) - $Index[$i-1].Start 
            }
        }

        ## Builds the WinGetSource Object with contents
        $i = $Offset + 2
        while($i -lt $WinGetSourceListRaw.Length) {
            $row = $WinGetSourceListRaw[$i]
            try {
                [bool] $TestNotTitles     = $WinGetSourceListRaw[0] -ne $row
                [bool] $TestNotHyphenLine = $WinGetSourceListRaw[1] -ne $row -and !$Row.Contains("---")
                [bool] $TestNotNoResults  = $row -ne "No package found matching input criteria."
            }
            catch {Wait-Debugger}

            if(!$TestNotNoResults) {
                Write-LogEntry -LogEntry "No package found matching input criteria." -Severity 1
            }

            ## If this is the first pass containing titles or the table line, skip.
            if($TestNotTitles -and $TestNotHyphenLine -and $TestNotNoResults) {
                $List = @{}

                foreach($item in $Index) {
                    if($Item.Ends) {
                            $List[$Item.Title] = $row.SubString($item.Start,$Item.Ends)
                    }
                    else {
                        $List[$item.Title] = $row.SubString($item.Start, $row.Length - $Item.Start)
                    }
                }

                $result += [pscustomobject]$list
            }
            $i++
        }
    }
    END
    {
        return $Result
    }
}

