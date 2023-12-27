#ifndef APPLICATIONS_CHECK_USE_PEEK_H_
#define APPLICATIONS_CHECK_USE_PEEK_H_
#include "./app_cfg.h"
#if USE_SERVICE_CHECK_USE_PEEK == ENABLED
#include "./queue/wl_queue.h"
#include "./fsm/simple_fsm.h"

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
