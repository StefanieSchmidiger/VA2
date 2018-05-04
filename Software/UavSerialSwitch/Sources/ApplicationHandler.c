/*
 * ApplicationHandler.c
 *
 *  Created on: 02.05.2018
 *      Author: Stefanie
 */

#include "ApplicationHandler.h"
#include "NetworkHandler.h"
#include "PackageHandler.h"
#include "SpiHandler.h"
#include "FRTOS.h"
#include "RNG.h"
#include "Config.h"
#include "ThroughputPrintout.h"
#include "Shell.h"
#include "LedRed.h"

/* --------------- prototypes ------------------- */
static bool processReceivedPayload(tWirelessPackage* pPackage);
static bool generateDataPackage(tUartNr deviceNr, tWirelessPackage* pPackage);
static BaseType_t pushToReadyToSendPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage);
static void pushPayloadOut(tWirelessPackage* package);
static bool pushNextStoredPackOut(tUartNr wlConn);
static bool pushOldestPackOutIfTimeout(tUartNr wlConn, bool forced);
static void initApplicationHandlerQueues(void);
static BaseType_t peekAtReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage);
static BaseType_t nofReceivedPayloadPacksInQueue(tUartNr uartNr);
static BaseType_t popFromReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage);



/* --------------- global variables -------------------- */
static xQueueHandle queueReadyToSendPacks[NUMBER_OF_UARTS]; /* Outgoing data to wireless side */
static xQueueHandle queueReceivedPayloadPacks[NUMBER_OF_UARTS]; /* Outgoing data to wireless side */
static char* queueNameReadyToSendPacks[] = {"queueGeneratedPacksFromDev0", "queueGeneratedPacksFromDev1", "queueGeneratedPacksFromDev2", "queueGeneratedPacksFromDev3"};
static char* queueNameReceivedPayload[] = {"queueReceivedPacksFromDev0", "queueReceivedPacksFromDev1", "queueReceivedPacksFromDev2", "queueReceivedPacksFromDev3"};
static uint16_t payloadNumTracker[NUMBER_OF_UARTS];
static uint16_t sysTimeLastPushedOutPack[NUMBER_OF_UARTS];
static uint16_t minSysTimeOfStoredPackagesForReordering[NUMBER_OF_UARTS];
static tWirelessPackage reorderingPacks[NUMBER_OF_UARTS][REORDERING_PACKAGES_BUFFER_SIZE];
static bool reorderingPacksOccupiedAtIndex[NUMBER_OF_UARTS][REORDERING_PACKAGES_BUFFER_SIZE];
static uint32_t nofReorderingPacksStored[NUMBER_OF_UARTS];


/*!
* \fn void networkHandler_TaskEntry(void)
* \brief Task generates packages from received bytes (received on device side) and sends those down to
* the NetworkHandler for transmission.
* Payload reordering is done here when configured
*/
void applicationHandler_TaskEntry(void* p)
{
	const TickType_t taskInterval = pdMS_TO_TICKS(config.ApplicationHandlerTaskInterval);
	tWirelessPackage package;
	TickType_t xLastWakeTime = xTaskGetTickCount(); /* Initialize the lastWakeTime variable with the current time. */


	for(;;)
	{
		vTaskDelayUntil( &xLastWakeTime, taskInterval ); /* Wait for the next cycle */
		/* generate data packages and put those into the package queue */
		for(int deviceNr = 0; deviceNr<NUMBER_OF_UARTS; deviceNr++)
		{
			/* generate packages from raw data bytes and send to correct readyToSendPacks queue */
			if(generateDataPackage(deviceNr, &package)) /* generate package from raw device data bytes */
			{
				if(pushToReadyToSendPacksQueue(deviceNr, &package) != pdTRUE)
				{
					vPortFree(package.payload);
				}
			}

			/* push payload of wireless package out on device side */
			if(nofReceivedPayloadPacksInQueue(deviceNr) > 0)
			{
				peekAtReceivedPayloadPacksQueue(deviceNr, &package);
				if(freeSpaceInTxByteQueue(MAX_14830_DEVICE_SIDE, package.devNum) >= package.payloadSize) /* check if enough space */
				{
					processReceivedPayload(&package);
					popFromReceivedPayloadPacksQueue(deviceNr, &package);
					vPortFree(package.payload);
				}
			}

			/* check if there is a timeout on the package reordering */
			if(config.PayloadNumberingProcessingMode[deviceNr] == PAYLOAD_REORDERING) /* package reordering configured for this device? */
			{
				while(pushOldestPackOutIfTimeout(deviceNr, false)) /* a timeout occured for oldest saved package? */
				{
					while(pushNextStoredPackOut(deviceNr)); /* push next packages out that are in order */
				}
			}

		}
	}
}

