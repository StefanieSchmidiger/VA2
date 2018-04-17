#include "Logger.h"
#include "FAT1.h"
#include "CLS1.h"
#include "FRTOS.h"
#include "Config.h"
#include "UTIL1.h"
#include "XF1.h"
#include "PackageHandler.h" /* tWirelessPackage */
#include "SpiHandler.h" /* tUartNr, tSpiSlave */

#define FILENAME_ARRAY_SIZE (35)

/* global variables, only used in this file */
static const char* const queueNameReceivedPackages[] = {"ReceivedPackagesForLogging0", "ReceivedPackagesForLogging1", "ReceivedPackagesForLogging2", "ReceivedPackagesForLogging3"};
static const char* const queueNameSentPackages[] = {"SentPackagesForLogging0", "SentPackagesForLogging1", "SentPackagesForLogging2", "SentPackagesForLogging3"};
static const char* const queueNameSentBytes[] = {"SentBytesForLogging0", "SentBytesForLogging1", "SentBytesForLogging2", "SentBytesForLogging3"};
static const char* const queueNameReceivedBytes[] = {"ReceivedBytesForLogging0", "ReceivedBytesForLogging1", "ReceivedBytesForLogging2", "ReceivedBytesForLogging3"};

static char filenameReceivedPackagesLogger[NUMBER_OF_UARTS][FILENAME_ARRAY_SIZE];
static char filenameSentBytesLogger[NUMBER_OF_UARTS][FILENAME_ARRAY_SIZE];
static char filenameReceivedBytesLogger[NUMBER_OF_UARTS][FILENAME_ARRAY_SIZE];
static char filenameSentPackagesLogger[NUMBER_OF_UARTS][FILENAME_ARRAY_SIZE];
static xQueueHandle queuePackagesToLog[2][NUMBER_OF_UARTS];  /* queuePackagesToLog[0] = received packages, queuePackagesToLog[1] = sent packages */
static xQueueHandle queueBytesToLog[2][1]; /* queueBytesToLog[0] = received bytes, queueBytesToLog[1] = sent bytes */
static FAT1_FATFS fileSystemObject;

/* prototypes */
static void initLoggerQueues(void);
static bool writeToFile(FIL* filePointer, char* fileName, char* logEntry);
static bool writePackLogHeader(FIL* filePointer, char* fileName);
static void packageToLogString(tWirelessPackage* pPack, char* logEntry, int logEntryStrSize);
static bool logPackages(xQueueHandle queue, FIL* filepointer, char* filename);
static bool logBytes(xQueueHandle queue, FIL* filepointer, char* filename);


