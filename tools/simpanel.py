#!/usr/bin/env python3
# ==================================================================================================
# @file   simpanel.py
#
# @author Daniel Zorychta
#
# @brief  Board panel of the POSIX simulator: visualization of the board model in a web browser
#         (LEDs, buttons, switches, values of device models, etc.).
#
#         The simulator started with options --board <script> --panel <socket> creates the Unix
#         domain socket of the panel. This program connects to the socket (and reconnects after
#         simulator restart) and serves the panel pages over HTTP. Signals of the panel are defined
#         by the board script (panel.output(), panel.input(), panel.value(), panel.control()), the
#         look of the panel is defined by HTML pages in sim-env/panel/<page>/ (the page name is
#         selected by panel.page() in the board script, default: board script name). Board without
#         its own page is shown by the generic page sim-env/panel/default/.
#
#         Pages use the script /panel.js served by this program:
#           - attributes: data-signal="<id>" (shows value: class "on", attribute data-value,
#             data-on/data-off images, data-format text), data-input="<id>" (controls: buttons,
#             checkboxes, sliders, fields), <canvas data-lcd="<id>"> (LCD and its touch: LTDC driver
#             "ltdc" or LCD of board model, panel.lcd() of board script),
#           - API: panel.on(id, fn), panel.set(id, value), panel.get(id), panel.onInit(fn),
#             panel.onStatus(fn), panel.onLcd(fn), panel.touch(lcd_id, x, y, pressed),
#             panel.format(fmt, value), panel.bind(root).
#
#         Protocol of the simulator socket (one message per line):
#           simulator -> panel: JSON objects {"t": "init", "page": ..., "board": ..., "signals": [...],
#                               "state": {...}}, {"t": "state", "state": {...}}, {"t": "lcd", "id": ...,
#                               "w": ..., "h": ..., "fmt": ..., "bg": [r, g, b], "y": ..., "n": ...,
#                               "data": ...} (changed lines of LCD framebuffer, base64, "w": 0: LCD is not
#                               active; id "ltdc": LTDC driver, other: LCDs of board models)
#           panel -> simulator: set <id> <type><value>, type: b (0/1), n (number), s (string),
#                               touch <x> <y> <0|1> <lcd id> (LCD touched/released)
#
#         Usage:
#           tools/simpanel.py [--socket build/dnx-panel.sock] [--dir sim-env/panel] [--port 8080]
#                             [--bind 127.0.0.1] [--page <name>]
#
#         HTTP interface: / (panel page), /p/<page>/... (page files), /panel.js, /events (server
#         sent events), /api/state (GET, JSON), /api/set (POST, JSON {"id": ..., "value": ...}),
#         /api/touch (POST, JSON {"x": ..., "y": ..., "pressed": true|false, "lcd": <id>}).
#
#         MIT License
#
#         Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#         Permission is hereby granted, free of charge, to any person obtaining a copy of this
#         software and associated documentation files (the "Software"), to deal in the Software
#         without restriction, including without limitation the rights to use, copy, modify,
#         merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
#         persons to whom the Software is furnished to do so, subject to the following conditions:
#
#         The above copyright notice and this permission notice shall be included in all copies or
#         substantial portions of the Software.
#
#         THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
#         INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
#         PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
#         FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
#         OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#         DEALINGS IN THE SOFTWARE.
# ==================================================================================================

import argparse
import base64
import json
import mimetypes
import os
import queue
import re
import socket
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, unquote

ID_RE = re.compile(r"^[A-Za-z0-9_.\-]+$")
RECONNECT_DELAY = 0.5   # s
QUEUE_SIZE = 100        # max messages waiting for browser (LCD frames can be large)
PIXEL_SIZE = {"argb8888": 4, "rgb888": 3, "rgb565": 2, "argb1555": 2, "argb4444": 2, "l8": 1, "al44": 1,
              "al88": 2}
KEEPALIVE = 15          # s, SSE comment sent to detect closed browser connections