/*!
* \fn void networkHandler_TaskInit(void)
* \brief Initializes all queues that are declared within application handler
*/
void applicationHandler_TaskInit(void)
{
	initApplicationHandlerQueues();
}



/*!
* \fn void initNetworkHandlerQueues(void)
* \brief This function initializes the array of queues
*/
static void initApplicationHandlerQueues(void)
{
#if configSUPPORT_STATIC_ALLOCATION
	static uint8_t xStaticQueuePacksToSend[NUMBER_OF_UARTS][ QUEUE_NUM_OF_READY_TO_SEND_WL_PACKS * sizeof(tWirelessPackage) ]; /* The variable used to hold the queue's data structure. */
	static uint8_t xStaticQueuePayloadReceived[NUMBER_OF_UARTS][ QUEUE_NUM_OF_RECEIVED_PAYLOAD_PACKS * sizeof(tWirelessPackage) ]; /* The variable used to hold the queue's data structure. */
	static StaticQueue_t ucQueueStoragePacksToSend[NUMBER_OF_UARTS]; /* The array to use as the queue's storage area. */
	static StaticQueue_t ucQueueStoragePayloadReceived[NUMBER_OF_UARTS]; /* The array to use as the queue's storage area. */
#endif
	for(int uartNr=0; uartNr<NUMBER_OF_UARTS; uartNr++)
	{
#if configSUPPORT_STATIC_ALLOCATION
		queueReadyToSendPacks[uartNr] = xQueueCreateStatic( QUEUE_NUM_OF_READY_TO_SEND_WL_PACKS, sizeof(tWirelessPackage), xStaticQueuePacksToSend[uartNr], &ucQueueStoragePacksToSend[uartNr]);
		queueReceivedPayloadPacks[uartNr] = xQueueCreateStatic( QUEUE_NUM_OF_RECEIVED_PAYLOAD_PACKS, sizeof(tWirelessPackage), xStaticQueuePayloadReceived[uartNr], &ucQueueStoragePayloadReceived[uartNr]);
#else
		queuePackagesToSend[uartNr] = xQueueCreate( QUEUE_NUM_OF_READY_TO_SEND_WL_PACKS, sizeof(tWirelessPackage));
		queueReceivedPayloadPacks[uartNr] = xQueueCreate( QUEUE_NUM_OF_RECEIVED_WL_PACKS, sizeof(tWirelessPackage));
#endif
		if((queueReadyToSendPacks[uartNr] == NULL) || (queueReceivedPayloadPacks == NULL))
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(queueReadyToSendPacks[uartNr], queueNameReadyToSendPacks[uartNr]);
		vQueueAddToRegistry(queueReceivedPayloadPacks[uartNr], queueNameReceivedPayload[uartNr]);
	}
}


