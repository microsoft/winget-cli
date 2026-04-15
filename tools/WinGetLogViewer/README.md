# WinGet Log Viewer

A VS Code extension that makes reading WinGet diagnostic log files fast and pleasant.

## Features

### Rich log viewer (WebView editor)
Opens automatically for `WinGet-*.log` and `WinGetCOM-*.log` files. Use the **"Open in WinGet Log Viewer"** command (or right-click any `.log` file in the Explorer) to open any log file in the viewer.

**Channel color badges** — each of the nine WinGet channels gets a distinct color:

| Badge | Channel | Description |
|-------|---------|-------------|
| ![FAIL](#) | `FAIL` | Failures / exceptions |
| ![CLI](#)  | `CLI`  | CLI command handling |
| ![SQL](#)  | `SQL`  | SQLite / index operations |
| ![REPO](#) | `REPO` | Repository / source operations |
| ![YAML](#) | `YAML` | YAML manifest parsing |
| ![CORE](#) | `CORE` | Core runtime |
| ![TEST](#) | `TEST` | Test infrastructure |
| ![CONF](#) | `CONF` | Configuration / DSC |
| ![WORK](#) | `WORK` | Workflow execution |

**Subchannel detection** — when a sub-component routes its log lines through a parent channel, the original `[CHAN]` tag appears at the start of the message. The viewer detects this and renders it as a secondary subchannel badge.

**Dynamic filters** — toggle individual channels and subchannels on/off with checkboxes. Use "All" / "None" quick links per section.

**Quick text search** — type in the Search box to live-filter visible lines.

**Severity highlighting** — rows that contain error/warning keywords get a colored left border and message highlighting.

**Time delta** — enable "Show time delta" to see the elapsed time between consecutive visible lines (`+Xms` / `+Xs`). Great for spotting slow operations.

**Jump to error** — use the ↑ Error / ↓ Error buttons to navigate between error and critical log lines.

**Follow mode** — enable "Follow (live tail)" to auto-scroll as the log file grows on disk. New line count is shown briefly in the status bar.

**Export filtered** — "Copy visible lines" sends all currently visible (filtered) raw log lines to the clipboard.

**Wrap long lines** — enable "Wrap long lines" to prevent horizontal scrolling on very long messages.

### Syntax highlighting (standard editor)
When a WinGet log file is opened in VS Code's standard text editor, the TextMate grammar provides coloring for timestamps, channel badges, subchannels, and error/warning keywords — no additional setup required.

## Log format

```
YY-MM-DD HH:MM:SS.mmm [CHAN] message
YY-MM-DD HH:MM:SS.mmm [CHAN] [SUBCHAN] message    ← subchannel variant
```

## Development

### Prerequisites
- Node.js 18+
- VS Code 1.85+

### Build

```bash
cd tools/WinGetLogViewer
npm install
npm run compile
```

### Run / Debug

1. Open `tools/WinGetLogViewer/` as a workspace in VS Code.
2. Press **F5** — this launches the Extension Development Host.
3. Open any `WinGet-*.log` or `WinGetCOM-*.log` file; the viewer opens automatically.

### Package

```bash
npm install -g @vscode/vsce
vsce package
```

This produces a `.vsix` file you can install via **Extensions: Install from VSIX…**

## Keyboard shortcuts

| Key | Action |
|-----|--------|
| **Ctrl+F** (in viewer) | Focus the search box |

## Roadmap / ideas

- Per-session filter presets (save/restore named filter combinations)
- Fold/collapse groups of lines by channel
- Correlation ID linking (when GUIDs appear in messages)
