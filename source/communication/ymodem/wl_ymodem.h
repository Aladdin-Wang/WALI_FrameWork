#ifndef __XMODEM_H
#define __XMODEM_H
#include "./app_cfg.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PACKET_CAN                      = 3,
    PACKET_EOT                      = 2,
    PACKET_ON_GOING                 = 1,     //!< on-going
    PACKET_CPL                      = 0,     //!< complete
    PACKET_TIMEOUT                  = -1,    //!< read timeout
    PACKET_INCORRECT_HEADER         = -2,    //!< incorrect head char
    PACKET_INCORRECT_PACKET_NUMBER  = -3,    //!< incorrect packet number
    PACKET_DUPLICATE_PACKET_NUMBER  = -4,    //!< duplicate packet number
    PACKET_INCORRECT_CHECKOUT       = -5,    //!< incorrect checkout
    PACKET_INCORRECT_NBlk           = -6,    //!< incorrect NBlk
    PACKET_FAIL                     = -7,    //!< packet faile
}ym_packet_state_t;


typedef enum {
    YMODEM_READ_TIMEOUT             = -1,
    YMODEM_READ_CPL                 = 0,
    YMODEM_READ_ONGOING             = 1,	
}ym_read_stat_t;

typedef struct ym_t ym_t;
typedef bool (ym_call_back)(ym_t *ptObj,uint8_t *pchBuffer,uint16_t *phwSize);
typedef ym_read_stat_t (ym_read_with_timeout_call_back)(ym_t *ptObj,uint8_t* pchByte,uint16_t hwSize,uint16_t hwFrameTimeout,uint16_t hwBytesTimeout);
typedef bool (ym_write_call_back)(ym_t *ptObj,uint8_t* pchByte,uint16_t hwSize);
struct ym_t{
	uint8_t                          *pchBuffer;
    uint16_t                          hwSize;
    ym_call_back                     *fnOnFilePath;    
    ym_call_back                     *fnOnFileData;
	ym_read_with_timeout_call_back   *fnReadDataWithTimeout;
	ym_write_call_back               *fnWriteData;	
};

typedef struct ym_package_t{
    uint8_t                          chState;
	uint8_t                          chByte;
    uint8_t                          chHead;
    uint8_t                          chBlk;
    uint8_t                          chNBlk;
    uint8_t                          chCheck[2];
    uint16_t                         hwCheck;
    uint8_t                          chPacketNum; 
	bool                             bIsPackagePath;  
}ym_package_t;

typedef struct ymodem_t{
	ym_t                             tYm;
	ym_package_t                     tPackage;
	uint8_t                          chState;
	uint8_t                          chByte;
	uint8_t                          chTryCount;
	ym_packet_state_t                tPackageState;
	uint8_t                          chEOTStep;
}ymodem_t;

bool ymodem_init(
	         ymodem_t                         *ptThis,
             uint8_t                          *pchBuffer,
             uint16_t                          hwSize,
             ym_call_back                     *fnOnFilePath,
		     ym_call_back                     *fnOnFileData,
	         ym_read_with_timeout_call_back   *fnReadDataWithTimeout,
	         ym_write_call_back               *fnWriteData
        );
ym_packet_state_t ymodem_receive( ymodem_t *ptThis);
ym_packet_state_t ymodem_send( ymodem_t *ptThis);

#ifdef __cplusplus
}
#endif

#endif
