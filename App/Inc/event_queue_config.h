/******************************************************************************
 * event_queue_config.h
 *
 * G0B1_Skeleton's configuration for the vendored event_queue module.
 *
 * Deliberately minimal -- this is the baseline new projects are cloned from,
 * so the module is WIRED BUT UNUSED: it compiles, and the API is available the
 * moment you #include "event_queue.h", but the baseline creates no queues and
 * references nothing. Because the build uses -ffunction-sections/-fdata-sections
 * with --gc-sections, an unreferenced module contributes nothing to the image;
 * adopting it costs zero bytes until you actually call it.
 *
 * To start using it: declare an event_queue_handle_t, call
 * x_event_queue_create(), and put/get. See App/event_queue/README.md -- it
 * opens with a complete ISR-producer / main-loop-consumer example.
 *
 * See App/event_queue/event_queue_config.h.example for the full commentary on
 * every setting.
 *
 * DO NOT #include THIS FILE DIRECTLY. event_queue.h includes it for you.
 ******************************************************************************/

#ifndef EVENT_QUEUE_H_INSIDE
#error "Do not include event_queue_config.h directly -- include event_queue.h instead."
#endif

#ifndef EVENT_QUEUE_CONFIG_H
#define EVENT_QUEUE_CONFIG_H

/*=============================================================================
 * 1. ALLOCATION
 *===========================================================================*/

/* Let x_event_queue_create() allocate a ring buffer when the create config
 * passes pv_buffer = NULL. Set to 0 to compile the allocator path out
 * entirely -- the module then references no allocator at all and every queue
 * must be given caller-owned storage. Left at 1 in the baseline so both
 * styles are available to a new project without editing anything. */
#define EVENT_QUEUE_ENABLE_MALLOC       1

/* EVENT_QUEUE_MALLOC / EVENT_QUEUE_FREE are deliberately LEFT UNDEFINED here.
 *
 * The module then falls through to the C library's malloc()/free(). Define
 * both -- plus whatever #include makes them visible -- to route ring
 * allocation through an RTOS or pool allocator instead, e.g.
 *
 *   #include "FreeRTOS.h"
 *   #define EVENT_QUEUE_MALLOC(u32_size)    pvPortMalloc(u32_size)
 *   #define EVENT_QUEUE_FREE(pv)            vPortFree(pv)
 */

/*=============================================================================
 * 2. GEOMETRY
 *===========================================================================*/

/* Ring size used when a create config leaves u32_size at 0 -- including the
 * all-defaults x_event_queue_create(&handle, NULL). Each record costs 4 bytes
 * of header plus its payload, rounded up to a multiple of 4. Must itself be a
 * multiple of 4 and at least EVENT_QUEUE_SIZE_MIN (8); both are static
 * asserted. Nothing in the baseline allocates this -- it is only a default
 * for queues a project chooses to create. */
#define EVENT_QUEUE_DEFAULT_SIZE        256

#endif /* EVENT_QUEUE_CONFIG_H */
