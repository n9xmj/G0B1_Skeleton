/**
 * @file    logging_config.h
 * @brief   Project-specific logging configuration and message-class tags.
 *
 * This is the header application modules include when they want the logging
 * "sugar" (LOGCT, LOG, RPRINTF, ...). It defines this project's message
 * classes -- enable/level, tag string and color -- and then pulls in the macro
 * helpers from the vendored module.
 *
 * APPLICATION-OWNED SEAM. Created by copying App/logging/logging_config_template.h
 * into App/Inc/ and customizing the tag list. Edit it freely; never edit the
 * files under App/logging/.
 *
 * Portable modules should NOT include this file -- they can include
 * "logging.h" directly if they need the low-level output functions without
 * inheriting this project's tags.
 *
 * Replaces the former debug_config.h, whose name predated the logging API.
 * Non-logging build options moved to device_config.h.
 */

#ifndef LOGGING_CONFIG_H
#define LOGGING_CONFIG_H

//------------------------------------------------------------------------------
// Knobs that must be set before the module headers are pulled in
//------------------------------------------------------------------------------
// logging.h defaults this via #ifndef, so setting it here (ahead of the include
// below) is what makes this project's choice stick.

#define LOG_WITH_TIMESTAMP              1

#include "logging.h"   // for log_color_t etc. (needed for the _COLOR values below)

//------------------------------------------------------------------------------
// Build configuration guards
//------------------------------------------------------------------------------
// Turn off logging if DEBUG is not defined (via the -DDEBUG compiler command
// line option).

#ifndef DEBUG
#undef DEBUG_LOGGING
#define DEBUG_LOGGING                   0
#endif

// Global debug logging enable
// Setting this to 0 disables most application-generated outputs.
// Debug menu inclusion is independent of this setting -- see DEBUG_MENU in
// device_config.h.

#if !defined(DEBUG_LOGGING)
// Change this to enable or disable all debug logging output
#define DEBUG_LOGGING                   1
#endif

//------------------------------------------------------------------------------
// Message classes and associated output tags/colors for this project
//------------------------------------------------------------------------------
// Each class needs three coordinated defines. Only the un-suffixed name is
// passed to a logging macro; it reaches _TAG and _COLOR itself via the ##
// token-pasting operator:
//
//   LOGCT(LOG_SYSTEM, "value = %d", n);   ->  uses LOG_SYSTEM_TAG / _COLOR

#if DEBUG_LOGGING

// Misc/system
#define LOG_SYSTEM                      1
#define LOG_SYSTEM_TAG                  "SYSTEM"
#define LOG_SYSTEM_COLOR                LOGC_BRIGHT_MAGENTA

// Job queue activty
#define LOG_JOBS                        0
#define LOG_JOBS_TAG                    "JOB"
#define LOG_JOBS_COLOR                  LOGC_WHITE

// EXTI interrupt reporting
#define LOG_EXTI                        0
#define LOG_EXTI_TAG                    "EXTI"
#define LOG_EXTI_COLOR                  LOGC_WHITE

#endif  // DEBUG_LOGGING

//------------------------------------------------------------------------------
// Pull in the macro sugar (LOGCT, LOG, LOGC, RPRINTF, DPRINTF, ...).
// The class defines above must precede this include.

#include "log_helpers.h"

#endif // LOGGING_CONFIG_H
