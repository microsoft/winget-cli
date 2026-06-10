# E2E module with resources.

enum Ensure
{
    Absent
    Present
}

# This resource just checks if a file is there or not with and if its with the specified content.
[DscResource()]
class E2EMalicious
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
