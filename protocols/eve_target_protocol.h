/**
 * EVE spatial awareness → WALL-E drive assist (JSON field names + semantics).
 * Payload type: MSG_EVE_TARGET_AWARENESS (see eve_protocol.h)
 */
#pragma once

/** JSON keys (UTF-8, match exactly in serializers) */
#define EVE_TARGET_JSON_PROTO "proto"
#define EVE_TARGET_JSON_ZONE "targetZone"
#define EVE_TARGET_JSON_CONFIDENCE "confidencePct"
#define EVE_TARGET_JSON_DISTANCE_MM "distanceMm"
#define EVE_TARGET_JSON_ACK "ackTriggered"
#define EVE_TARGET_JSON_TRACKING "trackingActive"
#define EVE_TARGET_JSON_BIAS "suggestedTurnBias"
#define EVE_TARGET_JSON_NODE "sourceNodeId"
#define EVE_TARGET_JSON_AGE_MS "eventAgeMs"
#define EVE_TARGET_JSON_HEAD_PAN "headPanDeg"
#define EVE_TARGET_JSON_NEAR_MM "nearThresholdMm"

/** targetZone string values */
#define EVE_TARGET_ZONE_NONE "NONE"
#define EVE_TARGET_ZONE_LEFT "LEFT"
#define EVE_TARGET_ZONE_CENTER "CENTER"
#define EVE_TARGET_ZONE_RIGHT "RIGHT"
#define EVE_TARGET_ZONE_MULTI "MULTI"
#define EVE_TARGET_ZONE_UNCERTAIN "UNCERTAIN"

#define EVE_TARGET_PROTOCOL_VERSION 1u
