/****************************************************************************
*  Copyright 2022 kk (https://github.com/Aladdin-Wang)                                    *
*                                                                           *
*  Licensed under the Apache License, Version 2.0 (the "License");          *
*  you may not use this file except in compliance with the License.         *
*  You may obtain a copy of the License at                                  *
*                                                                           *
*     http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                           *
*  Unless required by applicable law or agreed to in writing, software      *
*  distributed under the License is distributed on an "AS IS" BASIS,        *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
*  See the License for the specific language governing permissions and      *
*  limitations under the License.                                           *
*                                                                           *
****************************************************************************/

#ifndef __SERVE_SHELL_H_
#define __SERVE_SHELL_H_
#include "./app_cfg.h"
#if defined(WL_USING_SHELL)
#include "../subscribe_publish/wl_subscribe_publish.h"

/**
 * @ingroup msh
 *
 * This macro exports a command to module shell.
 *
 * @param command is the name of the command.
 * @param desc is the description of the command, which will show in help list.
 */
#define MSH_FUNCTION_EXPORT_CMD(name, cmd, desc)                      \
    const char __fsym_##cmd##_name[] __section(".rodata.name") = #cmd;    \
    const char __fsym_##cmd##_desc[] __section(".rodata.name") = #desc;   \
    const __used struct _msg_t __fsym_##cmd __section("FMshTab")= \
            {                           \
                                        __fsym_##cmd##_name,    \
                                        (msg_hanlder_t *)&name,     \
                                        __fsym_##cmd##_desc,    \
            };

#define MSH_CMD_EXPORT(command, desc)   \
    MSH_FUNCTION_EXPORT_CMD(command, command, desc)


typedef struct wl_shell_t {
	  wl_subscribe_publish_t    tShellSubPub;
    fsm(search_msg_map)       fsmSearchMsgMap;
    byte_queue_t              tByteInQueue;
    char                      chQueueBuf[MSG_ARG_LEN];
    char                      chLineBuf[MSG_ARG_LEN];
    bool                      bEchoMode;
    uint16_t                  hwLineLen;
    uint16_t                  hwLinePosition;
    uint16_t                  hwCurposPosition;
    uint16_t                  hwCurrenthistory;
    uint16_t                  hwHistoryCount;
    char                      cHistoryCmdBuf[SHELL_HISTORY_LINES][MSG_ARG_LEN];
} wl_shell_t;

extern wl_shell_t *wl_shell_init(wl_shell_t *ptObj);
extern void  wl_shell_read(wl_shell_t *ptObj, uint8_t *pchData, uint16_t hwLength);
extern void wl_shell_exec(wl_shell_t *ptObj);
extern void wl_shell_echo(wl_shell_t *ptObj, uint8_t *pchData, uint16_t hwLength);
extern bool wl_shell_get_echo(wl_shell_t *ptObj);
extern void wl_shell_set_echo(wl_shell_t *ptObj, bool bEchoMode);
#endif
#endif /* APPLICATIONS_CHECK_AGENT_XMODEM_H_ */