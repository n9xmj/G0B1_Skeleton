/******************************************************************************
 * globals.h
 *
 * Application-wide objects that more than one module needs to reach.
 ******************************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#include "nvmparams.h"

/*----------------------------------------------------------------------------
 * The application's parameter pool.
 *
 * OWNED BY THE APPLICATION, not by nvmparams. The module used to pre-declare a
 * default pool of its own; as a vendored module it no longer does, because a
 * library has no business deciding how many pools a project has or what they
 * are called.
 *
 * Instantiated and configured in app_main.c. Exported here because the pool
 * handle is the first argument to every nvmparams call.
 *
 * Note this declaration cannot live in nvmparams_config.h: that header is
 * included from the MIDDLE of nvmparams.h, before nvm_pool_t is defined.
 *--------------------------------------------------------------------------*/

extern nvm_pool_t g_x_nvm_param;

#endif /* GLOBALS_H */
