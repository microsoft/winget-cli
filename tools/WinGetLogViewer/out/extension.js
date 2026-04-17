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
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = __importStar(require("vscode"));
const logViewerProvider_1 = require("./logViewerProvider");
function activate(context) {
    const provider = new logViewerProvider_1.LogViewerProvider(context);
    // Register the custom editor for WinGet-*.log and WinGetCOM-*.log
    context.subscriptions.push(vscode.window.registerCustomEditorProvider(logViewerProvider_1.LogViewerProvider.viewType, provider, {
        webviewOptions: {
            retainContextWhenHidden: true,
        },
        supportsMultipleEditorsPerDocument: false,
    }));
    // "Open in WinGet Log Viewer" command — works on any .log file
    context.subscriptions.push(vscode.commands.registerCommand('wingetLogViewer.open', (uri) => {
        // When invoked from the editor title or context menu, uri is provided.
        // When invoked from the command palette, use the active editor's document.
        const target = uri ?? vscode.window.activeTextEditor?.document.uri;
        if (!target) {
            vscode.window.showErrorMessage('WinGet Log Viewer: no file to open. Open a log file first.');
            return;
        }
        provider.openFile(target);
    }));
}
function deactivate() {
    // Nothing to clean up; subscriptions are disposed automatically.
}
//# sourceMappingURL=extension.js.map