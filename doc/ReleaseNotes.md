## New in v1.12
MCP server available; run `winget mcp` for assistance on configuring your client.

## Experimental Features
* Experimental support for Fonts

---
### Experimental support for Fonts
The following snippet enables experimental support for fonts via `winget settings`. The `winget font list` command will list installed font families and the number of installed font faces.
```JSON
{
  "$schema" "https://aka.ms/winget-settings.schema.json",
  "experimentalFeatures": {
    "fonts": true
  }
}

```
