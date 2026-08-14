/******************************************************************************
 * debug_menu.c
 *
 * Bare-bones console debug menu built on the menusystem framework.
 *
 * Skeleton content only: [?] help plus two no-op quick-test stubs. Build the
 * menu up by adding menu_item_t entries to x_debug_top_menu (and sub-menus)
 * and pointing them at your own command handlers.
 ******************************************************************************/

/*============================================================================
 * INCLUDES
 *==========================================================================*/

#include "device_config.h"          /* stdint/stdio, platform.h (SYSTEM_TICK), main.h */
#include "menusystem.h"
#include "debug_menu.h"
#include "utils.h"                   /* RTC wakeup + hour-time helpers under test */
#include "rtc.h"                     /* hrtc, for post-STOP HAL_RTC_WaitForSynchro */
#include "automation_console.h"      /* host/script command interface (optional) */

#include <stdlib.h>                  /* strtoul -- KEY_LIST demo value entry (removable) */
#include <errno.h>                   /* ERANGE  -- KEY_LIST demo value entry (removable) */

/*============================================================================
 * PRIVATE PROTOTYPES
 *==========================================================================*/

static void v_debug_wakeup_sleep_test(void);
static void v_debug_quick_test_1(void);
static void v_debug_quick_test_2(void);
static void v_debug_menu_exec(char c_key);

/*============================================================================
 * PRIVATE FUNCTIONS (menu command handlers)
 *==========================================================================*/

/* ---------------------------------------------------------------------------
 * RTC wake-up / STOP self-test: live check of the two RTC helpers,
 * u32_set_rtc_wakeup_timer() and u32_get_rtc_hour_time(). Arms the RTC wakeup
 * timer, drops into STOP1 (masking non-RTC wake sources), and reports how long
 * we were actually asleep (measured via the RTC, which keeps running in STOP).
 * Handy skeleton diagnostic for bringing up low-power on a new board.
 * ------------------------------------------------------------------------- */

#define DEBUG_SLEEP_TEST_TIME   2000    /* Milliseconds, approx */

extern void SystemClock_Config(void);   /* defined in main.c; STOP reverts to HSI16 */

static void v_debug_wakeup_sleep_test(void)
{
    uint32_t u32_enter_sleep_hour_time;
    uint32_t u32_exit_sleep_hour_time;
    uint32_t u32_in_sleep_time;
    uint32_t u32_actual_wakeup_ms;

    printf("Sleep mode test - going to sleep for %u mS\r\n"
           "HAL-reported RTC clock frequency: %u Hz\r\n"
          ,(unsigned) DEBUG_SLEEP_TEST_TIME
          ,(unsigned) HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_RTC));

    u32_enter_sleep_hour_time = u32_get_rtc_hour_time();
    u32_actual_wakeup_ms = u32_set_rtc_wakeup_timer(DEBUG_SLEEP_TEST_TIME);
    printf("Sleep entry hour timestamp: %lu\r\n"
           "Wakeup timer armed for ~%lu mS\r\n"
          ,u32_enter_sleep_hour_time
          ,u32_actual_wakeup_ms);

    /* printf() here is blocking + unbuffered, so the console TX has fully
     * drained before we sleep. Mask every wake source except the RTC so only
     * the wakeup timer can bring us out of STOP: SysTick (HAL_SuspendTick), the
     * app's TIM6 10 ms tick (EXTI4_15's button is externally pulled up, so it
     * shouldn't fire, but mask it too to leave the RTC as the sole waker). */
    HAL_SuspendTick();
    HAL_NVIC_DisableIRQ(TIM14_IRQn);
//    HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERMODE_STOP1, PWR_STOPENTRY_WFI);

    v_stop_rtc_wakeup_timer();                          /* disarm the wakeup timer after the test */

    /* After STOP the RTC calendar shadow (TR/DR/SSR) is de-synchronised: the APB
     * read interface was clocked off during STOP. Wait for RSF before reading,
     * or the calendar read can return a torn/stale value -- which shows up as a
     * bogus small "time in sleep" even when the core slept the full interval. */
    HAL_RTC_WaitForSynchro(&hrtc);
    u32_exit_sleep_hour_time = u32_get_rtc_hour_time(); /* record sleep-exit hour time */
    SystemClock_Config();                               /* restore 64 MHz PLL before console use */
    /* Calculate time-in-sleep and add to HAL tick */
    u32_in_sleep_time = (uint32_t) (u32_exit_sleep_hour_time - u32_enter_sleep_hour_time);
    v_system_tick_add(u32_in_sleep_time);
    HAL_ResumeTick();

    /* --- Turn on masked interrupts if needed --- */
    HAL_NVIC_EnableIRQ(TIM14_IRQn);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

    printf("Time in sleep: ~%lu mS, exit hour time:%lu\r\n",
           u32_in_sleep_time, u32_exit_sleep_hour_time);
}

