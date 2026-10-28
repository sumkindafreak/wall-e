/**
 * EVE servos — LEDC PWM driver for the two configured channels:
 *   EVE_SERVO_HEAD_PAN  (EVE_SERVO_L_PIN)  and  EVE_SERVO_RIGHT_ARM (EVE_SERVO_R_PIN).
 *
 * - Disabled (no pulses at all) unless EVE_ENABLE_SERVOS = 1 AND the pin is set.
 * - No position feedback exists. This driver only knows the angle it last COMMANDED; the real
 *   horn position is unknown until the first pulse is sent, and again after a RELEASE stop.
 * - No pulses are sent at boot. The first move starts from the configured rest angle (a known,
 *   safe pose) and eases from there.
 * - Every move is eased (accelerate, cruise at deg/s, decelerate) and can be interrupted at any
 *   moment by a new target or a halt.
 * - Moves are refused while EVE is stopped, faulted or battery-inhibited (eve_safety).
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

void servoInit(void);
void servoTick(void);

// ---- Channel API (channel = EveServoChannel) ---------------------------------------------------
/** True when the channel is enabled, has a pin and its PWM attached. */
bool servoAvailable(uint8_t ch);
/** Calibrated logical limits (degrees). */
void servoGetLimits(uint8_t ch, int16_t* minDeg, int16_t* maxDeg);
/** Ease to deg (clamped to limits) at degPerSec. Returns false if refused / unavailable. */
bool servoMoveTo(uint8_t ch, int16_t deg, uint16_t degPerSec);
/**
 * Halt every channel at its current commanded angle.
 * stop=true : a STOP or fault -> apply EVE_SERVO_STOP_POLICY (HOLD keeps pulses, RELEASE stops them).
 * stop=false: link loss / override -> freeze and keep holding.
 */
void servoHaltAll(bool stop);
/** Last commanded angle, or -1 if nothing has been commanded (physical position unknown). */
int16_t servoGetCommandedDeg(uint8_t ch);
int16_t servoGetTargetDeg(uint8_t ch);
bool servoIsMoving(uint8_t ch);
/** True while pulses are being sent on the channel. */
bool servoOutputActive(uint8_t ch);

// ---- Legacy helpers used by the web UI, desktop companion, spatial awareness -------------------
void servoSetAngles(int16_t headDeg, int16_t armDeg);
/** Head pan target (eased at EVE_SERVO_DEFAULT_DPS). */
void servoSetHeadPanTarget(int16_t deg);
/** Right arm target (eased at EVE_SERVO_DEFAULT_DPS). */
void servoSetRightArmTarget(int16_t deg);