# ==================================================================================================
# Client script of panel pages (/panel.js)
# ==================================================================================================
PANEL_JS = r"""
/* dnx RTOS simulator panel client (served by tools/simpanel.py) */
(function () {
    "use strict";

    const panel = {
        signals: [],        // [{id, kind, ui}] defined by board script
        state: {},          // id -> value
        connected: false,   // simulator connected
        page: null,         // page selected by board script
        board: null,        // board script
    };

    const listeners = [];   // {id, fn, el}
    const initListeners = [];
    const statusListeners = [];

    function safe(fn, ...args) {
        try { fn(...args); } catch (e) { console.error(e); }
    }

    function emit(id, value) {
        for (let i = listeners.length - 1; i >= 0; i--) {
            const l = listeners[i];
            if (l.el && !l.el.isConnected) { listeners.splice(i, 1); continue; }
        }
        for (const l of listeners.slice()) {
            if (l.id === id || l.id === "*") safe(l.fn, value, id);
        }
    }

    // value is "active" (class "on")
    function truthy(v) {
        if (typeof v === "string") return v !== "" && v !== "0" && v !== "false";
        return !!v;
    }
    panel.truthy = truthy;

    // call fn(value, id) when value of signal is changed (id "*": all signals), fn is called
    // immediately with the current value; el: listener is removed when element is removed
    panel.on = function (id, fn, el) {
        const l = { id: id, fn: fn, el: el || null };
        listeners.push(l);
        if (id === "*") {
            for (const k in panel.state) safe(fn, panel.state[k], k);
        } else if (id in panel.state) {
            safe(fn, panel.state[id], id);
        }
        return function () { const i = listeners.indexOf(l); if (i >= 0) listeners.splice(i, 1); };
    };

    // call fn(signals) when board definition is received (also after simulator restart)
    panel.onInit = function (fn) {
        initListeners.push(fn);
        if (panel.page !== null) safe(fn, panel.signals);
    };

    // call fn(connected) when connection with simulator is changed
    panel.onStatus = function (fn) {
        statusListeners.push(fn);
        safe(fn, panel.connected);
    };

    panel.get = function (id) { return panel.state[id]; };

    // requests are sent in order (e.g. press and release of button)
    let requests = Promise.resolve();
    function post(url, data) {
        requests = requests.then(function () {
            return fetch(url, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data),
            });
        }).catch(function (e) { console.error(e); });
        return requests;
    }

    // send value of control to the board (panel.input()/panel.control() of board script)
    panel.set = function (id, value) {
        return post("/api/set", { id: id, value: value });
    };

    // LCD ---------------------------------------------------------------------------------------------
    // LCDs are identified by id (LTDC driver: "ltdc", LCDs of board models: panel.lcd() of board
    // script), all LCDs are handled the same way, <canvas data-lcd="<id>"> shows LCD <id>
    const lcdListeners = [];
    const lcdCanvases = [];     // [{el, id}]
    const lcdImages = {};       // id -> whole frame (ImageData)
    panel.lcds = {};            // id -> {id, w, h, fmt, bg} (active LCDs)

    // call fn(lcd, id) when LCD is changed, lcd: {id, w, h, fmt, bg} or null (not active), called
    // immediately for all active LCDs
    panel.onLcd = function (fn) {
        lcdListeners.push(fn);
        for (const id in panel.lcds) safe(fn, panel.lcds[id], id);
    };

    // touch of LCD id (x, y in pixels of LCD, pressed: false is release)
    panel.touch = function (id, x, y, pressed) {
        return post("/api/touch", { lcd: String(id), x: x, y: y, pressed: !!pressed });
    };

    // convert pixels of LTDC format to RGBA (alpha is blended with background color)
    function decode(fmt, s, bg, d) {
        const n = d.length / 4;
        const put = function (i, r, g, b, a) {
            const o = i * 4;
            d[o] = (r * a + bg[0] * (255 - a)) / 255;
            d[o + 1] = (g * a + bg[1] * (255 - a)) / 255;
            d[o + 2] = (b * a + bg[2] * (255 - a)) / 255;
            d[o + 3] = 255;
        };
        let v;
        for (let i = 0; i < n; i++) {
            switch (fmt) {
            case "argb8888": put(i, s[i * 4 + 2], s[i * 4 + 1], s[i * 4], s[i * 4 + 3]); break;
            case "rgb888": put(i, s[i * 3 + 2], s[i * 3 + 1], s[i * 3], 255); break;
            case "rgb565":
                v = s[i * 2] | (s[i * 2 + 1] << 8);
                put(i, ((v >> 11) & 31) * 255 / 31, ((v >> 5) & 63) * 255 / 63, (v & 31) * 255 / 31, 255);
                break;
            case "argb1555":
                v = s[i * 2] | (s[i * 2 + 1] << 8);
                put(i, ((v >> 10) & 31) * 255 / 31, ((v >> 5) & 31) * 255 / 31, (v & 31) * 255 / 31,
                    (v >> 15) ? 255 : 0);
                break;
            case "argb4444":
                v = s[i * 2] | (s[i * 2 + 1] << 8);
                put(i, ((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, (v & 15) * 17, (v >> 12) * 17);
                break;
            case "l8": put(i, s[i], s[i], s[i], 255); break;       // without CLUT: gray scale
            case "al44": v = (s[i] & 15) * 17; put(i, v, v, v, (s[i] >> 4) * 17); break;
            case "al88": put(i, s[i * 2], s[i * 2], s[i * 2], s[i * 2 + 1]); break;
            }
        }
    }

    function drawLcd(canvas, id, y, n) {
        const img = lcdImages[id];
        if (!img) {
            canvas.getContext("2d").clearRect(0, 0, canvas.width, canvas.height);
            return;
        }
        if (canvas.width !== img.width || canvas.height !== img.height) {
            canvas.width = img.width;
            canvas.height = img.height;
            y = 0;
            n = img.height;
        }
        canvas.getContext("2d").putImageData(img, 0, 0, 0, y, img.width, n);
    }

    function lcdMessage(msg) {
        const id = String(msg.id);
        const old = panel.lcds[id] || null;
        let lcd = null;
        if (!msg.w || !msg.h) {
            delete panel.lcds[id];
            delete lcdImages[id];
        } else {
            lcd = panel.lcds[id] = { id: id, w: msg.w, h: msg.h, fmt: msg.fmt, bg: msg.bg || [0, 0, 0] };
            let img = lcdImages[id];
            if (!img || img.width !== msg.w || img.height !== msg.h) {
                img = lcdImages[id] = new ImageData(msg.w, msg.h);
            }
            if (msg.data) {
                const bin = atob(msg.data);
                const src = new Uint8Array(bin.length);
                for (let i = 0; i < bin.length; i++) src[i] = bin.charCodeAt(i);
                const off = msg.y * msg.w * 4;
                decode(msg.fmt, src, lcd.bg, img.data.subarray(off, off + msg.n * msg.w * 4));
            }
        }
        for (let i = lcdCanvases.length - 1; i >= 0; i--) {
            const c = lcdCanvases[i];
            if (!c.el.isConnected) { lcdCanvases.splice(i, 1); continue; }
            if (c.id === id) drawLcd(c.el, id, msg.y || 0, msg.n || 0);
        }
        if (JSON.stringify(old) !== JSON.stringify(lcd)) {
            for (const fn of lcdListeners) safe(fn, lcd, id);
        }
    }

    // LCDs of init message: LCDs that are not active are removed
    function lcdInit(lcds) {
        for (const id of Object.keys(panel.lcds)) {
            if (!lcds || !lcds[id]) lcdMessage({ id: id, w: 0, h: 0 });
        }
    }

    // <canvas data-lcd="<id>">: LCD image, pointer is the touch of LCD
    function bindLcd(el) {
        const id = el.dataset.lcd;
        if (!id) { console.warn("panel: <canvas data-lcd> without LCD id", el); return; }
        lcdCanvases.push({ el: el, id: id });
        el.style.touchAction = "none";
        if (!el.style.imageRendering) el.style.imageRendering = "pixelated";
        const img = lcdImages[id];
        drawLcd(el, id, 0, img ? img.height : 0);

        let pressed = false, pos = [0, 0], pending = false, timer = null;
        const point = function (e) {
            // image area of canvas (without border and padding, e.g. bezel of LCD)
            const r = el.getBoundingClientRect(), s = getComputedStyle(el);
            const px = function (v) { return parseFloat(v) || 0; };
            const left = r.left + px(s.borderLeftWidth) + px(s.paddingLeft);
            const top = r.top + px(s.borderTopWidth) + px(s.paddingTop);
            const w = r.width - px(s.borderLeftWidth) - px(s.paddingLeft) - px(s.paddingRight) - px(s.borderRightWidth);
            const h = r.height - px(s.borderTopWidth) - px(s.paddingTop) - px(s.paddingBottom) - px(s.borderBottomWidth);
            return [Math.min(el.width - 1, Math.max(0, Math.floor((e.clientX - left) * el.width / w))),
                    Math.min(el.height - 1, Math.max(0, Math.floor((e.clientY - top) * el.height / h)))];
        };
        const release = function () {
            if (pressed) { pressed = false; panel.touch(id, pos[0], pos[1], false); }
        };
        el.addEventListener("pointerdown", function (e) {
            if (e.button !== 0 || !panel.lcds[id]) return;
            pressed = true;
            pos = point(e);
            try { el.setPointerCapture(e.pointerId); } catch (_) { }
            panel.touch(id, pos[0], pos[1], true);
            e.preventDefault();
        });
        el.addEventListener("pointermove", function (e) {
            if (!pressed) return;
            pos = point(e);
            if (timer) { pending = true; return; }
            panel.touch(id, pos[0], pos[1], true);
            timer = setTimeout(function tick() {
                if (pending && pressed) { pending = false; panel.touch(id, pos[0], pos[1], true); timer = setTimeout(tick, 30); }
                else timer = null;
            }, 30);
        });
        el.addEventListener("pointerup", release);
        el.addEventListener("pointercancel", release);
        el.addEventListener("lostpointercapture", release);
    }

    // printf-like formatting: %d %i %f %.Nf %e %x %X %o %b %s %% (flags: - 0 +, width)
    panel.format = function (fmt, v) {
        return String(fmt).replace(/%([-0+]*)(\d*)(?:\.(\d+))?([difeExXobs%])/g,
            function (m, flags, width, prec, conv) {
                if (conv === "%") return "%";
                const n = Number(v);
                let s;
                switch (conv) {
                case "d": case "i": s = String(Math.trunc(n)); break;
                case "f": s = n.toFixed(prec !== undefined ? +prec : 6); break;
                case "e": case "E": s = n.toExponential(prec !== undefined ? +prec : 6);
                          if (conv === "E") s = s.toUpperCase(); break;
                case "x": s = (Math.trunc(n) >>> 0).toString(16); break;
                case "X": s = (Math.trunc(n) >>> 0).toString(16).toUpperCase(); break;
                case "o": s = (Math.trunc(n) >>> 0).toString(8); break;
                case "b": s = (Math.trunc(n) >>> 0).toString(2); break;
                default:  s = typeof v === "number" && prec !== undefined ? v.toFixed(+prec) : String(v);
                }
                if (flags.includes("+") && "difeE".includes(conv) && n >= 0) s = "+" + s;
                const w = +width || 0;
                if (s.length < w) {
                    if (flags.includes("-")) s = s.padEnd(w);
                    else if (flags.includes("0") && conv !== "s") {
                        const sign = /^[+-]/.test(s) ? s[0] : "";
                        s = sign + s.slice(sign.length).padStart(w - sign.length, "0");
                    } else s = s.padStart(w);
                }
                return s;
            });
    };

    function show(el, v) {
        el.dataset.value = String(v);
        el.classList.toggle("on", truthy(v));
        if (el.tagName === "IMG" && (el.dataset.on || el.dataset.off)) {
            const src = truthy(v) ? el.dataset.on : el.dataset.off;
            if (src && el.getAttribute("src") !== src) el.setAttribute("src", src);
        }
        if (el.dataset.format !== undefined) {
            el.textContent = el.dataset.format ? panel.format(el.dataset.format, v) : String(v);
        }
    }

    function bindSignal(el) {
        panel.on(el.dataset.signal, function (v) { show(el, v); }, el);
    }

    function bindInput(el) {
        const id = el.dataset.input;
        const tag = el.tagName;
        const type = (el.type || "").toLowerCase();

        if (tag === "INPUT" && type === "checkbox") {
            el.addEventListener("change", function () { panel.set(id, el.checked); });
            panel.on(id, function (v) { el.checked = truthy(v); show(el, v); }, el);

        } else if ((tag === "INPUT" && type !== "button" && type !== "submit") || tag === "SELECT"
                   || tag === "TEXTAREA") {
            const numeric = type === "range" || type === "number" || el.dataset.type === "number";
            const value = function () { return numeric ? Number(el.value) : el.value; };
            if (type === "range") {
                // limited rate of slider updates
                let timer = null, pending = false;
                el.addEventListener("input", function () {
                    if (timer) { pending = true; return; }
                    panel.set(id, value());
                    timer = setTimeout(function tick() {
                        if (pending) { pending = false; panel.set(id, value()); timer = setTimeout(tick, 50); }
                        else timer = null;
                    }, 50);
                });
            } else {
                el.addEventListener("change", function () { panel.set(id, value()); });
            }
            panel.on(id, function (v) {
                if (document.activeElement !== el && v !== undefined && v !== null) el.value = v;
                show(el, v);
            }, el);

        } else {
            // button or any element: data-mode="push" (default, true while pressed) or "toggle"
            const mode = el.dataset.mode || "push";
            if (mode === "toggle") {
                el.addEventListener("click", function () { panel.set(id, !truthy(panel.state[id])); });
            } else {
                let pressed = false;
                const release = function () { if (pressed) { pressed = false; panel.set(id, false); } };
                el.addEventListener("pointerdown", function (e) {
                    if (e.button !== 0) return;
                    pressed = true;
                    try { el.setPointerCapture(e.pointerId); } catch (_) { }
                    panel.set(id, true);
                    e.preventDefault();
                });
                el.addEventListener("pointerup", release);
                el.addEventListener("pointercancel", release);
                el.addEventListener("lostpointercapture", release);
                el.addEventListener("keydown", function (e) {
                    if ((e.key === " " || e.key === "Enter") && !pressed && !e.repeat) {
                        pressed = true; panel.set(id, true); e.preventDefault();
                    }
                });
                el.addEventListener("keyup", function (e) {
                    if (e.key === " " || e.key === "Enter") { release(); e.preventDefault(); }
                });
            }
            panel.on(id, function (v) { show(el, v); }, el);
        }
    }

    // bind data-signal and data-input elements of root (default: document)
    panel.bind = function (root) {
        root = root || document;
        const all = [];
        if (root.matches) all.push(root);
        root.querySelectorAll("[data-signal], [data-input], canvas[data-lcd]").forEach(function (el) { all.push(el); });
        for (const el of all) {
            if (el.__panelBound) continue;
            if (el.dataset.signal) { bindSignal(el); el.__panelBound = true; }
            else if (el.dataset.input) { bindInput(el); el.__panelBound = true; }
            else if (el.tagName === "CANVAS" && el.dataset.lcd !== undefined) { bindLcd(el); el.__panelBound = true; }
        }
    };

    function setStatus(connected) {
        panel.connected = connected;
        const root = document.documentElement;
        root.classList.toggle("panel-connected", connected);
        root.classList.toggle("panel-disconnected", !connected);
        document.querySelectorAll("[data-panel-status]").forEach(function (el) {
            el.textContent = connected ? "connected" : "disconnected";
            el.classList.toggle("on", connected);
        });
        for (const fn of statusListeners) safe(fn, connected);
    }

    function handle(msg) {
        if (msg.t === "init") {
            panel.signals = msg.signals || [];
            panel.page = msg.page || "";
            panel.board = msg.board || "";
            panel.pageUrl = msg.page_url;
            const old = panel.state;
            panel.state = msg.state || {};
            // generic page follows the page selected by the board (/p/default/?all: no redirection)
            if (msg.page_url && location.pathname.startsWith("/p/default/") && !location.search
                && msg.page_url !== "/p/default/") {
                location.replace(msg.page_url);
                return;
            }
            setStatus(!!msg.connected);
            lcdInit(msg.lcds);
            for (const fn of initListeners) safe(fn, panel.signals);
            for (const k in panel.state) emit(k, panel.state[k]);
            for (const k in old) if (!(k in panel.state)) emit(k, undefined);
        } else if (msg.t === "state") {
            for (const k in msg.state) {
                panel.state[k] = msg.state[k];
                emit(k, msg.state[k]);
            }
        } else if (msg.t === "status") {
            setStatus(!!msg.connected);
        } else if (msg.t === "lcd") {
            lcdMessage(msg);
        }
    }

    function connect() {
        const es = new EventSource("/events");
        es.onmessage = function (e) {
            try { handle(JSON.parse(e.data)); } catch (err) { console.error(err); }
        };
        es.onerror = function () { setStatus(false); };
    }

    window.panel = panel;

    if (document.readyState === "loading") {
        document.addEventListener("DOMContentLoaded", function () { panel.bind(); setStatus(false); connect(); });
    } else {
        panel.bind(); setStatus(false); connect();
    }
})();
"""


