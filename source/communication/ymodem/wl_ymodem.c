#include "wl_ymodem.h"

#define SOH            0x01
#define STX            0x02
#define EOT            0x04
#define ACK            0x06
#define NAK            0x15
#define CAN            0x18
#define CRC_C          0x43

#define DLY_1S         (1000U)
#define DLY_3S         (3*DLY_1S)
#define DLY_10S        (10*DLY_1S)

#ifndef MODEM_MODEM_MAX_TRY_AGAN
#   define MODEM_MAX_TRY_AGAN          (10U)
#endif

#define MODEM_DATA_BUFFER_SIZE         (128ul)
#define MODEM_1K_DATA_BUFFER_SIZE      (1024ul)

#undef this
#define this        (*ptThis)


#ifdef YMODEM_USING_CRC_TABLE
static const uint16_t ccitt_table[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
static uint16_t CRC16(uint8_t *pchData, int hwLen)
{
    uint16_t crc = 0;
    while (hwLen-- > 0){
        crc = (crc << 8) ^ ccitt_table[((crc >> 8) ^ *pchData++) & 0xff];
    }
    return crc;
}
#else
static uint16_t CRC16(unsigned char *q, int len)
{
    uint16_t crc;
    char i;

    crc = 0;
    while (--len >= 0){
        crc = crc ^ (int) * q++ << 8;
        i = 8;
        do{
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        }while (--i);
    }

    return (crc);
}
#endif


static uint16_t check_sum(uint8_t *pchData, uint32_t hwLen)
{
    uint16_t checksum = 0;
    while(hwLen--){
        checksum  += *pchData++;
    }
    return checksum;
}

static ym_packet_state_t ymodem_unpackaging(ym_package_t *ptThis,ym_t *ptYm)
{
    enum { START = 0, READ_HEAD, READ_BLK, READ_NBLK, READ_DATA, READ_CHECK_L, READ_CHECK_H, CHECK_PACKAGE};
    switch(this.chState){
        case START:{
            this.chState = READ_HEAD;            
        }
        case READ_HEAD:{
            ym_read_stat_t tFsm = ptYm->fnReadDataWithTimeout(ptYm,&this.chHead,1,DLY_3S,DLY_1S);
            if(YMODEM_READ_CPL == tFsm){
                if(this.chHead == EOT){
                    this.chState = START;
                    return PACKET_EOT;
                }else if (this.chHead == CAN){
                    this.chState = START;
                    return PACKET_CAN;
                }else if(this.chHead == SOH){
                    ptYm->hwSize = MODEM_DATA_BUFFER_SIZE;
                    this.chState = READ_BLK;
                }else if(this.chHead == STX){
                    ptYm->hwSize = MODEM_1K_DATA_BUFFER_SIZE;
                    this.chState = READ_BLK;
                }else{
                    return PACKET_INCORRECT_HEADER;
                }
            }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            } 
        }
        case READ_BLK:{
            ym_read_stat_t tFsm = ptYm->fnReadDataWithTimeout(ptYm,&this.chBlk,1,DLY_1S,DLY_1S);        
            if(YMODEM_READ_CPL == tFsm){
                this.chState = READ_NBLK;
            }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            }             
        }
        case READ_NBLK:{
			ym_read_stat_t tFsm = ptYm->fnReadDataWithTimeout(ptYm,&this.chNBlk,1,DLY_1S,DLY_1S);    
            if(YMODEM_READ_CPL == tFsm){
                if(0xff != (this.chBlk ^ this.chNBlk)){
                    this.chState = START;
                    return PACKET_INCORRECT_NBlk;
                }
                this.chState = READ_DATA;
            }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            }                     
        }
        case READ_DATA:{ 
			ym_read_stat_t tFsm = ptYm->fnReadDataWithTimeout(ptYm,ptYm->pchBuffer,ptYm->hwSize,DLY_1S,DLY_1S);    
            if(YMODEM_READ_CPL == tFsm){
			    this.hwCheck = CRC16(&ptYm->pchBuffer[0],ptYm->hwSize);
                this.chState = READ_CHECK_L;               
            }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            }            
        }
        case READ_CHECK_L:{
			ym_read_stat_t tFsm = ptYm->fnReadDataWithTimeout(ptYm,&this.chCheck[0],1,DLY_1S,DLY_1S);           
            if(YMODEM_READ_CPL == tFsm){
                this.chState = READ_CHECK_H;
            }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            }          
        }
        case READ_CHECK_H:{
			ym_read_stat_t tFsm = ptYm->fnReadDataWithTimeout(ptYm,&this.chCheck[1],1,DLY_1S,DLY_1S);   
            if(YMODEM_READ_CPL == tFsm){
                if(this.hwCheck != (uint16_t)this.chCheck[0] * 256
                        + (uint16_t)this.chCheck[1]  ){
                    this.chState = START;
                    return PACKET_INCORRECT_CHECKOUT;
                }else{ 
                    this.chState = CHECK_PACKAGE;
                }
            }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            }                 
        }
        case CHECK_PACKAGE:{
            if(this.chPacketNum == this.chBlk ||
                    this.chBlk == (this.chPacketNum - 1)){
                if(this.chBlk == (this.chPacketNum - 1)){
                    this.chState = START;
                    return PACKET_DUPLICATE_PACKET_NUMBER;
                }else {
					if(this.bIsPackagePath == false){
					    ptYm->fnOnFileData(ptYm, ptYm->pchBuffer,&ptYm->hwSize);
					}else{
					    ptYm->fnOnFilePath(ptYm, ptYm->pchBuffer,&ptYm->hwSize);
					}						
                    this.chPacketNum ++;
                }                             
                this.chState = START;
                this.hwCheck = 0;  
                return PACKET_CPL;
            }else{
                this.chState = START;
                return  PACKET_INCORRECT_PACKET_NUMBER;
            }                   
        }            
    }
    return PACKET_ON_GOING;
}