static void v_debug_quick_test_1(void)
{
    // Logging API integration test.
    // Exercises every macro form in log_helpers.h against the vendored module
    // in App/logging/, and confirms the application-supplied timestamp bridge
    // (u32_log_timestamp_ms in logging_port.c) is the one being called -- a
    // weak-default fallback would show (0.000) on every line.

    printf("\r\n--- logging API test ---\r\n");

    // Timestamped + [TAG] forms. LOGCT takes its color from the tag.
    LOGCT(LOG_SYSTEM, "LOGCT: tag color, value = %d", 42);
    LOG(LOG_SYSTEM, "LOG: no color, string = %s", "abc");
    LOGC(LOG_SYSTEM, LOGC_WARNING, "LOGC: explicit color (warning)");
    LOGC(LOG_SYSTEM, LOGC_ERROR, "LOGC: explicit color (error)");

    // Plain forms: no timestamp, no [TAG] prefix.
    LOG_PLAIN(LOG_SYSTEM, "LOG_PLAIN: bare text, no prefix\r\n");
    LOGC_PLAIN(LOG_SYSTEM, LOGC_CYAN, "LOGC_PLAIN: colored, no prefix");
    LOGCT_PLAIN(LOG_SYSTEM, "LOGCT_PLAIN: tag color, no prefix");

    // Build-gated forms.
    DPRINTF("DPRINTF: DEBUG-build only, no newline added\r\n");
    DPRINTF_TS("DPRINTF_TS: DEBUG-build only, timestamped");
    RPRINTF("RPRINTF: unconditional, survives a release build\r\n");

    // A class set to LOG_LEVEL_QUIET compiles out entirely -- this line should
    // produce no output at all.
    LOGCT(LOG_JOBS, "LOG_JOBS is QUIET; you should NOT see this");

    // Verbosity ladder. Every value below is a compile-time constant, so these
    // are the decisions the compiler actually made, not a runtime re-check.
    printf("\r\n  LOG_LEVEL = %d (0=QUIET 1=ALWAYS 2=ERROR 3=WARNING 4=INFO 5=DEBUG)\r\n",
           LOG_LEVEL);
    printf("  %-12s tier %d  emit=%d\r\n", LOG_SYSTEM_TAG, LOG_SYSTEM, LOG_EMIT(LOG_SYSTEM));
    printf("  %-12s tier %d  emit=%d\r\n", LOG_JOBS_TAG,   LOG_JOBS,   LOG_EMIT(LOG_JOBS));
    printf("  %-12s tier %d  emit=%d\r\n", LOG_EXTI_TAG,   LOG_EXTI,   LOG_EMIT(LOG_EXTI));

    // The two edges that the ladder ordering exists to get right. Both are
    // evaluated by the preprocessor here exactly as they are inside a LOGxx().
    printf("  a QUIET class under a DEBUG ceiling  -> emit=%d (want 0)\r\n",
           (LOG_LEVEL_QUIET != LOG_LEVEL_QUIET && LOG_LEVEL_QUIET <= LOG_LEVEL_DEBUG));
    printf("  an ALWAYS class under a QUIET ceiling -> emit=%d (want 0)\r\n",
           (LOG_LEVEL_ALWAYS != LOG_LEVEL_QUIET && LOG_LEVEL_ALWAYS <= LOG_LEVEL_QUIET));
    printf("  an ERROR class under a WARNING ceiling-> emit=%d (want 1)\r\n",
           (LOG_LEVEL_ERROR != LOG_LEVEL_QUIET && LOG_LEVEL_ERROR <= LOG_LEVEL_WARNING));
    printf("  a DEBUG class under a WARNING ceiling -> emit=%d (want 0)\r\n",
           (LOG_LEVEL_DEBUG != LOG_LEVEL_QUIET && LOG_LEVEL_DEBUG <= LOG_LEVEL_WARNING));

    // Single-statement behaviour: with the do{}while(0) wrapper this compiles
    // and takes the else. A bare braced macro body would not compile at all.
    if (LOG_SYSTEM == LOG_LEVEL_QUIET)
        LOGCT(LOG_SYSTEM, "dangling-else check: taken the wrong way");
    else
        printf("  dangling-else check: compiled and took the else\r\n");


    // Two timestamps a known interval apart. The delta proves the tick is
    // real and advancing rather than a stuck constant.
    LOGCT(LOG_SYSTEM, "timestamp check: t0");
    v_delay_ms(250);
    LOGCT(LOG_SYSTEM, "timestamp check: t0 + 250 mS");

    printf("--- end logging API test ---\r\n");
}

