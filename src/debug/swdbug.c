/*
 * Copyright (C) 2022 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "debug/swdbug.h"
#include <stdio.h>
#include "kernel/os/os_time.h"
#include "driver/chip/hal_gpio.h"

static void swd_port_init(void)
{
    GPIO_InitParam param;
    param.driving = GPIO_DRIVING_LEVEL_1;
    param.mode = GPIOx_Pn_F0_INPUT;
    param.pull = GPIO_PULL_NONE;
    HAL_GPIO_Init(SWD_START_PORT, SWD_START_PIN, &param);
}

static uint8_t swd_input_read(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(SWD_START_PORT, SWD_START_PIN);
}

void swd_halt(swd_halt_mod type)
{
    if (type == HALT_BY_TIME) {
        int i = 10;
        printf("Start to debug after %d second!\n",i);
        for (; i > 0; i--) {
            OS_Sleep(1);
            printf("Start to test after %d second!\n",i);
        }
    } else if(type == HALT_BY_BUTTON) {
        swd_port_init();
        printf("Start to debug by pressing the button\n");
        int j = 5;
        while(j >= 0) {
            if(swd_input_read() == SWD_PRESS_VALUE) {
                OS_MSleep(10);
                j --;
            } else {
                j = 5;
            }
        }
    } else {}
}