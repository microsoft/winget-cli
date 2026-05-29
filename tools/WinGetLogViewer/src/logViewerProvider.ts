// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

import * as vscode from 'vscode';
import * as path   from 'path';
import * as fs     from 'fs';
import { parseLog, collectChannels, collectSubchannels, hasExplicitLevels } from './logParser';

/**
 * CustomReadonlyEditorProvider that renders WinGet log files in a rich WebView.
 */
export class LogViewerProvider implements vscode.CustomReadonlyEditorProvider {

    public static readonly viewType = 'wingetLogViewer.logViewer';

    /** Track open panels so they can be updated by the follow-mode watcher. */
    private readonly _panels = new Map<string, { panel: vscode.WebviewPanel }>();

    constructor(private readonly _context: vscode.ExtensionContext) {}

    // ── CustomReadonlyEditorProvider ────────────────────────────────

    async openCustomDocument(
        uri: vscode.Uri,
    ): Promise<vscode.CustomDocument> {
        return { uri, dispose: () => {} };
    }

    async resolveCustomEditor(
        document: vscode.CustomDocument,
        webviewPanel: vscode.WebviewPanel,
    ): Promise<void> {
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
                const raw     = fs.readFileSync(filePath, 'utf8');
                const entries = parseLog(raw);
                webviewPanel.webview.postMessage({
                    type:        'load',
                    entries,
                    channels:    collectChannels(entries),
                    subchannels: collectSubchannels(entries),
                    hasLevels:   hasExplicitLevels(entries),
                });
            } catch (err) {
                vscode.window.showErrorMessage(`WinGet Log Viewer: failed to read ${filePath}: ${err}`);
            }
        };

        webviewPanel.webview.onDidReceiveMessage(msg => {
            if (msg.type === 'ready') {
                sendLog();
            } else if (msg.type === 'export') {
                vscode.env.clipboard.writeText(msg.text).then(() => {
                    const lineCount = (msg.text as string).split('\n').length;
                    vscode.window.showInformationMessage(`WinGet Log Viewer: ${lineCount} line(s) copied to clipboard.`);
                });
            }
        });

        // Follow mode: poll for file changes using fs.watchFile (polling is reliable even when
        // another process holds the file open, unlike ReadDirectoryChangesW-based watchers).
        let lastMtime = 0;
        const sendUpdate = () => {
            try {
                const raw     = fs.readFileSync(filePath, 'utf8');
                const entries = parseLog(raw);
                webviewPanel.webview.postMessage({ type: 'update', entries });
            } catch { /* ignore read errors during live tail */ }
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
    public openFile(uri: vscode.Uri): void {
        const filePath = uri.fsPath;
        const fileName = path.basename(filePath);

        const panel = vscode.window.createWebviewPanel(
            LogViewerProvider.viewType,
            fileName,
            vscode.ViewColumn.Active,
            {
                enableScripts: true,
                localResourceRoots: [
                    vscode.Uri.file(path.join(this._context.extensionPath, 'media')),
                ],
                retainContextWhenHidden: true,
            },
        );

        panel.webview.html = this._buildHtml(panel.webview);

        const sendLoad = () => {
            try {
                const raw     = fs.readFileSync(filePath, 'utf8');
                const entries = parseLog(raw);
                panel.webview.postMessage({
                    type:        'load',
                    entries,
                    channels:    collectChannels(entries),
                    subchannels: collectSubchannels(entries),
                    hasLevels:   hasExplicitLevels(entries),
                });
            } catch (err) {
                vscode.window.showErrorMessage(`WinGet Log Viewer: failed to read ${filePath}: ${err}`);
            }
        };

        panel.webview.onDidReceiveMessage(msg => {
            if (msg.type === 'ready') {
                sendLoad();
            } else if (msg.type === 'export') {
                vscode.env.clipboard.writeText(msg.text).then(() => {
                    const lineCount = (msg.text as string).split('\n').length;
                    vscode.window.showInformationMessage(`WinGet Log Viewer: ${lineCount} line(s) copied to clipboard.`);
                });
            }
        });

        fs.watchFile(filePath, { interval: 500, persistent: false }, (curr, prev) => {
            if (curr.mtimeMs !== prev.mtimeMs) {
                try {
                    const raw     = fs.readFileSync(filePath, 'utf8');
                    const entries = parseLog(raw);
                    panel.webview.postMessage({ type: 'update', entries });
                } catch { /* ignore */ }
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

    private _buildHtml(webview: vscode.Webview): string {
        const nonce        = this._getNonce();
        const cspSource    = webview.cspSource;

        const cssUri = webview.asWebviewUri(
            vscode.Uri.file(path.join(this._context.extensionPath, 'media', 'viewer.css')),
        );
        const jsUri = webview.asWebviewUri(
            vscode.Uri.file(path.join(this._context.extensionPath, 'media', 'viewer.js')),
        );

        const templatePath = path.join(this._context.extensionPath, 'media', 'viewer.html');
        let html = fs.readFileSync(templatePath, 'utf8');
        html = html
            .replace(/\{\{nonce\}\}/g,        nonce)
            .replace(/\{\{cspSource\}\}/g,     cspSource)
            .replace(/\{\{viewerCssUri\}\}/g,  cssUri.toString())
            .replace(/\{\{viewerJsUri\}\}/g,   jsUri.toString());
        return html;
    }

    private _getNonce(): string {
        let text = '';
        const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
        for (let i = 0; i < 32; i++) {
            text += possible.charAt(Math.floor(Math.random() * possible.length));
        }
        return text;
    }
}
