/******************************************************************************
 * log_helpers.h
 *
 * VENDORED MODULE -- App/logging/ (the macro "sugar" layer)
 *
 * Provides LOG(), LOGC(), LOGCT(), the _PLAIN variants, DPRINTF() and
 * RPRINTF(). This file is part of the reusable logging component and is NOT
 * project-specific -- do not edit it per-project. Everything that varies
 * between projects lives in logging_config.h.
 *
 * Usage in an adopting project:
 *   - Copy logging_config_template.h from this directory into your app's
 *     include directory, rename it to logging_config.h, and edit the tags.
 *   - Application modules then #include "logging_config.h" to get the full
 *     tagged/colored/timestamped logging experience. That header pulls this
 *     one in for you; do not include this file directly.
 *   - Portable modules that must stay independent of any one project's tags
 *     can include "logging.h" instead and call the v_log*() functions.
 *
 * The tag definitions (LOG_FOO, LOG_FOO_TAG, LOG_FOO_COLOR) must be provided
 * by the including file BEFORE this header is processed -- the ## token
 * pasting below resolves them at the point of use. See
 * logging_config_template.h for the expected pattern.
 ******************************************************************************/

#ifndef LOG_HELPERS_H
#define LOG_HELPERS_H

#include "logging.h"        /* log_color_t, PRINTF_ATTR, the v_log*() prototypes */

//------------------------------------------------------------------------------
// Unconditional / DEBUG-only direct printf wrappers (no tag, no color, no
// filtering by the per-class LOG_* settings).

#ifdef DEBUG
  // DPRINTF(...)
  // Unconditional output when the DEBUG build option is enabled.
  // Does not add or modify the output text in any way.
  #define DPRINTF(...) \
          { v_log_printf(__VA_ARGS__); }
  // DPRINTF_TS(...)
  // Unconditional output when the DEBUG build option is enabled, with timestamp
  #define DPRINTF_TS(...) \
          { v_log_printf_time(__VA_ARGS__); }
#else
  #define DPRINTF(...)
  #define DPRINTF_TS(...)
#endif

//------------------------------------------------------------------------------
// RPRINTF() is an unconditional printf() output; it is simply an alias
// for printf() implemented as a function macro.
// Use of this macro is intended for output that should be generated
// in the release build; i.e. is not conditioned on the <DEBUG> build flag.

#define RPRINTF(...) printf(__VA_ARGS__)

//------------------------------------------------------------------------------
// Debug output macros (the main "sugar").
//
// These are only active when DEBUG_LOGGING is nonzero.
// When DEBUG_LOGGING == 0 they become no-ops.
//
// The LOG_* / LOGC_* / LOGCT_* variants use the per-class setting (e.g.
// LOG_SYSTEM) plus the associated LOG_SYSTEM_TAG and LOG_SYSTEM_COLOR that must
// be defined by the caller (in logging_config.h) before including this file.
//
// LOGCT(tag, fmt, ...) is the most common: timestamp + [TAG] + color-from-tag.

#if DEBUG_LOGGING

// LOG_PLAIN(tag, ...)
// Conditional output (tag != 0) without [TAG] or timestamp prefix text.
#define LOG_PLAIN(tag, ...) \
    { if (tag) { v_log_printf(__VA_ARGS__); } }

// LOGC_PLAIN(tag, color, fmt, ...)
// Same as LOG_PLAIN(), but provides option to change foreground color of all
// text printed by it on an ANSI terminal.
#define LOGC_PLAIN(tag, color, fmt, ...) \
    { if (tag) { v_logc_printf(color, fmt, ##__VA_ARGS__); } }

// LOGCT_PLAIN(tag, fmt, ...)
// Same as LOG_PLAIN, but sets foreground color to the one associated with
// the <tag>
#define LOGCT_PLAIN(tag, fmt, ...) \
    { if (tag) { v_logc_printf(tag ## _COLOR, fmt, ##__VA_ARGS__); } }

// LOG(tag, fmt, ...)
// Conditional output (tag != 0) WITH [TAG] prefix text.
#define LOG(tag, fmt, ...) \
    { if (tag) { v_log_printf_time_tag(tag ## _TAG, fmt, ##__VA_ARGS__); } }

// LOGC(tag, color, fmt, ...)
// Same as LOG(), but provides option to change foreground color of all text
// printed by it on an ANSI terminal.
#define LOGC(tag, color, fmt, ...) \
    { if (tag) { v_logc_printf_time_tag(tag ## _TAG, color, fmt, ##__VA_ARGS__); } }

// LOGCT(tag, fmt, ...)
// Same as LOG(), but sets foreground color for output to the value associated
// with <tag>
#define LOGCT(tag, fmt, ...) \
    { if (tag) { v_logc_printf_time_tag(tag ## _TAG, tag ## _COLOR, fmt, ##__VA_ARGS__); } }

#else // DEBUG_LOGGING

#define LOG_PLAIN(tag, ...)
#define LOGC_PLAIN(tag, color, ...)
#define LOGCT_PLAIN(tag, fmt, ...)
#define LOG(tag, fmt, ...)
#define LOGC(tag, color, fmt, ...)
#define LOGCT(tag, fmt, ...)

#endif // DEBUG_LOGGING

#endif /* LOG_HELPERS_H */
