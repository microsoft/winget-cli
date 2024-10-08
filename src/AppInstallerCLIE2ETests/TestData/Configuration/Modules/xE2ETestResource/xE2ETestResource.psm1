# E2E module with resources.

enum Ensure
{
    Absent
    Present
}

# This resource just checks if a file is there or not with and if its with the specified content.
[DscResource()]
class E2EFileResource
{
    [DscProperty(Key)]
    [string] $Path

    [DscProperty()]
    [Ensure] $Ensure = [Ensure]::Present

    [DscProperty()]
    [string] $Content = $null

    [E2EFileResource] Get()
    {
        if ([string]::IsNullOrEmpty($this.Path))
        {
            throw
        }

        $fileContent = $null
        if (Test-Path -Path $this.Path -PathType Leaf)
        {
            $fileContent = Get-Content $this.Path -Raw
        }

        $result = @{
            Path = $this.Path
            Content = $fileContent
        }

        return $result
    }

    [bool] Test()
    {
        $get = $this.Get()

        if (Test-Path -Path $this.Path -PathType Leaf)
        {
            if ($this.Ensure -eq [Ensure]::Present)
            {
                return $this.Content -eq $get.Content
            }
        }
        elseif ($this.Ensure -eq [Ensure]::Absent)
        {
            return $true
        }

        return $false
    }

    [void] Set()
    {
        if (-not $this.Test())
        {
            if (Test-Path -Path $this.Path -PathType Leaf)
            {
                if ($this.Ensure -eq [Ensure]::Present)
                {
                    Set-Content $this.Path $this.Content -NoNewline
                }
                else
                {
                    Remove-Item $this.Path
                }
            }
            else
            {
                if ($this.Ensure -eq [Ensure]::Present)
                {
                    Set-Content $this.Path $this.Content -NoNewline
                }
            }
        }
    }
}

[DscResource()]
class E2ETestResource
{
    [DscProperty(Key)]
    [string] $key

    [DscProperty(Mandatory)]
    [string] $secretCode

    [E2ETestResource] Get()
    {
        $result = @{
            key = "E2ETestResourceKey"
        }
        return $result
    }

    [bool] Test()
    {
        return $this.secretCode -eq "4815162342"
    }

    [void] Set()
    {
        if (-not $this.Test())
        {
            $global:DSCMachineStatus = 1
        }
    }
}

[DscResource()]
class E2ETestResourceThrows
{
    [DscProperty(Key)]
    [string] $key

    [E2ETestResourceThrows] Get()
    {
        $result = @{
            key = "E2ETestResourceThrowsKey"
        }
        throw "throws in Get"
        return $result
    }

    [bool] Test()
    {
        throw "throws in Test"
        return $false
    }

    [void] Set()
    {
        throw "throws in Set"
    }
}

[DscResource()]
class E2ETestResourceError
{
    [DscProperty(Key)]
    [string] $key

    [E2ETestResourceError] Get()
    {
        $result = @{
            key = "E2ETestResourceErrorKey"
        }
        Write-Error "Error in Get"
        return $result
    }

    [bool] Test()
    {
        Write-Error "Error in Test"
        return $true
    }

    [void] Set()
    {
        Write-Error "Error in Set"
    }
}

[DscResource()]
class E2ETestResourceTypes
{
    [DscProperty(Key)]
    [string] $key

    [DscProperty()]
    [boolean] $boolProperty

    [DscProperty()]
    [int] $intProperty;

    [DscProperty()]
    [double] $doubleProperty;

    [DscProperty()]
    [char] $charProperty;

    [DscProperty()]
    [Hashtable] $hashtableProperty;

    [E2ETestResourceTypes] Get()
    {
        $result = @{
            key = "E2ETestResourceTypesKey"
            boolProperty = $false
            intProperty = 0
            doubleProperty = 0.0
            charProperty = 'z'
            hashtableProperty = @{}
        }
        return $result
    }

    [bool] Test()
    {
        # Because we can't get the error stream from a class based resource, I throw so is easier to know if
        # there's something wrong.
        if ($this.boolProperty -ne $true)
        {
            throw "Failed boolProperty"
        }

        if ($this.intProperty -ne 3)
        {
            throw "Failed intProperty. Got $($this.intProperty)"
        }

        if ($this.doubleProperty -ne -9.876)
        {
            throw "Failed doubleProperty Got $($this.doubleProperty)"
        }

        if ($this.charProperty -ne 'f')
        {
            throw "Failed charProperty Got $($this.charProperty)"
        }

        if ($this.hashtableProperty.ContainsKey("secretStringKey"))
        {
            if ($this.hashtableProperty["secretStringKey"] -ne "secretCode")
            {
                throw "Failed comparing value of `$hashtableProperty.secretStringKey Got $($this.hashtableProperty["secretStringKey"])"
            }
        }
        else
        {
            throw "Failed finding secretStringKey in hashtableProperty"
        }

        if ($this.hashtableProperty.ContainsKey("secretIntKey"))
        {
            if ($this.hashtableProperty["secretIntKey"] -ne 123456)
            {
                throw "Failed comparing value of `$hashtableProperty.secretIntKey Got $($this.hashtableProperty["secretIntKey"])"
            }
        }
        else
        {
            throw "Failed finding secretIntKey in hashtableProperty"
        }

        return $true
    }

    [void] Set()
    {
        # no-op
    }
}

# This resource "crashes" the containing process (really it just exits)
[DscResource()]
class E2ETestResourceCrash
{
    [DscProperty(Key)]
    [string] $key

    [E2ETestResourceCrash] Get()
    {
        $result = @{
            key = "E2ETestResourceCrashKey"
        }
        [System.Environment]::Exit(0)
        return $result
    }

    [bool] Test()
    {
        [System.Environment]::Exit(0)
        return $true
    }

    [void] Set()
    {
        [System.Environment]::Exit(0)
    }
}

# This resource writes the current PID to the provided file path.
[DscResource()]
class E2ETestResourcePID
{
    [DscProperty(Key)]
    [string] $key

    [DscProperty(Mandatory)]
    [string] $directoryPath

    [E2ETestResourcePID] Get()
    {
        $result = @{
            key = "E2ETestResourcePID"
            directoryPath = $this.directoryPath
        }

        return $result
    }

    [bool] Test()
    {
        return $false
    }

    [void] Set()
    {
        if (Test-Path -Path $this.directoryPath)
        {
            $processId = [System.Diagnostics.Process]::GetCurrentProcess().Id
            $filePath = Join-Path -Path $this.directoryPath -ChildPath "$processId.txt"
            New-Item -Path $filePath -ItemType File -Force  
        }
    }
}

# This resource writes the current PSModulePath to the provided file path.
[DscResource()]
class E2ETestResourcePSModulePath
{
    [DscProperty(Key)]
    [string] $key

    [DscProperty(Mandatory)]
    [string] $outputPath

    [E2ETestResourcePSModulePath] Get()
    {
        $result = @{
            key = "E2ETestResourcePSModulePath"
            outputPath = $this.outputPath
        }

        return $result
    }

    [bool] Test()
    {
        return $false
    }

    [void] Set()
    {
        Set-Content -Path $this.outputPath -Value $env:PSModulePath -Force
    }
}
