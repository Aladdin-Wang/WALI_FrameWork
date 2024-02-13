/****************************************************************************
*  Copyright 2022 KK (https://github.com/WALI-KANG)                                    *
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

#ifndef _FLASH_BLOB_CFG_H_
#define _FLASH_BLOB_CFG_H_
#include "flash_blob.h"
/* ===================== Flash device Configuration ========================= */
extern const  flash_blob_t  onchip_flash_device;
/* flash device table */
#define FLASH_DEV_TABLE                                          \
{                                                                   \
    &onchip_flash_device                                            \
};                                                                \

#endif
