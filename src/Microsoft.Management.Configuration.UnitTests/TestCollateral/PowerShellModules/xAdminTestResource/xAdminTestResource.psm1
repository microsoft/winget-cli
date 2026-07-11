# Simple module that requires admin.
#Requires -RunAsAdministrator
enum Ensure
{
    Absent
    Present
}

[DscResource()]
class AdminResource
{
    [DscProperty(Key)]
    [string] $key

    [AdminResource] Get()
    {
        return $this
    }

    [bool] Test()
    {
        return $false
    }

    [void] Set()
    {
    }
}

