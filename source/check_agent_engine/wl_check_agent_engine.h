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

#ifndef __SERVE_CHECK_AGENT_ENGINE_H_
#define __SERVE_CHECK_AGENT_ENGINE_H_
#include "./app_cfg.h"
#if USE_SERVICE_CHECK_USE_PEEK == ENABLED
#include "../Generic/queue/wl_queue.h"
#include "../fsm/simple_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif
          
typedef  struct _check_agent_t check_agent_t;
typedef fsm_rt_t check_hanlder_t(void *ptThis);
struct _check_agent_t{
    check_agent_t *ptNext;
    check_hanlder_t *fnCheck;
    void * pAgent;
};

declare_simple_fsm(check_use_peek);
extern_fsm_implementation(check_use_peek);
extern_fsm_initialiser( check_use_peek)

extern_simple_fsm(check_use_peek,
    def_params(
            check_agent_t *ptFreeList;
            check_agent_t *ptCheckList;
    )
)
extern bool agent_register(fsm_check_use_peek_t *ptObj,check_agent_t *ptNewItem);
extern bool agent_unregister(fsm_check_use_peek_t *ptObj,check_agent_t *ptNote);

#ifdef __cplusplus
}
#endif

#endif
#endif /* APPLICATIONS_CHECK_USE_PEEK_H_ */