# ==================================================================================================
# Simulator connection and panel state
# ==================================================================================================
class Panel:
    def __init__(self, sock_path, pages_dir, page_override, verbose):
        self.sock_path = sock_path
        self.pages_dir = pages_dir
        self.page_override = page_override
        self.verbose = verbose
        self.lock = threading.Lock()
        self.sock = None
        self.connected = False
        self.page = None
        self.board = None
        self.signals = []
        self.state = {}
        self.clients = []
        self.lcds = {}          # active LCDs: id -> {"w", "h", "fmt", "bg", "frame"}

    def log(self, msg):
        if self.verbose:
            sys.stderr.write("simpanel: %s\n" % msg)
            sys.stderr.flush()

    def page_url(self):
        name = self.page_override or self.page
        if name and ID_RE.match(name) and os.path.isfile(os.path.join(self.pages_dir, name, "index.html")):
            return "/p/%s/" % name
        return "/p/default/"

    def init_message(self):
        lcds = {i: {k: lcd[k] for k in ("w", "h", "fmt", "bg")} for i, lcd in self.lcds.items()}
        return {"t": "init", "connected": self.connected, "page": self.page, "board": self.board,
                "page_url": self.page_url(), "signals": self.signals, "state": self.state, "lcds": lcds}

    def lcd_message(self, lcd_id):
        # whole LCD frame (called with lock)
        lcd = self.lcds[lcd_id]
        msg = {k: lcd[k] for k in ("w", "h", "fmt", "bg")}
        msg.update({"t": "lcd", "id": lcd_id, "y": 0, "n": lcd["h"],
                    "data": base64.b64encode(lcd["frame"]).decode("ascii")})
        return msg

    def subscribe(self):
        q = queue.Queue(maxsize=QUEUE_SIZE)
        with self.lock:
            self.clients.append(q)
            q.put(self.init_message())
            for lcd_id in self.lcds:
                q.put(self.lcd_message(lcd_id))
        return q

    def unsubscribe(self, q):
        with self.lock:
            if q in self.clients:
                self.clients.remove(q)

    def broadcast(self, msg):
        # called with lock
        for q in list(self.clients):
            try:
                q.put_nowait(msg)
            except queue.Full:
                # client does not read events: connection is closed
                self.clients.remove(q)
                try:
                    while True:
                        q.get_nowait()
                except queue.Empty:
                    pass
                q.put_nowait(None)

    def handle(self, msg):
        with self.lock:
            if msg.get("t") == "init":
                self.page = msg.get("page")
                self.board = msg.get("board")
                self.signals = msg.get("signals") or []
                self.state = msg.get("state") or {}
                self.log("board %s, page %s, %d signal(s)" % (self.board, self.page_url(), len(self.signals)))
                self.broadcast(self.init_message())
            elif msg.get("t") == "state":
                state = msg.get("state") or {}
                self.state.update(state)
                self.broadcast({"t": "state", "state": state})
            elif msg.get("t") == "lcd":
                self.handle_lcd(msg)

    def handle_lcd(self, msg):
        # changed lines of LCD (called with lock): the whole frame is kept for new browsers
        lcd_id = msg.get("id")
        if not isinstance(lcd_id, str) or not ID_RE.match(lcd_id):
            self.log("LCD message without valid id")
            return
        w, h, fmt = msg.get("w", 0), msg.get("h", 0), msg.get("fmt")
        if not w or not h or fmt not in PIXEL_SIZE:
            if self.lcds.pop(lcd_id, None):
                self.log("LCD %s is not active" % lcd_id)
            self.broadcast({"t": "lcd", "id": lcd_id, "w": 0, "h": 0})
            return
        line = w * PIXEL_SIZE[fmt]
        lcd = self.lcds.get(lcd_id)
        if not lcd or (lcd["w"], lcd["h"], lcd["fmt"]) != (w, h, fmt):
            lcd = self.lcds[lcd_id] = {"w": w, "h": h, "fmt": fmt, "frame": bytearray(line * h)}
            self.log("LCD %s %ux%u %s" % (lcd_id, w, h, fmt))
        lcd["bg"] = msg.get("bg", [0, 0, 0])
        y, data = msg.get("y", 0), base64.b64decode(msg.get("data", ""))
        if 0 <= y and y * line + len(data) <= len(lcd["frame"]):
            lcd["frame"][y * line:y * line + len(data)] = data
        self.broadcast(msg)

    def set_connected(self, connected):
        with self.lock:
            self.connected = connected
            self.broadcast({"t": "status", "connected": connected})

    def run(self):
        announced = False
        while True:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                s.connect(self.sock_path)
            except OSError:
                s.close()
                if not announced:
                    self.log("waiting for simulator (%s)" % self.sock_path)
                    announced = True
                time.sleep(RECONNECT_DELAY)
                continue

            announced = False
            self.log("simulator connected")
            with self.lock:
                self.sock = s
                self.lcds = {}  # LCDs are sent again by the simulator
            self.set_connected(True)

            buf = bytearray()
            try:
                while True:
                    data = s.recv(262144)
                    if not data:
                        break
                    start = len(buf)
                    buf += data
                    pos = 0
                    while True:
                        end = buf.find(b"\n", max(pos, start))
                        if end < 0:
                            break
                        line = bytes(buf[pos:end])
                        pos = end + 1
                        if line.strip():
                            try:
                                self.handle(json.loads(line))
                            except ValueError:
                                self.log("invalid message: %r" % line[:200])
                    del buf[:pos]
            except OSError:
                pass

            with self.lock:
                self.sock = None
            s.close()
            self.set_connected(False)
            self.log("simulator disconnected")

    def send_line(self, line):
        with self.lock:
            s = self.sock
            if not s:
                return False, "simulator is not connected"
            try:
                s.sendall(line.encode("utf-8"))
            except OSError as e:
                return False, str(e)
        return True, ""

    def touch(self, lcd_id, x, y, pressed):
        if not isinstance(lcd_id, str) or not ID_RE.match(lcd_id):
            return False, "invalid LCD id"
        lcd = self.lcds.get(lcd_id)
        if not lcd:
            return False, "LCD %s is not active" % lcd_id
        if not all(isinstance(v, (int, float)) and not isinstance(v, bool) for v in (x, y)):
            return False, "invalid coordinates"
        x = min(max(int(x), 0), lcd["w"] - 1)
        y = min(max(int(y), 0), lcd["h"] - 1)
        return self.send_line("touch %d %d %d %s\n" % (x, y, 1 if pressed else 0, lcd_id))

    def send(self, sig_id, value):
        if not isinstance(sig_id, str) or not ID_RE.match(sig_id):
            return False, "invalid signal id"
        if isinstance(value, bool):
            enc = "b1" if value else "b0"
        elif isinstance(value, (int, float)):
            enc = "n" + repr(value)
        elif isinstance(value, str):
            enc = "s" + value.replace("\r", " ").replace("\n", " ")
        else:
            return False, "unsupported value type"

        return self.send_line("set %s %s\n" % (sig_id, enc))


