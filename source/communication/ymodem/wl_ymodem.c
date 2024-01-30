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

#include "wl_ymodem.h"

#define SOH            0x01  //!< Start of 128-byte data packet
#define STX            0x02  //!< Start of 1024-byte data packet
#define EOT            0x04  //!< End of transmission
#define ACK            0x06  //!< Positive acknowledgment
#define NAK            0x15  //!< Negative acknowledgment
#define CAN            0x18  //!< Cancel transfer
#define CRC_C          0x43  //!< ASCII 'C'; prompt for CRC mode


#define DLY_1S         (1000U)
#define DLY_3S         (3*DLY_1S)
#define DLY_10S        (10*DLY_1S)

#ifndef MODEM_MODEM_MAX_TRY_AGAN
    #define MODEM_MAX_TRY_AGAN          (10U)
#endif

#define MODEM_DATA_BUFFER_SIZE         (128ul)
#define MODEM_1K_DATA_BUFFER_SIZE      (1024ul)

#undef this
#define this        (*ptThis)

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
} ymodem_packet_state_t;

#ifdef YMODEM_USING_CRC_TABLE
static const uint16_t ccitt_table[256] = {
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

    while (hwLen-- > 0) {
        crc = (crc << 8) ^ ccitt_table[((crc >> 8) ^ *pchData++) & 0xff];
    }

    return crc;
}
#else
uint16_t ymodem_crc16(unsigned char *q, int len)
{
    uint16_t crc;
    char i;

    crc = 0;

    while (--len >= 0) {
        crc = crc ^ (int) * q++ << 8;
        i = 8;

        do {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }

    return (crc);
}
#endif

/**
 * @brief Callback function called when YMODEM transfer is completed
 *
 * This function is marked as weak, allowing it to be overridden by a
 * stronger definition in a user application if needed. It is called when
 * the YMODEM transfer is completed, and the result of the transfer is
 * provided through the 'bResult' parameter.
 *
 * @param pObj Pointer to the object associated with the callback
 * @param bResult Result of the YMODEM transfer (true if successful, false otherwise)
 */
__attribute__((weak))
void ymodem_transfer_completed(void *pObj, bool bResult)
{


}
/**
 * @brief Receives and processes a Ymodem packet based on the current state and the data read.
 *
 * This function processes incoming Ymodem packets following the finite state machine approach,
 * handling different stages of the packet reception, error checking, and validation.
 *
 * @param ptThis Pointer to the Ymodem package structure.
 * @param ptOps Pointer to the Ymodem operations structure containing operational callbacks.
 * @param chPacketNum The expected packet number to validate against the received packet.
 * @return ymodem_packet_state_t The current state of the packet processing or an error state.
 */
static ymodem_packet_state_t ymodem_receive_package(ymodem_package_t *ptThis, ymodem_ops_t *ptOps, uint8_t chPacketNum)
{
    /* Macro to reset the finite state machine (FSM) */
#define YMODEM_RECEIVE_PACKAGE_RESET_FSM() do { this.chState = 0; } while(0)

    /* Enum defining FSM states for receiving a Ymodem packet */
    enum { START, READ_HEAD, READ_BLK, READ_NBLK, READ_DATA, READ_CHECK_L, READ_CHECK_H, CHECK_PACKAGE };

    /* Processing states using a switch-case statement */
    switch(this.chState) {
        case START: {
            /* Begin the process of reading a new packet by transitioning to the READ_HEAD state. */
            this.chState = READ_HEAD;
        }

        /* Fall through to the next state since there's no break. This is intended for execution continuation. */
        case READ_HEAD: {
            /* Attempt to read the header byte, which determines the packet type or end of transmission. */
            ymodem_read_stat_t tFsm = ptOps->fnReadDataWithTimeout(ptOps->pObj, &this.chHead, 1, DLY_3S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Successfully read the header. Now identify the packet type or end signal. */
                if(this.chHead == EOT) {
                    /* EOT header received, signaling the end of data packets transmission. */
                    YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                    return PACKET_EOT;
                } else if(this.chHead == CAN) {
                    /* CAN header received, indicating cancellation of the transmission process. */
                    YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                    return PACKET_CAN;
                } else if(this.chHead == SOH) {
                    /* SOH header signals a standard-size data packet is next. */
                    ptOps->hwSize = MODEM_DATA_BUFFER_SIZE;
                    this.chState = READ_BLK; /* Transition to read the packet number. */
                } else if(this.chHead == STX) {
                    /* STX header signals an extended-size data packet is next. */
                    ptOps->hwSize = MODEM_1K_DATA_BUFFER_SIZE;
                    this.chState = READ_BLK; /* Transition to read the packet number. */
                } else {
                    /* Header is none of the expected types, indicating an error. */
                    return PACKET_INCORRECT_HEADER;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout while reading header, signaling a transmission issue. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_TIMEOUT;
            } else {
                /* Fall through to continue reading the header in the next cycle. */
                break;
            }
        }

        case READ_BLK: {
            /* Read the packet number, which is a crucial part of validating the integrity of the packet. */
            ymodem_read_stat_t tFsm = ptOps->fnReadDataWithTimeout(ptOps->pObj, &this.chBlk, 1, DLY_1S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Packet number successfully read, proceed to read the inverse of the packet number. */
                this.chState = READ_NBLK;
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout occurred while trying to read the packet number, indicating a reception error. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_TIMEOUT;
            } else {
                /* Other errors while reading the packet number will cause a re-attempt in the next cycle. */
                break;
            }
        }

        case READ_NBLK: {
            /* Read the inverse of the packet number for validation purposes. */
            ymodem_read_stat_t tFsm = ptOps->fnReadDataWithTimeout(ptOps->pObj, &this.chNBlk, 1, DLY_1S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Inverse packet number read, check if it correctly complements the packet number. */
                if(0xFF != (this.chBlk ^ this.chNBlk)) {
                    /* Packet validation failure due to mismatch in packet numbers, requiring FSM reset. */
                    YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                    return PACKET_INCORRECT_NBlk;
                }

                /* Packet numbers are valid, transition to reading the packet data. */
                this.chState = READ_DATA;
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout while reading the inverse packet number, signaling an error in reception. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_TIMEOUT;
            } else {
                /* If the inverse packet number cannot be read, repeat the process in the next cycle. */
                break;
            }
        }

        case READ_DATA: {
            /* Read the actual data contained within the Ymodem packet. */
            ymodem_read_stat_t tFsm = ptOps->fnReadDataWithTimeout(ptOps->pObj, ptOps->pchBuffer, ptOps->hwSize, DLY_1S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Data read complete, compute CRC check for validation. */
                this.hwCheck = ymodem_crc16(ptOps->pchBuffer, ptOps->hwSize);
                /* Proceed to reading the first byte of the CRC check from the packet. */
                this.chState = READ_CHECK_L;
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout while reading data indicates a reception error, triggering FSM reset. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_TIMEOUT;
            } else {
                /* Other errors during data reading will result in another attempt in the next cycle. */
                break;
            }
        }

        case READ_CHECK_L: {
            /* Read the low byte of the CRC from the packet for additional validation. */
            ymodem_read_stat_t tFsm = ptOps->fnReadDataWithTimeout(ptOps->pObj, &this.chCheck[0], 1, DLY_1S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Low byte of CRC successfully read, continue to high byte. */
                this.chState = READ_CHECK_H;
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout while reading CRC low byte leads to FSM reset and error status. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_TIMEOUT;
            } else {
                /* If the CRC low byte cannot be read in this cycle, try again in the next one. */
                break;
            }
        }

        case READ_CHECK_H: {
            /* Read the high byte of the CRC from the packet, completing the CRC reading. */
            ymodem_read_stat_t tFsm = ptOps->fnReadDataWithTimeout(ptOps->pObj, &this.chCheck[1], 1, DLY_1S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Combine both CRC bytes to form the complete CRC value received with the packet. */
                if(this.hwCheck != ((uint16_t)this.chCheck[0] << 8) + (uint16_t)this.chCheck[1]) {
                    /* CRC mismatch, implying data corruption. Reset FSM and report checksum error. */
                    YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                    return PACKET_INCORRECT_CHECKOUT;
                } else {
                    /* CRC check passed, packet is valid, proceed to check packet number alignment. */
                    this.chState = CHECK_PACKAGE;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout while reading the CRC high byte is a critical error, reset FSM. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_TIMEOUT;
            } else {
                /* If CRC high byte hasn't been read successfully, stay in this state for another cycle. */
                break;
            }
        }

        case CHECK_PACKAGE: {
            /* Validate the current packet number against the expected number to ensure correct sequencing. */
            if(chPacketNum == this.chBlk || this.chBlk == (chPacketNum - 1)) {
                /* Correct packet received, or it's a repeat of the previous packet. */
                if(this.chBlk == (chPacketNum - 1)) {
                    /* Duplicate packet received, no further processing needed for this packet. */
                    YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                    return PACKET_DUPLICATE_PACKET_NUMBER;
                }

                /* Valid packet received, reset FSM and return complete status. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                this.hwCheck = 0; /* Reset the calculated CRC check value. */
                return PACKET_CPL;
            } else {
                /* Packet number does not match expected value, indicating wrong packet sequence. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_INCORRECT_PACKET_NUMBER;
            }
        }
    }

    /* Return the ongoing status if the packet isn't yet fully processed or an error hasn't occurred. */
    return PACKET_ON_GOING;
}
/**
 * @brief Handles the reception of a complete file using Ymodem protocol.
 *
 * This function drives the state machine for the complete process of receiving a file via the
 * Ymodem protocol. It involves sending initialization sequences, receiving file path and file
 * data packets, and appropriate responses to the sender.
 *
 * @param ptThis Pointer to the ongoing Ymodem context.
 * @return fsm_rt_t The status of the file reception, ongoing status, or an error code.
 */
fsm_rt_t ymodem_receive(ymodem_t *ptThis)
{
    /* Macro to reset the finite state machine (FSM) */
#define YMODEM_RECEIVE_RESET_FSM()    do{this.chState = 0;}while(0)
    /* Enum defining FSM states for receiving */
    enum { START = 0, SEND_C1, RECEIVE_PACK_PATH, SEND_ACK1, SEND_C2, RECEIVE_PACK_DATA, SEND_ANSWER, SEND_NAK, RECIVE_EOT, SEND_ACK2};

    /* Processing the incoming data using a switch-case statement based on the current state. */
    switch(this.chState) {
        case START: {
            /* Initialization of the FSM, setting up the state to transmit the initial 'C' character. */
            this.chTryCount = 0;
            this.chPacketNum = 0;
            this.chByte = CRC_C; /* 'C' character signals CRC mode for transmission. */
            this.chState = SEND_C1; /* Transition to the state for sending 'C'. */
        }

        case SEND_C1: {
            /* Sending the 'C' to the sender, indicating this receiver is ready for packet reception. */
            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                /* Success in sending 'C', proceed to state for receiving the file path packet. */
                this.chState = RECEIVE_PACK_PATH;
            } else {
                /* Sending failed, will retry by remaining in this state. */
                break;
            }
        }

        case RECEIVE_PACK_PATH: {
            /* Attempting to receive the file path packet, the first step in Ymodem transfer. */
            ymodem_packet_state_t tPackageState = ymodem_receive_package(&this.tPackage, &this.tOps, 0);

            /* Completion of file path package reception. */
            if(tPackageState == PACKET_CPL) {
                /* Successful reception, invoke user-defined callback to handle the file path. */
                if(this.tOps.fnOnFilePath(this.tOps.pObj, this.tOps.pchBuffer, &this.tOps.hwSize)) {
                    this.chByte = ACK; /* Setting ACK to acknowledge the file path packet. */
                } else {
                    /* File path handling by the callback failed, reset FSM and signal an error. */
                    ymodem_transfer_completed(this.tOps.pObj, false);
                    YMODEM_RECEIVE_RESET_FSM();
                    return fsm_rt_err;
                }

                /* Preparing to acknowledge the reception of the file path packet. */
                this.chState = SEND_ACK1;
            } else if(tPackageState == PACKET_TIMEOUT) {
                /* Timeout while waiting for the package, leading to an FSM reset and error signal. */
                YMODEM_RECEIVE_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* If the packet is not yet complete or failed, break to re-evaluate the state. */
                break;
            }
        }

        case SEND_ACK1: {
            /* Acknowledging the file path packet or reporting an error if the buffer was empty. */
            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                /* Check if the write operation signaled the end of the packet stream. */
                if(this.tOps.hwSize == 0) {
                    /* No more data, complete transmission and reset FSM. */
                    ymodem_transfer_completed(this.tOps.pObj, true);
                    YMODEM_RECEIVE_RESET_FSM();
                    return fsm_rt_cpl;
                }

                /* Move to the next packet number in preparation for the next data packet. */
                this.chPacketNum++;
                this.chByte = CRC_C; /* Prepare to send 'C' to request the next packet. */
                this.chState = SEND_C2; /* Transition to sending 'C' for data packets. */
            } else {
                /* Writing ACK failed, retry by remaining in this state. */
                break;
            }
        }

        case SEND_C2: {
            /* State for sending the 'C' once more, indicating readiness for the upcoming data packet. */
            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                /* Successfully sent 'C', expect data packet next. */
                this.chState = RECEIVE_PACK_DATA;
            } else {
                /* Failure in sending 'C', stay here to retry. */
                break;
            }
        }

        case RECEIVE_PACK_DATA: {
            /* Managing the reception of actual file data packets. */
            ymodem_packet_state_t tPackageState = ymodem_receive_package(&this.tPackage, &this.tOps, this.chPacketNum);

            /* Processing the outcome of the packet reception. */
            switch(tPackageState) {
                case PACKET_ON_GOING:
                    /* Data packet is being received, informing the system that reception is ongoing. */
                    return fsm_rt_on_going;

                case PACKET_FAIL:
                    /* Data packet reception failed, plan to send a cancel signal. */
                    this.chByte = CAN;
                    this.chState = SEND_ANSWER;
                    break;

                case PACKET_CAN:
                    /* Sender has cancelled transmission, acknowledge and prepare to close session. */
                    this.chByte = ACK;
                    this.chState = SEND_ANSWER;
                    break;

                case PACKET_CPL:

                    /* Complete data packet received, calling user callback to handle the data. */
                    if(this.tOps.fnOnFileData(this.tOps.pObj, this.tOps.pchBuffer, &this.tOps.hwSize)) {
                        /* Data handled successfully, prepare to acknowledge receipt. */
                        this.chPacketNum++;
                        this.chByte = ACK;
                        this.chTryCount = 0; /* Reset retry counter after successful packet reception. */
                    } else {
                        /* Data handling failed, send cancel signal to transmitter. */
                        this.chByte = CAN;
                    }

                    this.chState = SEND_ANSWER;
                    break;

                case PACKET_INCORRECT_HEADER:
                case PACKET_INCORRECT_PACKET_NUMBER:
                case PACKET_INCORRECT_NBlk:
                case PACKET_INCORRECT_CHECKOUT:
                case PACKET_TIMEOUT:
                    /* Any irregularity or timeout in packet reception leads to error handling. */
                    /* Retry count is incremented, and according to the retry limit, a NAK or CAN is sent. */
                    this.chTryCount++;

                    if(this.chTryCount >= MODEM_MAX_TRY_AGAN) {
                        this.chByte = CAN;
                    } else {
                        this.chByte = NAK;
                    }

                    this.chState = SEND_ANSWER;
                    break;

                case PACKET_DUPLICATE_PACKET_NUMBER:
                    /* Duplicate packets are noted and handled with a possible retry or cancellation. */
                    this.chTryCount++;

                    if(this.chTryCount >= MODEM_MAX_TRY_AGAN) {
                        this.chByte = CAN;
                    } else {
                        this.chByte = ACK;
                    }

                    this.chState = SEND_ANSWER;
                    break;

                case PACKET_EOT:
                    /* End Of Transmission (EOT) signifies the end, prepare to send NAK before final ACK. */
                    this.chByte = NAK;
                    this.chState = SEND_NAK;
                    return fsm_rt_on_going;

                default:
                    /* Any other unknown packet state leads to breaking the switch to re-evaluate. */
                    break;
            }
        }

        case SEND_ANSWER: {
            /* Responding to the sender based on the result of the packet reception. */
            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                /* Response successfully sent. Determine the next action based on the last received packet type. */
                if(this.chByte == CAN) {
                    /* If a cancellation was sent, reset FSM and return error. */
                    ymodem_transfer_completed(this.tOps.pObj, false);
                    YMODEM_RECEIVE_RESET_FSM();
                    return fsm_rt_err;
                } else {
                    /* For other responses, continue by trying to receive the next data packet. */
                    this.chState = RECEIVE_PACK_DATA;
                    break;
                }
            }
        }

        case SEND_NAK: {
            /* Responding to the sender based on the result of the packet reception. */
            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                this.chState = RECIVE_EOT;
            }
        }

        case RECIVE_EOT: {
            /* Attempt to receive EOT reply to conclude the transmission. */
            ymodem_read_stat_t tFsm = this.tOps.fnReadDataWithTimeout(this.tOps.pObj, &this.chByte, 1, DLY_3S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Received the final EOT, acknowledge and prepare to conclude the session. */
                if(this.chByte == EOT) {
                    this.chState = SEND_ACK2;
                } else {
                    /* If the received byte is not EOT, remain in this state to retry. */
                    break;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout waiting for EOT signifies an error, reset FSM and report it. */
                ymodem_transfer_completed(this.tOps.pObj, false);
                YMODEM_RECEIVE_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* On any other read status, stay in this state to re-attempt EOT receipt. */
                break;
            }
        }

        case SEND_ACK2: {
            /* Final acknowledgment after EOT to complete the session. */
            this.chByte = ACK;

            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                /* Successfully sent the final ACK, reset FSM and signal session completion. */
                YMODEM_RECEIVE_RESET_FSM();
                return fsm_rt_on_going;
            }

            /* If sending ACK fails, remain in this state for a retry. */
        }
    }

    /* Return the FSM ongoing status if not completed or no error has occurred. */
    return fsm_rt_on_going;
}

/**
 * @brief Initiates and manages the sending of a Ymodem packet.
 *
 * This function is structured following the finite state machine (FSM) approach to send a Ymodem packet.
 * It handles the sequential sending of packet elements including the header, block numbers (and their
 * complements), the data payload, and the CRC checksum for data integrity. The FSM approach ensures
 * that the process is robust and can handle the sending retries and error checking.
 *
 * @param ptThis Pointer to the Ymodem package structure.
 * @param ptOps Pointer to the Ymodem operations structure containing operational callbacks necessary for data writing.
 * @param chPacketNum The current packet number being sent, which is used for packet sequence control and validation.
 * @return ymodem_packet_state_t The current state of the packet sending process or a completed (PACKET_CPL) state.
 */
static ymodem_packet_state_t ymodem_send_package(ymodem_package_t *ptThis, ymodem_ops_t *ptOps, uint8_t chPacketNum)
{
    /* Macro to reset the Ymodem send package finite state machine (FSM) */
#define YMODEM_SEND_PACKAGE_RESET_FSM() do{this.chState = 0;}while(0)

    /* Enum defining FSM states for sending a Ymodem packet */
    enum { START = 0, SEND_HEAD, SEND_BLK, SEND_NBLK, SEND_DATA, SEND_CHECK_L, SEND_CHECK_H};

    /* Processing states using a switch-case statement */
    switch(this.chState) {
        case START: {
            /* Determine the header type and compute CRC of data to be sent, then go to send header state. */
            if(ptOps->hwSize <= MODEM_DATA_BUFFER_SIZE) {
                /* Use standard header for smaller packets. */
                this.chHead = SOH;
                /* Compute CRC for data validation; it will be sent after the data. */
                this.hwCheck = ymodem_crc16(ptOps->pchBuffer, MODEM_DATA_BUFFER_SIZE);
            } else {
                /* Use 1K header for larger packets. */
                this.chHead = STX;
                /* Compute CRC for data validation; it will be sent after the data. */
                this.hwCheck = ymodem_crc16(ptOps->pchBuffer, MODEM_1K_DATA_BUFFER_SIZE);
            }

            /* Transition to the state of sending the packet header. */
            this.chState = SEND_HEAD;
        }

        case SEND_HEAD: {
            /* Send the packet header to the receiver. */
            if(ptOps->fnWriteData(ptOps->pObj, &this.chHead, 1)) {
                /* If the header is successfully sent, move on to sending the block number. */
                this.chState = SEND_BLK;
            } else {
                /* On failure to send header, remain in this state to retry. */
                break;
            }
        }

        case SEND_BLK: {
            /* Set the packet number and send it to the receiver. */
            this.chBlk = chPacketNum;

            if(ptOps->fnWriteData(ptOps->pObj, &this.chBlk, 1)) {
                /* If the block number is successfully sent, proceed to send its complement. */
                this.chState = SEND_NBLK;
            } else {
                /* If sending the block number fails, stay in this state to retry. */
                break;
            }
        }

        case SEND_NBLK: {
            /* Set the inverse of the packet number and send it to the receiver. */
            this.chNBlk = ~chPacketNum;

            if(ptOps->fnWriteData(ptOps->pObj, &this.chNBlk, 1)) {
                /* If the complement is successfully sent, proceed to send the actual data. */
                this.chState = SEND_DATA;
            } else {
                /* On failure to send the complement, remain in this state for a retry. */
                break;
            }
        }

        case SEND_DATA: {
            /* Send the payload data to the receiver. */
            if(ptOps->fnWriteData(ptOps->pObj, ptOps->pchBuffer,
                                  ((ptOps->hwSize <= MODEM_DATA_BUFFER_SIZE) ? MODEM_DATA_BUFFER_SIZE : MODEM_1K_DATA_BUFFER_SIZE))) {
                /* If the data is successfully sent, proceed to send the first byte of the CRC. */
                this.chState = SEND_CHECK_L;
            } else {
                /* On a failure to send data, re-attempt sending in the next cycle. */
                break;
            }
        }

        case SEND_CHECK_L: {
            /* Send the high byte of the computed CRC to the receiver for validation of the data. */
            this.chCheck[0] = (uint8_t)(this.hwCheck >> 8);

            if(ptOps->fnWriteData(ptOps->pObj, &this.chCheck[0], 1)) {
                /* If the high byte of CRC is sent, move on to send the low byte of CRC. */
                this.chState = SEND_CHECK_H;
            } else {
                /* If sending the high byte of CRC fails, stay in this state for a retry. */
                break;
            }
        }

        case SEND_CHECK_H: {
            /* Send the low byte of the computed CRC to complete the packet sending process. */
            this.chCheck[1] = (uint8_t)this.hwCheck;

            if(ptOps->fnWriteData(ptOps->pObj, &this.chCheck[1], 1)) {
                /* On successful transmission of the entire packet, reset the FSM and signal completion. */
                YMODEM_RECEIVE_PACKAGE_RESET_FSM();
                return PACKET_CPL;
            } else {
                /* If the low byte of CRC fails to send, remain in this state to retry. */
                break;
            }
        }
    }

    /* Return the ongoing status if the packet sending is not completed. */
    return PACKET_ON_GOING;
}

/**
 * @brief Executes the file transfer using the Ymodem protocol as a finite state machine.
 *
 * This function processes each state required to perform file transfer using Ymodem protocol.
 * It handles initialization, sending each packet, receiving acknowledgments, and retransmissions
 * in case of errors. This function calls upon ymodem_send_package to handle packet preparation
 * and sending, and uses callbacks for reading and writing the data.
 *
 * @param ptThis Pointer to the instance of Ymodem control structure that maintains state and settings.
 * @return fsm_rt_t The return value indicates the status of the transfer: fsm_rt_cpl for completion,
 *                  fsm_rt_err for error, or fsm_rt_on_going for an ongoing operation.
 */
fsm_rt_t ymodem_send(ymodem_t *ptThis)
{
    /* Define macro to reset the FSM state of Ymodem send operation */
#define YMODEM_SEND_RESET_FSM()    do{this.chState = 0;}while(0)

    /* Enumeration of the FSM states */
    enum {
        START = 0,
        RECEIVE_C1,
        SEND_PACK_PATH,
        RECEIVE_ACK1,
        RECEIVE_C2,
        SEND_PACK_DATA,
        RECEIVE_ANSWER,
        SEND_EOT1,
        RECEIVE_NAK,
        SEND_EOT2,
        RECEIVE_ACK2
    };

    /* Main switch to control the Ymodem send operation state machine */
    switch (this.chState) {
        /**
         * START state: Initial state of the Ymodem send operation.
         * It initializes the try count, packet number, and clears the transfer buffer.
         * The state will then transition to RECEIVE_C1 to await synchronization with the receiver.
         */
        case START: {
            /* Set attempt counter to zero */
            this.chTryCount = 0;
            /* Set starting packet number to zero */
            this.chPacketNum = 0;
            /* Clear the data buffer to prepare for new transaction */
            memset(this.tOps.pchBuffer, 0, MODEM_DATA_BUFFER_SIZE);
            /* Transition to the RECEIVE_C1 state */
            this.chState = RECEIVE_C1;
        }

        /**
         * RECEIVE_C1 state: Await initial handshake from the receiver,
         * expecting a 'C' character signaling CRC mode.
         */
        case RECEIVE_C1: {
            ymodem_read_stat_t tFsm = this.tOps.fnReadDataWithTimeout(this.tOps.pObj, &this.chByte, 1, DLY_3S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                if(this.chByte == CRC_C) {
                    /* 'C' received, proceed to send packet with file path information */
                    if(this.tOps.fnOnFilePath(this.tOps.pObj, this.tOps.pchBuffer, &this.tOps.hwSize)) {
                        this.chState = SEND_PACK_PATH;
                    } else {
                        /* File path retrieval failed, reset state machine */
                        ymodem_transfer_completed(this.tOps.pObj, false);
                        YMODEM_SEND_RESET_FSM();
                        return fsm_rt_err;
                    }
                } else {
                    /* Non-C character received; remain in current state */
                    break;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* ACK waiting period timed out; reset state machine */
                YMODEM_SEND_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* Read in progress or other error; remain in current state */
                break;
            }
        }

        /**
         * SEND_PACK_PATH state: Transmit the initial 128-byte packet containing
         * the null-terminated file path, followed by file size or other metadata.
         */
        case SEND_PACK_PATH: {
            /* Assemble and send the initial packet with the file path */
            ymodem_packet_state_t tPackageState = ymodem_send_package(&this.tPackage, &this.tOps, 0);

            if(tPackageState == PACKET_CPL) {
                /* Packet sent successfully, await ACK from receiver */
                this.chState = RECEIVE_ACK1;
            } else {
                /* Packet incomplete or error occurred; retry later */
                break;
            }
        }

        /**
         * RECEIVE_ACK1 state: Awaits the ACK signal from the receiver, confirming
         * successful receipt of the file path packet.
         */
        case RECEIVE_ACK1: {
            /* Read the next byte expecting an ACK in response to the file path packet */
            ymodem_read_stat_t tFsm = this.tOps.fnReadDataWithTimeout(this.tOps.pObj, &this.chByte, 1, DLY_10S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                if(this.chByte == ACK) {
                    /* ACK received; prepare to send data packets */
                    if(this.tOps.hwSize == 0) {
                        /* No more data to send; transfer complete */
                        ymodem_transfer_completed(this.tOps.pObj, true);
                        YMODEM_SEND_RESET_FSM();
                        return fsm_rt_cpl;
                    } else {
                        /* More data pending; increment packet number and proceed */
                        this.chPacketNum++;
                        this.chState = RECEIVE_C2;
                    }
                } else {
                    /* Non-ACK character received; remain in current state */
                    break;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* ACK waiting period timed out; reset state machine */
                ymodem_transfer_completed(this.tOps.pObj, false);
                YMODEM_SEND_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* Read in progress or other error; remain in current state */
                break;
            }
        }

        /**
         * RECEIVE_C2 state: Waiting for a 'C' from the receiver, indicating that it
         * is ready to receive the data packet.
         */
        case RECEIVE_C2: {
            ymodem_read_stat_t tFsm = this.tOps.fnReadDataWithTimeout(this.tOps.pObj, &this.chByte, 1, DLY_10S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                if(this.chByte == CRC_C) {
                    /* Validate received CRC_C before proceeding to send packet data */
                    if(this.tOps.fnOnFileData(this.tOps.pObj, this.tOps.pchBuffer, &this.tOps.hwSize)) {
                        /* Prepare and process file data for transmission */
                        if(this.tOps.hwSize <= MODEM_DATA_BUFFER_SIZE) {
                            /* If hwSize is less than buffer size, pad the rest with CTRLZ (EOF marker) */
                            memset(&this.tOps.pchBuffer[this.tOps.hwSize], 0x1A, MODEM_DATA_BUFFER_SIZE - this.tOps.hwSize);
                        } else if(this.tOps.hwSize <= MODEM_1K_DATA_BUFFER_SIZE) {
                            /* If hwSize is less than 1K buffer size, pad as well */
                            memset(&this.tOps.pchBuffer[this.tOps.hwSize], 0x1A, MODEM_1K_DATA_BUFFER_SIZE - this.tOps.hwSize);
                        } else {
                            /* hwSize too large, reset state machine and return error */
                            ymodem_transfer_completed(this.tOps.pObj, false);
                            YMODEM_SEND_RESET_FSM();
                            return fsm_rt_err;
                        }

                        /* Data prepared, proceed to send data packet */
                        this.chState = SEND_PACK_DATA;
                    } else {
                        /* Callback for file data failed, reset FSM and return error */
                        ymodem_transfer_completed(this.tOps.pObj, false);
                        YMODEM_SEND_RESET_FSM();
                        return fsm_rt_err;
                    }
                } else {
                    /* No 'C' received, remain in current state */
                    break;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout elapsed while waiting for 'C', reset FSM and return error */
                ymodem_transfer_completed(this.tOps.pObj, false);
                YMODEM_SEND_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* Other issues in reading, remain in current state */
                break;
            }
        }

        /**
         * SEND_PACK_DATA state: Send the actual file data packet(s) to the receiver.
         */
        case SEND_PACK_DATA: {
            /* Send a single ymodem packet and check for completion */
            ymodem_packet_state_t tPackageState = ymodem_send_package(&this.tPackage, &this.tOps, this.chPacketNum);

            if(tPackageState == PACKET_CPL) {
                /* Packet sent successfully, wait for receiver's response */
                this.chState = RECEIVE_ANSWER;
            } else {
                /* Packet sending failed or incomplete, remain in current state */
                break;
            }
        }

        /**
         * RECEIVE_ANSWER state: Process the receiver's response after sending the data packet.
         * Expect ACK for success, NAK for retry, or CAN for cancellation.
         */
        case RECEIVE_ANSWER: {
            ymodem_read_stat_t tFsm = this.tOps.fnReadDataWithTimeout(this.tOps.pObj, &this.chByte, 1, DLY_10S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Successfully received a response from receiver */
                if(this.chByte == ACK) {
                    /* Received ACK, packet transmission succeeded */
                    this.chTryCount = 0; // Reset the retry count
                    this.chPacketNum++; // Prepare the next packet

                    if(this.tOps.hwSize < MODEM_DATA_BUFFER_SIZE) {
                        /* If hwSize is less than the standard buffer size, prepare to send EOT (End Of Transmission) */
                        this.chState = SEND_EOT1;
                    } else if(this.tOps.hwSize < MODEM_1K_DATA_BUFFER_SIZE) {
                        /* If hwSize is less than the standard buffer size, prepare to send EOT (End Of Transmission) */
                        this.chState = SEND_EOT1;
                    } else {
                        /* Call the callback to fill buffer with next part of data */
                        if(this.tOps.fnOnFileData(this.tOps.pObj, this.tOps.pchBuffer, &this.tOps.hwSize)) {
                            /* Prepare and process file data for transmission */
                            if(this.tOps.hwSize == 0) {
                                /* No data left to send, prepare to send EOT (End Of Transmission) */
                                this.chState = SEND_EOT1;
                            } else if(this.tOps.hwSize <= MODEM_DATA_BUFFER_SIZE) {
                                /* If hwSize is less than buffer size, pad the rest with CTRLZ (EOF marker) */
                                memset(&this.tOps.pchBuffer[this.tOps.hwSize], 0x1A, MODEM_DATA_BUFFER_SIZE - this.tOps.hwSize);
                                this.chState = SEND_PACK_DATA;
                                break;
                            } else if(this.tOps.hwSize <= MODEM_1K_DATA_BUFFER_SIZE) {
                                /* If hwSize is less than 1K buffer size, pad as well */
                                memset(&this.tOps.pchBuffer[this.tOps.hwSize], 0x1A, MODEM_1K_DATA_BUFFER_SIZE - this.tOps.hwSize);
                                this.chState = SEND_PACK_DATA;
                                break;
                            } else {
                                /* hwSize too large, reset state machine and return error */
                                ymodem_transfer_completed(this.tOps.pObj, false);
                                YMODEM_SEND_RESET_FSM();
                                return fsm_rt_err;
                            }
                        } else {
                            /* Callback failed, reset FSM and return error */
                            ymodem_transfer_completed(this.tOps.pObj, false);
                            YMODEM_SEND_RESET_FSM();
                            return fsm_rt_err;
                        }
                    }
                } else if (this.chByte == NAK) {
                    /* Received NAK, packet transmission failed, retry */
                    this.chTryCount++;

                    if(this.chTryCount > MODEM_MAX_TRY_AGAN) {
                        /* Max retries reached, reset FSM and return error */
                        ymodem_transfer_completed(this.tOps.pObj, false);
                        YMODEM_SEND_RESET_FSM();
                        return fsm_rt_err;
                    } else {
                        /* Retry sending the last packet */
                        this.chState = SEND_PACK_DATA;
                    }
                } else if(this.chByte == CAN) {
                    /* Received CAN, transmission cancelled, reset FSM and return error */
                    ymodem_transfer_completed(this.tOps.pObj, false);
                    YMODEM_SEND_RESET_FSM();
                    return fsm_rt_err;
                } else {
                    /* Received an unexpected response, stay in the current state */
                    break;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timed out waiting for receiver's response, reset FSM and return error */
                ymodem_transfer_completed(this.tOps.pObj, false);
                YMODEM_SEND_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* Other error in reading, stay in the current state */
                break;
            }
        }

        /**
         * SEND_EOT1 state: This state sends the first End Of Transmission (EOT) byte
         * to the receiver, signaling that there are no more data packets to send.
         */
        case SEND_EOT1: {
            this.chByte = EOT; // Set the byte to EOT (End Of Transmission)

            /* Write the EOT byte and check if it's sent successfully */
            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                /* EOT sent, proceed to the next state to await NAK from receiver */
                this.chState = RECEIVE_NAK;
            } else {
                /* If unable to send EOT, maintain state to try again */
                break;
            }
        }

        /**
         * RECEIVE_NAK state: After sending an EOT, this state awaits a NAK
         * from the receiver, signifying it has acknowledged the EOT and is ready
         * for the next step of termination sequence.
         */
        case RECEIVE_NAK: {
            ymodem_read_stat_t tFsm = this.tOps.fnReadDataWithTimeout(this.tOps.pObj, &this.chByte, 1, DLY_10S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Check if the byte received is a NAK */
                if(this.chByte == NAK) {
                    /* NAK received, indicating receiver got the EOT and is waiting for closure */
                    this.chState = SEND_EOT2;
                } else {
                    /* If the response is not NAK, remain in this state and retry */
                    break;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout waiting for NAK, reset FSM to handle error */
                ymodem_transfer_completed(this.tOps.pObj, false);
                YMODEM_RECEIVE_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* Reading still in progress or other error, stay in state */
                break;
            }
        }

        /**
         * SEND_EOT2 state: This state attempts to send another EOT byte to the receiver,
         * ensuring the termination sequence is well understood.
         */
        case SEND_EOT2: {
            this.chByte = EOT; // Set the byte to EOT (End Of Transmission) again

            /* Send the EOT byte again and check for success */
            if(this.tOps.fnWriteData(this.tOps.pObj, &this.chByte, 1)) {
                /* EOT sent again, now await for final acknowledgment with ACK */
                this.chState = RECEIVE_ACK2;
            } else {
                /* If EOT fails to send, remain in this state for another attempt */
                break;
            }
        }

        /**
         * RECEIVE_ACK2 state: This state waits for a final ACK from the receiver,
         * which would indicate that the transmission has been successfully terminated.
         */
        case RECEIVE_ACK2: {
            ymodem_read_stat_t tFsm = this.tOps.fnReadDataWithTimeout(this.tOps.pObj, &this.chByte, 1, DLY_10S, DLY_1S);

            if(YMODEM_READ_CPL == tFsm) {
                /* Check if the byte received is an ACK */
                if(this.chByte == ACK) {
                    /* Final ACK received, transmission successfully terminated */
                    YMODEM_RECEIVE_RESET_FSM(); // Reset the FSM for next transaction
                    return fsm_rt_on_going; // Return completion status
                } else {
                    /* If the response is not ACK, remain in this state and wait */
                    break;
                }
            } else if(YMODEM_READ_TIMEOUT == tFsm) {
                /* Timeout waiting for ACK, reset FSM to handle error */
                ymodem_transfer_completed(this.tOps.pObj, false);
                YMODEM_RECEIVE_RESET_FSM();
                return fsm_rt_err;
            } else {
                /* Reading still in progress or other error, stay in state */
                break;
            }
        }
    }

    return fsm_rt_on_going;
}
/**
 * Initializes the ymodem_t structure with the required callbacks and buffer.
 *
 * @param ptThis Pointer to the ymodem_t instance that is to be initialized.
 * @param pObj Pointer to derived class object.
 * @param pchBuffer Pointer to the buffer that will be used for storing file data.
 * @param hwSize The size of the buffer pointed to by pchBuffer.
 * @param fnOnFilePath Pointer to the callback function to get the file path.
 * @param fnOnFileData Pointer to the callback function to manage file data.
 * @param fnReadDataWithTimeout Pointer to the callback function for reading data
 *                              with specified timeout.
 * @param fnWriteData Pointer to the callback function for writing data.
 *
 * @return Returns true if all pointers are valid and the structure has been
 *         initialized successfully, false otherwise.
 *
 * The function checks for the validity of all provided pointers to ensure they are
 * not NULL. It is necessary to provide all callback functions and a buffer pointer
 * for the Ymodem session to work correctly. The function assigns the user-provided
 * callbacks and buffer to the ymodem_t structure's operations component (tOps),
 * preparing the ymodem session for use with specific file transfer operations.
 */
bool ymodem_init(
    ymodem_t                             *ptThis,
    void                                 *pObj,
    uint8_t                              *pchBuffer,
    uint16_t                              hwSize,
    ymodem_call_back                     *fnOnFilePath,
    ymodem_call_back                     *fnOnFileData,
    ymodem_read_with_timeout_call_back   *fnReadDataWithTimeout,
    ymodem_write_call_back               *fnWriteData)
{
    /* Validate all the input pointers to ensure they are not NULL. */
    if (NULL == pchBuffer || NULL == pObj || NULL == fnOnFilePath ||  NULL == fnOnFileData ||
        NULL == fnReadDataWithTimeout || NULL == fnWriteData) {
        /* If any pointer is NULL, the initialization cannot proceed, return false. */
        return false;
    }

    this.chState = 0;
    /* Assign the provided callback functions to the operations member of the ymodem structure. */
    this.tOps.pObj = pObj;
    this.tOps.fnOnFilePath = fnOnFilePath;
    this.tOps.fnOnFileData = fnOnFileData;
    this.tOps.fnReadDataWithTimeout = fnReadDataWithTimeout;
    this.tOps.fnWriteData = fnWriteData;

    /* Set the provided buffer and its size to the operations member of the ymodem structure. */
    this.tOps.pchBuffer = pchBuffer;
    this.tOps.hwSize = hwSize;

    /* Return true to indicate successful initialization. */
    return true;
}
