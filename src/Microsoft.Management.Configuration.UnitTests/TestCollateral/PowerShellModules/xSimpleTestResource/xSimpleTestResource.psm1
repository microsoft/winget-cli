# Simple module with resources.

enum Ensure
{
    Absent
    Present
}

# This resource just checks if a file is there or not with and if its with the specified content.
[DscResource()]
class SimpleFileResource
{
    [DscProperty(Key)]
    [string] $Path

    [DscProperty()]
    [Ensure] $Ensure = [Ensure]::Present

    [DscProperty()]
    [string] $Content = $null

    [SimpleFileResource] Get()
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
class SimpleTestResource
{
    [DscProperty(Key)]
    [string] $key

    [DscProperty(Mandatory)]
    [string] $secretCode

    [SimpleTestResource] Get()
    {
        $result = @{
            key = "SimpleTestResourceKey"
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
class SimpleTestResourceThrows
{
    [DscProperty(Key)]
    [string] $key

    [SimpleTestResourceThrows] Get()
    {
        $result = @{
            key = "SimpleTestResourceThrowsKey"
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
class SimpleTestResourceError
{
    [DscProperty(Key)]
    [string] $key

    [SimpleTestResourceError] Get()
    {
        $result = @{
            key = "SimpleTestResourceErrorKey"
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