static ym_packet_state_t ymodem_packaging(ym_package_t *ptThis,ym_t *ptYm)
{
    enum { START = 0, READ_BYTE, GET_PACKAGE};
    switch(this.chState){
        case START:{
            this.chState = READ_BYTE;            
        }	
        case READ_BYTE:{
            ym_read_stat_t tFsm = ptYm->fnReadDataWithTimeout(ptYm,&this.chByte,1,DLY_10S,DLY_1S);        
            if(YMODEM_READ_CPL == tFsm){
				if(this.chByte == CRC_C){
                    this.chState = GET_PACKAGE;                  				
				}else if(this.chByte == ACK && this.bIsPackagePath == false){
                	this.chPacketNum++;
                    this.chState = GET_PACKAGE;					
				}else if(this.chByte == NAK){
                    return PACKET_INCORRECT_PACKET_NUMBER;
				}else if(this.chByte == CAN){
					this.chState = START;
                    return PACKET_FAIL;
				}else{
					return PACKET_INCORRECT_HEADER;	
				}
            }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            }             
        }	
        case GET_PACKAGE:{
			memset(&ptYm->pchBuffer[3],0x1A,ptYm->hwSize);
			if(this.bIsPackagePath != false){
				ptYm->fnOnFilePath(ptYm, &ptYm->pchBuffer[3],&ptYm->hwSize);
			}else{
				ptYm->fnOnFileData(ptYm, &ptYm->pchBuffer[3],&ptYm->hwSize);				
			}
			ptYm->pchBuffer[1] = this.chPacketNum;
			ptYm->pchBuffer[2] = ~this.chPacketNum;	
		
			if(ptYm->hwSize == 0){
				ptYm->hwSize = 0;
                this.chState = START;
			    return PACKET_EOT;					
			}else if(ptYm->hwSize <= MODEM_DATA_BUFFER_SIZE){
				ptYm->pchBuffer[0] = SOH;				
				if(ptYm->hwSize < MODEM_DATA_BUFFER_SIZE){
					this.hwCheck = CRC16(&ptYm->pchBuffer[3],MODEM_DATA_BUFFER_SIZE);
					ptYm->hwSize = MODEM_DATA_BUFFER_SIZE + 5;
			        ptYm->pchBuffer[ptYm->hwSize - 1] = (uint8_t)this.hwCheck;	
			        ptYm->pchBuffer[ptYm->hwSize - 2] = (uint8_t)(this.hwCheck>>8);						
					return PACKET_EOT;
				}	
				ptYm->hwSize = MODEM_DATA_BUFFER_SIZE + 5;				
			}else if(ptYm->hwSize <= MODEM_1K_DATA_BUFFER_SIZE){
				ptYm->pchBuffer[0] = STX;
				if(ptYm->hwSize < MODEM_1K_DATA_BUFFER_SIZE){
					this.hwCheck = CRC16(&ptYm->pchBuffer[3],MODEM_1K_DATA_BUFFER_SIZE);
				    ptYm->hwSize = MODEM_1K_DATA_BUFFER_SIZE + 5;						
			        ptYm->pchBuffer[ptYm->hwSize - 1] = (uint8_t)this.hwCheck;	
			        ptYm->pchBuffer[ptYm->hwSize - 2] = (uint8_t)(this.hwCheck>>8);
					return PACKET_EOT;
				}
				ptYm->hwSize = MODEM_1K_DATA_BUFFER_SIZE + 5;
			}else{
				this.chState = START;
                return PACKET_FAIL;			
			}
			this.hwCheck = CRC16(&ptYm->pchBuffer[3],ptYm->hwSize - 5);
			ptYm->pchBuffer[ptYm->hwSize - 1] = (uint8_t)this.hwCheck;	
			ptYm->pchBuffer[ptYm->hwSize - 2] = (uint8_t)(this.hwCheck>>8);					
            this.chState = START;
			return PACKET_CPL;				  
        }			
	}

    return PACKET_ON_GOING;
}