/*!
* \fn static void generateDataPackage(uint8_t deviceNumber, Queue* dataSource)
* \brief Function to generate a data package, reading data from the data source.
* \param deviceNumber: Number of device.
* \param dataSource: Pointer to the queue where the data where the data is read.
* \param wPackage: Pointer to package structure.
* \return true if a package was generated and saved in wPackage, false otherwise.
*/
static bool generateDataPackage(tUartNr deviceNr, tWirelessPackage* pPackage)
{
	static uint32_t tickTimeSinceFirstCharReceived[NUMBER_OF_UARTS]; /* static variables are initialized as 0 by default */
	static bool dataWaitingToBeSent[NUMBER_OF_UARTS];
	static const uint8_t packHeaderBuf[PACKAGE_HEADER_SIZE - 1] = { PACK_START, PACK_TYPE_DATA_PACKAGE, 0, 0, 0, 0, 0, 0, 0, 0 };
	char infoBuf[60];

	uint16_t numberOfBytesInRxQueue = (uint16_t) numberOfBytesInRxByteQueue(MAX_14830_DEVICE_SIDE, deviceNr);
	uint32_t timeWaitedForPackFull = xTaskGetTickCount()-tickTimeSinceFirstCharReceived[deviceNr];

	if((deviceNr >= NUMBER_OF_UARTS) || (pPackage == NULL)) /* check validity of function parameters */
	{
		return false;
	}
	/* check if enough data to fill package (when not configured to 0) or maximum wait time for full package done */
	if( ( (numberOfBytesInRxQueue >= config.UsualPacketSizeDeviceConn[deviceNr]) && (0 != config.UsualPacketSizeDeviceConn[deviceNr]) ) ||
		(numberOfBytesInRxQueue >= PACKAGE_MAX_PAYLOAD_SIZE) ||
		( (dataWaitingToBeSent[deviceNr] == true) && (timeWaitedForPackFull >= pdMS_TO_TICKS(config.PackageGenMaxTimeout[deviceNr])) ) )
	{
		/* reached usual packet size or timeout, generate package
		 * check if there is enough space to store package in queue before generating it
		 * Dropping of data in case all queues are full is handled in byte queue.
		 * When reading data from HW buf but no space in byte queue, oldest bytes will be popped from queue and dropped.
		 * Hopefully, this will do and no dropping of data on purpose is needed anywhere else for Rx side. */
		/* limit payload of package */
		if(numberOfBytesInRxQueue > PACKAGE_MAX_PAYLOAD_SIZE)
		{
			numberOfBytesInRxQueue = PACKAGE_MAX_PAYLOAD_SIZE;
		}
		/* Put together package */
		/* put together payload by allocating memory and copy data */
		pPackage->payloadSize = numberOfBytesInRxQueue;
		pPackage->payload = (uint8_t*) FRTOS_pvPortMalloc(numberOfBytesInRxQueue*sizeof(int8_t));
		if(pPackage->payload == NULL) /* malloc failed */
			return false;
		/* get data from queue */
		for (uint16_t cnt = 0; cnt < pPackage->payloadSize; cnt++)
		{
			if(popFromByteQueue(MAX_14830_DEVICE_SIDE, deviceNr, &pPackage->payload[cnt]) != pdTRUE) /* ToDo: handle queue failure */
			{
				UTIL1_strcpy(infoBuf, sizeof(infoBuf), "Warning: Pop from UART ");
				UTIL1_strcatNum8u(infoBuf, sizeof(infoBuf), pPackage->devNum);
				UTIL1_strcat(infoBuf, sizeof(infoBuf), " not successful");
				pushMsgToShellQueue(infoBuf);
				numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][deviceNr] += cnt;
				FRTOS_vPortFree(pPackage->payload);
				pPackage->payload = NULL;
				return false;
			}
		}
		/* put together the rest of the header */
		pPackage->packType = PACK_TYPE_DATA_PACKAGE;
		pPackage->devNum = deviceNr;
		pPackage->payloadNr = ++payloadNumTracker[deviceNr];
		/* reset last timestamp */
		tickTimeSinceFirstCharReceived[deviceNr] = 0;
		/* reset flag that timestamp was updated */
		dataWaitingToBeSent[deviceNr] = false;
	}
	else if(numberOfBytesInRxQueue > 0)
	{
		/* there is data available, update timestamp that data is available (but only if the timestamp wasn't already updated) */
		if (dataWaitingToBeSent[deviceNr] == false)
		{
			tickTimeSinceFirstCharReceived[deviceNr] = xTaskGetTickCount();
			dataWaitingToBeSent[deviceNr] = true;
		}
		return false;
	}
	else
	{
		/* package was not generated */
		return false;
	}
}



