# ESP-NOW and Wi-Fi Channel (WALL-E)

Quick reference for **why nodes stop hearing each other** despite correct code.

---

## Rule

ESP-NOW uses the **same 2.4 GHz radio** as Wi-Fi. Peers must operate on the **same Wi-Fi channel** as the interface used for coordination.

In this project, the **base** usually creates softAP **`WALL-E-Control`**. All ESP-NOW peers should use that **channel**.

---

## Checklist

1. Note the **channel** of `WALL-E-Control` (router admin UI, or serial print on base at boot if implemented).
2. If vision/audio/dock use **STA to home Wi-Fi**, ensure the **home AP** uses the **same channel** as the base AP, **or** connect those nodes only to the WALL-E AP during bring-up.
3. After changing routers or AP placement, **re-verify** ESP-NOW.

---

## Control link (Base ↔ CYD)

- **Sequence + CRC:** CYD v2 `ControlPacket` uses a monotonic `seq` and CRC-8. The base **drops** staled/replayed seq (backward wraps) and **drops** forward jumps larger than `WALLE_CTRL_MAX_SEQ_JUMP` (see `firmware_common/include/walle_link_packet.h`), and sets a short **WEAK** link hint on the controller when that happens.
- **Telemetry send failures:** If base → CYD `esp_now_send` fails repeatedly, the base may **delete and re-add** the controller peer. The master may **refresh** its own ESP-NOW peer if TX failures spike (see `wall_e_master_controller/espnow_control.cpp`).

---

## Related

- [../ARCHITECTURE.md](../ARCHITECTURE.md)  
- [../REPO_AUDIT.md](../REPO_AUDIT.md)  
