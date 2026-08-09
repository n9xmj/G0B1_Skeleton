/**
 * @file    device_config.h
 * @brief   Product options and constant parameter settings
 */
#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging_config.h"
#include "main.h"
#include "platform.h"
#include "globals.h"

//------------------------------------------------------------------------------
// Build options
//------------------------------------------------------------------------------
//
// Turn off build-time options if DEBUG is not defined (via the -DDEBUG
// compiler command line option). Logging answers DEBUG for itself, in
// logging_config.h.

#ifndef DEBUG
#undef DEBUG_MENU
#define DEBUG_MENU                      0
#endif

// Debug menu system enable.
// Set this to allow the debug menu system to be included. Independent of the
// logging configuration in logging_config.h.

#ifndef DEBUG_MENU
#define DEBUG_MENU                      1
#endif

//------------------------------------------------------------------------------

#define FIRMWARE_VERSION                "0.1.0.0.0"
#define PRODUCT_NAME                    "G0B1_Skeleton"
#define PLATFORM_NAME                   "NUCLEO-G0B1RE"

#if defined(DEBUG)
#define BUILD_CONFIG                    "DEBUG"
#else 
#define BUILD_CONFIG                    "RELEASE"
#endif

//------------------------------------------------------------------------------
// Console (uart_stream)
//------------------------------------------------------------------------------
//
// Ring-buffer sizes for the console UART bound to uart_stream. The RX ring must
// hold a whole command line: getchar() pulls one byte per main-loop pass and
// cannot keep up with a sustained 921600-baud burst, so an incoming line has to
// be able to sit in the ring while the console drains it. A ring SMALLER than
// the longest legal line cannot receive that line at all.
//
// The queue leaves one slot empty, so usable capacity is one byte less than the
// size given here.

#define DEV_CONFIG_CONSOLE_TX_BUF_SIZE                                      1024
#define DEV_CONFIG_CONSOLE_RX_BUF_SIZE                                      1024

//------------------------------------------------------------------------------
// Automation console (App/automation_console)
//------------------------------------------------------------------------------
//
// Build switch for the machine-facing command console. 1 compiles it in: the
// debug-menu 'a' entry, the 0xDA SCRIPT-mode sentinel, and the @/$ example
// commands. 0 compiles it out entirely -- the module bodies drop to nothing and
// the entry points become inert inline stubs, so no call site needs an #ifdef.
// See App/automation_console/automation_console.h.

#define DEV_CONFIG_ENABLE_AUTOMATION_CONSOLE                                   1

//------------------------------------------------------------------------------
// Misc
//------------------------------------------------------------------------------

#define DEV_CONFIG_NVM_COMMIT_DELAY_MS                                      5000

#endif //DEVICE_CONFIG_H
