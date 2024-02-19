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

#include "wl_shell.h"
#include "../subscribe_publish/wl_subscribe_publish_agent.h"
#if defined(WL_USING_SHELL)
#include <stdio.h>
#include <string.h>
#include <assert.h>
#undef this
#define this        (*ptThis)

extern const int FSymTab$$Base;
extern const int FSymTab$$Limit;
extern wl_subscribe_publish_t    tShellSubPub;

def_topic(&tShellSubPub, shell_topic);
def_topic(&tShellSubPub, echo_topic, uint8_t*, uint16_t);

static void shell_push_history(wl_shell_t *ptObj);

/**
 * @brief Read input from the shell
 *
 * @param ptObj Pointer to the wl_shell_t object
 * @param pchData Pointer to the input data
 * @param hwLength Length of the input data
 */
void  wl_shell_read(wl_shell_t *ptObj, uint8_t *pchData, uint16_t hwLength)
{
    wl_shell_t *(ptThis) = ptObj;
    assert(NULL != ptObj);
    /*
    * handle control key
    * up key  : 0x1b 0x5b 0x41
    * down key: 0x1b 0x5b 0x42
    * right key:0x1b 0x5b 0x43
    * left key: 0x1b 0x5b 0x44
    */
    enum input_stat {
        WAIT_NORMAL,
        WAIT_SPEC_KEY,
        WAIT_FUNC_KEY,
    };
    static enum input_stat s_tInputStat = WAIT_NORMAL;

    for(uint16_t i = 0; i < hwLength; i++) {
        if(pchData[i] == 0) {
            continue;
        } else if(pchData[i] == 0x1b) {
            s_tInputStat = WAIT_SPEC_KEY;
            continue;
        } else if(s_tInputStat == WAIT_SPEC_KEY) {
            if(pchData[i] == 0x5b) {
                s_tInputStat = WAIT_FUNC_KEY;
                continue;
            }

            s_tInputStat = WAIT_NORMAL;
        } else if(s_tInputStat == WAIT_FUNC_KEY) {
            s_tInputStat = WAIT_NORMAL;

            if(pchData[i] == 0x41) {
                if (this.hwCurrenthistory > 0)
                    this.hwCurrenthistory --;
                else {
                    this.hwCurrenthistory = 0;
                    continue;
                }

                this.hwLinePosition = strlen(this.cHistoryCmdBuf[this.hwCurrenthistory]);
                this.hwLineLen = this.hwLinePosition;
                memcpy(this.chLineBuf, this.cHistoryCmdBuf[this.hwCurrenthistory], MSG_ARG_LEN);
            } else if(pchData[i] == 0x42) {
                /* next history */
                if (this.hwCurrenthistory < this.hwHistoryCount - 1)
                    this.hwCurrenthistory ++;
                else {
                    /* set to the end of history */
                    if (this.hwHistoryCount != 0)
                        this.hwCurrenthistory = this.hwHistoryCount - 1;
                    else
                        continue;
                }

                this.hwLinePosition = strlen(this.cHistoryCmdBuf[this.hwCurrenthistory]);
                this.hwLineLen = this.hwLinePosition;
                memcpy(this.chLineBuf, this.cHistoryCmdBuf[this.hwCurrenthistory], MSG_ARG_LEN);
            } else if(pchData[i] == 0x43) {
                if(this.hwLinePosition < this.hwLineLen) {
                    this.hwLinePosition ++;
                }
            } else if(pchData[i] == 0x44) {
                if(this.hwLinePosition != 0) {
                    this.hwLinePosition --;
                }
            }

            continue;
        } else {
            s_tInputStat = WAIT_NORMAL;
        }

        if (pchData[i] == '\r' || pchData[i]  == '\n') {
            if(this.hwLinePosition  == strlen(this.chLineBuf) && this.hwLinePosition != 0) {
                shell_push_history(ptObj);
                this.chLineBuf[this.hwLinePosition++] = pchData[i];
                enqueue(&this.tByteInQueue, this.chLineBuf, this.hwLinePosition );
                publish(&tShellSubPub, __MSG_TOPIC(shell_topic));
            }

            memset(this.chLineBuf, 0, sizeof(this.chLineBuf));
            this.hwLinePosition = 0;
        } else if(pchData[i] == 0x7f || pchData[i] == 0x08 ) { /* handle backspace key */
            if (this.hwLinePosition != 0) {
                this.chLineBuf[--this.hwLinePosition] = 0;
                this.hwLineLen = this.hwLinePosition;
            }
        } else {
            if (this.hwLinePosition < (MSG_ARG_LEN - 1)) {
                this.chLineBuf[this.hwLinePosition++] = pchData[i];
                this.hwLineLen = this.hwLinePosition;
            } else {
                this.hwLinePosition = 0;
                this.hwLineLen = this.hwLinePosition;
            }
        }
    }

    if(this.bEchoMode != false) {
        publish(&tShellSubPub, __MSG_TOPIC(echo_topic), pchData, hwLength);
    }
}

