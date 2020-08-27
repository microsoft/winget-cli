cd "$(system.defaultWorkingDirectory)\src\x64\Release\IndexHostService"

dotnet .\IndexHostService.dll "$(Agent.TempDirectory)\TestLocalIndex"