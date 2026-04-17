"use strict";
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.LogViewerProvider = void 0;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
const logParser_1 = require("./logParser");
/**
 * CustomReadonlyEditorProvider that renders WinGet log files in a rich WebView.
 */
class LogViewerProvider {
    constructor(_context) {
        this._context = _context;
        /** Track open panels so they can be updated by the follow-mode watcher. */
        this._panels = new Map();
    }
    // ── CustomReadonlyEditorProvider ────────────────────────────────
    async openCustomDocument(uri) {
        return { uri, dispose: () => { } };
    }
    async resolveCustomEditor(document, webviewPanel) {
        const filePath = document.uri.fsPath;
        webviewPanel.webview.options = {
            enableScripts: true,
            localResourceRoots: [
                vscode.Uri.file(path.join(this._context.extensionPath, 'media')),
            ],
        };
        webviewPanel.webview.html = this._buildHtml(webviewPanel.webview);
        // Send initial data once the webview is ready.
        const sendLog = () => {
            try {
                const raw = fs.readFileSync(filePath, 'utf8');
                const entries = (0, logParser_1.parseLog)(raw);
                webviewPanel.webview.postMessage({
                    type: 'load',
                    entries,
                    channels: (0, logParser_1.collectChannels)(entries),
                    subchannels: (0, logParser_1.collectSubchannels)(entries),
                    hasLevels: (0, logParser_1.hasExplicitLevels)(entries),
                });
            }
            catch (err) {
                vscode.window.showErrorMessage(`WinGet Log Viewer: failed to read ${filePath}: ${err}`);
            }
        };
        webviewPanel.webview.onDidReceiveMessage(msg => {
            if (msg.type === 'ready') {
                sendLog();
            }
            else if (msg.type === 'export') {
                vscode.env.clipboard.writeText(msg.text).then(() => {
                    const lineCount = msg.text.split('\n').length;
                    vscode.window.showInformationMessage(`WinGet Log Viewer: ${lineCount} line(s) copied to clipboard.`);
                });
            }
        });
        // Follow mode: poll for file changes using fs.watchFile (polling is reliable even when
        // another process holds the file open, unlike ReadDirectoryChangesW-based watchers).
        let lastMtime = 0;
        const sendUpdate = () => {
            try {
                const raw = fs.readFileSync(filePath, 'utf8');
                const entries = (0, logParser_1.parseLog)(raw);
                webviewPanel.webview.postMessage({ type: 'update', entries });
            }
            catch { /* ignore read errors during live tail */ }
        };
        fs.watchFile(filePath, { interval: 500, persistent: false }, (curr, prev) => {
            if (curr.mtimeMs !== prev.mtimeMs) {
                sendUpdate();
            }
        });
        const key = filePath;
        this._panels.set(key, { panel: webviewPanel });
        webviewPanel.onDidDispose(() => {
            fs.unwatchFile(filePath);
            this._panels.delete(key);
        });
    }
    // ── Public: open any log file in a new panel ─────────────────────
    /**
     * Open a specific file URI in the viewer (used by the "Open in WinGet Log Viewer" command).
     */
    openFile(uri) {
        const filePath = uri.fsPath;
        const fileName = path.basename(filePath);
        const panel = vscode.window.createWebviewPanel(LogViewerProvider.viewType, fileName, vscode.ViewColumn.Active, {
            enableScripts: true,
            localResourceRoots: [
                vscode.Uri.file(path.join(this._context.extensionPath, 'media')),
            ],
            retainContextWhenHidden: true,
        });
        panel.webview.html = this._buildHtml(panel.webview);
        const sendLoad = () => {
            try {
                const raw = fs.readFileSync(filePath, 'utf8');
                const entries = (0, logParser_1.parseLog)(raw);
                panel.webview.postMessage({
                    type: 'load',
                    entries,
                    channels: (0, logParser_1.collectChannels)(entries),
                    subchannels: (0, logParser_1.collectSubchannels)(entries),
                    hasLevels: (0, logParser_1.hasExplicitLevels)(entries),
                });
            }
            catch (err) {
                vscode.window.showErrorMessage(`WinGet Log Viewer: failed to read ${filePath}: ${err}`);
            }
        };
        panel.webview.onDidReceiveMessage(msg => {
            if (msg.type === 'ready') {
                sendLoad();
            }
            else if (msg.type === 'export') {
                vscode.env.clipboard.writeText(msg.text).then(() => {
                    const lineCount = msg.text.split('\n').length;
                    vscode.window.showInformationMessage(`WinGet Log Viewer: ${lineCount} line(s) copied to clipboard.`);
                });
            }
        });
        fs.watchFile(filePath, { interval: 500, persistent: false }, (curr, prev) => {
            if (curr.mtimeMs !== prev.mtimeMs) {
                try {
                    const raw = fs.readFileSync(filePath, 'utf8');
                    const entries = (0, logParser_1.parseLog)(raw);
                    panel.webview.postMessage({ type: 'update', entries });
                }
                catch { /* ignore */ }
            }
        });
        const key = filePath + '#manual';
        this._panels.set(key, { panel });
        panel.onDidDispose(() => {
            fs.unwatchFile(filePath);
            this._panels.delete(key);
        });
    }
    // ── HTML builder ─────────────────────────────────────────────────
    _buildHtml(webview) {
        const nonce = this._getNonce();
        const cspSource = webview.cspSource;
        const cssUri = webview.asWebviewUri(vscode.Uri.file(path.join(this._context.extensionPath, 'media', 'viewer.css')));
        const jsUri = webview.asWebviewUri(vscode.Uri.file(path.join(this._context.extensionPath, 'media', 'viewer.js')));
        const templatePath = path.join(this._context.extensionPath, 'media', 'viewer.html');
        let html = fs.readFileSync(templatePath, 'utf8');
        html = html
            .replace(/\{\{nonce\}\}/g, nonce)
            .replace(/\{\{cspSource\}\}/g, cspSource)
            .replace(/\{\{viewerCssUri\}\}/g, cssUri.toString())
            .replace(/\{\{viewerJsUri\}\}/g, jsUri.toString());
        return html;
    }
    _getNonce() {
        let text = '';
        const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
        for (let i = 0; i < 32; i++) {
            text += possible.charAt(Math.floor(Math.random() * possible.length));
        }
        return text;
    }
}
exports.LogViewerProvider = LogViewerProvider;
LogViewerProvider.viewType = 'wingetLogViewer.logViewer';
//# sourceMappingURL=logViewerProvider.js.map