/**
 * @brief Execute shell commands
 *
 * @param ptObj Pointer to the wl_shell_t object
 */
void wl_shell_exec(wl_shell_t *ptObj)
{
    wl_shell_t *(ptThis) = ptObj;
    assert(NULL != ptObj);

    while(call_fsm( search_msg_map,  &this.fsmSearchMsgMap ) != fsm_rt_cpl);
}

/**
 * @brief Echo shell input
 *
 * @param ptObj Pointer to the wl_shell_t object
 * @param pchData Pointer to the input data
 * @param hwLength Length of the input data
 */
void wl_shell_echo(wl_shell_t *ptObj, uint8_t *pchData, uint16_t hwLength)
{
    wl_shell_t *(ptThis) = ptObj;
    assert(NULL != ptObj);
    /*
    * handle control key
    * up key  : 0x1b 0x5b 0x41
    * down key: 0x1b 0x5b 0x42
    * right key:0x1b 0x5b 0x43
    * left key: 0x1b 0x5b 0x44
    */
    enum input_stat {
        WAIT_NORMAL,
        WAIT_SPEC_KEY,
        WAIT_FUNC_KEY,
    };
    static enum input_stat s_tInputStat = WAIT_NORMAL;

    for(uint16_t i = 0; i < hwLength; i++) {
        if(pchData[i] == 0) {
            continue;
        } else if(pchData[i] == 0x1b) {
            s_tInputStat = WAIT_SPEC_KEY;
            continue;
        } else if(s_tInputStat == WAIT_SPEC_KEY) {
            if(pchData[i] == 0x5b) {
                s_tInputStat = WAIT_FUNC_KEY;
                continue;
            }

            s_tInputStat = WAIT_NORMAL;
        } else if(s_tInputStat == WAIT_FUNC_KEY) {
            s_tInputStat = WAIT_NORMAL;

            if(pchData[i] == 0x41) {
                if(strlen(this.chLineBuf) > 0) {
                    printf("\033[2K\r-_- %s", this.chLineBuf);
                }
            } else if(pchData[i] == 0x42) {
                if(strlen(this.chLineBuf) > 0) {
                    printf("\033[2K\r-_- %s", this.chLineBuf);
                }
            } else if(pchData[i] == 0x43) {
                if(this.hwCurposPosition < this.hwLineLen) {
                    printf("%c", this.chLineBuf[this.hwCurposPosition++]);
                }
            } else if(pchData[i] == 0x44) {
                if(this.hwCurposPosition != 0) {
                    this.hwCurposPosition--;
                    printf("\b");
                }
            }

            continue;
        } else {
            s_tInputStat = WAIT_NORMAL;
        }

        if (pchData[i] == '\r' || pchData[i]  == '\n') {
            this.hwCurposPosition = 0;
            printf("\r\n-_- ");
        } else if(pchData[i] == 0x7f || pchData[i] == 0x08 ) { /* handle backspace key */
            if(this.hwCurposPosition != 0) {
                this.hwCurposPosition--;
                printf("\b \b");
            }
        } else {
            this.hwCurposPosition++;
            printf("%c", pchData[i]);
        }
    }
}

/**
 * @brief Initialize the shell
 *
 * @param ptObj Pointer to the wl_shell_t object
 * @return wl_shell_t* Pointer to the initialized wl_shell_t object
 */