static bool ymodem_packaging_init(ym_package_t *ptThis)
{
    if(ptThis == NULL){
		return false;
	}
	
	this.chState = 0;

    return true;	
}

static bool ymodem_unpackaging_init(ym_package_t *ptThis)
{
	if(ptThis == NULL){
		return false;
	}
    this.chState = 0;
	
	return true;
}

bool ymodem_init(
		 ymodem_t                         *ptThis,
		 uint8_t                          *pchBuffer,
         uint16_t                          hwSize,
         ym_call_back                     *fnOnFilePath,
		 ym_call_back                     *fnOnFileData,
		 ym_read_with_timeout_call_back   *fnReadDataWithTimeout,
		 ym_write_call_back               *fnWriteData)
{
	if (NULL == pchBuffer || NULL == fnOnFilePath ||  NULL == fnOnFileData ||
			 NULL == fnReadDataWithTimeout || NULL == fnWriteData) {
		return false;
	}
	this.tYm.fnOnFilePath = fnOnFilePath;    
	this.tYm.fnOnFileData = fnOnFileData;
	this.tYm.fnReadDataWithTimeout = fnReadDataWithTimeout;
	this.tYm.fnWriteData = fnWriteData;		
	this.tYm.pchBuffer = pchBuffer;
	this.tYm.hwSize = hwSize;
	
	return true;		
}

ym_packet_state_t ymodem_receive( ymodem_t *ptThis)
{
    enum { START = 0, HANDSHAKE, PACK_PATH, SEND_ACK, SEND_C, PACK_DATA, SEND};

    switch(this.chState){
        case START:{   
            this.chTryCount = 0;
			this.tPackage.bIsPackagePath = true;
			this.tPackage.chPacketNum = 0;
			this.chState = HANDSHAKE;
            //break;			
		}

        case HANDSHAKE:{
			this.chByte = CRC_C;
			if(this.tYm.fnWriteData(&this.tYm,&this.chByte,1)) {
				this.chState = PACK_PATH;
			}  
            //break;			
        }
        case PACK_PATH:{
			this.tPackageState = ymodem_unpackaging(&this.tPackage,&this.tYm);
            if(this.tPackageState == PACKET_CPL){
				this.chByte = ACK;
				this.chState = SEND_ACK;
			}else if(this.tPackageState == PACKET_TIMEOUT){
				this.chState = HANDSHAKE;
                break;				
			}else{
				break;
			}
		}	
        case SEND_ACK:{
			if(this.tYm.fnWriteData(&this.tYm,&this.chByte,1)) {	
                this.tPackage.bIsPackagePath = false;				
				this.chByte = CRC_C;
				this.chState = SEND_C;                 
			}
            break;			
		}
        case SEND_C:{
			if(this.tYm.fnWriteData(&this.tYm,&this.chByte,1)) {					
				this.chByte = ACK;
				this.chState = PACK_DATA;                 
			}					
		}		
        case PACK_DATA:{
			this.tPackageState = ymodem_unpackaging(&this.tPackage,&this.tYm);
			switch(this.tPackageState){
				case PACKET_ON_GOING: {
					 return this.tPackageState;
				}
				case PACKET_FAIL: {
					 this.chByte = CAN;
					 break;
				}
				case PACKET_CAN:
					 this.chByte = ACK;
				     break;
				case PACKET_CPL:
					 this.chByte = ACK;
					 this.chTryCount = 0;
					 break;
				case PACKET_INCORRECT_HEADER:
				case PACKET_INCORRECT_PACKET_NUMBER:
				case PACKET_INCORRECT_NBlk:
				case PACKET_INCORRECT_CHECKOUT:		
				case PACKET_TIMEOUT: 
					 this.chTryCount++;
					 if(this.chTryCount >= MODEM_MAX_TRY_AGAN){
						 this.chByte = CAN;
					 }else{
						 this.chByte = NAK;
					 }					 
					 break;
				case PACKET_DUPLICATE_PACKET_NUMBER: {  
					 this.chTryCount++;  
					 if(this.chTryCount >= MODEM_MAX_TRY_AGAN){
						 this.chByte = CAN;
					 }else{
						 this.chByte = ACK;
					 }						
					 break;
				}  
				case PACKET_EOT: {  
					 if(this.chByte == ACK){
						 this.chByte = NAK;
					 }else{
						 this.chByte = ACK;
					 }						 
					 break;
				}                 
			} 
			this.chState = SEND;
            //break;			
        }
	
        case SEND:{
			if(this.tYm.fnWriteData(&this.tYm,&this.chByte,1)) {					
				if(this.tPackageState == PACKET_CAN || this.chByte == CAN){
                    this.chState = START;
				}else{
					if(this.tPackageState == PACKET_EOT && this.chByte == ACK  ){
						this.chState = START;
					}else{
					    this.chState = PACK_DATA;
					}
				}
			}					
		}
	    break;     
	}
	return this.tPackageState;
}

