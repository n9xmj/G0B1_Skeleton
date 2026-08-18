/******************************************************************************
 * nvmparams_config.h
 *
 * G0B1_Skeleton's configuration for the vendored nvmparams module.
 *
 * Deliberately minimal -- this is the baseline new projects are cloned from,
 * so it carries one example parameter and nothing else. Add your project's
 * IDs to the enum below and delete the example when it stops being useful.
 *
 * See App/nvmparams/nvmparams_config.h.example for the full commentary on
 * every setting, and App/nvmparams/README.md for the adoption guide.
 *
 * DO NOT #include THIS FILE DIRECTLY. nvmparams.h includes it for you.
 ******************************************************************************/

#ifndef NVMPARAMS_H_INSIDE
#error "Do not include nvmparams_config.h directly -- include nvmparams.h instead."
#endif

#ifndef NVMPARAMS_CONFIG_H
#define NVMPARAMS_CONFIG_H

/*=============================================================================
 * 1. POOL GEOMETRY
 *===========================================================================*/

/* One STM32G0 flash page is 2 KB and the whole page is reserved by the linker
 * script (NVM_FLASH in STM32G0B1RETX_FLASH.ld), so there is room to grow. */
#define NVM_POOL_SIZE_DEFAULT               0x200

/* Part of the on-media layout. SET ONCE, NEVER CHANGE -- altering it after a
 * pool exists silently misreads every object. Must be a multiple of 4. */
#define NVM_LABEL_MAX_LENGTH                16

/*=============================================================================
 * 2. OPTIONAL FEATURES
 *===========================================================================*/

/* Let nvmparams malloc() the pool buffer when the config passes
 * p_v_ram_buffer = NULL. Set to 0 to compile the allocator out entirely. */
#define NVM_ENABLE_INTERNAL_MALLOC          1

/* NVM_LOG_ERROR is deliberately LEFT UNDEFINED here.
 *
 * The module then compiles away every log site and names no logging library at
 * all -- which is exactly the property that lets it drop into a headless
 * project. Leaving it undefined in the baseline keeps that path exercised.
 *
 * To route the module's errors into this project's logging, define a class in
 * logging_config.h and map it:
 *
 *     #include "logging_config.h"
 *     #define NVM_LOG_ERROR(fmt, ...)   LOGCT(LOG_NVM, fmt, ##__VA_ARGS__)
 *
 * Arguments must be side-effect free -- they are discarded when undefined.
 */

/*=============================================================================
 * 3. APPLICATION PARAMETER IDs
 *===========================================================================*/

/* Anchored at NVM_ID_APP_FIRST so the module can prove at compile time that
 * the list has not run into its reserved range (0xFF00..0xFFFF). Add new
 * parameters AT THE END -- inserting in the middle renumbers everything after
 * it and orphans the matching objects in any pool already written. */

typedef enum
{
    /* Example parameter. Replace with your project's own. */
    NVM_PARAM_TEST_1 = NVM_ID_APP_FIRST,

    /* Marker -- not a parameter. Keep last. */
    NVM_PARAM_APP_LAST
}
app_nvm_param_t;

_Static_assert(NVM_PARAM_APP_LAST <= NVM_ID_APP_MAX,
               "Too many application NVM parameter IDs -- the list has run into "
               "the range reserved by nvmparams (see NVM_ID_APP_MAX).");

/*=============================================================================
 * 4. STORAGE DRIVERS
 *===========================================================================*/

/* The STM32 internal-flash example driver, renamed from
 * nvm_driver_stm_flash.c.example. Swap it for another example, or your own,
 * by changing these declarations and the pool config in app_main.c. */

extern nvm_error_t x_nvm_drv_stm_flash_read (const nvm_media_t *p_x_media);
extern nvm_error_t x_nvm_drv_stm_flash_write(const nvm_media_t *p_x_media);

#endif /* NVMPARAMS_CONFIG_H */
