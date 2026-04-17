---
applyTo: "tools/WinGetLogViewer/**"
---
# WinGet Log Viewer — Copilot Instructions

This is a VS Code extension that renders WinGet diagnostic log files in a rich WebView custom editor.

## Log Format

```
YYYY-MM-DD HH:MM:SS.mmm <L> [CHAN    ] message
YYYY-MM-DD HH:MM:SS.mmm <L> [CHAN    ] [SUBCHAN] message   ← subchannel variant
```

- `<L>` is optional; values: `V`=Verbose, `I`=Info, `W`=Warning, `E`=Error, `C`=Critical
- Channel is padded to 8 chars with spaces inside the brackets
- Subchannel: when a sub-component routes logs through a parent, its original `[CHAN]` tag appears at the start of `message` and is treated as a subchannel
- Older files without `<L>` are also supported
- Main parse regex (used in both `src/logParser.ts` and `media/viewer.js`):
  ```
  /^(\d{2,4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+(?:<([VIWEC])>\s+)?\[([A-Z]{2,8})\s*\]\s?(.*)$/
  ```

## Architecture

```
src/
  extension.ts          — activate/deactivate; registers provider and openFile command
  logParser.ts          — parses raw text → LogEntry[]; compiled to out/logParser.js
  logViewerProvider.ts  — CustomTextEditorProvider; manages WebView lifecycle, follow mode
media/
  viewer.html           — WebView HTML template (nonces injected at runtime for CSP)
  viewer.css            — All styles; --row-height CSS var must match ROW_HEIGHT in viewer.js
  viewer.js             — All client-side logic (not compiled — served directly)
syntaxes/
  winget-log.tmLanguage.json  — TextMate grammar for standard editor syntax highlighting
```

## Virtual Scroll Architecture

- All rows are **uniform height** (`ROW_HEIGHT = 20` px in `viewer.js`, `--row-height: 20px` in `viewer.css`) — these **must stay in sync**
- `displayRows[]` is a flat array built from filtered log entries; continuation lines are promoted to full rows with parent metadata replicated
- `logSpacerTop` and `logSpacerBot` are `<div>` elements whose `.style.height` creates the illusion of the full list
- `overflow-anchor: none` on `#log-container` is **critical** — without it, when `logSpacerTop` first grows from 0, CSS scroll anchoring cascades and jumps the view to the end
- `scheduledRender` must be declared on its **own line** (not embedded in a comment) or it will be silently undefined

## WebView CSP Rules

- No inline `style="..."` attributes in HTML — **always use CSS classes or `element.style.property = value`** via CSSOM
- Script/style nonces are replaced at runtime in `logViewerProvider.ts`
- After any edit to `media/viewer.js`, validate syntax: `node --check media/viewer.js`

## Follow Mode

- Uses **Node.js `fs.watchFile({ interval: 500, persistent: false })`**, not VS Code's `createFileSystemWatcher`
- Reason: `ReadDirectoryChangesW` (used by VS Code's watcher) misses events when another process holds the file open — WinGet keeps log files open while writing
- `fs.unwatchFile()` is called in `panel.onDidDispose()` to clean up
- Both `resolveCustomEditor` and `openFile` code paths set up the watcher

## Ready Handshake

The WebView sends `{ type: 'ready' }` when its JS has loaded. `logViewerProvider.ts` waits for this message before calling `sendLog()`. This prevents `postMessage` being dropped before the listener is registered.

## Channel Colors

| Channel | Color (CSS class) | Emoji proxy in README |
|---------|-------------------|-----------------------|
| `FAIL`  | red               | 🔴 |
| `CLI`   | blue              | 🔵 |
| `SQL`   | purple            | 🟣 |
| `REPO`  | light-blue/cyan   | 🩵 |
| `YAML`  | yellow            | 🟡 |
| `CORE`  | white/light-gray  | ⬜ |
| `TEST`  | green             | 🟢 |
| `CONF`  | gray              | 🩶 |
| `WORK`  | orange            | 🟠 |

## Build & Package

```bash
# Compile TypeScript (required after any .ts change)
npx tsc

# Build + package VSIX
npm run package          # → winget-log-viewer-<version>.vsix

# Install locally
code --install-extension winget-log-viewer-0.1.0.vsix
```

## .vscodeignore Notes

- `out/` must **NOT** be in `.vscodeignore` — the compiled JS lives there and is the extension entrypoint
- `src/`, `tsconfig.json`, `out/**/*.map` are excluded (source only, not needed at runtime)

## Related C++ Files

The C++ logging infrastructure (in the parent repo) writes the `<L>` level marker:
- `src/AppInstallerCommonCore/FileLogger.cpp` — `ToLogLine()` writes `<L> ` between timestamp and `[CHAN]`
- `src/AppInstallerSharedLib/AppInstallerLogging.h/.cpp` — `GetLevelChar(Level)` returns V/I/W/E/C
- `src/AppInstallerCLICore/Commands/DebugCommand.h/.cpp` — `LogViewerTestCommand` exercises all viewer features (`winget debug log-viewer [--follow]`)

## Known Gotchas

- **Scroll jump at 8–9 wheel clicks**: caused by `overflow-anchor` — fixed with `overflow-anchor: none` on `#log-container`
- **Follow mode not updating**: VS Code's file watcher uses `ReadDirectoryChangesW` and misses events when WinGet holds the file open — use `fs.watchFile()` polling instead
- **`postMessage` dropped on open**: webview JS may not be listening yet — always use the ready-handshake
- **Orphaned JS fragments**: regex replacements can leave stale code that causes silent runtime errors — always validate with `node --check media/viewer.js`
- **CRLF logs**: the parser handles both `\r\n` and `\n` line endings; trailing empty lines are discarded