static bool processReceivedPayload(tWirelessPackage* pPackage)
{
	char infoBuf[50];
	/* send data out at correct device side if packages received in order*/
	switch (config.PayloadNumberingProcessingMode[pPackage->devNum])
	{
	case PAYLOAD_NUMBER_IGNORED:
		pushPayloadOut(pPackage);
		sysTimeLastPushedOutPack[pPackage->devNum] = pPackage->packNr; /* no need to keep track of package numbering, but done anyway here */
		break;
	case PAYLOAD_REORDERING:
		if (sysTimeLastPushedOutPack[pPackage->devNum] <= pPackage->packNr) /* old package received */
		{
			break;
		}
		else if (sysTimeLastPushedOutPack[pPackage->devNum] != pPackage->packNr + 1) /* package not received in order, saving to buffer needed */

		{
			tWirelessPackage nextPack;
			/* package cant be stored, no empty space */
			if (nofReorderingPacksStored[pPackage->devNum]
					>= sizeof(reorderingPacksOccupiedAtIndex[pPackage->devNum])) {
				pushOldestPackOutIfTimeout(pPackage->devNum, true); /* force oldest package to be pushed out */
				while (pushNextStoredPackOut(pPackage->devNum))
					; /* push any other packages out that are now stored in order */
			}
			/* iterate through array to find an empty spot and store package there */
			for (int i = 0; i < sizeof(reorderingPacks[pPackage->devNum]); i++) {
				if (reorderingPacksOccupiedAtIndex[pPackage->devNum][i] != 1) /* spot empty? */
				{
					reorderingPacks[pPackage->devNum][i] = *pPackage; /* save package at new spot */
					reorderingPacksOccupiedAtIndex[pPackage->devNum][i] = 1; /*mark spot as occupied */
					nofReorderingPacksStored[pPackage->devNum]++;
					break; /* leave for loop */
				}
			}
		} else if (sysTimeLastPushedOutPack[pPackage->devNum]
				== pPackage->packNr + 1) /* package received in order */
				{
			pushPayloadOut(pPackage); /* push out current package */
			/* payload is freed at the end of this function */
			sysTimeLastPushedOutPack[pPackage->devNum] = pPackage->packNr; /* update package number tracker */
			while (pushNextStoredPackOut(pPackage->devNum))
				;
		}
		break;
	case ONLY_SEND_OUT_NEW_PAYLOAD:
		if (sysTimeLastPushedOutPack[pPackage->devNum] < pPackage->packNr) /* package is newer than the last one pushed out */
		{
			pushPayloadOut(pPackage);
			sysTimeLastPushedOutPack[pPackage->devNum] = pPackage->packNr;
		}
		break;
	default:
		UTIL1_strcpy(infoBuf, sizeof(infoBuf),"Error: Wrong configuration for PACK_NUMBERING_PROCESSING_MODE, value not in range\r\n");
		LedRed_On();
		pushMsgToShellQueue(infoBuf);
		pushPayloadOut(pPackage);
		sysTimeLastPushedOutPack[pPackage->devNum] = pPackage->packNr; /* no need to keep track of package numbering, but done anyway here */
	}
	return true;
}


/*!
* \fn static void pushPayloadOut(tWirelessPackage package)
* \brief Pushes the payload of a wireless package out on the correct device
* \param pPackage: pointer to package to be pushed out
*/
static void pushPayloadOut(tWirelessPackage* pPackage)
{
	static char infoBuf[80];
	for(uint16_t cnt=0; cnt<pPackage->payloadSize; cnt++)
	{
		if(pushToByteQueue(MAX_14830_DEVICE_SIDE, pPackage->devNum, &pPackage->payload[cnt]) == pdFAIL)
		{
			XF1_xsprintf(infoBuf, "%u: Warning: Push to device byte array for UART %u failed", xTaskGetTickCount(), pPackage->devNum);
			pushMsgToShellQueue(infoBuf);
			numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][pPackage->devNum]++;
		}
	}
}


