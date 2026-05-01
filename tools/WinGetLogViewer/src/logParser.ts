// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

/**
 * A parsed WinGet log entry.
 *
 * Log line format (new):
 *   YYYY-MM-DD HH:MM:SS.mmm <L> [CHAN] [SUBCHAN] message
 *
 * Log line format (old, without level marker):
 *   YYYY-MM-DD HH:MM:SS.mmm [CHAN] [SUBCHAN] message
 *
 * The subchannel is optional and appears when a sub-component routes its logs
 * through a parent channel. In that case the original channel tag is embedded
 * as the first token of the message text.
 */
export interface LogEntry {
    /** 0-based index in the original file. */
    lineIndex: number;
    /** Raw, unmodified primary log line. */
    raw: string;
    /** Timestamp string as it appears in the file, e.g. "2026-04-14 22:39:42.041". */
    timestamp: string;
    /** Explicit level char from <L> marker (V/I/W/E/C), or undefined for old-format logs. */
    levelChar?: LevelChar;
    /** Primary channel name, e.g. "CLI", "CORE", "FAIL". */
    channel: string;
    /** Optional subchannel detected from the message prefix, e.g. "REPO". */
    subchannel?: string;
    /** Message text after channel (and optional subchannel) have been stripped. */
    message: string;
    /** Severity derived from the explicit level marker when present. */
    severity: Severity;
    /**
     * Raw text of any lines that followed this entry without a timestamp of their own.
     * These are continuation lines (e.g. multi-line output, stack traces) that belong
     * to this log entry.
     */
    continuationLines?: string[];
}

export type LevelChar = 'V' | 'I' | 'W' | 'E' | 'C';
export type Severity  = 'verbose' | 'info' | 'warning' | 'error' | 'crit';

const LEVEL_CHAR_TO_SEVERITY: Record<LevelChar, Severity> = {
    V: 'verbose',
    I: 'info',
    W: 'warning',
    E: 'error',
    C: 'crit',
};

/** All known WinGet primary channels in display order. */
export const KNOWN_CHANNELS: readonly string[] = [
    'FAIL', 'CLI', 'SQL', 'REPO', 'YAML', 'CORE', 'TEST', 'CONF', 'WORK',
];

/** All level chars in severity order. */
export const LEVEL_CHARS: readonly LevelChar[] = ['V', 'I', 'W', 'E', 'C'];

// YYYY-MM-DD HH:MM:SS.mmm [optional: <L> ] [CHAN(+spaces)] rest-of-line
// Year may be 2 or 4 digits for forward/backward compat.
const LOG_LINE_RE = /^(\d{2,4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+(?:<([VIWEC])>\s+)?\[([A-Z]{2,8})\s*\]\s?(.*)$/;

// Optional subchannel at the very start of the message: [CHAN] or [CHAN ] …
const SUBCHANNEL_RE = /^\[([A-Z]{2,8})\s*\]\s?(.*)$/;

/**
 * Parse a raw log text (full file content) into an array of LogEntry objects.
 * Lines that do not match the expected format are skipped.
 */
export function parseLog(text: string): LogEntry[] {
    const lines = text.split(/\r?\n/);
    // Drop trailing empty lines (standard file ending or blank lines at EOF).
    while (lines.length > 0 && lines[lines.length - 1] === '') { lines.pop(); }
    const entries: LogEntry[] = [];

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const m = LOG_LINE_RE.exec(line);
        if (!m) {
            // Continuation line — attach to the most recent entry.
            if (entries.length > 0) {
                const last = entries[entries.length - 1];
                (last.continuationLines ??= []).push(line);
            }
            continue;
        }

        const [, timestamp, rawLevel, channel, rest] = m;
        const levelChar = rawLevel as LevelChar | undefined;

        let subchannel: string | undefined;
        let message = rest;

        const sub = SUBCHANNEL_RE.exec(rest);
        if (sub) {
            subchannel = sub[1];
            message    = sub[2];
        }

        const severity: Severity = levelChar
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
export function collectChannels(entries: LogEntry[]): string[] {
    const found = new Set(entries.map(e => e.channel));
    const ordered = KNOWN_CHANNELS.filter(c => found.has(c));
    // Append any unexpected channel names after the known ones.
    for (const c of found) {
        if (!KNOWN_CHANNELS.includes(c)) { ordered.push(c); }
    }
    return ordered;
}

/** Collect the unique set of subchannels present in the given entries, sorted. */
export function collectSubchannels(entries: LogEntry[]): string[] {
    const found = new Set<string>();
    for (const e of entries) {
        if (e.subchannel) { found.add(e.subchannel); }
    }
    return [...found].sort();
}

/** Returns true if the log file contains any entries with an explicit level marker. */
export function hasExplicitLevels(entries: LogEntry[]): boolean {
    return entries.some(e => e.levelChar !== undefined);
}
