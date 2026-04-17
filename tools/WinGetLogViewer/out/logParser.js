"use strict";
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
Object.defineProperty(exports, "__esModule", { value: true });
exports.LEVEL_CHARS = exports.KNOWN_CHANNELS = void 0;
exports.parseLog = parseLog;
exports.collectChannels = collectChannels;
exports.collectSubchannels = collectSubchannels;
exports.hasExplicitLevels = hasExplicitLevels;
const LEVEL_CHAR_TO_SEVERITY = {
    V: 'verbose',
    I: 'info',
    W: 'warning',
    E: 'error',
    C: 'crit',
};
/** All known WinGet primary channels in display order. */
exports.KNOWN_CHANNELS = [
    'FAIL', 'CLI', 'SQL', 'REPO', 'YAML', 'CORE', 'TEST', 'CONF', 'WORK',
];
/** All level chars in severity order. */
exports.LEVEL_CHARS = ['V', 'I', 'W', 'E', 'C'];
// YYYY-MM-DD HH:MM:SS.mmm [optional: <L> ] [CHAN(+spaces)] rest-of-line
// Year may be 2 or 4 digits for forward/backward compat.
const LOG_LINE_RE = /^(\d{2,4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+(?:<([VIWEC])>\s+)?\[([A-Z]{2,8})\s*\]\s?(.*)$/;
// Optional subchannel at the very start of the message: [CHAN] or [CHAN ] …
const SUBCHANNEL_RE = /^\[([A-Z]{2,8})\s*\]\s?(.*)$/;
/**
 * Parse a raw log text (full file content) into an array of LogEntry objects.
 * Lines that do not match the expected format are skipped.
 */
function parseLog(text) {
    const lines = text.split(/\r?\n/);
    // Drop trailing empty lines (standard file ending or blank lines at EOF).
    while (lines.length > 0 && lines[lines.length - 1] === '') {
        lines.pop();
    }
    const entries = [];
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const m = LOG_LINE_RE.exec(line);
        if (!m) {
            // Continuation line — attach to the most recent entry.
            if (entries.length > 0) {
                const last = entries[entries.length - 1];
                (last.continuationLines ?? (last.continuationLines = [])).push(line);
            }
            continue;
        }
        const [, timestamp, rawLevel, channel, rest] = m;
        const levelChar = rawLevel;
        let subchannel;
        let message = rest;
        const sub = SUBCHANNEL_RE.exec(rest);
        if (sub) {
            subchannel = sub[1];
            message = sub[2];
        }
        const severity = levelChar
            ? LEVEL_CHAR_TO_SEVERITY[levelChar]
            : 'info';
        entries.push({
            lineIndex: i,
            raw: line,
            timestamp,
            levelChar,
            channel,
            subchannel,
            message,
            severity,
        });
    }
    return entries;
}
/** Collect the unique set of channels present in the given entries, preserving known order. */
function collectChannels(entries) {
    const found = new Set(entries.map(e => e.channel));
    const ordered = exports.KNOWN_CHANNELS.filter(c => found.has(c));
    // Append any unexpected channel names after the known ones.
    for (const c of found) {
        if (!exports.KNOWN_CHANNELS.includes(c)) {
            ordered.push(c);
        }
    }
    return ordered;
}
/** Collect the unique set of subchannels present in the given entries, sorted. */
function collectSubchannels(entries) {
    const found = new Set();
    for (const e of entries) {
        if (e.subchannel) {
            found.add(e.subchannel);
        }
    }
    return [...found].sort();
}
/** Returns true if the log file contains any entries with an explicit level marker. */
function hasExplicitLevels(entries) {
    return entries.some(e => e.levelChar !== undefined);
}
//# sourceMappingURL=logParser.js.map