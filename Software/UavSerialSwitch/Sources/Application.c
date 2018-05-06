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



void APP_Run(void)
{
	/* Task reads ini file from SD card. The ini file content is needed by other tasks upon their taskentry.
	 * This is the reason that only SysInit task is created on startup and all other tasks are created within SysInit task
	 * once the SD card has been read */
  if (xTaskCreate(SysInit_TaskEntry, "Init", 4000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2,  NULL) != pdPASS) {
          for(;;){}} /* error */


  vTaskStartScheduler();
  for(;;) {}
}

