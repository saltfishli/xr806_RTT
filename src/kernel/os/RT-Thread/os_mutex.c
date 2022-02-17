/**
 * @file os_mutex.c
 * @author XRADIO IOT WLAN Team
 */

/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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

#include "kernel/os/os_mutex.h"
#include "os_util.h"
#include <rtthread.h>
#include "debug/backtrace.h"

OS_Status OS_MutexCreate(OS_Mutex_t *mutex)
{
	OS_HANDLE_ASSERT(!OS_MutexIsValid(mutex), mutex->handle);
	// mutex->handle = xSemaphoreCreateMutex();
	mutex->handle = rt_mutex_create("NULL", RT_IPC_FLAG_PRIO);
	if (mutex->handle == NULL) {
		OS_ERR("err %"OS_HANDLE_F"\n", mutex->handle);
		return OS_FAIL;
	}

	return OS_OK;
}

OS_Status OS_MutexDelete(OS_Mutex_t *mutex)
{
	OS_HANDLE_ASSERT(OS_MutexIsValid(mutex), mutex->handle);
	// vSemaphoreDelete(mutex->handle);
	rt_mutex_delete (mutex->handle);
	OS_MutexSetInvalid(mutex);
	return OS_OK;
}

OS_Status OS_MutexLock(OS_Mutex_t *mutex, OS_Time_t waitMS)
{
	rt_err_t ret;

	OS_HANDLE_ASSERT(OS_MutexIsValid(mutex), mutex->handle);
    // if(OS_IsISRContext()) {
    //     return OS_E_ISR;
    // }
    OS_Time_t ms = OS_IsISRContext() ? 0 : waitMS;
	ret = rt_mutex_take(mutex->handle, rt_tick_from_millisecond(ms));
	if (ret != RT_EOK) {
		OS_DBG("%s() fail @ %d, %"OS_TIME_F" ms\n", __func__, __LINE__, ms);
		return OS_FAIL;
	}

	return OS_OK;
}

OS_Status OS_MutexUnlock(OS_Mutex_t *mutex)
{
	rt_err_t ret;

	OS_HANDLE_ASSERT(OS_MutexIsValid(mutex), mutex->handle);
	ret = rt_mutex_release(mutex->handle);
	if (ret != RT_EOK) {
		OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
		return OS_FAIL;
	}

	return OS_OK;
}

OS_Status OS_RecursiveMutexCreate(OS_Mutex_t *mutex)
{
	OS_HANDLE_ASSERT(!OS_MutexIsValid(mutex), mutex->handle);
	mutex->handle = rt_mutex_create("NULL", RT_IPC_FLAG_PRIO);
	if (mutex->handle == NULL) {
		OS_ERR("err %"OS_HANDLE_F"\n", mutex->handle);
		return OS_FAIL;
	}

	return OS_OK;
}

OS_Status OS_RecursiveMutexDelete(OS_Mutex_t *mutex)
{
    printf("OS_RecursiveMutexDelete %p\n",mutex);
	return OS_MutexDelete(mutex);
}

OS_Status OS_RecursiveMutexLock(OS_Mutex_t *mutex, OS_Time_t waitMS)
{
	rt_err_t ret;

	OS_HANDLE_ASSERT(OS_MutexIsValid(mutex), mutex->handle);
    // if(OS_IsISRContext()) {
    //     return OS_E_ISR;
    // }
    OS_Time_t ms = OS_IsISRContext() ? 0 : waitMS;
	ret = rt_mutex_take(mutex->handle, rt_tick_from_millisecond(ms));
	if (ret != RT_EOK) {
		OS_DBG("%s() fail @ %d, %"OS_TIME_F" ms\n", __func__, __LINE__, ms);
		return OS_FAIL;
	}
	return OS_OK;
}

OS_Status OS_RecursiveMutexUnlock(OS_Mutex_t *mutex)
{
	rt_err_t ret;

	OS_HANDLE_ASSERT(OS_MutexIsValid(mutex), mutex->handle);
	ret = rt_mutex_release(mutex->handle);
	if (ret != RT_EOK) {
		OS_DBG("%s() fail @ %d\n", __func__, __LINE__);
		return OS_FAIL;
	}

	return OS_OK;
}

OS_ThreadHandle_t OS_MutexGetOwner(OS_Mutex_t *mutex)
{
	if (!OS_MutexIsValid(mutex)) {
		return OS_INVALID_HANDLE;
	}

	return ((rt_mutex_t)mutex->handle)->owner;
}
