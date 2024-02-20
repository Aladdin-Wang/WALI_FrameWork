#include "subscribe_publish_agent.h"

#undef this
#define this        (*ptThis)
	
subscribe_publish_agent_t *subscribe_publish_agent_init(subscribe_publish_agent_t *ptObj)
{
    assert(NULL != ptObj);
    subscribe_publish_agent_t *(ptThis) = ptObj;	
	
    this.tCheckAgent.pAgent = &this.tSubPub.fsmSearchTopicMap;
    this.tCheckAgent.fnCheck = (check_hanlder_t *)search_msg_map;;
    this.tCheckAgent.ptNext = NULL;	
	
	  wl_subscribe_publish_init(&this.tSubPub);
	  return ptObj;
}