# ==================================================================================================
# HTTP server
# ==================================================================================================
class Handler(BaseHTTPRequestHandler):
    server_version = "simpanel"
    protocol_version = "HTTP/1.1"
    panel = None

    def log_message(self, fmt, *args):
        if self.panel.verbose:
            sys.stderr.write("simpanel: %s - %s\n" % (self.address_string(), fmt % args))

    def reply(self, code, body, ctype="text/plain; charset=utf-8", headers=()):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        for k, v in headers:
            self.send_header(k, v)
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        path = unquote(urlparse(self.path).path)

        if path == "/":
            self.reply(302, "", headers=[("Location", self.panel.page_url())])
        elif path == "/panel.js":
            self.reply(200, PANEL_JS, "text/javascript; charset=utf-8")
        elif path == "/events":
            self.events()
        elif path == "/api/state":
            with self.panel.lock:
                body = json.dumps(self.panel.init_message())
            self.reply(200, body, "application/json")
        elif path.startswith("/p/"):
            self.page_file(path[3:])
        else:
            self.reply(404, "not found\n")

    def do_POST(self):
        path = urlparse(self.path).path
        if path not in ("/api/set", "/api/touch"):
            self.reply(404, "not found\n")
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            req = json.loads(self.rfile.read(length) or b"{}")
            if path == "/api/set":
                ok, err = self.panel.send(req.get("id"), req.get("value"))
            else:
                ok, err = self.panel.touch(req.get("lcd"), req.get("x"), req.get("y"), bool(req.get("pressed")))
        except (ValueError, AttributeError):
            ok, err = False, "invalid request"
        if ok:
            self.reply(200, '{"ok":true}', "application/json")
        else:
            self.reply(409 if err == "simulator is not connected" else 400,
                       json.dumps({"ok": False, "error": err}), "application/json")

    def page_file(self, rel):
        name, sep, sub = rel.partition("/")
        if not ID_RE.match(name):
            self.reply(404, "not found\n")
            return
        if not sep:
            # /p/<name> -> /p/<name>/ (relative paths of page)
            self.reply(301, "", headers=[("Location", "/p/%s/" % name)])
            return
        base = os.path.realpath(os.path.join(self.panel.pages_dir, name))
        file = os.path.realpath(os.path.join(base, sub or "index.html"))
        if os.path.isdir(file):
            file = os.path.join(file, "index.html")
        if not (file == base or file.startswith(base + os.sep)) or not os.path.isfile(file):
            self.reply(404, "not found: %s\n" % rel)
            return
        ctype = mimetypes.guess_type(file)[0] or "application/octet-stream"
        if ctype.startswith("text/") or ctype in ("application/javascript", "image/svg+xml"):
            ctype += "; charset=utf-8"
        with open(file, "rb") as f:
            self.reply(200, f.read(), ctype)

    def events(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "close")
        self.end_headers()
        self.close_connection = True

        q = self.panel.subscribe()
        try:
            while True:
                try:
                    msg = q.get(timeout=KEEPALIVE)
                except queue.Empty:
                    self.wfile.write(b": keepalive\n\n")
                    self.wfile.flush()
                    continue
                if msg is None:
                    break
                self.wfile.write(b"data: " + json.dumps(msg).encode("utf-8") + b"\n\n")
                self.wfile.flush()
        except OSError:
            pass
        finally:
            self.panel.unsubscribe(q)


