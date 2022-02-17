/**
 * @file os_queue.c
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

#include "kernel/os/os_queue.h"
#include "os_util.h"
// #include "queue.h"
#include <rtthread.h>

OS_Status OS_QueueCreate(OS_Queue_t *queue, uint32_t queueLen, uint32_t itemSize)
{
//	OS_HANDLE_ASSERT(!OS_QueueIsValid(queue), queue->handle);

	// queue->handle = xQueueCreate(queueLen, itemSize);
	queue->handle = rt_mq_create("NULL", itemSize, queueLen, RT_IPC_FLAG_PRIO);
	if (queue->handle == NULL) {
		OS_ERR("err %"OS_HANDLE_F"\n", queue->handle);
		return OS_FAIL;
	}

	return OS_OK;
}

OS_Status OS_QueueDelete(OS_Queue_t *queue)
{
	rt_err_t ret;

	OS_HANDLE_ASSERT(OS_QueueIsValid(queue), queue->handle);

	ret = rt_mq_delete((rt_mq_t)queue->handle);
	if (ret != RT_EOK) {
		OS_ERR("queue %"OS_HANDLE_F" delete fail\n", queue->handle);
		return OS_FAIL;
	}

	OS_QueueSetInvalid(queue);
	return OS_OK;
}

OS_Status OS_QueueSend(OS_Queue_t *queue, const void *item, OS_Time_t waitMS)
{
	rt_err_t ret;
	OS_Time_t ms = OS_IsISRContext() ? 0 : waitMS;

	OS_HANDLE_ASSERT(OS_QueueIsValid(queue), queue->handle);

	ret = rt_mq_send_wait(queue->handle, item, ((rt_mq_t)queue->handle)->msg_size, ms);
	if (ret != RT_EOK) {
		OS_DBG("%s() fail @ %d, %"OS_TIME_F" ms\n", __func__, __LINE__, ms);
		return OS_FAIL;
	}

	return OS_OK;
}

OS_Status OS_QueueReceive(OS_Queue_t *queue, void *item, OS_Time_t waitMS)
{
	// BaseType_t ret;
	// BaseType_t taskWoken;
	rt_err_t ret;
	OS_Time_t ms = OS_IsISRContext() ? 0 : waitMS;

	OS_HANDLE_ASSERT(OS_QueueIsValid(queue), queue->handle);

	// ret = xQueueReceive(queue->handle, item, OS_CalcWaitTicks(waitMS));
	ret = rt_mq_recv(queue->handle, item, ((rt_mq_t)queue->handle)->msg_size, ms);
	if (ret != RT_EOK) {
		OS_DBG("%s() fail @ %d, %"OS_TIME_F" ms\n", __func__, __LINE__, waitMS);
		return OS_FAIL;
	}

	return OS_OK;
}
