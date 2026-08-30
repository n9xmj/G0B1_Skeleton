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

//------------------------------------------------------------------------------
// NVM pool ownership label.
//
//  >>> CHANGE THIS WHEN YOU ADOPT THE SKELETON <<<
//
// Written into the pool header and CHECKED at init: a pool whose label is not
// ours is wiped and rebuilt from defaults (see u8_nvm_label_matches() in
// app_main.c). Give every project that can be loaded onto the same board a
// DIFFERENT label.
//
// Why it matters: the .nvmdata section is NOLOAD, so the pool survives
// reflashing. A board shared between two projects will otherwise hand this
// firmware a pool written by the other one -- valid signature, self-consistent
// contents, completely foreign IDs. Neither the signature nor a CRC can detect
// that; only the label carries provenance.
//
// "UNNAMED" is a deliberate placeholder, not a default worth keeping: it is
// meant to look unfilled, and the check logs a loud warning at boot for as long
// as it is left in place. Setting it to PRODUCT_NAME is a reasonable choice --
// then a fork inherits a distinct label for free.
//
// At most NVM_LABEL_MAX_LENGTH (16) characters; asserted in app_main.c.
//------------------------------------------------------------------------------

#define NVM_POOL_LABEL                  "UNNAMED"

// The value NVM_POOL_LABEL ships with. Only used to notice that nobody has
// changed it yet -- leave this alone when you set a real label above.
#define NVM_POOL_LABEL_UNSET            "UNNAMED"

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
// Nothing here. The console is a vendored module and owns its own settings
// file: see App/Inc/automation_console_config.h, copied from the template in
// App/automation_console/. That is where the build switch, the buffer sizes and
// the timeouts live.

//------------------------------------------------------------------------------
// Misc
//------------------------------------------------------------------------------

#define DEV_CONFIG_NVM_COMMIT_DELAY_MS                                      5000

#endif //DEVICE_CONFIG_H