wl_shell_t *wl_shell_init(wl_shell_t *ptObj)
{
    wl_shell_t *(ptThis) = ptObj;
    assert(NULL != ptObj);

    this.bEchoMode = SHELL_OPTION_ECHO;

    queue_init(&this.tByteInQueue, this.chQueueBuf, sizeof(this.chQueueBuf), true);
    init_fsm(search_msg_map, &this.fsmSearchMsgMap, args((msg_t *)&FSymTab$$Base, (msg_t *)&FSymTab$$Limit, &this.tByteInQueue, true));

    subscribe(&tShellSubPub, __MSG_TOPIC(shell_topic), &this, SLOT(wl_shell_exec));
    subscribe(&tShellSubPub, __MSG_TOPIC(echo_topic), &this, SLOT(wl_shell_echo));

    return ptObj;
}

/**
 * @brief get echo mode
 *
 * @param ptObj Pointer to the wl_shell_t object
 */
bool wl_shell_get_echo(wl_shell_t *ptObj)
{
    wl_shell_t *(ptThis) = ptObj;
    assert(NULL != ptObj);
    return this.bEchoMode;
}

/**
 * @brief set echo mode
 *
 * @param ptObj Pointer to the wl_shell_t object
 * @param bEchoMode to set the Echo Mode
 */
void wl_shell_set_echo(wl_shell_t *ptObj, bool bEchoMode)
{
    wl_shell_t *(ptThis) = ptObj;
    assert(NULL != ptObj);
    this.bEchoMode = bEchoMode;
}

static void shell_push_history(wl_shell_t *ptObj)
{
    wl_shell_t *(ptThis) = ptObj;
    assert(NULL != ptObj);

    if (this.hwLinePosition != 0) {
        /* push history */
        if (this.hwHistoryCount >= SHELL_HISTORY_LINES) {
            /* if current cmd is same as last cmd, don't push */
            if (memcmp(&this.cHistoryCmdBuf[SHELL_HISTORY_LINES - 1], this.chLineBuf, MSG_ARG_LEN)) {
                /* move history */
                int index;

                for (index = 0; index < SHELL_HISTORY_LINES - 1; index ++) {
                    memcpy(&this.cHistoryCmdBuf[index][0],
                           &this.cHistoryCmdBuf[index + 1][0], MSG_ARG_LEN);
                }

                memset(&this.cHistoryCmdBuf[index][0], 0, MSG_ARG_LEN);
                memcpy(&this.cHistoryCmdBuf[index][0], this.chLineBuf, this.hwLinePosition);

                /* it's the maximum history */
                this.hwHistoryCount = SHELL_HISTORY_LINES;
            }
        } else {
            /* if current cmd is same as last cmd, don't push */
            if (this.hwHistoryCount == 0 || memcmp(&this.cHistoryCmdBuf[this.hwHistoryCount - 1], this.chLineBuf, MSG_ARG_LEN)) {
                this.hwCurrenthistory = this.hwHistoryCount;
                memset(&this.cHistoryCmdBuf[this.hwHistoryCount][0], 0, MSG_ARG_LEN);
                memcpy(&this.cHistoryCmdBuf[this.hwHistoryCount][0], this.chLineBuf, this.hwLinePosition);
                /* increase count and set current history position */
                this.hwHistoryCount ++;
            }
        }
    }

    this.hwCurrenthistory = this.hwHistoryCount;
}

static int msh_help(int argc, char **argv)
{
    printf("\r\nshell commands:\r\n");
    {
        msg_t *ptMsgTableBase = (msg_t *)&FSymTab$$Base;
        msg_t *ptMsgTableLimit = (msg_t *)&FSymTab$$Limit;

        for (uint32_t i = 0; &ptMsgTableBase[i] != ptMsgTableLimit; i++) {
            printf("%-16s - %s\r\n", ptMsgTableBase[i].pchMessage, ptMsgTableBase[i].pchDesc);
        }
    }
    return 0;
}
MSH_FUNCTION_EXPORT_CMD(msh_help, help, shell help);

#endif
