#pragma once

#include <Arduino.h>

void apiSecurityInit(void);

/** If no token is configured, always returns true. */
bool apiSecurityTokenOk(void);

/** If token invalid: sends 401 JSON + CORS, returns true (caller should return). */
bool apiSecurityRejectIfBadToken(void);

void apiSecurityAudit(const char* line);

String apiSecurityGetAuditJSON(void);
String apiSecurityGetDashboardJSON(void);

/** POST body JSON: {"token":"new","old_token":""} — set API token (empty disables). */
bool apiSecurityApplyTokenBody(const char* body, size_t len, char* errBuf, size_t errLen);
