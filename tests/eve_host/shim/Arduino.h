/**
 * Host-test shim: just enough of <Arduino.h> for EVE's plain-C++ modules (config.h and
 * eve_expression_state.h include <Arduino.h> for types only). Nothing here touches hardware.
 */
#pragma once
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define F(x) (x)
#define NEO_GRB 0
#define NEO_KHZ800 0
