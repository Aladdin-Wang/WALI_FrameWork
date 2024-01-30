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

#include "./app_cfg.h"
#include <stdint.h>
#include <stdbool.h>
#include "flash_blob.h"
#include "user_data.h"
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
/* Avoids the semihosting issue */
__asm("  .global __ARM_use_no_argv\n");
#elif defined(__GNUC__)
/* Disables part of C/C++ runtime startup/teardown */
void __libc_init_array (void) {}
#endif

#if defined(__CC_ARM)
__asm void modify_stack_pointer_and_start_app(uint32_t r0_sp, uint32_t r1_pc)
{
    MOV SP, R0
    BX R1
}
#elif defined(__GNUC__)
void modify_stack_pointer_and_start_app(uint32_t r0_sp, uint32_t r1_pc)
{
    uint32_t z = 0;
    __asm volatile (  "msr    control, %[z]   \n\t"
                      "isb                    \n\t"
                      "mov    sp, %[r0_sp]    \n\t"
                      "bx     %[r1_pc]"
                      :
                      :   [z] "l" (z),
                      [r0_sp] "l" (r0_sp),
                      [r1_pc] "l" (r1_pc)
                   );
}
#else
#error "Unknown compiler!"
#endif

extern uint8_t chUserData[];
static user_data_t *ptUserData = (user_data_t *)chUserData;
void go_to_boot(uint8_t *pchDate, uint16_t hwLength);

static uint8_t chBootData[3][32];
typedef struct {
    void (*fnGoToBoot)(uint8_t *pchDate, uint16_t hwLength);
    bool (*target_flash_init)(uint32_t addr);
    bool (*target_flash_uninit)(uint32_t addr);
    int  (*target_flash_read)(uint32_t addr, uint8_t *buf, size_t size);
    int  (*target_flash_write)(uint32_t addr, const uint8_t *buf, size_t size);
    int  (*target_flash_erase)(uint32_t addr, size_t size);
} boot_ops_t;

__attribute__((used))
static const boot_ops_t tBootOps  __attribute__ ((section(".ARM.__at_0x08001000"))) = {
    .fnGoToBoot = go_to_boot,
    .target_flash_init = target_flash_init,
    .target_flash_erase = target_flash_erase,
    .target_flash_write = target_flash_write,
    .target_flash_read = target_flash_read,
    .target_flash_uninit = target_flash_uninit
};

/**********************************************************************



***********************************************************************/

void go_to_boot(uint8_t *pchDate, uint16_t hwLength)
{
    memset(chBootData, 0, sizeof(chBootData));

    for(int i = 0; i < hwLength && hwLength < USER_DATA_SIZE_ALIGND; i++) {
        chUserData[i] = pchDate[i];
    }

    target_flash_init(APP_PART_ADDR);
    target_flash_write((APP_PART_ADDR + APP_PART_SIZE - 96 - USER_DATA_SIZE_ALIGND), chUserData, USER_DATA_SIZE_ALIGND);
    target_flash_write((APP_PART_ADDR + APP_PART_SIZE - 32), chBootData[2], 32);
    target_flash_uninit(APP_PART_ADDR);
}

void start_to_boot(void)
{
    memset(chBootData, 0, sizeof(chBootData));
    target_flash_erase(APP_PART_ADDR + APP_PART_SIZE - 96, 96);
    target_flash_write((APP_PART_ADDR + APP_PART_SIZE - 96 - 2 * (USER_DATA_SIZE_ALIGND)), chUserData, USER_DATA_SIZE_ALIGND);
    target_flash_write((APP_PART_ADDR + APP_PART_SIZE - 64), chBootData[1], 32);
}

void end_to_boot(void)
{
    memset(chBootData, 0X00, sizeof(chBootData));
    target_flash_write((APP_PART_ADDR + APP_PART_SIZE - 96), chBootData[0], 32);
}

__attribute__((constructor))
static void start_application(void)
{
    do {
        target_flash_read((APP_PART_ADDR + APP_PART_SIZE - 96), chBootData[0], 96);

        if ((0 == *(uint32_t *)&chBootData[2])) {		                                               /* */
            target_flash_read((APP_PART_ADDR + APP_PART_SIZE - 96 - USER_DATA_SIZE_ALIGND), chUserData, USER_DATA_SIZE_ALIGND);
            break;
        }

        if ((0 == *(uint32_t *)&chBootData[1]) && (0XFFFFFFFF == *(uint32_t *)&chBootData[0]) ) {		/* */
            target_flash_read((APP_PART_ADDR + APP_PART_SIZE - 96 - 2 * (USER_DATA_SIZE_ALIGND)), chUserData, USER_DATA_SIZE_ALIGND);
            break;
        }

        if (((*(volatile uint32_t *)(APP_PART_ADDR + 4)) & 0xff000000) != 0x08000000) {
            break;
        }

        if (((*(volatile uint32_t *)APP_PART_ADDR) & 0x20000000) != 0x20000000) {
            break;
        }

        modify_stack_pointer_and_start_app(*(volatile uint32_t *)APP_PART_ADDR,
                                           (*(volatile uint32_t *)(APP_PART_ADDR + 4)));

    } while(0);
}

