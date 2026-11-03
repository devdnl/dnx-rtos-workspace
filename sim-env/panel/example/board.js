/*
 * Elements of the example panel handled by the panel API (panel.on()):
 *   - LED bars: <div class="bar" data-bits="<signal>" [data-active-low]>,
 *     8 LEDs (bit 7 on the left) shown from the integer value of signal,
 *   - thermometer: fill level from the TEMP signal.
 */

// LED bars
document.querySelectorAll("[data-bits]").forEach(function (bar) {
    const leds = [];
    for (let n = 7; n >= 0; n--) {
        const cell = document.createElement("span");
        cell.className = "cell";
        const led = document.createElement("span");
        led.className = "led";
        const label = document.createElement("small");
        label.textContent = n;
        cell.append(led, label);
        bar.append(cell);
        leds[n] = led;
    }
    const activeLow = bar.hasAttribute("data-active-low");
    panel.on(bar.dataset.bits, function (v) {
        v = Number(v) || 0;
        for (let n = 0; n < 8; n++) {
            const bit = ((v >> n) & 1) === 1;
            leds[n].classList.toggle("on", activeLow ? !bit : bit);
        }
    });
});

// thermometer: -20 .. 100 C
panel.on("TEMP", function (v) {
    const pct = Math.max(0, Math.min(100, (Number(v) + 20) / 120 * 100));
    document.getElementById("thermo-fill").style.height = pct + "%";
});

// LCD blocks (<div class="lcd" data-lcd-block="<LCD id>">): information when LCD
// is not active (LTDC driver is not initialized, display of model is off)
panel.onLcd(function (lcd, id) {
    document.querySelectorAll(".lcd[data-lcd-block]").forEach(function (block) {
        if (block.dataset.lcdBlock === id) block.classList.toggle("off", !lcd);
    });
});

// board script name
panel.onInit(function () {
    document.getElementById("board").textContent = panel.board || "";
});