class Server(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


# ==================================================================================================
# Main
# ==================================================================================================
def main():
    root = os.path.realpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

    parser = argparse.ArgumentParser(description="dnx RTOS simulator board panel (web browser)")
    parser.add_argument("--socket", default=os.path.join(root, "build", "dnx-panel.sock"),
                        help="panel socket of the simulator (option --panel)")
    parser.add_argument("--dir", default=os.path.join(root, "sim-env", "panel"), help="directory of panel pages")
    parser.add_argument("--port", type=int, default=8080, help="HTTP port")
    parser.add_argument("--bind", default="127.0.0.1",
                        help="HTTP address (0.0.0.0: available for other computers)")
    parser.add_argument("--page", default=None, help="page shown instead of the page selected by board")
    parser.add_argument("--exit-with-parent", action="store_true",
                        help="exit when the parent process exits (started in background by make run-sim)")
    parser.add_argument("-v", "--verbose", action="store_true", help="show messages and HTTP requests")
    args = parser.parse_args()

    mimetypes.add_type("text/javascript", ".js")
    mimetypes.add_type("image/svg+xml", ".svg")

    panel = Panel(args.socket, os.path.realpath(args.dir), args.page, args.verbose)
    Handler.panel = panel

    try:
        server = Server((args.bind, args.port), Handler)
    except OSError as e:
        sys.stderr.write("simpanel: unable to start HTTP server on %s:%d: %s\n" % (args.bind, args.port, e))
        return 1

    threading.Thread(target=panel.run, daemon=True).start()

    if args.exit_with_parent:
        parent = os.getppid()

        def watch_parent():
            while os.getppid() == parent:
                time.sleep(0.5)
            os._exit(0)

        threading.Thread(target=watch_parent, daemon=True).start()

    host = "localhost" if args.bind in ("127.0.0.1", "0.0.0.0", "") else args.bind
    sys.stderr.write("simpanel: http://%s:%d (simulator socket %s)\n" % (host, args.port, args.socket))
    sys.stderr.flush()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
