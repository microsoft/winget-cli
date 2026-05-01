// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

import * as vscode from 'vscode';
import { LogViewerProvider } from './logViewerProvider';

export function activate(context: vscode.ExtensionContext): void {
    const provider = new LogViewerProvider(context);

    // Register the custom editor for WinGet-*.log and WinGetCOM-*.log
    context.subscriptions.push(
        vscode.window.registerCustomEditorProvider(
            LogViewerProvider.viewType,
            provider,
            {
                webviewOptions: {
                    retainContextWhenHidden: true,
                },
                supportsMultipleEditorsPerDocument: false,
            },
        ),
    );

    // "Open in WinGet Log Viewer" command — works on any .log file
    context.subscriptions.push(
        vscode.commands.registerCommand('wingetLogViewer.open', (uri?: vscode.Uri) => {
            // When invoked from the editor title or context menu, uri is provided.
            // When invoked from the command palette, use the active editor's document.
            const target = uri ?? vscode.window.activeTextEditor?.document.uri;
            if (!target) {
                vscode.window.showErrorMessage('WinGet Log Viewer: no file to open. Open a log file first.');
                return;
            }
            provider.openFile(target);
        }),
    );
}

export function deactivate(): void {
    // Nothing to clean up; subscriptions are disposed automatically.
}
