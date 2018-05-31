/*
 * Application.c
 *
 *  Created on: 10.10.2017
 *      Author: Erich Styger Local
 */

#include "WAIT1.h"
#include "FRTOS.h"
#include "Shell.h"
#include <stdlib.h> // atoi
#include <stdbool.h> //true/false
#include "MINI.h" //read SD card
#include "SysInit.h" // init task, reads config
#include "SpiHandler.h"
#include "PackageHandler.h"
#include "NetworkHandler.h"
#include "TransportHandler.h"
#include "Shell.h"
#include "Logger.h"



void APP_Run(void)
{
	/* Task reads ini file from SD card. The ini file content is needed by other tasks upon their taskentry.
	 * This is the reason that only SysInit task is created on startup and all other tasks are created within SysInit task
	 * once the SD card has been read */

	/* make sure all queues are initialized before being accessed from other tasks */
	Shell_TaskInit(); /* 0.25kB */
	logger_TaskInit(); /* 2x4x(queuelength)x56B */
	spiHandler_TaskInit(); /* 10kB when queuelength = 512, 2x4x2x(queueLength)x1B */
	packageHandler_TaskInit(); /* 2x4x(queueLength)x56B = 3kB */
	networkHandler_TaskInit(); /* 2x4x(queueLength)x56B = 3kB */
	transportHandler_TaskInit();

  if (xTaskCreate(SysInit_TaskEntry, "Init", 4000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2,  NULL) != pdPASS) {
          for(;;){}} /* error */


  vTaskStartScheduler();
  for(;;) {}
}