void logger_TaskEntry(void* p)
{
	uint32_t timestampLastLog;
	static FIL fpPacks[2][NUMBER_OF_UARTS]; /* static because of its size */
	static FIL fpBytes[2][1]; /* static because of its size */

	/* ------------- init logger ----------------------- */
	(void) p; /* p not used -> no compiler warning */
	/* open log files and write log header into all of them */
	for(int uartNr = 0; uartNr < NUMBER_OF_UARTS; uartNr++)
	{
		char fullFileNamePacks[FILENAME_ARRAY_SIZE];
		char fullFileNameBytes[FILENAME_ARRAY_SIZE];
		tWirelessPackage pack;

		/* append .log at end of filename */
		UTIL1_strcpy(fullFileNamePacks, FILENAME_ARRAY_SIZE, filenameSentPackagesLogger[uartNr]);
		UTIL1_strcat(fullFileNamePacks, FILENAME_ARRAY_SIZE, ".log");
		/* open file and move to end of file */
		if (FAT1_open(&fpPacks[SENT_PACKAGE][uartNr], fullFileNamePacks, FA_OPEN_ALWAYS|FA_WRITE)!=FR_OK) /* open file */
			while(1){}
		if (FAT1_lseek(&fpPacks[SENT_PACKAGE][uartNr], FAT1_f_size(&fpPacks[SENT_PACKAGE][uartNr])) != FR_OK || fpPacks[SENT_PACKAGE][uartNr].fptr != FAT1_f_size(&fpPacks[SENT_PACKAGE][uartNr])) /* move to the end of file */
			while(1){}


		/* append .log at end of filename */
		UTIL1_strcpy(fullFileNamePacks, FILENAME_ARRAY_SIZE, filenameReceivedPackagesLogger[uartNr]);
		UTIL1_strcat(fullFileNamePacks, FILENAME_ARRAY_SIZE, ".log");
		/* open file and move to end of file */
		if (FAT1_open(&fpPacks[RECEIVED_PACKAGE][uartNr], fullFileNamePacks, FA_OPEN_ALWAYS|FA_WRITE)!=FR_OK) /* open file */
			while(1){}
		if (FAT1_lseek(&fpPacks[RECEIVED_PACKAGE][uartNr], FAT1_f_size(&fpPacks[RECEIVED_PACKAGE][uartNr])) != FR_OK || fpPacks[RECEIVED_PACKAGE][uartNr].fptr != FAT1_f_size(&fpPacks[RECEIVED_PACKAGE][uartNr])) /* move to the end of file */
			while(1){}

		/* append .log at end of filename */
		UTIL1_strcpy(fullFileNameBytes, FILENAME_ARRAY_SIZE, filenameReceivedBytesLogger[uartNr]);
		UTIL1_strcat(fullFileNameBytes, FILENAME_ARRAY_SIZE, ".log");
		/* open file and move to end of file */
		if (FAT1_open(&fpBytes[RECEIVED_PACKAGE][uartNr], fullFileNameBytes, FA_OPEN_ALWAYS|FA_WRITE)!=FR_OK) /* open file */
			while(1){}
		if (FAT1_lseek(&fpBytes[RECEIVED_PACKAGE][uartNr], FAT1_f_size(&fpBytes[RECEIVED_PACKAGE][uartNr])) != FR_OK || fpBytes[RECEIVED_PACKAGE][uartNr].fptr != FAT1_f_size(&fpBytes[RECEIVED_PACKAGE][uartNr])) /* move to the end of file */
			while(1){}


		/* append .log at end of filename */
		UTIL1_strcpy(fullFileNameBytes, FILENAME_ARRAY_SIZE, filenameSentBytesLogger[uartNr]);
		UTIL1_strcat(fullFileNameBytes, FILENAME_ARRAY_SIZE, ".log");
		/* open file and move to end of file */
		if (FAT1_open(&fpBytes[SENT_PACKAGE][uartNr], fullFileNameBytes, FA_OPEN_ALWAYS|FA_WRITE)!=FR_OK) /* open file */
			while(1){}
		if (FAT1_lseek(&fpBytes[SENT_PACKAGE][uartNr], FAT1_f_size(&fpBytes[SENT_PACKAGE][uartNr])) != FR_OK || fpBytes[SENT_PACKAGE][uartNr].fptr != FAT1_f_size(&fpBytes[SENT_PACKAGE][uartNr])) /* move to the end of file */
			while(1){}

		/* write log header into files */
		if(writePackLogHeader(&fpPacks[SENT_PACKAGE][uartNr], filenameSentPackagesLogger[uartNr]))
		{
			FAT1_sync(&fpPacks[SENT_PACKAGE][uartNr]);
		}
		if(writePackLogHeader(&fpPacks[RECEIVED_PACKAGE][uartNr], filenameReceivedPackagesLogger[uartNr]))
		{
			FAT1_sync(&fpPacks[RECEIVED_PACKAGE][uartNr]);
		}
	}


	/* ------------- endless loop ----------------------- */

	const TickType_t taskInterval = pdMS_TO_TICKS(config.LoggerTaskInterval); /* task interval  */
	TickType_t lastWakeTime = xTaskGetTickCount(); /* Initialize the xLastWakeTime variable with the current time. */

	for(;;) /* main loop */
	{
		vTaskDelayUntil( &lastWakeTime, taskInterval ); /* Wait for the next cycle */
		for(int uartNr = 0; uartNr < NUMBER_OF_UARTS; uartNr++)
		{
			/* write string of packages into buffer */
			logPackages(queuePackagesToLog[SENT_PACKAGE][uartNr], &fpPacks[SENT_PACKAGE][uartNr], filenameSentPackagesLogger[uartNr]);
			logPackages(queuePackagesToLog[RECEIVED_PACKAGE][uartNr], &fpPacks[RECEIVED_PACKAGE][uartNr], filenameReceivedPackagesLogger[uartNr]);
			if(uartNr==0)
			{
				logBytes(queueBytesToLog[SENT_PACKAGE][uartNr], &fpBytes[SENT_PACKAGE][uartNr], filenameSentBytesLogger[uartNr]);
				logBytes(queueBytesToLog[RECEIVED_PACKAGE][uartNr], &fpBytes[RECEIVED_PACKAGE][uartNr], filenameReceivedBytesLogger[uartNr]);
			}
			/* SD_CARD_WRITE_INTERVAL_MS passed? -> sync file system*/
			if(xTaskGetTickCount() - timestampLastLog >= pdMS_TO_TICKS(config.SdCardSyncInterval_s*1000) )
			{
				if(uartNr == 0)
				{
					FAT1_sync(&fpBytes[SENT_PACKAGE][uartNr]);
					FAT1_sync(&fpBytes[RECEIVED_PACKAGE][uartNr]);
				}
				FAT1_sync(&fpPacks[SENT_PACKAGE][uartNr]);
				FAT1_sync(&fpPacks[RECEIVED_PACKAGE][uartNr]);
				timestampLastLog = xTaskGetTickCount();
			}
		}
	}
}