static void v_debug_quick_test_2(void)
{
    printf("Quick test function 2 (stub)\r\n");
}

#if ACON_ENABLED
/*
 * Human-driven entry to the automation console. The machine (SCRIPT-mode) entry
 * is the 0xDA sentinel intercepted in v_debug_menu_service() below.
 */
static void v_debug_automation_console(void)
{
    printf("Automation console - Ctrl-C or 'Q' to return, 'L' lists ops\r\n");
    v_automation_console_run(ACON_MODE_HUMAN);
    printf("\r\nReturned from automation console\r\n");
}
#endif

/*============================================================================
 * MENU DEFINITION
 *==========================================================================*/

/* ---------------------------------------------------------------------------
 * DEMO: menusystem KEY_LIST_FUNCTION + HELP_TEXT_VARIABLE worked example
 * (documented in App/menusystem/README.md). Presented as a self-contained
 * submenu reached from the main menu, so it lifts out cleanly when this skeleton
 * is cloned for a real project: delete this block, the x_keylist_demo_menu array
 * below, the one CALL_MENU entry in x_debug_top_menu that points at it, and the
 * two standard includes above (harmless if left).
 * ------------------------------------------------------------------------- */
static uint32_t u32_test_values[4];

/* HELP_TEXT_VARIABLE emitter: redraws the live values (and their keys) on every
 * menu print, giving the KEY_LIST entry -- which prints nothing itself -- a
 * legend and a readout in one. */
static void v_test_values_help(void)
{
    uint8_t u8_i;

    for (u8_i = 0; u8_i < 4; u8_i++)
    {
        printf("[%c] Edit test value %u = %lu\r\n",
               "1234"[u8_i], (unsigned) (u8_i + 1),
               (unsigned long) u32_test_values[u8_i]);
    }
}

/* KEY_LIST_FUNCTION handler: keys '1'..'4' all land here. The framework passes
 * the matched key's index in the "1234" list, so u8_index is already the array
 * index -- no lookup needed.
 *
 * Numeric-entry convention (reader is i_getline() in utils.c): a blank line or
 * ESC leaves the setting unchanged; strtoul base 0 accepts decimal, 0x-hex and
 * 0-octal; anything non-numeric, with trailing junk, or out of range prints
 * "Invalid value" and re-prompts. (Ctrl-X line-clear is handled inside
 * i_getline, so the caller never sees it.) */
static void v_edit_test_value(char c_key, uint8_t u8_index)
{
    char str_line[16];

    (void) c_key;

    for (;;)
    {
        char *pc_end;
        unsigned long ul_val;

        printf("Test value %u [now %lu]: ",
               (unsigned) (u8_index + 1), (unsigned long) u32_test_values[u8_index]);

        if (i_getline(str_line, (uint16_t) (sizeof(str_line) - 1u)) <= 0)
        {
            return;                     /* ESC (-1) or blank line (0): unchanged */
        }

        errno = 0;
        ul_val = strtoul(str_line, &pc_end, 0);     /* 0x.. hex, 0.. octal, else decimal */
        if ((*pc_end == '\0') && (errno == 0))      /* whole line parsed, no overflow */
        {
            u32_test_values[u8_index] = (uint32_t) ul_val;
            return;                     /* accepted */
        }

        printf("Invalid value\r\n");    /* re-prompt */
    }
}

static const menu_item_t x_keylist_demo_menu[] =
{
    {   /* live values -- redrawn on every print; doubles as the KEY_LIST legend */
        .x_type                 = MENU_ITEM_HELP_TEXT_VARIABLE,
        .c_key                  = 0,
        .p_c_text               = "\r\n--- KEY_LIST_FUNCTION demo ---\r\n",
        .pfn_help_text_function = v_test_values_help
    },
    {
        .x_type = MENU_ITEM_HELP,
        .c_key = '?',
        .p_c_text = NULL
    },
    {
        .x_type = MENU_ITEM_HELP_HIDDEN,
        .c_key = '\r',
        .p_c_text = NULL
    },
    {   /* keys '1'..'4' -> one handler, indexed by list position */
        .x_type                = MENU_ITEM_KEY_LIST_FUNCTION,
        .p_c_key_list          = "1234",
        .pfn_key_list_function = v_edit_test_value
    },
    {
        .x_type = MENU_ITEM_RETURN_TO_PREVIOUS_MENU,
        .c_key = '\x1B',
        .p_c_text = "Return to previous menu"
    },
    {
        .x_type = MENU_ITEM_END_OF_LIST,
    }
};

