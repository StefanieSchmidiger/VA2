#include <Config.h>
#include <CRC1.h>
#include <FreeRTOS.h>
#include <NetworkHandler.h>
#include <portmacro.h>
#include <projdefs.h>
#include <PE_Types.h>
#include <queue.h>
#include <stdio.h> // modulo
#include <RNG.h> // for random sessionNr
#include <stdbool.h>
#include <stdint.h>
#include "UTIL1.h" // strcat
#include <string.h> // strlen
#include <Shell.h> // to print out debugInfo
#include <task.h>
#include <ThroughputPrintout.h>
#include "Logger.h"
#include "LedRed.h"


/* global variables, only used in this file */
static uint8_t sessionNr;
static xQueueHandle queuePackagesToSend[NUMBER_OF_UARTS]; /* Incoming data from wireless side stored here */
static LDD_TDeviceData* crcNH;
static tWirelessPackage unacknowledgedPackages[MAX_NUMBER_OF_UNACK_PACKS_STORED];
static bool unacknowledgedPackagesOccupiedAtIndex[MAX_NUMBER_OF_UNACK_PACKS_STORED];
static int numberOfUnacknowledgedPackages;
static uint32_t sentPackNumTracker[NUMBER_OF_UARTS];
static uint32_t recPackNumTracker[NUMBER_OF_UARTS];
static tWirelessPackage reorderingPacks[NUMBER_OF_UARTS][REORDERING_PACKAGES_BUFFER_SIZE];
static int reorderingPacksHeadPointer[NUMBER_OF_UARTS];
static SemaphoreHandle_t ackReceived[NUMBER_OF_UARTS];

/* prototypes of local functions */
static bool generateDataPackage(tUartNr wlConn, tWirelessPackage* pPackage, uint8_t sessionNr);
static void initNetworkHandlerQueues(void);
static void initSempahores(void);
static bool processReceivedPackage(tUartNr wlConn);
static bool sendAndStoreGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr rawDataUartNr);
static bool storeNewPackageInUnacknowledgedPackagesArray(tWirelessPackage* pPackage);
static char* queueName[] = {"queuePackagesToSend0", "queuePackagesToSend1", "queuePackagesToSend2", "queuePackagesToSend3"};
static uint8_t getWlConnectionToUse(tUartNr uartNr, uint8_t desiredPrio);
static bool generateAckPackage(tWirelessPackage* pReceivedDataPack, tWirelessPackage* pAckPack);
static void handleResendingOfUnacknowledgedPackages(void);
BaseType_t pushToSentPackagesQueue(tUartNr wlConn, tWirelessPackage package);
static void pushPayloadOut(tWirelessPackage package);


/*!
* \fn void networkHandler_TaskEntry(void)
* \brief Task generates packages from received bytes (received on device side) and sends those down to
* the packageHandler for transmission.
* When acknowledges are configured, resending is handled here.
*/
void networkHandler_TaskEntry(void* p)
{
	const TickType_t taskInterval = pdMS_TO_TICKS(config.NetworkHandlerTaskInterval);
	tWirelessPackage package;
	TickType_t xLastWakeTime = xTaskGetTickCount(); /* Initialize the lastWakeTime variable with the current time. */


	for(;;)
	{
		vTaskDelayUntil( &xLastWakeTime, taskInterval ); /* Wait for the next cycle */
		/* generate data packages and put those into the package queue */
		for(int deviceNr = 0; deviceNr<NUMBER_OF_UARTS; deviceNr++)
		{
			/* generate packages from raw data bytes and send to correct packagesToSend queue */
			if(generateDataPackage(deviceNr, &package, sessionNr)) /* generate package from raw device data bytes */
				sendAndStoreGeneratedWlPackage(&package, deviceNr); /* send the generated package to the correct queue and store it internally if ACK is configured */

			/* extract data from received packages, send ACK and send raw data to corresponding UART interface */
			if(numberOfPacksInReceivedPacksQueue(deviceNr) > 0)
				processReceivedPackage(deviceNr);

			/* handle resend in case acknowledge not received */
			handleResendingOfUnacknowledgedPackages();
		}
	}
}

/*!
* \fn void networkHandler_TaskInit(void)
* \brief Initializes all queues that are declared within network handler
* and generates random session number
*/
void networkHandler_TaskInit(void)
{
	initNetworkHandlerQueues();
	initSempahores();
	crcNH = CRC1_Init(NULL);
	LDD_TDeviceData* rng = RNG_Init(NULL); /* initializes random number generator */
	uint32_t randomNumber;

	/* generate random 8bit session number */
	if(rng == NULL)
		while(true){} /* RNG could not be initialized */
	RNG_GetRandomNumber(rng, &randomNumber);
	sessionNr = (uint8_t) randomNumber;
}



