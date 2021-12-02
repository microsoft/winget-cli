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
        $this.Name       = $a.TrimEnd()
        $this.Argument   = $b.TrimEnd()
        $this.Data       = $c.TrimEnd()
        $this.Identifier = $d.TrimEnd()
        $this.Type       = $e.TrimEnd()
    }

    WinGetSource ([string[]]$a)
    {
        $this.name       = $a[0].TrimEnd()
        $this.Argument   = $a[1].TrimEnd()
        $this.Data       = $a[2].TrimEnd()
        $this.Identifier = $a[3].TrimEnd()
        $this.Type       = $a[4].TrimEnd()
    }
    
    WinGetSource ([WinGetSource]$a)
    {
        $this.Name       = $a.Name.TrimEnd()
        $this.Argument   = $a.Argument.TrimEnd()
        $this.Data       = $a.Data.TrimEnd()
        $this.Identifier = $a.Identifier.TrimEnd()
        $this.Type       = $a.Type.TrimEnd()

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
        $this.Name    = $a.TrimEnd()
        $this.Id      = $b.TrimEnd()
        $this.Version = $c.TrimEnd()
        $this.Available = $d.TrimEnd()
        $this.Source  = $e.TrimEnd()
    }
    
    WinGetPackage ([WinGetPackage] $a) {
        $this.Name    = $a.Name
        $this.Id      = $a.Id
        $this.Version = $a.Version
        $this.Available = $a.Available
        $this.Source  = $a.Source

    }
    WinGetPackage ([psobject] $a) {
        $this.Name      = $a.Name
        $this.Id        = $a.Id
        $this.Version   = $a.Version
        $this.Available = $a.Available
        $this.Source    = $a.Source
    }
    
    WinGetSource ([string[]]$a)
    {
        $this.name      = $a[0].TrimEnd()
        $this.Id        = $a[1].TrimEnd()
        $this.Version   = $a[2].TrimEnd()
        $this.Available = $a[3].TrimEnd()
        $this.Source    = $a[4].TrimEnd()
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
        $rgex = $IndexTitles -join "|"
        for ($Offset=0; $Offset -lt $WinGetSourceListRaw.Length; $Offset++) {
            if($WinGetSourceListRaw[$Offset].Split(" ")[0].Trim() -match $rgex) {
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