void logger_TaskInit(void)
{
	const CLS1_StdIOType* io = CLS1_GetStdio();
	tWirelessPackage pack;
	bool cardMounted = 0;

	if(io == NULL)
	{
		while(true){} /* no shell assigned */
	}

	if(config.LoggingEnabled)
	{
		initLoggerQueues();

		//(void)FAT1_CheckCardPresence(&cardMounted, (unsigned char*)"0" /*volume*/, &fileSystemObject, io);
		//if(cardMounted != true)
			//FAT1_MountFileSystem(&fileSystemObject, "", io);


		/* change into log folder or create log folder if non-existent */
		if(FAT1_ChangeDirectory("LogFiles", io) != ERR_OK) /* logging directory doesn't exist? */
		{
			if(FAT1_MakeDirectory("LogFiles", io) != ERR_OK) /* create logging directory */
				while(1){} /* unable to create directory */
			if(FAT1_ChangeDirectory("LogFiles", io) != ERR_OK) /* switch to logging directory */
				while(1){} /* could not switch to logging directory */
		}

		/* create variable for filenames for future use within this task */
		for(char uartNr = '0'; uartNr < NUMBER_OF_UARTS + '0'; uartNr++)
		{
			if(XF1_xsprintf(filenameReceivedPackagesLogger[(int)(uartNr-'0')], "./rxOnCon%c", uartNr) <= 0)
				while(1){}
			if(XF1_xsprintf(filenameSentPackagesLogger[(int)(uartNr-'0')], "./txOnCon%c", uartNr) <= 0)
				while(1){}
			if(XF1_xsprintf(filenameReceivedBytesLogger[(int)(uartNr-'0')], "./rxBytWl%c", uartNr) <= 0)
				while(1){}
			if(XF1_xsprintf(filenameSentBytesLogger[(int)(uartNr-'0')], "./txBytWl%c", uartNr) <= 0)
				while(1){}
		}
	}
}

