/**
 * @file os_thread.c
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

#include "kernel/os/os_thread.h"
#include "os_util.h"
// #include "task.h"
#include <rtthread.h>

/* Macro used to convert OS_Priority to the kernel's real priority */
#define OS_KERNEL_PRIO(prio) (prio)

OS_Status OS_ThreadCreate(OS_Thread_t *thread, const char *name,
                          OS_ThreadEntry_t entry, void *arg,
                          OS_Priority priority, uint32_t stackSize)
{
	// BaseType_t ret;
	rt_err_t ret = RT_ERROR;

	OS_HANDLE_ASSERT(!OS_ThreadIsValid(thread), thread->handle);

	thread->handle = rt_thread_create(name, entry, arg, stackSize, priority, 100);
	if (thread->handle == NULL) goto exit;
	ret = rt_thread_startup(thread->handle);

	exit:
	if (ret != RT_EOK) {
		OS_ERR("err %"OS_BASETYPE_F"\n", ret);
		OS_ThreadSetInvalid(thread);
		return OS_FAIL;
	}
	return OS_OK;
}

OS_Status OS_ThreadDelete(OS_Thread_t *thread)
{
	rt_thread_t handle, curHandle;
	// TaskHandle_t handle;
	// TaskHandle_t curHandle;


	if (thread == NULL) {
		// vTaskDelete(NULL); /* delete self */
		rt_thread_delete(rt_thread_self());
		return OS_OK;
	}

	OS_HANDLE_ASSERT(OS_ThreadIsValid(thread), thread->handle);

	handle = thread->handle;
	// curHandle = xTaskGetCurrentTaskHandle();
	curHandle = rt_thread_self();
	if (handle == curHandle) {
		/* delete self */
		OS_ThreadSetInvalid(thread);
		// vTaskDelete(NULL);
		rt_thread_delete(handle);
	} else {
		/* delete other thread */
		OS_WRN("thread %"OS_HANDLE_F" delete %"OS_HANDLE_F"\n", curHandle, handle);
		// vTaskDelete(handle);
		rt_thread_delete(handle);
		OS_ThreadSetInvalid(thread);
	}

	return OS_OK;
}

void vTaskDelay(uint32_t ticks)
{
	rt_thread_delay(ticks);
}

void OS_ThreadSleep(OS_Time_t msec)
{
	// vTaskDelay((TickType_t)OS_MSecsToTicks(msec));
	rt_thread_mdelay(msec);
}

void OS_ThreadYield(void)
{
	// taskYIELD();
	rt_thread_yield();
}

OS_ThreadHandle_t OS_ThreadGetCurrentHandle(void)
{
	// return (OS_ThreadHandle_t)xTaskGetCurrentTaskHandle();
	return rt_thread_self();
}

void OS_ThreadStartScheduler(void)
{
    rt_system_scheduler_start();
	// vTaskStartScheduler();
}

void vTaskSuspendAll()
{
    rt_enter_critical();
}

void OS_ThreadSuspendScheduler(void)
{
    rt_enter_critical();
}

long xTaskResumeAll(void)
{
	rt_exit_critical();
    return 0;
}

void OS_ThreadResumeScheduler(void)
{
	rt_exit_critical();
}

int OS_ThreadIsSchedulerRunning(void)
{
	return (rt_critical_level() == 0);
}

// #if INCLUDE_uxTaskGetStackHighWaterMark
#if 0
uint32_t OS_ThreadGetStackMinFreeSize(OS_Thread_t *thread)
{
	TaskHandle_t handle;

	if (thread != NULL) {
		if (OS_ThreadIsValid(thread)) {
			handle = thread->handle;
		} else {
			return 0;
		}
	} else {
		handle = NULL;
	}

	return (uxTaskGetStackHighWaterMark(handle) * sizeof(StackType_t));
}
#endif

#ifdef RT_USING_OVERFLOW_CHECK
//TODO
// void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
// {
// 	OS_ERR("task %p(%s) stack over flow\n", xTask, pcTaskName);
// 	OS_ABORT();
// }
#endif

// #if (configUSE_TRACE_FACILITY == 1)
#if 0
void OS_ThreadList(void)
{
	TaskStatus_t *taskStatusArray;
	UBaseType_t taskNum, i;
	char state;

	taskNum = uxTaskGetNumberOfTasks();
	taskStatusArray = OS_Malloc(taskNum * sizeof(TaskStatus_t));
	if (taskStatusArray == NULL) {
		OS_ERR("no mem\n");
		return;
	}

	i = uxTaskGetSystemState(taskStatusArray, taskNum, NULL);
	if (i != taskNum) {
		OS_WRN("task num %lu != %lu\n", i, taskNum);
	}

	OS_LOG(1, "%*sState Pri Idx StkCur     StkBot     StkFree StkFreeMin\n",
	       -configMAX_TASK_NAME_LEN, "Name");
	for (i = 0; i < taskNum; ++i) {
		OS_LOG(1, "%*.*s", -configMAX_TASK_NAME_LEN, configMAX_TASK_NAME_LEN,
		       taskStatusArray[i].pcTaskName);

		switch (taskStatusArray[i].eCurrentState) {
		case eReady:
			state = 'R';
			break;
		case eBlocked:
			state = 'B';
			break;
		case eSuspended:
			state = 'S';
			break;
		case eDeleted:
			state = 'D';
			break;
		default:
			state = '?';
			break;
		}
		OS_LOG(1, "%-5c %-3lu %-3lu %-10p %-10p %-7u %-u\n",
		          state,
		          taskStatusArray[i].uxCurrentPriority,
		          taskStatusArray[i].xTaskNumber,
		          taskStatusArray[i].pxTopOfStack,
#if (CONFIG_OS_FREERTOS_VER == 80203)
		          taskStatusArray[i].pxStack,
		          (taskStatusArray[i].pxTopOfStack - taskStatusArray[i].pxStack) * sizeof(StackType_t),
#elif (CONFIG_OS_FREERTOS_VER == 100201)
		          taskStatusArray[i].pxStackBase,
		          (taskStatusArray[i].pxTopOfStack - taskStatusArray[i].pxStackBase) * sizeof(StackType_t),
#endif
		          taskStatusArray[i].usStackHighWaterMark * sizeof(StackType_t));
	}
	OS_Free(taskStatusArray);
}

#else

void OS_ThreadList(void)
{
	OS_LOG(1, "OS_ThreadList() not supported, please set configUSE_TRACE_FACILITY to 1\n");
}

#endif
