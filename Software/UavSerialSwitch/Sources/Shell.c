/**
 * \file
 * \brief Shell and console interface implementation.
 * \author Erich Styger
 *
 * This module implements the front to the console/shell functionality.
 */

#include "Platform.h"
#include "Shell.h"
#include "CLS1.h"
#include "Application.h"
#include "FRTOS.h"
#if PL_HAS_TEENSY_LED
	#include "LedTeensy.h"
#else
	#include "LedGreen.h"
#endif
#if PL_HAS_SD_CARD
  #include "FAT1.h"
  #include "TmDt1.h"
#endif
#include "KIN1.h"
#include "Config.h"

/* prototypes */
void pullMsgFromQueueAndPrint(void);

/* global variables */
static unsigned char localConsole_buf[48];
static xQueueHandle msgQueue;

static const CLS1_ParseCommandCallback CmdParserTable[] =
{
  CLS1_ParseCommand,
#if FRTOS1_PARSE_COMMAND_ENABLED
  FRTOS_ParseCommand,
#endif
#if LED_TEENSY_PARSE_COMMAND_ENABLED
  LedTeensy_ParseCommand,
#else
  LedGreen_ParseCommand,
#endif
#if PL_HAS_SD_CARD
#if FAT1_PARSE_COMMAND_ENABLED
  FAT1_ParseCommand,
#endif
#endif
#if TmDt1_PARSE_COMMAND_ENABLED
  TmDt1_ParseCommand,
#endif
#if KIN1_PARSE_COMMAND_ENABLED
  KIN1_ParseCommand,
#endif
  NULL /* Sentinel */
};


/*!
* \fn void Shell_TaskEntry (void *pvParameters)
* \brief Parses commands in shell and prints debug information if configured in ini file.
*/
void Shell_TaskEntry (void *pvParameters)
{
	const TickType_t taskInterval = pdMS_TO_TICKS(config.ShellTaskInterval); /* task interval  */
	TickType_t lastWakeTime = xTaskGetTickCount(); /* Initialize the xLastWakeTime variable with the current time. */

	#if CLS1_DEFAULT_SERIAL
		CLS1_ConstStdIOTypePtr ioLocal = CLS1_GetStdio();
	#endif
	#if PL_HAS_SD_CARD
			bool cardMounted = FALSE;
			static FAT1_FATFS fileSystemObject;
	#endif

	for(;;)
	{
		vTaskDelayUntil( &lastWakeTime, taskInterval ); /* Wait for the next cycle */

		#if PL_HAS_SD_CARD
			//(void)FAT1_CheckCardPresence(&cardMounted, (unsigned char*)"0" /*volume*/, &fileSystemObject, CLS1_GetStdio());
		#endif
		#if CLS1_DEFAULT_SERIAL
			(void)CLS1_ReadAndParseWithCommandTable(localConsole_buf, sizeof(localConsole_buf), ioLocal, CmdParserTable);
		#endif

		pullMsgFromQueueAndPrint();

		vTaskDelay(50/portTICK_RATE_MS);
	}
}


/*!
* \fn void Shell_TaskInit(void)
* \brief Initializes the queue used for debug message storage
*/
void Shell_TaskInit(void)
{
  localConsole_buf[0] = '\0';
  CLS1_Init();

#if configSUPPORT_STATIC_ALLOCATION
	static uint8_t xStaticQueue[ QUEUE_NUM_OF_CHARS_WL_RX_QUEUE * sizeof(uint8_t) ]; /* The variable used to hold the queue's data structure. */
	static StaticQueue_t ucQueueStorage; /* The array to use as the queue's storage area. */
	msgQueue = xQueueCreateStatic( MAX_NUMBER_OF_MESSAGES_STORED, sizeof(char*), xStaticQueue, &ucQueueStorage);
#else
	msgQueue = xQueueCreate( MAX_NUMBER_OF_MESSAGES_STORED, sizeof(char*));
#endif
	if(msgQueue == NULL)
		for(;;){} /* malloc for queue failed */
	vQueueAddToRegistry(msgQueue, "DebugMessageQueue");
}



/*!
* \fn void pullMsgFromQueueAndPrint(void)
* \brief Pulls debug messages from queue and sends them to shell if GenerateDebugOutput == 1
*/
void pullMsgFromQueueAndPrint(void)
{
  char* pMsg;
  while(xQueueReceive(msgQueue, &pMsg, 0) == pdTRUE)
  {
	  if(config.GenerateDebugOutput)
		  CLS1_SendStr(pMsg, CLS1_GetStdio()->stdOut);
	  FRTOS_vPortFree(pMsg); /* free memory allocated when message was pushed into queue */
  }
}

/*!
* \fn BaseType_t pushMsgToShellQueue(char* msg, int numberOfChars)
* \brief Stores pData in queue
* \param pMsg: The location where the string is stored
* \return Status if xQueueSendToBack has been successful
*/
BaseType_t pushMsgToShellQueue(char* pMsg)
{
	/* limit string in shell in case terminating zero not added */
	int numberOfChars = (strlen(pMsg) <= MAX_NUMBER_OF_CHARS_PER_MESSAGE) ? strlen(pMsg) : MAX_NUMBER_OF_CHARS_PER_MESSAGE; /* limit message length in queue */
	/* saves config data in queue if debug output enabled */
	if(config.GenerateDebugOutput)
	{
		/* allocate memory for string in queue */
		char* pTmpMsg;
		pTmpMsg = (char*) FRTOS_pvPortMalloc(numberOfChars * sizeof(char));
		if(pTmpMsg == NULL)
		{
			return pdFAIL;
		}
		UTIL1_strcpy(pTmpMsg, numberOfChars+1, pMsg); /* copy string to new memory location */
		/* push string to queue */
		if(xQueueSendToBack(msgQueue, &pTmpMsg, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_SHELL_MS) ) != pdTRUE)
		{
			/* free memory before returning */
			FRTOS_vPortFree(pTmpMsg); /* free memory allocated when message was pushed into queue */
			return pdFAIL;
		}
	}
	return pdTRUE; /* also return success if debug output not enabled */
}