/*!
* \fn void initNetworkHandlerQueues(void)
* \brief This function initializes the array of queues
*/
static void initNetworkHandlerQueues(void)
{
#if configSUPPORT_STATIC_ALLOCATION
	static uint8_t xStaticQueue[NUMBER_OF_UARTS][ QUEUE_NUM_OF_WL_PACK_TO_SEND * sizeof(tWirelessPackage) ]; /* The variable used to hold the queue's data structure. */
	static StaticQueue_t ucQueueStorage[NUMBER_OF_UARTS]; /* The array to use as the queue's storage area. */
#endif
	for(int uartNr=0; uartNr<NUMBER_OF_UARTS; uartNr++)
	{
#if configSUPPORT_STATIC_ALLOCATION
		queuePackagesToSend[uartNr] = xQueueCreateStatic( QUEUE_NUM_OF_WL_PACK_TO_SEND, sizeof(tWirelessPackage), xStaticQueue[uartNr], &ucQueueStorage[uartNr]);
#else
		queuePackagesToSend[uartNr] = xQueueCreate( QUEUE_NUM_OF_WL_PACK_TO_SEND, sizeof(tWirelessPackage));
#endif
		if(queuePackagesToSend[uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(queuePackagesToSend[uartNr], queueName[uartNr]);
	}
}

/*!
* \fn static void initSempahores(void)
* \brief This function initializes the array of semaphores
*/
static void initSempahores(void)
{
	for(int uartNr=0; uartNr<NUMBER_OF_UARTS; uartNr++)
	{
		ackReceived[uartNr] = FRTOS_xSemaphoreCreateBinary();
		if(ackReceived[uartNr] == NULL)
			while(true){} /* malloc failed */
		FRTOS_xSemaphoreGive(ackReceived[uartNr]); /* sending package = taking semaphore, receiving ack = giving it back */
	}
}


/*!
* \fn static bool sendAndStoreGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr rawDataUartNr)
* \brief Sends the generated package to package handler for sending and stores it in internal buffer if ACK is expected according to config file
* \param pPackage: pointer to package that should be sent to out
* \param rawDataUartNr: device number where raw data was read that is now put into this package
* \return true if a package was sent to package handler and stored in buffer successfully
*/
static bool sendAndStoreGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr rawDataUartNr)
{
	uint8_t wlConn = 0;
	char infoBuf[50];
	/* find configured WL connection number for first send of this package */
	for(int prio=1; prio <= NUMBER_OF_UARTS; prio++)
	{
		wlConn = getWlConnectionToUse(rawDataUartNr, prio);
		if(wlConn >= NUMBER_OF_UARTS) /* maximum priority in config file reached */
		{
			numberOfDroppedPackages[UTIL1_constrain(getWlConnectionToUse(rawDataUartNr, UTIL1_constrain(prio-1, 1, NUMBER_OF_UARTS-1)), 1, NUMBER_OF_UARTS-1)]; /* make sure the prio for function call is within range */
			XF1_xsprintf(infoBuf, "Warning: Couldn't push newly generated package from device %u to package queue\r\n", rawDataUartNr);
			pushMsgToShellQueue(infoBuf);
			return false;
		}
		if(pushToSentPackagesQueue(wlConn,*pPackage) != pdTRUE)
			continue; /* try next priority -> go though for-loop again */
		/* update throughput printout */
		numberOfPacksSent[wlConn]++;
		numberOfPayloadBytesSent[wlConn] += pPackage->payloadSize;
		/* store generated package in internal array if acknowledge is expected from this WL connection */
		if(config.SendAckPerWirelessConn[wlConn])
		{
			pPackage->sendAttemptsLeftPerWirelessConnection[wlConn]--;
			pPackage->timestampLastSendAttempt[wlConn] = xTaskGetTickCount();
			pPackage->timestampFirstSendAttempt = xTaskGetTickCount();
			if(storeNewPackageInUnacknowledgedPackagesArray(pPackage) == true)
				return true; /* success */
		}
		else /* no ack expected */
		{
			return true; /* success */
		}

	}
	/* ToDo: handle failure of storeNewPackageInUnacknowledgedPackagesArray() */
	XF1_xsprintf(infoBuf, "Warning: Unacknowledged packages array is full\r\n");
	pushMsgToShellQueue(infoBuf);
	FRTOS_vPortFree(pPackage->payload); /* free memory since it wont be freed when popped from queue */
	numberOfDroppedPackages[wlConn]++;
	return false;
}

/*!
* \fn static uint8_t getWlConnectionToUse(tUartNr uartNr, uint8_t desiredPrio)
* \brief Checks which wireless connection number is configured with the desired priority
* \return wlConnectionToUse: a number between 0 and (NUMBER_OF_UARTS-1). This priority is not configured if NUMBER_OF_UARTS is returned.
*/
static uint8_t getWlConnectionToUse(tUartNr uartNr, uint8_t desiredPrio)
{
	uint8_t wlConnectionToUse = 0;
	while ( wlConnectionToUse < NUMBER_OF_UARTS && config.PrioWirelessConnDev[uartNr][wlConnectionToUse] != desiredPrio ) ++wlConnectionToUse;
	return wlConnectionToUse;
}

/*!
* \fn static bool processReceivedPackage(tUartNr wirelessConnNr)
* \brief Pops received package from queue and checks if it is ACK or Data package.
* ACK package -> deletes the package from the buffer where we wait for ACKS.
* data package -> generates ACK and sends it to package handler queue for packages to send.
* \param wirelessConnNr: The wireless device number where we look for received packages
*/
static bool processReceivedPackage(tUartNr wlConn)
{
	static tWirelessPackage defaultPackage;
	tWirelessPackage package;
	char infoBuf[150];
	defaultPackage.devNum = 44;

	/* if it is a data package -> check if there is enough space on byte queue of device side */
	if(peekAtReceivedPackQueue(wlConn, &package) != pdTRUE) /* peek at package to find out payload size for space on Device Tx Bytes queue */
		return false; /* peek not successful */
	if((package.packType == PACK_TYPE_DATA_PACKAGE) &&
			freeSpaceInTxByteQueue(MAX_14830_DEVICE_SIDE, package.devNum) < package.payloadSize) /* enough space to push device bytes down? */
		return false; /* not enough space */
	/* ToDo: check if ack queue full in case ack will be generated and needs to be pushed down */
	/* pop package from queue to send it out */
	if(popReceivedPackFromQueue(wlConn, &package) != pdTRUE) /* actually remove package from queue */
		return false; /* coun't be removed */
	if(package.packType == PACK_TYPE_DATA_PACKAGE) /* data package received */
	{
		/* check if data is valid */
		if(package.payloadSize > PACKAGE_MAX_PAYLOAD_SIZE)
			package.payloadSize = PACKAGE_MAX_PAYLOAD_SIZE;
		if(package.devNum > NUMBER_OF_UARTS)
			package.devNum = NUMBER_OF_UARTS-1;
		/* send data out at correct device side if packages received in order*/
		switch((int) config.PackNumberingProcessingMode)
		{
		case PACKAGE_REORDERING:
			if(recPackNumTracker[package.devNum] != package.sysTime + 1) /* package not received in order, saving to buffer needed */
			{
				uint8_t index = package.sysTime - recPackNumTracker[package.devNum]; /* new packNum - old packNum = ringbuffer index */
				uint8_t cnt = reorderingPacksHeadPointer[package.devNum];
				tWirelessPackage tmpPack;
				unsigned char flagMinNumOfPacksDiscarded = 0;
				if(index >= REORDERING_PACKAGES_BUFFER_SIZE) /* index out of range */
				{
					// Todo: discard old packages instead of discarding newest one
					index = index % REORDERING_PACKAGES_BUFFER_SIZE;
					do
					{
						if(cnt == index)
							flagMinNumOfPacksDiscarded = 1;
						tmpPack = reorderingPacks[package.devNum][cnt];
						if(tmpPack.devNum != defaultPackage.devNum) /* package saved under this index */
						{
							pushPayloadOut(tmpPack); /* push out current package */
							recPackNumTracker[tmpPack.devNum] = tmpPack.sysTime; /* update package number tracker */
						}
						if((++cnt) >= REORDERING_PACKAGES_BUFFER_SIZE) /* start over at beginning of array (ringbuffer) if index out of range */
							cnt = 0;
					} while((tmpPack.devNum != defaultPackage.devNum) && flagMinNumOfPacksDiscarded);
				}
				if(index + reorderingPacksHeadPointer[package.devNum] >= REORDERING_PACKAGES_BUFFER_SIZE) /* end reached -> start over at beginning of array */
					index = index - (REORDERING_PACKAGES_BUFFER_SIZE - reorderingPacksHeadPointer[package.devNum]);
				reorderingPacks[package.devNum][index] = package;
			}
			if(recPackNumTracker[package.devNum] == package.sysTime + 1) /* package received in order */
			{
				do
				{
					pushPayloadOut(package); /* push out current package */
					recPackNumTracker[package.devNum] = package.sysTime; /* update package number tracker */
					/* update head pointer to received packages ringbuffer */
					reorderingPacks[package.devNum][reorderingPacksHeadPointer[package.devNum]] = defaultPackage; /* needed if do-while loop is executed again with package popped from ringbuffer */
					reorderingPacksHeadPointer[package.devNum]++;
					if(reorderingPacksHeadPointer[package.devNum] >= REORDERING_PACKAGES_BUFFER_SIZE) /* start over at beginning of array (ringbuffer) if index out of range */
						reorderingPacksHeadPointer[package.devNum] = 0;
					package = reorderingPacks[package.devNum][reorderingPacksHeadPointer[package.devNum]]; /* pop next package from queue */
				} while(package.devNum != defaultPackage.devNum); /* next stored ringbuffer element not empty? */
			}
			break;
		case ONLY_SEND_OUT_NEW_PACKAGES:
			if(recPackNumTracker[package.devNum] <= package.sysTime) /* package is newer than the last one pushed out */
			{
				pushPayloadOut(package);
				recPackNumTracker[package.devNum] = package.sysTime;
			}
			break;
		case WAIT_FOR_ACK_BEFORE_SENDING_NEXT_PACK:
			pushPayloadOut(package);/* send out data stream right away because packages are in right order */
			recPackNumTracker[package.devNum] = package.sysTime;
			break;
		case PACKAGE_NUMBER_IGNORED:
			pushPayloadOut(package);
			recPackNumTracker[package.devNum] = package.sysTime; /* no need to keep track of package numbering, but done anyway here */
			break;
		default:
			UTIL1_strcpy(infoBuf, sizeof(infoBuf), "Error: Wrong configuration for PACK_NUMBERING_PROCESSING_MODE, value not in range\r\n");
			LedRed_On();
			pushMsgToShellQueue(infoBuf);
			pushPayloadOut(package);
			recPackNumTracker[package.devNum] = package.sysTime; /* no need to keep track of package numbering, but done anyway here */
		}



		/* generate ACK if it is configured and send it to package queue */
		if(config.SendAckPerWirelessConn[wlConn])
		{
			tWirelessPackage ackPackage;
			if(generateAckPackage(&package, &ackPackage) == false) /* allocates payload memory block for ackPackage, ToDo: handle malloc fault */
			{
				UTIL1_strcpy(infoBuf, sizeof(infoBuf), "Warning: Could not allocate payload memory for acknowledge\r\n");
				pushMsgToShellQueue(infoBuf);
				numberOfDroppedAcks[wlConn]++;
			}
			if(pushToSentPackagesQueue(wlConn, ackPackage) != pdTRUE) // ToDo: try sending ACK package out on wireless connection configured (just like data package, iterate through priorities) */
			{
				XF1_xsprintf(infoBuf, "Warning: ACK for wireless number %u could not be pushed to queue\r\n", wlConn);
				pushMsgToShellQueue(infoBuf);
				numberOfDroppedPackages[wlConn]++;
				FRTOS_vPortFree(ackPackage.payload); /* free memory since it wont be done on popping from queue */
				UTIL1_strcpy(infoBuf, sizeof(infoBuf), "Warning: Acknowledge cannot be sent because package queue full\r\n");
				pushMsgToShellQueue(infoBuf);
			}
			/* memory of ackPackage is freed after package in PackageHandler task, extracted and byte wise pushed to byte queue */
			numberOfAcksSent[wlConn]++;
		}
	}
	else /* acknowledge package received */
	{
		/* iterate though unacknowledged packages to find the corresponding one */
		for(int index=0; index < MAX_NUMBER_OF_UNACK_PACKS_STORED; index++)
		{
			/* check if this index holds an unacknowledged package */
			if(unacknowledgedPackagesOccupiedAtIndex[index])
			{
				uint32_t sysTime = package.payload[0];
				sysTime |= (package.payload[1] << 8);
				sysTime |= (package.payload[2] << 16);
				sysTime |= (package.payload[3] << 24);
				/* check if this is the package we got the acknowledge for */
				if(		unacknowledgedPackages[index].devNum == package.devNum &&
						unacknowledgedPackages[index].sysTime == package.sysTime   )
				{
					/* free memory of saved package if we got ACK */
					FRTOS_vPortFree(unacknowledgedPackages[index].payload);
					numberOfDroppedAcks[wlConn]++;
					unacknowledgedPackagesOccupiedAtIndex[index] = false;
					numberOfUnacknowledgedPackages--;
					FRTOS_vPortFree(package.payload); /* free memory for package popped from queue */
					return true; /* unacknowledged package found, leave for-loop */
				}
			}
		}
		XF1_xsprintf(infoBuf, "Warning: Got ACK on wireless connection %u but no saved package found -> check ACK configuration on both sides %u\r\n", wlConn);
		pushMsgToShellQueue(infoBuf);
		FRTOS_vPortFree(package.payload);
		return false; /* found no matching data package for this acknowledge */
	}
	FRTOS_vPortFree(package.payload); /* free memory for package popped from queue */
	return true;
}


/*!
* \fn static void pushPayloadOut(tWirelessPackage package)
* \brief Pushes the payload of a wireless package out on the correct device
* \param package: package to be pushed out
*/
static void pushPayloadOut(tWirelessPackage package)
{
	static char infoBuf[80];
	for(uint16_t cnt=0; cnt<package.payloadSize; cnt++)
	{
		if(pushToByteQueue(MAX_14830_DEVICE_SIDE, package.devNum, &package.payload[cnt]) == pdFAIL)
		{
			XF1_xsprintf(infoBuf, "Warning: Push to device byte array for UART %u failed", package.devNum);
			pushMsgToShellQueue(infoBuf);
			numberOfDroppedBytes[package.devNum]++;
		}
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
static bool generateDataPackage(tUartNr deviceNr, tWirelessPackage* pPackage, uint8_t sessionNr)
{
	static uint32_t tickTimeSinceFirstCharReceived[NUMBER_OF_UARTS]; /* static variables are initialized as 0 by default */
	static bool dataWaitingToBeSent[NUMBER_OF_UARTS];
	static uint8_t packHeaderBuf[PACKAGE_HEADER_SIZE - 1] = { PACK_START, PACK_TYPE_DATA_PACKAGE, 0, 0, 0, 0, 0, 0, 0, 0 };
	char infoBuf[50];

	uint16_t numberOfBytesInRxQueue = (uint16_t) numberOfBytesInRxByteQueue(MAX_14830_DEVICE_SIDE, deviceNr);
	uint32_t timeWaitedForPackFull = xTaskGetTickCount()-tickTimeSinceFirstCharReceived[deviceNr];

	/* check if enough data to fill package (when not configured to 0) or maximum wait time for full package done */
	if (((numberOfBytesInRxQueue >= config.UsualPacketSizeDeviceConn[deviceNr]) && (0 != config.UsualPacketSizeDeviceConn[deviceNr])) ||
		(numberOfBytesInRxQueue >= PACKAGE_MAX_PAYLOAD_SIZE) ||
		((dataWaitingToBeSent[deviceNr] == true) && (timeWaitedForPackFull >= pdMS_TO_TICKS(config.PackageGenMaxTimeout[deviceNr]))))
	{
		/* reached usual packet size or timeout, generate package
		 * check if there is enough space to store package in queue before generating it
		 * Dropping of data in case all queues are full is handled in byte queue.
		 * When reading data from HW buf but no space in byte queue, oldest bytes will be popped from queue and dropped.
		 * Hopefully, this will do and no dropping of data on purpose is needed anywhere else for Rx side. */
		uint8_t wlConn = NUMBER_OF_UARTS;
		for(int prio = 1; prio <= NUMBER_OF_UARTS; prio++) /* go thorough all configured priorities to see if any queue has space */
		{
			wlConn = getWlConnectionToUse(deviceNr, prio);
			if(wlConn >= NUMBER_OF_UARTS) /* check if this priority has been configured */
				return false;
			if(uxQueueMessagesWaiting(queuePackagesToSend[wlConn]) < QUEUE_NUM_OF_WL_PACK_TO_SEND) /* space left in the queue? */
				break; /* found a wlConn where there is space to store package */
		}
		if(wlConn >= NUMBER_OF_UARTS) /* there is no queue with space for this package */
			return false;
		//ToDo: if((config.PackNumberingProcessingMode == WAIT_FOR_ACK_BEFORE_SENDING_NEXT_PACK) && (FRTOS_xSemaphoreTake(ackReceived[wlConn], 0) != pdTRUE))
			//ToDo: return false; /* still waiting for an acknowledge on this connection
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
				numberOfDroppedBytes[deviceNr] += cnt;
				FRTOS_vPortFree(pPackage->payload);
				return false;
			}
		}
		/* calculate CRC */
		uint32_t crc16;
		CRC1_ResetCRC(crcNH);
		CRC1_SetCRCStandard(crcNH, LDD_CRC_MODBUS_16); // ToDo: use LDD_CRC_MODBUS_16 afterwards, MODBUS only for backwards compatibility with old SW
		CRC1_GetBlockCRC(crcNH, pPackage->payload, pPackage->payloadSize, &crc16);
		pPackage->crc16payload = (uint16_t) crc16;
		/* put together the rest of the header */
		pPackage->packType = PACK_TYPE_DATA_PACKAGE;
		pPackage->devNum = deviceNr;
		pPackage->sessionNr = sessionNr;
		pPackage->sysTime = ++sentPackNumTracker[deviceNr];
		/* calculate crc */
		CRC1_ResetCRC(crcNH);
		CRC1_GetCRC8(crcNH, PACK_START);
		CRC1_GetCRC8(crcNH, PACK_TYPE_DATA_PACKAGE);
		CRC1_GetCRC8(crcNH, pPackage->devNum);
		CRC1_GetCRC8(crcNH, pPackage->sessionNr);
		CRC1_GetCRC8(crcNH, *((uint8_t*)(&pPackage->sysTime) + 3));
		CRC1_GetCRC8(crcNH, *((uint8_t*)(&pPackage->sysTime) + 2));
		CRC1_GetCRC8(crcNH, *((uint8_t*)(&pPackage->sysTime) + 1));
		CRC1_GetCRC8(crcNH, *((uint8_t*)(&pPackage->sysTime) + 0));
		CRC1_GetCRC8(crcNH, *((uint8_t*)(&pPackage->payloadSize) + 1));
		pPackage->crc8Header = CRC1_GetCRC8(crcNH, *((uint8_t*)(&pPackage->payloadSize) + 0));
		/* reset last timestamp */
		tickTimeSinceFirstCharReceived[deviceNr] = 0;
		/* reset flag that timestamp was updated */
		dataWaitingToBeSent[deviceNr] = false;
		/* set all information about package sending */
		for(int index=0; index < NUMBER_OF_UARTS; index++)
		{
			pPackage->timestampLastSendAttempt[index] = 0;
			pPackage->totalNumberOfSendAttemptsPerWirelessConnection[index] = config.SendCntWirelessConnDev[deviceNr][index];
			pPackage->sendAttemptsLeftPerWirelessConnection[index] = config.SendCntWirelessConnDev[deviceNr][index];
		}
		return true;
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

/*!
* \fn static bool generateAckPackage(tWirelessPackage* pReceivedDataPack, tWirelessPackage* pAckPack)
* \brief Function to generate a receive acknowledge package, reading data from the data source.
* \param pAckPack: Pointer to acknowledge packet to be created
* \param pReceivedDataPack: Pointer to the structure that holds the wireless package that the acknowledge is generated for
* \return true if a package was generated and saved, false otherwise.
*/
static bool generateAckPackage(tWirelessPackage* pReceivedDataPack, tWirelessPackage* pAckPack)
{
	/* buffer in order to be able to save the header for CRC calculation. -1 in length of buffer because we don't need the CRC itself in the buffer */
	/* default header = { PACK_START, PACK_TYPE_REC_ACKNOWLEDGE, 0, 0, 0, 0, 0, 0, 0, 0 } */
	/* prepare wireless package */
	pAckPack->packType = PACK_TYPE_REC_ACKNOWLEDGE;
	pAckPack->devNum = pReceivedDataPack->devNum;
	pAckPack->sessionNr = sessionNr;
	pAckPack->sysTime = xTaskGetTickCount();
	pAckPack->payloadSize = sizeof(uint32_t);	/* as payload, the timestamp of the package to be acknowledged is saved */
	/* calculate crc for header */
	CRC1_ResetCRC(crcNH);
	CRC1_SetCRCStandard(crcNH, LDD_CRC_MODBUS_16); // ToDo: use LDD_CRC_MODBUS_16 afterwards, MODBUS only for backwards compatibility with old SW
	CRC1_GetCRC8(crcNH, PACK_START);
	CRC1_GetCRC8(crcNH, PACK_TYPE_REC_ACKNOWLEDGE);
	CRC1_GetCRC8(crcNH, pAckPack->devNum);
	CRC1_GetCRC8(crcNH, pAckPack->sessionNr);
	CRC1_GetCRC8(crcNH, *((uint8_t*)(&pAckPack->sysTime) + 3));
	CRC1_GetCRC8(crcNH, *((uint8_t*)(&pAckPack->sysTime) + 2));
	CRC1_GetCRC8(crcNH, *((uint8_t*)(&pAckPack->sysTime) + 1));
	CRC1_GetCRC8(crcNH, *((uint8_t*)(&pAckPack->sysTime) + 0));
	CRC1_GetCRC8(crcNH, *((uint8_t*)(&pAckPack->payloadSize) + 1));
	pAckPack->crc8Header = CRC1_GetCRC8(crcNH, *((uint8_t*)(&pAckPack->payloadSize) + 0));
	/* get space for acknowladge payload (which consists of sysTime of datapackage*/
	pAckPack->payload = (uint8_t*) FRTOS_pvPortMalloc(pAckPack->payloadSize*sizeof(int8_t));
	if(pAckPack->payload == NULL) /* malloc failed */
		return false;
	/* generate payload */
	for (uint16_t cnt = 0; cnt < pAckPack->payloadSize; cnt++)
	{
		pAckPack->payload[cnt] = *((uint8_t*)(&pReceivedDataPack->sysTime) + cnt);
	}
	/* generate CRC16 for payload */
	uint32_t crc16;
	CRC1_ResetCRC(crcNH);
	CRC1_GetBlockCRC(crcNH, pAckPack->payload, pAckPack->payloadSize, &crc16);
	pAckPack->crc16payload = (uint16_t) crc16;
	return true;
}

/*!
* \fn static void handleResendingOfUnacknowledgedPackages(void)
* \brief Checks weather a package should be resent because ACK not received.
*/
static void handleResendingOfUnacknowledgedPackages(void)
{
	int unackPackagesLeft = numberOfUnacknowledgedPackages;
	static char infoBuf[128];
	static tWirelessPackage package;
	for (int index = 0; index < MAX_NUMBER_OF_UNACK_PACKS_STORED; index++)
	{
		if(unackPackagesLeft <= 0) /* leave iteration through array when there are no more packages in there */
		{
			return;
		}
		if(unacknowledgedPackagesOccupiedAtIndex[index] == 1) /* there is a package stored at this index */
		{
			unackPackagesLeft --;
			int prio = 1;
			while(prio <= NUMBER_OF_UARTS) /* iterate though all priorities (starting at highest priority) to see if resend is required */
			{
				int wlConn = getWlConnectionToUse(unacknowledgedPackages[index].devNum, prio);
				/* max number of resends done for all connections or maximum delay in config reached for this package */
				if((wlConn >= NUMBER_OF_UARTS) ||
						(unacknowledgedPackages[index].timestampFirstSendAttempt + pdMS_TO_TICKS(config.DelayDismissOldPackagePerDev[unacknowledgedPackages[index].devNum]) < xTaskGetTickCount()) )
				{
					XF1_xsprintf(infoBuf, "Warning: Max number of retries reached and no ACK received -> discard package for device %u\r\n", unacknowledgedPackages[index].devNum);
					pushMsgToShellQueue(infoBuf);
					FRTOS_vPortFree(unacknowledgedPackages[index].payload); /* free allocated memory when package dropped*/
					unacknowledgedPackagesOccupiedAtIndex[index] = 0;
					numberOfUnacknowledgedPackages--;
					prio = NUMBER_OF_UARTS; /* leave iteration over priorities */
					numberOfDroppedPackages[unacknowledgedPackages[index].devNum]++; /* update throughput printout */
				}
				if(unacknowledgedPackages[index].sendAttemptsLeftPerWirelessConnection[wlConn] > 0)
				{
					/* is timeout for ACK done?*/
					if(xTaskGetTickCount() - unacknowledgedPackages[index].timestampLastSendAttempt[wlConn] > pdMS_TO_TICKS(config.ResendDelayWirelessConnDev[wlConn][unacknowledgedPackages[index].devNum]))
					{
						/* create new package for queue because memory is freed once package is pulled from queue */
						package = unacknowledgedPackages[index];
						package.payload = (uint8_t*) FRTOS_pvPortMalloc(package.payloadSize*sizeof(int8_t)); // ToDo: check queue state before allocating memory for package
						if(package.payload == NULL)
						{
							return; /* leave this entire function if malloc fails! */
						}
						/* fill package.payload with payload data */
						for(int cnt = 0; cnt < package.payloadSize; cnt++)
						{
							package.payload[cnt] = unacknowledgedPackages[index].payload[cnt];
						}
						/* send package */
						if(pushToSentPackagesQueue(wlConn, package) == pdTRUE)
						{
							unacknowledgedPackages[index].sendAttemptsLeftPerWirelessConnection[wlConn]--;
							unacknowledgedPackages[index].timestampLastSendAttempt[wlConn] = xTaskGetTickCount();
							//sprintf(infoBuf, "Retry to send package\r\n");
							//pushMsgToShellQueue(infoBuf, strlen(infoBuf));
							prio = NUMBER_OF_UARTS; /* leave priority-while loop because package will be sent, now to wait on acknowledge */
							/* update throughput printout */
							numberOfPayloadBytesSent[wlConn] += unacknowledgedPackages[index].payloadSize;
						}
						else
						{
							numberOfDroppedPackages[wlConn]++;
							FRTOS_vPortFree(package.payload); /* free memory since it wont be popped from queue and freed there */
						}
					}
					else /* package has already been sent, now waiting on acknowledge -> leave loop for this package, find next package to check */
						prio = NUMBER_OF_UARTS; /* leave priority-while loop */
				}
				/* no send attempt left but we are waiting for ACK of very last send attempt on one wireless connection */
				else if((unacknowledgedPackages[index].sendAttemptsLeftPerWirelessConnection[wlConn] == 0) &&  (xTaskGetTickCount() - unacknowledgedPackages[index].timestampLastSendAttempt[wlConn] < pdMS_TO_TICKS(config.ResendDelayWirelessConnDev[wlConn][unacknowledgedPackages[index].devNum])))
				{
					prio = NUMBER_OF_UARTS;
				}
				prio++;
			}
		}
	}
}


/*!
* \fn bool storeNewPackageInUnacknowledgedPackagesArray(tWirelessPackage* pPackage)
* \brief Finds free space in array and stores package in array for unacknowledged packages.
* \param pPackage: Pointer to package that should be stored
* \return true if successful, false if array is full
*/
static bool storeNewPackageInUnacknowledgedPackagesArray(tWirelessPackage* pPackage)
{
	for(int index=0; index < MAX_NUMBER_OF_UNACK_PACKS_STORED; index++)
	{
		if(unacknowledgedPackagesOccupiedAtIndex[index] == 0) /* there is no package stored at this index */
		{
			unacknowledgedPackages[index] = *pPackage;
			unacknowledgedPackages[index].payload = FRTOS_pvPortMalloc(unacknowledgedPackages[index].payloadSize*sizeof(int8_t));
			if(unacknowledgedPackages[index].payload == NULL)
				return false;
			for(int cnt = 0; cnt < pPackage->payloadSize; cnt++)
			{
				unacknowledgedPackages[index].payload[cnt] = pPackage->payload[cnt];
			}
			unacknowledgedPackagesOccupiedAtIndex[index] = 1;
			numberOfUnacknowledgedPackages++;
			return true;
		}
	}
	return false;
}

/*!
* \fn ByseType_t popReadyToSendPackFromQueue(tUartNr uartNr, tWirelessPackage *pPackage)
* \brief Stores a single package from the selected queue in pPackage.
* \param uartNr: UART number the package should be transmitted to.
* \param pPackage: The location where the package should be stored
* \return Status if xQueueReceive has been successful, pdFAIL if uartNr was invalid or pop unsuccessful
*/
BaseType_t popReadyToSendPackFromQueue(tUartNr uartNr, tWirelessPackage* pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueueReceive(queuePackagesToSend[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_NETW_HANDLER_MS) );
	}
	return pdFAIL; /* if uartNr was not in range */
}

/*!
* \fn ByseType_t peekAtNextReadyToSendPack(tUartNr uartNr, tWirelessPackage *pPackage)
* \brief Stores a single package from the selected queue in pPackage. Package will not be deleted from queue!
* \param uartNr: UART number the package should be transmitted to.
* \param pPackage: The location where the package should be stored
* \return Status if xQueuePeek has been successful, pdFAIL if uartNr was invalid or pop unsuccessful
*/
BaseType_t peekAtNextReadyToSendPack(tUartNr uartNr, tWirelessPackage *pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return FRTOS_xQueuePeek(queuePackagesToSend[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_NETW_HANDLER_MS) );
	}
	return pdFAIL; /* if uartNr was not in range */
}


/*!
* \fn uint16_t numberOfPackagesReadyToSend(tUartNr uartNr)
* \brief Returns the number of packages stored in the queue that are ready to be sent via Wireless
* \param uartNr: UART number the package should be transmitted to.
* \return Number of packages waiting to be sent out
*/
uint16_t numberOfPackagesReadyToSend(tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return uxQueueMessagesWaiting(queuePackagesToSend[uartNr]);
	}
	return 0; /* if uartNr was not in range */
}

