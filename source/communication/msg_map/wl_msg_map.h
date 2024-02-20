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

#ifndef __SERVE_MSG_MAP_H_
#define __SERVE_MSG_MAP_H_
#include "./app_cfg.h"
#if defined(WL_USING_MSG_MAP)
#include "wl_msg_check_str.h"
#include "wl_msg_check_arg.h"
#include "../../generic/queue/wl_queue.h"
#include "../../fsm/simple_fsm.h"
#ifdef __cplusplus
extern "C" {
#endif

#define __section(x)               __attribute__((section(x)))
#define __used                     __attribute__((used))


/********************************************************************************************/
typedef struct _msg_t msg_t;
typedef fsm_rt_t msg_hanlder_t(int argc, char **argv);
struct _msg_t{
    const char *pchMessage;
    msg_hanlder_t *fnHandler;
    const char *pchDesc;
};

declare_simple_fsm(search_msg_map);
extern_fsm_implementation(search_msg_map);
extern_fsm_initialiser( search_msg_map,
        args(
		    msg_t *ptMsgTableBase,
		    msg_t *ptMsgTableLimit,
		    byte_queue_t *ptQueue,
		    bool bArgIsString
        ))
extern_simple_fsm(search_msg_map,
    def_params(
			fsm(check_string)  fsmCheckStr;
			fsm(check_arg)     fsmCheckArg;
			msg_t              *ptMsgTableBase;
			msg_t              *ptMsgTableLimit;	
            uint8_t            chByte;            
			uint16_t           hwIndex;
            byte_queue_t       *ptQueue;
			bool               bArgIsString;
			bool               bIsRequestDrop;
			char               *argv[MSG_ARG_MAX];
			int                argc;
    )
)

#ifdef __cplusplus
}
#endif
#endif
#endif /* MSG_MAP_MSG_MAP_H_ */