ym_packet_state_t ymodem_send( ymodem_t *ptThis)
{
    enum { START = 0, PACK_PATH,SEND_PATH,READ_ACK,PACK_DATA,SEND_DATA,READ_BYTE,SEND_EOT};

    switch(this.chState){
        case START:{ 		
            this.chTryCount = 0;
            this.chEOTStep = 0;		
			this.tPackage.chPacketNum = 0;
			this.tPackage.bIsPackagePath = true;
			this.chState = PACK_PATH;			
		}		
        case PACK_PATH:{ 		
            this.tPackageState = ymodem_packaging(&this.tPackage,&this.tYm);      
            if(this.tPackageState == PACKET_CPL){				
			    this.chState = SEND_PATH;
			}else{
                break;
            } 			
		}
        case SEND_PATH:{ 		
			if(this.tYm.fnWriteData(&this.tYm,this.tYm.pchBuffer,this.tYm.hwSize)) {					
                 this.chState = READ_ACK;
			}else{
                 break;
			}				
		}		
        case READ_ACK:{
			ym_read_stat_t tFsm = this.tYm.fnReadDataWithTimeout(&this.tYm,&this.chByte,1,DLY_10S,DLY_1S); 
			if(YMODEM_READ_CPL == tFsm){
				if(this.chByte == ACK){		
					this.tPackage.chPacketNum = 1;	
				    this.tPackage.bIsPackagePath = false;					
					this.chState = PACK_DATA;
				}else if(this.chByte == CAN){
			        this.chState = START;
                    return PACKET_FAIL;	
				}else{
					return PACKET_INCORRECT_HEADER;							
				}
		    }else if(YMODEM_READ_TIMEOUT == tFsm){
                this.chState = START;
                return PACKET_TIMEOUT;
            }else{
                break;
            } 
		}
        case PACK_DATA:{ 		
            this.tPackageState = ymodem_packaging(&this.tPackage,&this.tYm);
            if(this.tPackageState == PACKET_CPL || this.tPackageState == PACKET_EOT){
				this.chTryCount = 0;
                this.chState = SEND_DATA;
			}else if(this.tPackageState == PACKET_INCORRECT_PACKET_NUMBER){
				this.chTryCount++;
                if(this.chTryCount > MODEM_MAX_TRY_AGAN){
                    this.chState = START;
                    return PACKET_FAIL;					
				}
                this.chState = SEND_DATA;				
			}else if(this.tPackageState == PACKET_FAIL || this.tPackageState == PACKET_TIMEOUT){
                    this.chState = START;
                    return PACKET_FAIL;					
			}else{
                break;
			}				
		}	
        case SEND_DATA:{ 		
			if(this.tYm.fnWriteData(&this.tYm,this.tYm.pchBuffer,this.tYm.hwSize)) {					
				 if(this.tPackageState == PACKET_EOT){
					 this.chState = READ_BYTE;
				 }else{
                     this.chState = PACK_DATA;
                     break;					 
				 }
			}				
		}			
        case READ_BYTE:{
			if(this.tYm.hwSize == 0 && this.chEOTStep == 0){
				this.chEOTStep ++;
				this.chState = SEND_EOT;					
			}else{
				ym_read_stat_t tFsm = this.tYm.fnReadDataWithTimeout(&this.tYm,&this.chByte,1,DLY_10S,DLY_1S);        
				if(tFsm == YMODEM_READ_CPL){
					if(this.chByte == ACK && this.chEOTStep == 0 ){
						this.chEOTStep ++;
						this.chState = SEND_EOT;					
					}else if(this.chByte == NAK && this.chEOTStep == 1 ){
						this.chEOTStep ++;
						this.chState = SEND_EOT;					
					}else if(this.chByte == ACK && this.chEOTStep == 2 ){
						this.chState = START;
						return PACKET_CPL;	
					}else if(this.chByte == CAN){
						this.chState = START;
						return PACKET_CPL;	
					}else{
						return PACKET_INCORRECT_HEADER;					
					}
				}else if(YMODEM_READ_TIMEOUT == tFsm){
					this.chState = START;
					return PACKET_TIMEOUT;
				}else{
					break;
				} 
		    }			
		}		
        case SEND_EOT:{
			this.chByte = EOT;
			if(this.tYm.fnWriteData(&this.tYm,&this.chByte,1)) {
                 this.chState = READ_BYTE;
				 break;
			}	
		} 	
	}
	return this.tPackageState;
}
