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

#include "wl_subscribe_publish.h"
#if defined(WL_USING_SUBSCRIBE_PUBLISH)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef this
#define this        (*ptThis)

wl_subscribe_publish_t *wl_subscribe_publish_init(wl_subscribe_publish_t *ptObj)
{
    assert(NULL != ptObj);

    wl_subscribe_publish_t *(ptThis) = ptObj;

    extern const int FMsgTab$$Base;
    extern const int FMsgTab$$Limit;
	
    queue_init(&this.tByteInQueue,this.chQueueBuf,sizeof(this.chQueueBuf),true);
    init_fsm(search_msg_map, &this.fsmSearchTopicMap, args((msg_t *)&FMsgTab$$Base, (msg_t *)&FMsgTab$$Limit, &this.tByteInQueue,false));
    return ptObj;
}

/**
 * @brief Execute shell commands
 *
 * @param ptObj Pointer to the wl_shell_t object
 */
void wl_subscribe_publish_exec(wl_subscribe_publish_t *ptObj)
{
    wl_subscribe_publish_t *(ptThis) = ptObj;
    assert(NULL != ptObj);

    while(call_fsm( search_msg_map,  &this.fsmSearchTopicMap ) != fsm_rt_cpl);
}

#endif

