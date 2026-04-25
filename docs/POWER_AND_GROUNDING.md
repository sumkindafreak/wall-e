# Power distribution and grounding (WALL‑E / ESP32 stack)

These rules are **build practices** for the multi-module robot: they do not change firmware, but they prevent the most common field failures (brownouts, I2C/SPI glitches, random resets).

## 1. Star ground and zones

- **Logic ground** (ESP32, 3.3 V regulators, sensors, small-signal returns) should meet at **one** point (often the main power connector negative or a dedicated “GND stud”).
- **Motor / high-current returns** (L298N, servo bus, NeoPixel high current) should run on **thick** runs back to the **battery negative** (or distribution block), then tie to logic GND at **one** point only (star), not daisy-chained through thin signal GND wires.
- Avoid routing **motor current** through the same thin return as **I2C/SPI ground** for long distances; use a **short, fat** join at the star point.

## 2. Bulk and ceramic capacitors

- **Every** motor driver board: **100 µF–470 µF electrolytic** (or polymer) **+ 100 nF ceramic** across supply, as close to the driver chip as possible.
- **Every** servo distribution point: **bulk** (e.g. 470 µF–1000 µF) **+ 100 nF** at the **bus** entry; add **100 nF** near groups of connectors if the bus is long.
- **ESP32-S3 module / 3.3 V LDO input**: follow datasheet (typically **10 µF + 100 nF** at the reg), keep leads short.
- **NeoPixel strips:** supply **at both ends** for long runs; add **cap** at the feed; avoid sharing one thin GND for pixels + sensitive ADC if you can split returns to the star.

## 3. Fusing and distribution

- **Fuse** each **battery feeder** to a subsystem (drive, servos, 5 V rail, 12 V if any) with appropriate current; **one** main fuse at the pack is not enough to limit fault current in a harness short.
- After a fault, find **why** the fuse blew (stall, pinched wire, bad connector) before replacing.

## 4. Regulators and battery sag

- **Servos and motors** should not be fed from the same **small** 3.3 V/5 V LDO that powers the ESP32 if the budget is tight: use a **dedicated** 5 V buck for servos, **5 V → 3.3 V** for logic, and keep **separate** high-current and logic paths as above.
- **Brownout:** If the ESP32 resets when servos or motors start, you have a **voltage/ground** problem, not a software problem—add bulk caps, heavier wire, and star grounding before chasing bugs in code.

## 5. Noise and comms (I2C / SPI / UART)

- Keep **I2C and SPI** wires **short**, **twist** SDA/SCL (or SCK/MOSI) with GND, and avoid running parallel to **motor leads** for long distances.
- If you must cross motor wires, do it **at 90°** to reduce inductive pickup.
- ** UART between modules **: common GND is mandatory; if links are long, consider **low baud**, **series resistors** (e.g. 100 Ω–220 Ω) at the sender, and **TVS** or **transorb** on the connector if you have outdoor/long harnesses (optional, DIY).

## 6. Connectors and harnesses

- Prefer **keyed** connectors per sub-harness; document **pinout** in the repo (one sheet per “Head”, “Base”, “CYD”, “Audio”).
- **Strain relief** and **service loops** at moving joints (head, arms) save continuity.

## 7. What to measure when something “random” happens

- **Vcc at ESP** under load (scope or DVM with min-hold) during worst-case motion.
- **Ground difference** (mV) between logic GND and battery negative at the far end of the robot—**high** mV = bad return path.
- If resets correlate with **motors** only, **power layout** is the first place to look.

This document is the **“power discipline”** companion to the v2 **node health** and **cyd comms** telemetry in firmware: use both together for reliable shows.
