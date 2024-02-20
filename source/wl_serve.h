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

#ifndef __USE_SERVICE_H__
#define __USE_SERVICE_H__

/*============================ INCLUDES ======================================*/
#include "./app_cfg.h"
#include "./communication/shell/wl_shell.h"
#include "./communication/subscribe_publish/wl_subscribe_publish.h"
#include "./communication/logging/wl_log.h"
#include "./flash_blob/wl_flash_blob.h"
extern bool service_init(void);


#endif