/*!
* \fn static void pushNextStoredPackOut(tUartNr uartNr)
* \brief Iterates through the stored packages and pushes the next one out if it is the right order
* \param uartNr: which device is affected
*/
static bool pushNextStoredPackOut(tUartNr uartNr)
{
	tWirelessPackage nextPack;
	uint32_t nofReorderingPacksLeft = nofReorderingPacksStored[uartNr];
	for(int i=0; i<sizeof(reorderingPacks[uartNr]); i++)
	{
		if(reorderingPacksOccupiedAtIndex[uartNr][i])
		{
			nofReorderingPacksLeft--;
			nextPack = reorderingPacks[uartNr][i];
			if(nextPack.packNr == sysTimeLastPushedOutPack[uartNr]+1)
			{
				pushPayloadOut(&nextPack);
				vPortFree(nextPack.payload);
				reorderingPacksOccupiedAtIndex[uartNr][i] = 0;
				nofReorderingPacksStored[uartNr]--;
				sysTimeLastPushedOutPack[uartNr] = nextPack.packNr;
				return true;
			}
		}
		if(nofReorderingPacksLeft <= 0)
		{
			/* no need to iterate over the entire array if we know that there are no more packages stored */
			return false;
		}
	}
	return false;
}

/*!
* \fn static bool pushOldestPackOutIfTimeout(tUartNr uartNr, bool forced)
* \brief Iterates through the receivedPackages array and pushes the package out with the
* lowest sysTime if its timeout is done or if forced == true then the oldest package is pushed out
* no matter if its timeout is done or not.
* \param uartNr: specify which device side
* \param forced: force oldest package out, no matter if timeout done or not -> freeing space
*/
static bool pushOldestPackOutIfTimeout(tUartNr uartNr, bool forced)
{
	tWirelessPackage oldestPack;
	tWirelessPackage tmpPack;
	bool firstIteration = true;
	int oldestPackIndex = 0;

	/* find package with the oldest timestamp (sysTime */
	for(int i=0; i<sizeof(reorderingPacks[uartNr]); i++)
	{
		if(reorderingPacksOccupiedAtIndex[uartNr][i]) /* there is a package stored at this index */
		{
			tmpPack = reorderingPacks[uartNr][i];
			if(firstIteration == true) /* on first iteration, initialize variable with any package */
			{
				oldestPack = tmpPack;
				oldestPackIndex = i;
				firstIteration = false; /* only initialize once */
			}

			if(tmpPack.packNr < oldestPack.packNr) /* found older package than the previous one */
			{
				oldestPack = tmpPack;
				oldestPackIndex = i;
			}
		}
	}

	if(firstIteration == true) /* the array is empty, no package found */
	{
		return false;
	}

	if(forced || (oldestPack.timestampPackageReceived + config.PayloadReorderingTimeout[uartNr] > xTaskGetTickCount())) /* oldest package is timed out or needs to be pushed out */
	{
		pushPayloadOut(&oldestPack);
		vPortFree(oldestPack.payload);
		oldestPack.payload = NULL;
		reorderingPacksOccupiedAtIndex[uartNr][oldestPackIndex] = false;
		nofReorderingPacksStored[uartNr]--;
		sysTimeLastPushedOutPack[uartNr] = oldestPack.packNr;
		return true;
	}
	else /* oldest package is not timed out */
	{
		return false;
	}
}





/*!
* \fn static ByseType_t pushToReadyToSendPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
* \brief Stores package in queue
* \param uartNr: UART number where data was collected.
* \param pPackage: The location of the package to be pushed to the queue
* \return Status if xQueueSendToBack has been successful
*/
static BaseType_t pushToReadyToSendPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueueSendToBack(queueReadyToSendPacks[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(APPLICATION_HANDLER_QUEUE_DELAY) );
	}

	return pdFAIL; /* if uartNr was not in range */
}



