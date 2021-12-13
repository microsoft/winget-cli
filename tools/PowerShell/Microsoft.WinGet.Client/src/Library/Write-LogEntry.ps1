Function Write-LogEntry
{
    PARAM(
        [Parameter(Position=0, Mandatory=$true)]  [string] $LogEntry,
        [Parameter(Position=1, Mandatory=$false)] [int]    $Severity=1,
        [Parameter(Position=2, Mandatory=$false)] [string] $FontColor="",
        [Parameter(Position=3, Mandatory=$false)] [int]    $Indent = 0,
        [Parameter(Position=4, Mandatory=$false)] [switch] $NoNewLine
    )
    BEGIN
    {
        if($FontColor -eq "") {
            switch ($Severity) {
                "1" {
                    ## Informational Response
                    $FontColor     = "White"
                    $MessagePreFix = ""
                }
                "2" {
                    ## Warning Response
                    $FontColor = "Yellow"
                    $MessagePreFix = "WARNING:  "
                }
                "3" {
                    ## Error Response
                    $FontColor = "Red"
                    $MessagePreFix = "ERROR:    "
                }
            }
        }
        ## Combines the logging message and the message type as a prefix
        $LogEntry = $MessagePreFix + $LogEntry

        ## Indents the message when viewed on the screen.
        $LogEntry = $LogEntry.PadLeft($LogEntry.Length + (2 * $Indent) )
    }
    PROCESS
    {
        ## Writes logging to the screen
        if($NoNewLine) {
            Write-Host -Object $LogEntry -ForegroundColor $FontColor -NoNewline
        }
        else {
            Write-Host -Object $LogEntry -ForegroundColor $FontColor
        }
    }
    END
    {
        return
    }
}