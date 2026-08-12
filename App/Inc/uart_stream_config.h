/**
 * @file uart_stream_config.h
 * @brief G0B1_Skeleton's settings for the App/uart_stream module.
 *
 * Copied from App/uart_stream/uart_stream_config_template.h and edited. The
 * template carries the full commentary on every knob; this file keeps only what
 * a reader of THIS project needs.
 */

#ifndef UART_STREAM_CONFIG_H
#define UART_STREAM_CONFIG_H

//------------------------------------------------------------------------------
// Family header
//------------------------------------------------------------------------------
// The one line to change on a different STM32 series. Supplies the HAL types
// the public API is written in (UART_HandleTypeDef, USART_TypeDef, IRQn_Type),
// the HAL calls uart_stream.c makes, and the CMSIS core intrinsics queue.c uses
// for its critical sections.
//
// This says "main.h" where the template says "stm32g0xx_hal.h", deliberately.
// main.h contains nothing but #include "stm32g0xx_hal.h", so the two are
// identical to the compiler -- but naming the HAL header directly from a header
// in App/Inc wrecks CubeIDE's CDT indexer: measured 21 unresolved inclusions
// and 2.1% unresolved names against 3 and 0.13% via main.h, i.e. thousands of
// phantom errors in the Problems view with an image that builds byte-identical.
// CDT resolves main.h in main.c's context and reuses that; it has no context
// for a bare App/Inc header, so stm32g0xx.h hits its "select your target
// device" #error. See the template for the full note.
//
// PORTING: on another series this becomes "stm32g4xx_hal.h" et al. -- or that
// project's own main.h, same trade. Note the module has a SECOND family
// boundary this include does not cover: the clock-mux selector list in
// u32_uart_stream_kernel_clock().

#include "main.h"

//------------------------------------------------------------------------------
// Instance table size
//------------------------------------------------------------------------------
// The starter project binds one UART -- the console. 6 is the template default,
// kept so a clone that provisions more does not have to find this file first.

#define UART_STREAM_MAX_INSTANCES           6

//------------------------------------------------------------------------------
// Flush timeouts, milliseconds
//------------------------------------------------------------------------------
// The TC figure is a FLOOR, not the bound. Once the ring drains, the wait for
// hardware TC is derived per flush from the rate actually in effect, so a slow
// instance needs no adjustment here.

#define UART_STREAM_FLUSH_DRAIN_TIMEOUT_MS  50U
#define UART_STREAM_FLUSH_TC_TIMEOUT_MS     2U

//------------------------------------------------------------------------------
// Blocking-write deadline, milliseconds
//------------------------------------------------------------------------------

#define UART_STREAM_TX_BLOCK_TIMEOUT_MS     100U

#endif // UART_STREAM_CONFIG_H