/*!
* \fn void initLoggerQueues(void)
* \brief This function initializes the array of queues
*/
static void initLoggerQueues(void)
{
#if configSUPPORT_STATIC_ALLOCATION
	static uint8_t xStaticQueueSentPacks[NUMBER_OF_UARTS][ QUEUE_NUM_OF_PACK_LOG_ENTRIES * sizeof(tWirelessPackage) ]; /* The variable used to hold the queue's data structure. */
	static uint8_t xStaticQueueRecPacks[NUMBER_OF_UARTS][ QUEUE_NUM_OF_PACK_LOG_ENTRIES * sizeof(tWirelessPackage) ];
	static uint8_t xStaticQueueSentBytes[1][ QUEUE_NUM_OF_BYTE_LOG_ENTRIES * sizeof(uint8_t) ];
	static uint8_t xStaticQueueRecBytes[1][ QUEUE_NUM_OF_BYTE_LOG_ENTRIES * sizeof(uint8_t) ];
	static StaticQueue_t ucQueueStorageSentPacks[NUMBER_OF_UARTS]; /* The array to use as the queue's storage area. */
	static StaticQueue_t ucQueueStorageRecPacks[NUMBER_OF_UARTS];
	static StaticQueue_t ucQueueStorageSentBytes[1];
	static StaticQueue_t ucQueueStorageRecBytes[1];

	queueBytesToLog[SENT_PACKAGE][0] = xQueueCreateStatic( QUEUE_NUM_OF_BYTE_LOG_ENTRIES, sizeof(uint8_t), xStaticQueueSentBytes[0], &ucQueueStorageSentBytes[0]);
	queueBytesToLog[RECEIVED_PACKAGE][0] = xQueueCreateStatic( QUEUE_NUM_OF_BYTE_LOG_ENTRIES, sizeof(uint8_t), xStaticQueueRecBytes[0], &ucQueueStorageRecBytes[0]);
	if(queueBytesToLog[SENT_PACKAGE][0] == NULL)
		while(true){} /* malloc for queue failed */
	vQueueAddToRegistry(queueBytesToLog[SENT_PACKAGE][0], queueNameSentBytes[0]);
	if(queueBytesToLog[RECEIVED_PACKAGE][0] == NULL)
		while(true){} /* malloc for queue failed */
	vQueueAddToRegistry(queueBytesToLog[RECEIVED_PACKAGE][0], queueNameReceivedBytes[0]);
#endif
	for(int uartNr = 0; uartNr<NUMBER_OF_UARTS; uartNr++)
	{
#if configSUPPORT_STATIC_ALLOCATION
		queuePackagesToLog[SENT_PACKAGE][uartNr] = xQueueCreateStatic( QUEUE_NUM_OF_PACK_LOG_ENTRIES, sizeof(tWirelessPackage), xStaticQueueSentPacks[uartNr], &ucQueueStorageSentPacks[uartNr]);
		queuePackagesToLog[RECEIVED_PACKAGE][uartNr] = xQueueCreateStatic( QUEUE_NUM_OF_PACK_LOG_ENTRIES, sizeof(tWirelessPackage), xStaticQueueRecPacks[uartNr], &ucQueueStorageRecPacks[uartNr]);
#else
		queuePackagesToLog[SENT_PACKAGE][uartNr] = xQueueCreate( QUEUE_NUM_OF_PACK_LOG_ENTRIES, sizeof(tWirelessPackage));
		queuePackagesToLog[RECEIVED_PACKAGE][uartNr] = xQueueCreate( QUEUE_NUM_OF_PACK_LOG_ENTRIES, sizeof(tWirelessPackage));
#endif
		if(queuePackagesToLog[SENT_PACKAGE][uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(queuePackagesToLog[SENT_PACKAGE][uartNr], queueNameSentPackages[uartNr]);

		if(queuePackagesToLog[RECEIVED_PACKAGE][uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(queuePackagesToLog[RECEIVED_PACKAGE][uartNr], queueNameReceivedPackages[uartNr]);
	}
}

/*!
* \fn static bool logPackages(xQueueHandle queue, FIL* filepointer, char* filename)
* \brief Writes the package content to a log file
* \param queue: queue where package is pulled from for logging
* \param filePointer: Pointer to the file where header should be written into
* \param fileName: name of the file where log header is written into, fileName without the .log ending
* \return true if successful, false if unsuccessful:
*/
static bool logPackages(xQueueHandle queue, FIL* filepointer, char* filename)
{
	tWirelessPackage pack;
	FRESULT res;

	/* concat string for all packages in queue */
	for(int i=0; i<uxQueueMessagesWaiting(queue); i++)
	{
		/* allocate string memory for package to string conversion */
		if(xQueuePeek( queue, &pack, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_LOGGER_MS) ) != pdTRUE ) { return false; }/* is there a package to log? peek before pop because malloc might fail and we need to know malloc size beforehand*/
		if(pack.payloadSize <= 0) { return false; }/* invalid package, results in faulty malloc call */
		char* singlePackLog = (char*) FRTOS_pvPortMalloc(pack.payloadSize*sizeof(char) + 100);
		if(singlePackLog == NULL) { return false; } /* malloc failed */

		/* convert package to string */
		singlePackLog[0] = 0; /* empty string */
		packageToLogString(&pack, singlePackLog, pack.payloadSize*sizeof(char) + 100); /* generate string for this package */

		/* clean up logging queue */
		if(xQueueReceive(queue, &pack, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_LOGGER_MS) ) == pdTRUE) /* pop package from queue */
		{
			writeToFile(filepointer, filename, singlePackLog); /* don't do sd card sync in every interval, very costly spi operation */
			vPortFree(pack.payload);
			pack.payload = NULL;
		}
		FRTOS_vPortFree(singlePackLog); /* free memory allocated when message was pushed into queue */
		singlePackLog = NULL;
	}
}

/*!
* \fn static bool logPackages(xQueueHandle queue, FIL* filepointer, char* filename)
* \brief Writes the package content to a log file
* \param queue: queue where package is pulled from for logging
* \param filePointer: Pointer to the file where header should be written into
* \param fileName: name of the file where log header is written into, fileName without the .log ending
* \return true if successful, false if unsuccessful:
*/
static bool logBytes(xQueueHandle queue, FIL* filepointer, char* filename)
{
	uint8_t data;
	uint16_t nofBytesInQueue = uxQueueMessagesWaiting(queue);

	/* no bytes to log? */
	if(nofBytesInQueue <= 0)
		return false;

	/* allocate string memory for byte to string conversion */
	char* logString = (char*) FRTOS_pvPortMalloc(nofBytesInQueue*3); /* 1 byte of data = 2 characters in hex plus the semicolon */
	if(logString == NULL)
		return false;
	logString[0] = 0; /* empty string */

	/* concat string for all packages in queue */
	for(int i=0; i<nofBytesInQueue; i++)
	{
		/* convert data bytes to string */
		if(xQueueReceive(queue, &data, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_LOGGER_MS) ) == pdTRUE) /* pop package from queue */
		{
			char strSingleByte[] = {0,0,0,0}; /* strSingleByte = empty */
			UTIL1_strcatNum8Hex(strSingleByte, sizeof(strSingleByte), data); /* convert data byte to hex */
			UTIL1_strcat(logString, nofBytesInQueue*3, strSingleByte); /* append to log string */
			UTIL1_strcat(logString, nofBytesInQueue*3, ";"); /* append semicolon to log string */
		}
	}
	writeToFile(filepointer, filename, logString);
	vPortFree(logString);
}

/*!
* \fn static bool writeToFile(FIL* filePointer, char* fileName, char* logEntry)
* \brief Writes a string into a file
* \param filePointer: Pointer to the file where header should be written into
* \param fileName: name of the file where log header is written into, fileName without the .log ending
* \param logEntry: string that should be written into the file, zeroterminated!
* \return true if successful, false if unsuccessful:
*/
static bool writeToFile(FIL* filePointer, char* fileName, char* logEntry)
{
  uint8_t timestamp[FILENAME_ARRAY_SIZE];
  UINT bw;
  TIMEREC time;
  const CLS1_StdIOType* io = CLS1_GetStdio();

	#if 0 // ToDo: add timestamp to logging
	  if (TmDt1_GetTime(&time)!=ERR_OK) /* get time */
		  return false;
	  timestamp[0] = '\0';
	  UTIL1_strcatNum8u(timestamp, sizeof(timestamp), time.Hour);
	  UTIL1_chcat(timestamp, sizeof(timestamp), ':');
	  UTIL1_strcatNum8u(timestamp, sizeof(timestamp), time.Min);
	  UTIL1_chcat(timestamp, sizeof(timestamp), ':');
	  UTIL1_strcatNum8u(timestamp, sizeof(timestamp), time.Sec);
	  UTIL1_chcat(timestamp, sizeof(timestamp), ';');
	  if (FAT1_write(filePointer, timestamp, UTIL1_strlen((char*)timestamp), &bw)!=FR_OK)
	  {
		return false;
	  }
	#endif

	if (FAT1_write(filePointer, logEntry, UTIL1_strlen((char*)logEntry), &bw)!=FR_OK) /* write data */
	  return false;
	else
	  return true;
}

/*!
* \fn static bool writePackLogHeader(FIL* filePointer, char* fileName)
* \brief Writes the log header into the file pointed to by filePointer with the name fileName
* \param filePointer: Pointer to the file where header should be written into
* \param fileName: name of the file where log header is written into, fileName without the .log ending
* \return true if successful, false if unsuccessful:
*/
static bool writePackLogHeader(FIL* filePointer, char* fileName)
{
	char logHeader[] = "\r\n\r\nPackageType;DeviceNumber;SessionNumber;SystemTime;PayloadSize;CRC8_Header;Payload;CRC16_Payload\r\n";
	return writeToFile(filePointer, fileName, logHeader);
}

/*!
* \fn static void packageToLogString(tWirelessPackage pPack, char* logEntry, int logEntryStrSize)
* \brief Puts all data froma  wirelessPackage into a string for logging
* \param package: The wireless package itself
* \param rxTxPackage: Information weather this is a received package or a sent package
* \param wlConnNr: wireless connection number over which package was received/sent
* \return pdTRUE if successful, pdFAIL if unsuccessful:
*/
static void packageToLogString(tWirelessPackage* pPack, char* logEntry, int logEntryStrSize)
{
	static char strNum[9]; /* 8 digits needed to convert uint32 */
	logEntry[0] = 0;
	strNum[0] = 0;
	UTIL1_strcatNum8Hex(strNum, sizeof(strNum), pPack->packType);
	UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	UTIL1_strcat(logEntry, logEntryStrSize, ";");
	strNum[0] = 0;
	UTIL1_strcatNum8Hex(strNum, sizeof(strNum), pPack->devNum);
	UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	UTIL1_strcat(logEntry, logEntryStrSize, ";");
	strNum[0] = 0;
	UTIL1_strcatNum8Hex(strNum, sizeof(strNum), pPack->sessionNr);
	UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	UTIL1_strcat(logEntry, logEntryStrSize, ";");
	strNum[0] = 0;
	UTIL1_strcatNum32Hex(strNum, sizeof(strNum), pPack->sysTime);
	UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	UTIL1_strcat(logEntry, logEntryStrSize, ";");
	strNum[0] = 0;
	UTIL1_strcatNum16Hex(strNum, sizeof(strNum), pPack->payloadSize);
	UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	UTIL1_strcat(logEntry, logEntryStrSize, ";");
	strNum[0] = 0;
	UTIL1_strcatNum8Hex(strNum, sizeof(strNum), pPack->crc8Header);
	UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	UTIL1_strcat(logEntry, logEntryStrSize, ";");
	for(int i=0; i<pPack->payloadSize; i++)
	{
		strNum[0] = 0;
		UTIL1_strcatNum8Hex(strNum, sizeof(strNum), pPack->payload[i]);
		UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	}
	UTIL1_strcat(logEntry, logEntryStrSize, ";");
	strNum[0] = 0;
	UTIL1_strcatNum16Hex(strNum, sizeof(strNum), pPack->crc16payload);
	UTIL1_strcat(logEntry, logEntryStrSize, strNum);
	UTIL1_strcat(logEntry, logEntryStrSize, "\r\n");
}

/*!
* \fn BaseType_t pushPackageToLoggerQueue(tWirelessPackage pPackage, tRxTxPackage rxTxPackage, tUartNr uartNr)
* \brief Logs package content by copying payload of pPackage into new package
* \param pPackage: The wireless package itself
* \param rxTxPackage: Information weather this is a received package or a sent package
* \param wlConnNr: wireless connection number over which package was received/sent
* \return pdTRUE if successful, pdFAIL if unsuccessful:
*/
BaseType_t pushPackageToLoggerQueue(tWirelessPackage* pPackage, tRxTxPackage rxTxPackage, tUartNr wlConnNr)
{
	if((wlConnNr >= NUMBER_OF_UARTS) || (rxTxPackage > SENT_PACKAGE) || (pPackage == NULL) || (pPackage->payload == NULL)) /* invalid arguments -> return immediately */
	{
		return pdFAIL;
	}
	/* generate new package to push on logging queue */
	tWirelessPackage tmpPackage = *pPackage;
	tmpPackage.payload = (uint8_t*) FRTOS_pvPortMalloc(tmpPackage.payloadSize*sizeof(int8_t));
	if(tmpPackage.payload == NULL) /* malloc failed */
	{
		return pdTRUE; /* because package handling was successful, only logging failure */
	}
	/* copy payload into new package */
	for(int cnt=0; cnt < tmpPackage.payloadSize; cnt++)
	{
		tmpPackage.payload[cnt] = pPackage->payload[cnt];
	}
	if(xQueueSendToBack(queuePackagesToLog[rxTxPackage][wlConnNr], &tmpPackage, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_LOGGER_QUEUE_OPERATION_MS) ) != pdTRUE) /* pushing successful? */
	{
		/* free memory before returning */
		FRTOS_vPortFree(tmpPackage.payload); /* free memory allocated when message was pushed into queue */
		tmpPackage.payload = NULL;
		return pdFAIL;
	}
	return pdTRUE; /* return success*/
}


BaseType_t pushByteToLoggerQueue(uint8_t byte, tRxTxPackage rxTxPackage, tUartNr wlConnNr)
{
	if((wlConnNr >= 1) || (rxTxPackage > SENT_PACKAGE)) /* invalid arguments -> return immediately */
	{
		return pdFAIL;
	}

	if(xQueueSendToBack(queuePackagesToLog[rxTxPackage][wlConnNr], &byte, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_LOGGER_QUEUE_OPERATION_MS) ) != pdTRUE) /* pushing successful? */
	{
		return pdFAIL;
	}
	return pdTRUE; /* return success*/
}
