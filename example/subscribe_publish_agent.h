#ifndef APPLICATIONS_SUBSCRIBE_AND_PUBLISH_H_
#define APPLICATIONS_SUBSCRIBE_AND_PUBLISH_H_
#include "./communication/subscribe_publish/wl_subscribe_publish.h"
#include "./check_agent_engine/wl_check_agent_engine.h"

typedef struct subscribe_publish_agent_t {
    check_agent_t          tCheckAgent;
    wl_subscribe_publish_t tSubPub;
} subscribe_publish_agent_t;

extern subscribe_publish_agent_t *subscribe_publish_agent_init(subscribe_publish_agent_t *ptObj);

#endif