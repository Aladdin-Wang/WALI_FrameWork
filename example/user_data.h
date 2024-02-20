#ifndef __USER_DATA_H__
#define __USER_DATA_H__
#include "stdint.h"
typedef struct {
    char chProjectName[16];
    char chHardWareVersion[16];
    char chSoftBootVersion[16];
    char chSoftAppVersion[16];
	char wBoardId;
	char chPortName[16];
    uint32_t wPortBaudrate;	
	uint32_t wRestartTime;   
}msgSig_t;
typedef struct {
    union {
        msgSig_t sig;
        uint8_t B[sizeof(msgSig_t)];
    } msg_data;  
}user_data_t;

#endif