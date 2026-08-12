/**
 * @file automation_console_config.h
 * @brief G0B1_Skeleton's settings for the App/automation_console module.
 *
 * Copied from App/automation_console/automation_console_config_template.h and
 * edited. The template carries the full commentary on every knob; this file
 * keeps only what a reader of THIS project needs, plus a note wherever the
 * value differs from the template default.
 */

#ifndef AUTOMATION_CONSOLE_CONFIG_H
#define AUTOMATION_CONSOLE_CONFIG_H

#include "platform.h"           // SYSTEM_TICK(), PUMP_POLLING_TASK()
#include "device_config.h"      // PRODUCT_NAME, PLATFORM_NAME, FIRMWARE_VERSION

//------------------------------------------------------------------------------
// Build switch
//------------------------------------------------------------------------------
// 1 compiles the console in: the debug-menu 'a' entry, the 0xDA SCRIPT-mode
// sentinel, and the @/$ example commands. 0 compiles it out entirely -- the
// module bodies drop to nothing and the entry points become inert inline stubs,
// so no call site needs an #ifdef.

#define ACON_ENABLE                     1

//------------------------------------------------------------------------------
// Platform hooks
//------------------------------------------------------------------------------

#define ACON_TICK_MS()                  SYSTEM_TICK()
#define ACON_PUMP()                     PUMP_POLLING_TASK()

//------------------------------------------------------------------------------
// Identity, reported by the V builtin
//------------------------------------------------------------------------------

#define ACON_ID_PRODUCT                 PRODUCT_NAME
#define ACON_ID_PLATFORM                PLATFORM_NAME
#define ACON_ID_FIRMWARE                FIRMWARE_VERSION
#define ACON_ID_BUILD                   BUILD_CONFIG

//------------------------------------------------------------------------------
// Parsing limits
//------------------------------------------------------------------------------
// The starter project's example commands take at most two fields; 6 leaves room
// to experiment without editing this file.

#define ACON_MAX_ARGS                   6u

//------------------------------------------------------------------------------
// Buffer sizes, bytes
//------------------------------------------------------------------------------
// ACON_LINE_MAX must stay comfortably below DEV_CONFIG_CONSOLE_RX_BUF_SIZE
// (1024 in device_config.h) -- a line longer than the RX ring cannot be
// received at all.

#define ACON_LINE_MAX                   512
#define ACON_EMIT_MAX                   128

//------------------------------------------------------------------------------
// Timeouts, milliseconds
//------------------------------------------------------------------------------

#define ACON_IDLE_TIMEOUT_MS            15000
#define ACON_TX_TIMEOUT_MS              100

#endif // AUTOMATION_CONSOLE_CONFIG_H
