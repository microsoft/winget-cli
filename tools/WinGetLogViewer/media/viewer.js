// @ts-check
// WinGet Log Viewer — WebView client script
// Communicates with the extension host via postMessage.

(function () {
    'use strict';

    const vscode = acquireVsCodeApi();

    // ── State ────────────────────────────────────────────────────────
    /** @type {import('../src/logParser').LogEntry[]} */
    let allEntries    = [];
    /** @type {import('../src/logParser').LogEntry[]} */
    let visibleEntries = [];

    /**
     * Flat list of rows to render — primary entries plus continuation lines
     * each promoted to their own row with parent metadata replicated.
     * @type {Array<{timestamp:string, levelChar:string|null, channel:string, subchannel:string|null, message:string, severity:string, isContinuation:boolean, deltaMs:number}>}
     */
    let displayRows = [];

    /** @type {Set<string>} */
    let enabledChannels    = new Set();
    /** @type {Set<string>} */
    let enabledSubchannels = new Set();
    /** @type {Set<string>} — active level chars; null means file has no level markers (filter hidden) */
    let enabledLevels = new Set(['V','I','W','E','C']);
    /** Whether the loaded file contains explicit level markers. */
    let fileHasLevels = false;

    let searchText   = '';
    let showDelta    = false;
    let followMode   = false;
    let wrapLines    = false;

    // Virtual scroll
    const ROW_HEIGHT    = 20; // px — matches CSS var(--row-height)
    let   scrollTop     = 0;
    let   containerH    = 0;
    let   renderBuffer  = 40; // extra rows above/below viewport

    // Error navigation
    let lastErrorJumpIndex = -1;

    // ── DOM refs ─────────────────────────────────────────────────────
    const levelList      = /** @type {HTMLUListElement}  */ (document.getElementById('level-list'));
    const levelSec       = /** @type {HTMLElement}       */ (document.getElementById('level-section'));
    const channelList    = /** @type {HTMLUListElement}  */ (document.getElementById('channel-list'));
    const subchannelList = /** @type {HTMLUListElement}  */ (document.getElementById('subchannel-list'));
    const subchannelSec  = /** @type {HTMLElement}       */ (document.getElementById('subchannel-section'));
    const searchInput    = /** @type {HTMLInputElement}  */ (document.getElementById('search-input'));
    const statsText      = /** @type {HTMLSpanElement}   */ (document.getElementById('stats-text'));
    const statusIndicator= /** @type {HTMLSpanElement}   */ (document.getElementById('status-indicator'));
    const logContainer   = /** @type {HTMLDivElement}    */ (document.getElementById('log-container'));
    const logRows        = /** @type {HTMLDivElement}    */ (document.getElementById('log-rows'));
    const logSpacerTop   = /** @type {HTMLDivElement}    */ (document.getElementById('log-spacer-top'));
    const logSpacerBot   = /** @type {HTMLDivElement}    */ (document.getElementById('log-spacer-bottom'));
    const optDelta       = /** @type {HTMLInputElement}  */ (document.getElementById('opt-delta'));
    const optFollow      = /** @type {HTMLInputElement}  */ (document.getElementById('opt-follow'));
    const optWrap        = /** @type {HTMLInputElement}  */ (document.getElementById('opt-wrap'));
    const btnPrevError   = /** @type {HTMLButtonElement} */ (document.getElementById('btn-prev-error'));
    const btnNextError   = /** @type {HTMLButtonElement} */ (document.getElementById('btn-next-error'));
    const btnExport      = /** @type {HTMLButtonElement} */ (document.getElementById('btn-export'));

    // ── Helpers ──────────────────────────────────────────────────────
    /** @param {string} ch */
    function channelClass(ch) {
        const known = ['FAIL','CLI','SQL','REPO','YAML','CORE','TEST','CONF','WORK'];
        return known.includes(ch) ? 'ch-' + ch : '';
    }
    /** @param {string} ch */
    function subchannelClass(ch) {
        const known = ['FAIL','CLI','SQL','REPO','YAML','CORE','TEST','CONF','WORK'];
        return known.includes(ch) ? 'sub-' + ch : '';
    }
    /** @param {string} lv - level char V/I/W/E/C */
    function levelClass(lv) { return 'lv-' + lv; }

    const LEVEL_LABELS = { V: 'Verbose', I: 'Info', W: 'Warning', E: 'Error', C: 'Crit' };

    /**
     * Format a millisecond delta into a compact string.
     * @param {number} ms
     */
    function formatDelta(ms) {
        if (ms < 0)      { return ''; }
        if (ms < 1000)   { return '+' + ms + 'ms'; }
        if (ms < 60000)  { return '+' + (ms / 1000).toFixed(2) + 's'; }
        return '+' + (ms / 60000).toFixed(1) + 'm';
    }

    /**
     * Parse the timestamp string "YYYY-MM-DD HH:MM:SS.mmm" into a Date ms value.
     * Also handles legacy 2-digit year "YY-MM-DD ...".
     * Returns NaN on failure.
     * @param {string} ts
     */
    function parseTimestampMs(ts) {
        // "2026-04-14 22:39:42.041" (4-digit year) or "26-04-14 ..." (2-digit year)
        const m = ts.match(/^(\d{2,4})-(\d{2})-(\d{2}) (\d{2}):(\d{2}):(\d{2})\.(\d{3})$/);
        if (!m) { return NaN; }
        const [, yr, mo, dd, hh, mm, ss, ms] = m.map(Number);
        const fullYear = yr < 100 ? 2000 + yr : yr;
        return Date.UTC(fullYear, mo - 1, dd, hh, mm, ss, ms);
    }

    /** Escape HTML special characters. @param {string} s */
    function esc(s) {
        return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
    }

    const KW_RE = /(Error|Fail(?:ed|ure)?|Exception|HRESULT|Crit(?:ical)?)|(Warn(?:ing)?|Deprecated)|(0x[0-9a-fA-F]{1,8})/gi;

    /** Annotate error/warning/hex keywords in message HTML. @param {string} s */
    function annotateMessage(s) {
        return esc(s).replace(KW_RE, (_, err, warn, hex) => {
            if (err)  { return '<span class="kw-error">'   + esc(err)  + '</span>'; }
            if (warn) { return '<span class="kw-warning">' + esc(warn) + '</span>'; }
            if (hex)  { return '<span class="kw-hex">'     + esc(hex)  + '</span>'; }
            return _;
        });
    }

    /**
     * Build innerHTML for a message, applying keyword annotation and search highlighting.
     * Search matches are wrapped in <mark class="search-match">.
     * @param {string} raw  - raw (unescaped) message text
     * @param {boolean} [keywords] - whether to apply keyword annotation (default true)
     */
    function buildMessageHtml(raw, keywords = true) {
        if (!searchText) {
            return keywords ? annotateMessage(raw) : esc(raw);
        }
        const lower      = raw.toLowerCase();
        const needle     = searchText.toLowerCase();
        const needleLen  = needle.length;
        let result = '';
        let pos    = 0;
        let idx;
        while ((idx = lower.indexOf(needle, pos)) !== -1) {
            const before = raw.slice(pos, idx);
            const match  = raw.slice(idx, idx + needleLen);
            result += keywords ? annotateMessage(before) : esc(before);
            result += '<mark class="search-match">' + esc(match) + '</mark>';
            pos = idx + needleLen;
        }
        const after = raw.slice(pos);
        result += keywords ? annotateMessage(after) : esc(after);
        return result;
    }

    // ── Filter logic ─────────────────────────────────────────────────
    /** Flatten visibleEntries (including continuation lines) into uniform display rows. */
    function buildDisplayRows() {
        displayRows = [];
        let prevTimestampMs = NaN;
        for (const entry of visibleEntries) {
            const curMs = parseTimestampMs(entry.timestamp);
            const delta = (!isNaN(curMs) && !isNaN(prevTimestampMs)) ? curMs - prevTimestampMs : -1;
            if (!isNaN(curMs)) { prevTimestampMs = curMs; }

            displayRows.push({
                timestamp: entry.timestamp,
                levelChar: entry.levelChar ?? null,
                channel: entry.channel,
                subchannel: entry.subchannel ?? null,
                message: entry.message,
                severity: entry.severity,
                isContinuation: false,
                deltaMs: delta
            });

            for (const cl of (entry.continuationLines || [])) {
                displayRows.push({
                    timestamp: entry.timestamp,
                    levelChar: entry.levelChar ?? null,
                    channel: entry.channel,
                    subchannel: entry.subchannel ?? null,
                    message: cl,
                    severity: entry.severity,
                    isContinuation: true,
                    deltaMs: -1
                });
            }
        }
    }

    function applyFilters() {
        const lowerSearch = searchText.toLowerCase();

        visibleEntries = allEntries.filter(e => {
            if (!enabledChannels.has(e.channel))                          { return false; }
            if (e.subchannel && !enabledSubchannels.has(e.subchannel))    { return false; }
            // Level filter only applies when the file has explicit markers and the entry has one.
            if (fileHasLevels && e.levelChar && !enabledLevels.has(e.levelChar)) { return false; }
            if (lowerSearch && !e.message.toLowerCase().includes(lowerSearch) &&
                               !e.channel.toLowerCase().includes(lowerSearch)) {
                return false;
            }
            return true;
        });

        updateStats();
        buildDisplayRows();
        renderVirtual(true);
    }

    function updateStats() {
        statsText.textContent = visibleEntries.length + ' visible / ' + allEntries.length + ' total lines';
    }

    // ── Sidebar builder ──────────────────────────────────────────────
    /**
     * Wire a filter-item <li> so clicking anywhere on it (except the checkbox itself) toggles the checkbox.
     * @param {HTMLLIElement} li
     * @param {HTMLInputElement} cb
     */
    function wireFilterItem(li, cb) {
        li.addEventListener('click', e => {
            if (e.target === cb) { return; }
            cb.checked = !cb.checked;
            cb.dispatchEvent(new Event('change'));
        });
    }

    /**
     * @param {string[]} channels
     * @param {Map<string,number>} counts
     */
    function buildChannelList(channels, counts) {
        channelList.innerHTML = '';
        for (const ch of channels) {
            const count = counts.get(ch) ?? 0;
            const li    = document.createElement('li');
            li.className = 'filter-item';

            const cb = document.createElement('input');
            cb.type    = 'checkbox';
            cb.checked = enabledChannels.has(ch);
            cb.addEventListener('change', () => {
                if (cb.checked) { enabledChannels.add(ch); }
                else            { enabledChannels.delete(ch); }
                applyFilters();
            });

            const badge = document.createElement('span');
            badge.className = 'filter-badge ' + channelClass(ch);
            badge.textContent = ch;

            const cnt = document.createElement('span');
            cnt.className   = 'filter-count';
            cnt.textContent = String(count);

            li.appendChild(cb);
            li.appendChild(badge);
            li.appendChild(cnt);
            wireFilterItem(li, cb);
            channelList.appendChild(li);
        }
    }

    /**
     * @param {string[]} subchannels
     * @param {Map<string,number>} counts
     */
    function buildSubchannelList(subchannels, counts) {
        if (subchannels.length === 0) {
            subchannelSec.style.display = 'none';
            return;
        }
        subchannelSec.style.display = '';
        subchannelList.innerHTML = '';
        for (const sc of subchannels) {
            const count = counts.get(sc) ?? 0;
            const li    = document.createElement('li');
            li.className = 'filter-item';

            const cb = document.createElement('input');
            cb.type    = 'checkbox';
            cb.checked = enabledSubchannels.has(sc);
            cb.addEventListener('change', () => {
                if (cb.checked) { enabledSubchannels.add(sc); }
                else            { enabledSubchannels.delete(sc); }
                applyFilters();
            });

            const badge = document.createElement('span');
            badge.className   = 'filter-badge ' + subchannelClass(sc);
            badge.textContent = sc;
            badge.style.border = '1px solid currentColor';

            const cnt = document.createElement('span');
            cnt.className   = 'filter-count';
            cnt.textContent = String(count);

            li.appendChild(cb);
            li.appendChild(badge);
            li.appendChild(cnt);
            wireFilterItem(li, cb);
            subchannelList.appendChild(li);
        }
    }

    /**
     * Build the Levels filter section. Hidden when the file has no explicit level markers.
     * @param {Map<string,number>} counts - map of levelChar → count
     */
    function buildLevelList(counts) {
        if (!fileHasLevels) {
            levelSec.style.display = 'none';
            return;
        }
        levelSec.style.display = '';
        levelList.innerHTML = '';
        const order = ['V','I','W','E','C'];
        for (const lv of order) {
            const count = counts.get(lv) ?? 0;
            if (count === 0) { continue; }

            const li = document.createElement('li');
            li.className = 'filter-item';

            const cb = document.createElement('input');
            cb.type    = 'checkbox';
            cb.checked = enabledLevels.has(lv);
            cb.addEventListener('change', () => {
                if (cb.checked) { enabledLevels.add(lv); }
                else            { enabledLevels.delete(lv); }
                applyFilters();
            });

            const badge = document.createElement('span');
            badge.className   = 'filter-badge log-level ' + levelClass(lv);
            badge.textContent = lv;

            const label = document.createElement('span');
            label.textContent = LEVEL_LABELS[lv] ?? lv;

            const cnt = document.createElement('span');
            cnt.className   = 'filter-count';
            cnt.textContent = String(count);

            li.appendChild(cb);
            li.appendChild(badge);
            li.appendChild(label);
            li.appendChild(cnt);
            wireFilterItem(li, cb);
            levelList.appendChild(li);
        }
    }

    // ── Virtual scroll renderer ───────────────────────────────────────
    let scheduledRender = false;

    /**
     * Render only the rows currently visible in the viewport.
     * @param {boolean} [reset] Reset scroll position to top (or bottom if follow mode).
     */
    function renderVirtual(reset = false) {
        const total = displayRows.length;

        if (reset) {
            logContainer.scrollTop = followMode ? total * ROW_HEIGHT : 0;
        }
        scrollTop   = logContainer.scrollTop;
        containerH  = logContainer.clientHeight;

        const firstVisible = Math.max(0, Math.floor(scrollTop / ROW_HEIGHT) - renderBuffer);
        const lastVisible  = Math.min(total - 1, Math.ceil((scrollTop + containerH) / ROW_HEIGHT) + renderBuffer);

        logSpacerTop.style.height = (firstVisible * ROW_HEIGHT) + 'px';
        logSpacerBot.style.height = (Math.max(0, total - 1 - lastVisible) * ROW_HEIGHT) + 'px';

        const fragment = document.createDocumentFragment();
        for (let i = firstVisible; i <= lastVisible; i++) {
            fragment.appendChild(buildRow(i));
        }
        logRows.innerHTML = '';
        logRows.appendChild(fragment);
    }

    /**
     * Build a single log row element.
     * @param {number} idx Index into displayRows.
     */
    function buildRow(idx) {
        const entry = displayRows[idx];
        const row   = document.createElement('div');
        const contClass = entry.isContinuation ? ' is-continuation' : '';
        row.className = 'log-row sev-' + entry.severity + contClass + (wrapLines ? ' wrap-lines' : '');
        row.dataset.idx = String(idx);

        // Continuation rows show only the raw message text; no prefix columns.
        if (entry.isContinuation) {
            const msg = document.createElement('span');
            msg.className = 'log-message';
            msg.innerHTML = buildMessageHtml(entry.message, false);
            row.appendChild(msg);
            return row;
        }

        // Timestamp
        const ts = document.createElement('span');
        ts.className   = 'log-timestamp';
        ts.textContent = entry.timestamp;
        row.appendChild(ts);

        // Time delta (primary rows only)
        if (showDelta && !entry.isContinuation) {
            const delta = document.createElement('span');
            delta.className = 'log-delta';
            if (entry.deltaMs >= 0) {
                delta.textContent = formatDelta(entry.deltaMs);
                if (entry.deltaMs >= 60000)     { delta.classList.add('delta-very-slow'); }
                else if (entry.deltaMs >= 1000) { delta.classList.add('delta-slow'); }
            }
            row.appendChild(delta);
        }

        // Level badge (only when explicit marker present)
        if (entry.levelChar) {
            const lv = document.createElement('span');
            lv.className   = 'log-level ' + levelClass(entry.levelChar);
            lv.textContent = entry.levelChar;
            row.appendChild(lv);
        }

        // Channel badge
        const ch = document.createElement('span');
        ch.className   = 'log-channel ' + channelClass(entry.channel);
        ch.textContent = entry.channel;
        row.appendChild(ch);

        // Optional subchannel badge
        if (entry.subchannel) {
            const sc = document.createElement('span');
            sc.className   = 'log-subchannel ' + subchannelClass(entry.subchannel);
            sc.textContent = entry.subchannel;
            row.appendChild(sc);
        }

        // Message
        const msg = document.createElement('span');
        msg.className = 'log-message';
        msg.innerHTML = buildMessageHtml(entry.message);
        row.appendChild(msg);

        return row;
    }

    // ── Scroll handler (virtual scroll repaint) ───────────────────────
    let scrollRaf = 0;
    logContainer.addEventListener('scroll', () => {
        if (scrollRaf) { return; }
        scrollRaf = requestAnimationFrame(() => {
            scrollRaf = 0;
            renderVirtual(false);
        });
    });

    // Resize
    const resizeObs = new ResizeObserver(() => {
        if (!scheduledRender) {
            scheduledRender = true;
            requestAnimationFrame(() => {
                scheduledRender = false;
                renderVirtual(false);
            });
        }
    });
    resizeObs.observe(logContainer);

    // ── Error navigation ─────────────────────────────────────────────
    /** @param {'next'|'prev'} dir */
    function jumpToError(dir) {
        const errors = displayRows
            .map((e, i) => ({ i, sev: e.severity, isCont: e.isContinuation }))
            .filter(x => !x.isCont && (x.sev === 'error' || x.sev === 'crit'));
        if (errors.length === 0) { return; }

        let target;
        if (dir === 'next') {
            target = errors.find(x => x.i > lastErrorJumpIndex) ?? errors[0];
        } else {
            const rev = [...errors].reverse();
            target = rev.find(x => x.i < lastErrorJumpIndex) ?? errors[errors.length - 1];
        }
        lastErrorJumpIndex = target.i;
        logContainer.scrollTop = target.i * ROW_HEIGHT - containerH / 2;
        renderVirtual(false);
    }

    btnNextError.addEventListener('click', () => jumpToError('next'));
    btnPrevError.addEventListener('click', () => jumpToError('prev'));

    // ── Option toggles ────────────────────────────────────────────────
    optDelta.addEventListener('change', () => {
        showDelta = optDelta.checked;
        renderVirtual(false);
    });
    optFollow.addEventListener('change', () => {
        followMode = optFollow.checked;
        if (followMode) {
            logContainer.scrollTop = displayRows.length * ROW_HEIGHT;
        }
    });
    optWrap.addEventListener('change', () => {
        wrapLines = optWrap.checked;
        renderVirtual(false);
    });

    // ── Search ────────────────────────────────────────────────────────
    let searchTimer = 0;
    searchInput.addEventListener('input', () => {
        clearTimeout(searchTimer);
        searchTimer = setTimeout(() => {
            searchText = searchInput.value;
            applyFilters();
        }, 150);
    });

    // ── Level / Channel / Subchannel "All / None" buttons ────────────
    document.getElementById('levels-all')?.addEventListener('click', () => {
        for (const cb of /** @type {NodeListOf<HTMLInputElement>} */ (levelList.querySelectorAll('input[type=checkbox]'))) {
            cb.checked = true;
        }
        enabledLevels = new Set(['V','I','W','E','C']);
        applyFilters();
    });
    document.getElementById('levels-none')?.addEventListener('click', () => {
        for (const cb of /** @type {NodeListOf<HTMLInputElement>} */ (levelList.querySelectorAll('input[type=checkbox]'))) {
            cb.checked = false;
        }
        enabledLevels = new Set();
        applyFilters();
    });
    document.getElementById('channels-all')?.addEventListener('click', () => {
        for (const cb of /** @type {NodeListOf<HTMLInputElement>} */ (channelList.querySelectorAll('input[type=checkbox]'))) {
            cb.checked = true;
        }
        enabledChannels = new Set(allEntries.map(e => e.channel));
        applyFilters();
    });
    document.getElementById('channels-none')?.addEventListener('click', () => {
        for (const cb of /** @type {NodeListOf<HTMLInputElement>} */ (channelList.querySelectorAll('input[type=checkbox]'))) {
            cb.checked = false;
        }
        enabledChannels = new Set();
        applyFilters();
    });
    document.getElementById('subchannels-all')?.addEventListener('click', () => {
        for (const cb of /** @type {NodeListOf<HTMLInputElement>} */ (subchannelList.querySelectorAll('input[type=checkbox]'))) {
            cb.checked = true;
        }
        enabledSubchannels = new Set(allEntries.filter(e => e.subchannel).map(e => /** @type {string} */ (e.subchannel)));
        applyFilters();
    });
    document.getElementById('subchannels-none')?.addEventListener('click', () => {
        for (const cb of /** @type {NodeListOf<HTMLInputElement>} */ (subchannelList.querySelectorAll('input[type=checkbox]'))) {
            cb.checked = false;
        }
        enabledSubchannels = new Set();
        applyFilters();
    });

    // ── Export ────────────────────────────────────────────────────────
    btnExport.addEventListener('click', () => {
        const lines = [];
        for (const e of visibleEntries) {
            lines.push(e.raw);
            if (e.continuationLines) { lines.push(...e.continuationLines); }
        }
        vscode.postMessage({ type: 'export', text: lines.join('\n') });
    });

    // ── Message handler (from extension host) ─────────────────────────
    window.addEventListener('message', event => {
        const msg = event.data;
        switch (msg.type) {
            case 'load': {
                handleLoad(msg);
                break;
            }
            case 'update': {
                handleUpdate(msg);
                break;
            }
        }
    });

    /**
     * Full initial load (or reload after file change in non-follow mode).
     * @param {{ entries: any[], channels: string[], subchannels: string[], hasLevels: boolean }} msg
     */
    function handleLoad(msg) {
        allEntries    = msg.entries;
        fileHasLevels = msg.hasLevels;

        /** @type {Map<string,number>} */
        const chCounts  = new Map();
        /** @type {Map<string,number>} */
        const subCounts = new Map();
        /** @type {Map<string,number>} */
        const lvCounts  = new Map();
        for (const e of allEntries) {
            chCounts.set(e.channel, (chCounts.get(e.channel) ?? 0) + 1);
            if (e.subchannel) { subCounts.set(e.subchannel, (subCounts.get(e.subchannel) ?? 0) + 1); }
            if (e.levelChar)  { lvCounts.set(e.levelChar,   (lvCounts.get(e.levelChar)   ?? 0) + 1); }
        }

        // First load: enable everything
        enabledChannels    = new Set(msg.channels);
        enabledSubchannels = new Set(msg.subchannels);
        enabledLevels      = new Set(['V','I','W','E','C']);

        buildLevelList(lvCounts);
        buildChannelList(msg.channels, chCounts);
        buildSubchannelList(msg.subchannels, subCounts);
        applyFilters();
        statusIndicator.textContent = '';
    }


    /**
     * Incremental update (follow mode — new lines appended).
     * @param {{ entries: any[] }} msg
     */
    function handleUpdate(msg) {
        const prevLen = allEntries.length;
        allEntries = msg.entries;

        const newChannels    = new Set(allEntries.map(e => e.channel));
        const newSubchannels = new Set(allEntries.filter(e => e.subchannel).map(e => /** @type {string} */(e.subchannel)));
        const newLevels      = new Set(allEntries.filter(e => e.levelChar).map(e => /** @type {string} */(e.levelChar)));

        let needsRebuild = false;
        for (const ch of newChannels)    { if (!enabledChannels.has(ch))    { enabledChannels.add(ch);    needsRebuild = true; } }
        for (const sc of newSubchannels) { if (!enabledSubchannels.has(sc)) { enabledSubchannels.add(sc); needsRebuild = true; } }
        for (const lv of newLevels)      { if (!enabledLevels.has(lv))      { enabledLevels.add(lv);      needsRebuild = true; } }

        if (needsRebuild) {
            /** @type {Map<string,number>} */
            const chCounts  = new Map();
            /** @type {Map<string,number>} */
            const subCounts = new Map();
            /** @type {Map<string,number>} */
            const lvCounts  = new Map();
            for (const e of allEntries) {
                chCounts.set(e.channel, (chCounts.get(e.channel) ?? 0) + 1);
                if (e.subchannel) { subCounts.set(e.subchannel, (subCounts.get(e.subchannel) ?? 0) + 1); }
                if (e.levelChar)  { lvCounts.set(e.levelChar,   (lvCounts.get(e.levelChar)   ?? 0) + 1); }
            }
            buildLevelList(lvCounts);
            buildChannelList([...newChannels], chCounts);
            buildSubchannelList([...newSubchannels], subCounts);
        }

        applyFilters();

        if (followMode) {
            logContainer.scrollTop = displayRows.length * ROW_HEIGHT;
            renderVirtual(false);
        }

        const added = allEntries.length - prevLen;
        if (added > 0) {
            statusIndicator.textContent = '+' + added + ' new';
            setTimeout(() => { statusIndicator.textContent = ''; }, 3000);
        }
    }

    // Signal to the extension host that the webview is ready to receive data.
    vscode.postMessage({ type: 'ready' });
}());