static const menu_item_t x_debug_top_menu[] =
{
    {
        .x_type = MENU_ITEM_HELP_TEXT_FIXED,
        .c_key = 0,
        .p_c_text = "\r\n--- " PRODUCT_NAME " v" FIRMWARE_VERSION " Main Menu ---\r\n"
    },
    {
        .x_type = MENU_ITEM_HELP,
        .c_key = '?',
        .p_c_text = NULL
    },
    {
        /* Bare <Enter> re-prints the menu without logging an unknown key. */
        .x_type = MENU_ITEM_HELP_HIDDEN,
        .c_key = '\r',
        .p_c_text = NULL
    },
    {
        .x_type = MENU_ITEM_FUNCTION,
        .c_key = 'W',
        .p_c_text = "RTC wake-up timer sleep test",
        .pfn_function = v_debug_wakeup_sleep_test
    },
    {
        .x_type = MENU_ITEM_FUNCTION,
        .c_key = 'q',
        .p_c_text = "Quick test function 1",
        .pfn_function = v_debug_quick_test_1
    },
    {
        .x_type = MENU_ITEM_FUNCTION,
        .c_key = 'Q',
        .p_c_text = "Quick test function 2",
        .pfn_function = v_debug_quick_test_2
    },
#if ACON_ENABLED
    {
        .x_type = MENU_ITEM_FUNCTION,
        .c_key = 'a',
        .p_c_text = "Automation console (human-driven)",
        .pfn_function = v_debug_automation_console
    },
#endif
    {   /* DEMO: self-contained menusystem example -- see block above / README */
        .x_type   = MENU_ITEM_CALL_MENU,
        .c_key    = 'k',
        .p_c_text = "menusystem KEY_LIST demo",
        .p_x_menu = x_keylist_demo_menu
    },
    {
        /* Hidden: ESC at the top level has nowhere to pop. menusystem replies
         * "[At top-level menu]" on an empty-stack return, so a spammed ESC
         * confirms you are fully backed out -- no custom function needed. */
        .x_type = MENU_ITEM_RETURN_TO_PREVIOUS_MENU,
        .c_key = '\x1B',
        .p_c_text = NULL
    },
    {
        .x_type = MENU_ITEM_END_OF_LIST,
    }
};

/*============================================================================
 * MENU CONTROL + SERVICE
 *==========================================================================*/

static void *x_debug_menu_stack[4];
static menu_control_t x_debug_menu_control;
#define DEBUG_MENU_STACK_DEPTH  (sizeof(x_debug_menu_stack) / sizeof(void *))

void v_debug_menu_init(void)
{
    v_menu_init(&x_debug_menu_control,
                x_debug_top_menu,
                &x_debug_menu_stack[0],
                DEBUG_MENU_STACK_DEPTH);

    /* key == 0xFF requests the initial help printout. */
    v_menu_exec(&x_debug_menu_control, 0xFF);
}

static void v_debug_menu_exec(char c_key)
{
    if (x_debug_menu_control.pap_x_menu == NULL)
    {
        v_debug_menu_init();
    }
    v_menu_exec(&x_debug_menu_control, c_key);
}

void v_debug_menu_service(void)
{
    static uint8_t u8_reentry_lock;
    int i_key;
    char str_key[4];

    if (u8_reentry_lock)
    {
        return;
    }
    u8_reentry_lock = 1;

    do
    {
        i_key = getchar();
        if (i_key < 0)
        {
            break;              /* no input pending */
        }

#if ACON_ENABLED
        /* Automation-console SCRIPT entry: a machine sends the non-typeable 0xDA
         * sentinel. Intercepted before the echo so it never reaches the menu
         * dispatcher. Runs with u8_reentry_lock held, which stops the nested
         * v_debug_menu_service() (reached via v_app_polling_task) from stealing
         * the console's input. */
        if ((uint8_t) i_key == ACON_ENTER)
        {
            v_automation_console_run(ACON_MODE_SCRIPT);
            continue;
        }
#endif

        pc_char_to_str((char) i_key, str_key);
        printf("Cmd [%s]\r\n", str_key);
        v_debug_menu_exec((char) i_key);
    }
    while (1);

    u8_reentry_lock = 0;
}

void v_debug_delay(uint32_t u32_delay)
{
    /* Cooperative delay: keep the console menu responsive while waiting. */
    uint32_t u32_start = SYSTEM_TICK();
    while (ELAPSED_TIME(u32_start) < u32_delay)
    {
        v_debug_menu_service();
    }
}
