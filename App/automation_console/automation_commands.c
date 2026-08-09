/******************************************************************************
 * automation_commands.c
 *
 * Application command handlers for the automation console, and the
 * g_x_acon_command[] table the core dispatches into.
 *
 * This is the SKELETON command set: two worked examples and nothing product-
 * specific. Add real commands here by writing a handler and adding a row to the
 * table below; the core (automation_console.c) supplies the executive, framing
 * and the builtins (quit / list / version / no-op). See automation_console.h
 * for the command-author API.
 *
 * The whole file is gated by ACON_ENABLED so it compiles out when a project sets
 * DEV_CONFIG_ENABLE_AUTOMATION_CONSOLE to 0.
 ******************************************************************************/

#include "device_config.h"          /* stdint/stdio, ACON_* */
#include "automation_console.h"

#if ACON_ENABLED

/*
 * @ -- echo the comma-separated fields back as CSV. The PARSED-ARGUMENT idiom:
 * split the line with u8_acon_args() and act on the fields. Almost every real
 * command is shaped like this; here the "action" is merely to echo them.
 *
 *   @,12,ab,text  ->  =@,12,ab,text        (@ with no args -> =@)
 */
static void v_acon_op_echo_args(char c_op, char *pc_line)
{
    static char s_ac_csv[ACON_LINE_MAX];
    char *ap_c_arg[ACON_MAX_ARGS];
    uint8_t u8_argc = u8_acon_args(pc_line, ap_c_arg, ACON_MAX_ARGS);
    char ac_op[4];
    uint16_t u16_len = 0;
    uint8_t u8_i;

    if (u8_argc == 0u)
    {
        v_acon_ok(c_op);                    /* "=@" -- nothing to echo */
        return;
    }

    s_ac_csv[0] = '\0';
    for (u8_i = 0u; u8_i < u8_argc; u8_i++)
    {
        int i_n = snprintf(&s_ac_csv[u16_len], sizeof(s_ac_csv) - u16_len,
                           "%s%s", (u8_i > 0u) ? "," : "", ap_c_arg[u8_i]);
        if ((i_n < 0) || ((uint16_t) i_n >= (uint16_t) (sizeof(s_ac_csv) - u16_len)))
        {
            break;                          /* buffer full: emit what fits */
        }
        u16_len += (uint16_t) i_n;
    }

    v_acon_emit(ACON_SIG_OK, "%s,%s", pc_acon_op_name(c_op, ac_op), s_ac_csv);
}

/*
 * $ -- echo the whole line back verbatim, commas and all. The RAW-LINE idiom: a
 * command that does NOT want the comma splitter reads the line directly. Passing
 * the line as the argument to "%s" (never as the format) also means a '%' in the
 * data is data, not a conversion.
 *
 *   $hello,world  ->  =$hello,world
 *
 * The text is whatever survived the reader, which strips CR/LF but not other
 * control bytes; sanitise here if a downstream consumer needs printable-only.
 */
static void v_acon_op_echo_raw(char c_op, char *pc_line)
{
    (void) c_op;
    v_acon_emit(ACON_SIG_OK, "%s", pc_line);
}

/*============================================================================
 * COMMAND TABLE  (the application-owned port point; core dispatches into this)
 *==========================================================================*/

const acon_op_t g_x_acon_command[] =
{
    { '@', v_acon_op_echo_args, "echo args as CSV (example)" },
    { '$', v_acon_op_echo_raw,  "echo raw text (example)"    },
};

const uint8_t g_u8_acon_command_count =
    (uint8_t) (sizeof(g_x_acon_command) / sizeof(g_x_acon_command[0]));

#endif /* ACON_ENABLED */