/*!
* \fn ByseType_t popToReadyToSendPackFromQueue(tUartNr uartNr, tWirelessPackage* pPackage)
* \brief Pops a package from queue
* \param uartNr: UART number where data was collected.
* \param pPackage: Pointer to empty package
* \return Status if xQueueReceive has been successful
*/
BaseType_t popFromReadyToSendPackQueue(tUartNr uartNr, tWirelessPackage* pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueueReceive(queueReadyToSendPacks[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(APPLICATION_HANDLER_QUEUE_DELAY) );
	}
	return pdFAIL; /* if uartNr was not in range */
}

/*!
* \fn ByseType_t nofReadyToSendPackInQueue(tUartNr uartNr)
* \brief Peeks at queue to see how many elements are inside
* \param uartNr: UART number where data was collected.
* \return Status if xQueueReceive has been successful
*/
BaseType_t nofReadyToSendPackInQueue(tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return uxQueueMessagesWaiting( queueReadyToSendPacks[uartNr] );
	}
	return 0; /* if uartNr was not in range */
}

/*!
* \fn ByseType_t peekAtReadyToSendPackInQueue(tUartNr uartNr)
* \brief Peeks at queue and copies next element into pPackage
* \param uartNr: UART number where data was collected.
* \param pPackage: Pointer to tWirelessPackage element where data will be copied
* \return Status if xQueuePeek has been successful
*/
BaseType_t peekAtReadyToSendPackInQueue(tUartNr uartNr, tWirelessPackage* pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueuePeek( queueReadyToSendPacks[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(APPLICATION_HANDLER_QUEUE_DELAY) );
	}
	return 0; /* if uartNr was not in range */
}


/*!
* \fn ByseType_t pushToReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
* \brief Stores package in queue
* \param uartNr: UART number where data was collected.
* \param pPackage: The location of the package to be pushed to the queue
* \return Status if xQueueSendToBack has been successful
*/
BaseType_t pushToReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueueSendToBack(queueReceivedPayloadPacks[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(APPLICATION_HANDLER_QUEUE_DELAY) );
	}

	return pdFAIL; /* if uartNr was not in range */
}



/*!
* \fn static ByseType_t popFromReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
* \brief Pops a package from queue
* \param uartNr: UART number where data was collected.
* \param pPackage: Pointer to empty package
* \return Status if xQueueReceive has been successful
*/
static BaseType_t popFromReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueueReceive(queueReceivedPayloadPacks[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(APPLICATION_HANDLER_QUEUE_DELAY) );
	}
	return pdFAIL; /* if uartNr was not in range */
}

/*!
* \fn ByseType_t peekAtReceivedPayloadPacksQueue(tUartNr uartNr)
* \brief Peeks at queue and copies next element into pPackage
* \param uartNr: UART number where data was collected.
* \param pPackage: Pointer to tWirelessPackage element where data will be copied
* \return Status if xQueuePeek has been successful
*/
static BaseType_t peekAtReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueuePeek( queueReceivedPayloadPacks[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(APPLICATION_HANDLER_QUEUE_DELAY) );
	}
	return 0; /* if uartNr was not in range */
}


/*!
* \fn static ByseType_t nofReceivedPayloadPacksInQueue(tUartNr uartNr)
* \brief Peeks at queue to see how many elements are inside
* \param uartNr: UART number where data was collected.
* \return Status if xQueueReceive has been successful
*/
static BaseType_t nofReceivedPayloadPacksInQueue(tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return uxQueueMessagesWaiting( queueReceivedPayloadPacks[uartNr] );
	}
	return 0; /* if uartNr was not in range */
}


/*!
* \fn ByseType_t freeSpaceInReceivedPayloadPacksQueue(tUartNr uartNr)
* \brief Returns number of free slots in queue
* \param uartNr: UART number where payload will be pushed out
* \return Number of elements that can still be pushed to this queue
*/
BaseType_t freeSpaceInReceivedPayloadPacksQueue(tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return ( QUEUE_NUM_OF_RECEIVED_PAYLOAD_PACKS - uxQueueMessagesWaiting( queueReceivedPayloadPacks[uartNr] ));
	}
	return 0; /* if uartNr was not in range */
}

