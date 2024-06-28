
/** @file cli_console.c
 *
 * @license
 * @description
 */

#include "project_config.h"
#if (MODULE_ENABLE(SUPPORT_DEBUG_CONSOLE_CLI))
#include "types.h"
#include "util_printf.h"
#include "util_log.h"
#include "util_string.h"

#include "shell.h"
#include "bsp.h"
#include "bsp_console.h"

#include "FreeRTOS.h"
#include "task.h"
//=============================================================================
//                  Constant Definition
//=============================================================================
#define CLI_STACK_SIZE          TASK_CALC_STACK_SIZE(1 << 10)
//=============================================================================
//                  Macro Definition
//=============================================================================
#define CLI_CMD_DEEP        (8)
#define CLI_LINE_SIZE       (256)
#define CLI_HISTORY_SIZE    SHELL_CALC_HISTORY_BUFFER(CLI_LINE_SIZE, CLI_CMD_DEEP)
//=============================================================================
//                  Structure Definition
//=============================================================================

//=============================================================================
//                  Global Data Definition
//=============================================================================
static char         g_line_buf[CLI_LINE_SIZE] = {0};
static char         g_history[CLI_HISTORY_SIZE] = {0};
static sh_args_t    g_sh_args = {0};

extern int stdout_string(char *str, int length);
extern int stdin_str(char *pBuf, int length);
//=============================================================================
//                  Private Function Definition
//=============================================================================
static int
_sh_io_init(sh_set_t *pSet_info)
{
    return 0;
}

static int
_sh_io_read(
    uint8_t     *pBuf,
    uint32_t    length,
    void        *pExtra)
{
    static cb_stdin_string_t    stdin_gets = 0;
    uint32_t                    byte_cnt = 0;

    if ( !stdin_gets )
    {
        stdin_gets = bsp_console_stdin_str;
    }

    byte_cnt = stdin_gets((char *)pBuf, length);
    return byte_cnt;
}

static int
_sh_io_write(
    uint8_t     *pBuf,
    uint32_t    length,
    void        *pExtra)
{
    static cb_stdout_string_t   stdout_puts = 0;
    int                         len  = 0;

    if ( !stdout_puts )
    {
        stdout_puts = bsp_console_stdout_string;
    }

    stdout_puts((char *)pBuf, length);

    return len;
}

static int _sh_io_flush(void)
{

    static cb_stdio_flush_t   stdio_flush = 0;
    if ( !stdio_flush )
    {
        stdio_flush = bsp_console_stdio_flush;
    }

    return stdio_flush();
}

static const sh_io_desc_t   g_sh_std_io =
{
    .cb_init   = _sh_io_init,
    .cb_read   = _sh_io_read,
    .cb_write  = _sh_io_write,
    .cb_flush  = _sh_io_flush,
};

static int _sh_cb_alarm(struct sh_args *pArg)
{
    vTaskDelay(pArg->delay_time_ms);
    return 0;
}

static void
_task_console(void *pArgv)
{
    sh_set_t    sh_set = {0};

    sh_set.pLine_buf    = g_line_buf;
    sh_set.line_buf_len = sizeof(g_line_buf);
    sh_set.pHistory_buf = g_history;
    sh_set.line_size    = sizeof(g_line_buf);
    sh_set.cmd_deep     = CLI_CMD_DEEP;
    sh_set.history_buf_size = SHELL_CALC_HISTORY_BUFFER(sh_set.line_size, sh_set.cmd_deep);
    shell_init((sh_io_desc_t *)&g_sh_std_io, &sh_set);

    g_sh_args.is_blocking = 1;
    g_sh_args.delay_time_ms = 40;
    g_sh_args.cb_regular_alarm = _sh_cb_alarm;

    shell_proc(&g_sh_args);

    return;
}
//=============================================================================
//                  Public Function Definition
//=============================================================================
static int
_cli_cmd_atplus(int argc, char **argv, cb_shell_out_t log_out, void *pExtra)
{
    log_out("%s\n", argv[0]);
    return 0;
}
static sh_cmd_t  g_cli_cmd_atplus __cli_cmd_pool =
{
    .pCmd_name      = "AT+",
    .cmd_exec       = _cli_cmd_atplus,
    .pDescription   = "AT+cmd\n"
    "  usage: AT+xxx\n"
    "    e.g. AT+advenable",
};
int
cli_console_init(void)
{
    xTaskCreate(_task_console, "cli", CLI_STACK_SIZE, NULL, TASK_PRIORITY_APP, NULL);

    shell_register_cmd(&g_cli_cmd_atplus);
    return 0;
}
#endif