/*!
* \fn ByseType_t pushToSentPackagesQueue(tUartNr wlConn, tWirelessPackage package)
* \brief Stores the sent package in correct queue.
* \param wlConn: UART number where package was received.
* \param package: The package that was sent
* \return Status if xQueueSendToBack has been successful, pdFAIL if push unsuccessful
*/
BaseType_t pushToSentPackagesQueue(tUartNr wlConn, tWirelessPackage package)
{
	if(xQueueSendToBack(queuePackagesToSend[wlConn], &package, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_NETW_HANDLER_MS) ) == pdTRUE)
	{
		if(config.LoggingEnabled)
		{
			/* generate new package because payload is freed upon queue pull */
			tWirelessPackage tmpPackage = package;
			tmpPackage.payload = (uint8_t*) FRTOS_pvPortMalloc(tmpPackage.payloadSize*sizeof(int8_t));
			if(tmpPackage.payload == NULL) /* malloc failed */
			{
				return pdTRUE; /* because package handling was successful, only logging failure */
			}
			/* copy payload into new package */
			for(int cnt=0; cnt < tmpPackage.payloadSize; cnt++)
			{
				tmpPackage.payload[cnt] = package.payload[cnt];
			}
			pushPackageToLoggerQueue(tmpPackage, SENT_PACKAGE, wlConn);
		}
		return pdTRUE;
	}
	return pdFAIL;
}



