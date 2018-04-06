#include "SpiHandler.h" // pushToQueue and popFromQueue
#include "PackageHandler.h"
#include "NetworkHandler.h"
#include "Config.h"
#include "CRC1.h" // crc_8, crc_16
#include "XF1.h" // xsprintf
#include "Shell.h" // to print out debug information
#include "ThroughputPrintout.h"
#include "LedRed.h"
#include "LedOrange.h"
#include "Logger.h"

#define TASKDELAY_QUEUE_FULL_MS 1


/* global variables, only used in this file */
static xQueueHandle ReceivedPackages[NUMBER_OF_UARTS]; /* Outgoing data to wireless side stored here */
static tWirelessPackage nextDataPacketToSend[NUMBER_OF_UARTS]; /* data buffer of outgoing wireless packages, stored in here once pulled from queue */
static LDD_TDeviceData* crcPH;
static const char* queueName[] = {"ReceivedPackages0", "ReceivedPackages1", "ReceivedPackages2", "ReceivedPackages3"};
uint8_t numOfInvalidRecWirelessPack[NUMBER_OF_UARTS];


/* prototypes */
void initPackageHandlerQueues(void);
void packageHandler_TaskInit(void);
static bool sendPackageToWirelessQueue(tUartNr wlConn, tWirelessPackage* pPackage);
static bool sendNonPackStartCharacter(tUartNr uartNr, uint8_t* pCharToSend);
static void readAndExtractWirelessData(uint8_t wlConn);
static bool checkForPackStartReplacement(uint8_t* ptrToData, uint16_t* dataCntr, uint16_t* patternReplaced);
uint16_t numberOfPacksInReceivedPacksQueue(tUartNr uartNr);
static BaseType_t pushToReceivedPackagesQueue(tUartNr wlConn, tWirelessPackage* pPackage);


/*! \struct sWiReceiveHandlerStates
*  \brief Possible states of the wireless receive handler.
*/
typedef enum eWiReceiveHandlerStates
{
	STATE_START,
	STATE_READ_HEADER,
	STATE_READ_PAYLOAD
} tWiReceiveHandlerStates;


/*!
* \fn void packageHandler_TaskEntry(void)
* \brief Task assembles packages from bytes and puts it on ReceivedPackages queue.
* Generated packages are popped from PackagesToSend queue are sent to byte queue for transmission.
*/
void packageHandler_TaskEntry(void* p)
{
	static tWirelessPackage package;
	const TickType_t taskInterval = pdMS_TO_TICKS(config.PackageHandlerTaskInterval);
	TickType_t xLastWakeTime = xTaskGetTickCount(); /* Initialize the lastWakeTime variable with the current time. */

	for(;;)
	{
		vTaskDelayUntil( &xLastWakeTime, taskInterval ); /* Wait for the next cycle */
		for(int wlConn = 0; wlConn < NUMBER_OF_UARTS; wlConn++)
		{
			/* send packages byte wise to spi queue as long as there is enough space available for a full package */
			for(int i=numberOfPackagesReadyToSend(wlConn); i > 0; i--)
			{
				/* check how much space is needed for next data package */
				if(peekAtNextReadyToSendPack(wlConn, &package) != pdTRUE)
				{
					break; /* leave inner while-loop if queue access unsuccessful and continue with next wlConn */
				}
				/* enough space for next package available? */
				if(freeSpaceInTxByteQueue(MAX_14830_WIRELESS_SIDE, wlConn) > (TOTAL_WL_PACKAGE_SIZE + package.payloadSize))
				{
					if(popReadyToSendPackFromQueue(wlConn, &package) == pdTRUE) /* there is a package ready for sending */
					{
						if(sendPackageToWirelessQueue(wlConn, &package) != true) /* ToDo: handle resending of package */
						{
							/* entire package could not be pushed to queue byte wise, only fraction in queue now */
							numberOfDroppedPackages[wlConn]++;
							FRTOS_vPortFree(package.payload); /* free memory of package before returning from while loop */
							package.payload = NULL;
							break; /* exit while loop, no more packages are extracted for this uartNr */
						}
						else
						{
							FRTOS_vPortFree(package.payload); /* free memory of package once it is sent to device */
							package.payload = NULL;
						}
					}
				}
				else /* not enough space available for next package */
				{
					char infoBuf[50];
					UTIL1_strcpy(infoBuf, sizeof(infoBuf), "Pushing pack to wireless not possible, too little space");
					pushMsgToShellQueue(infoBuf);
					break; /* leave inner while-loop */
				}
			}
			/* assemble received bytes to form a full data package */
			if(numberOfPacksInReceivedPacksQueue(wlConn) < QUEUE_NUM_OF_WL_PACK_RECEIVED) /* there is space available in Queue */
			{
				if(numberOfBytesInRxByteQueue(MAX_14830_WIRELESS_SIDE, wlConn) > 0) /* there are characters waiting */
				{
					readAndExtractWirelessData(wlConn);
				}
			}
		}
	}
}

/*!
* \fn void packageHandler_TaskInit(void)
* \brief Initializes queues created by package handler and HW CRC generator
*/
void packageHandler_TaskInit(void)
{
	initPackageHandlerQueues();
	crcPH = CRC1_Init(NULL);
}

/*!
* \fn void initPackageHandlerQueues(void)
* \brief This function initializes the array of queues
*/
void initPackageHandlerQueues(void)
{
#if configSUPPORT_STATIC_ALLOCATION
	static uint8_t xStaticQueue[NUMBER_OF_UARTS][ QUEUE_NUM_OF_WL_PACK_RECEIVED * sizeof(tWirelessPackage) ]; /* The variable used to hold the queue's data structure. */
	static StaticQueue_t ucQueueStorage[NUMBER_OF_UARTS]; /* The array to use as the queue's storage area. */
#endif
	for(int uartNr = 0; uartNr<NUMBER_OF_UARTS; uartNr++)
	{
#if configSUPPORT_STATIC_ALLOCATION
		ReceivedPackages[uartNr] = xQueueCreateStatic( QUEUE_NUM_OF_WL_PACK_RECEIVED, sizeof(tWirelessPackage), xStaticQueue[uartNr], &ucQueueStorage[uartNr]);
#else
		ReceivedPackages[uartNr] = xQueueCreate( QUEUE_NUM_OF_WL_PACK_RECEIVED, sizeof(tWirelessPackage));
#endif
		if(ReceivedPackages[uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(ReceivedPackages[uartNr], queueName[uartNr]);
	}
}



/*!
* \fn static bool sendPackageToWirelessQueue(Queue* pToWirelessQueue, tWirelessPackage* pToWirelessPackage)
* \brief Function to send the desired package to the desired queue character for character.
* \param pToWirelessQueue: Queue that should be used to send the characters.
* \param pPackage: Pointer to wireless package that needs to be sent.
* \ret true if successful, false otherwise.
*/
static bool sendPackageToWirelessQueue(tUartNr wlConn, tWirelessPackage* pPackage)
{
	static char infoBuf[100];
	if ((wlConn > NUMBER_OF_UARTS) || (pPackage == NULL) || (pPackage->payloadSize > PACKAGE_MAX_PAYLOAD_SIZE))
	{
		XF1_xsprintf(infoBuf, "Error: Implementation error occurred on pushing package byte wise to wireless %u queue\r\n", wlConn);
		LedRed_On();
		pushMsgToShellQueue(infoBuf);
		return false;
	}
	static uint8_t startChar = PACK_START;

	taskENTER_CRITICAL();
	if(pushToByteQueue(MAX_14830_WIRELESS_SIDE, wlConn, &startChar) != pdTRUE)
	{
		numberOfDroppedPackages[wlConn]++;
		taskEXIT_CRITICAL();
		return false;
	}
	if(sendNonPackStartCharacter(wlConn, &pPackage->packType))
		if(sendNonPackStartCharacter(wlConn, &pPackage->devNum))
			if(sendNonPackStartCharacter(wlConn, &pPackage->sessionNr))
				if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->sysTime) + 3))
					if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->sysTime) + 2))
						if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->sysTime) + 1))
							if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->sysTime) + 0))
								if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->payloadSize) + 1))
									if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->payloadSize) + 0))
										if(sendNonPackStartCharacter(wlConn, &pPackage->crc8Header))
										{
											for (uint16_t cnt = 0; cnt < pPackage->payloadSize; cnt++)
											{
												sendNonPackStartCharacter(wlConn, &pPackage->payload[cnt]);
											}
											if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->crc16payload) + 1))
												if(sendNonPackStartCharacter(wlConn, (uint8_t*)(&pPackage->crc16payload) + 0))
												{
													/* also send two fill bytes at the end - just that we're able to make sure on the receive side that we got all replacements */
													/* TODO this is "dirty" workaround that leads to more traffic, to be replaced with better solution (but in this case also the wireless package extractor needs to be adjusted) */
													static uint8_t repChar = PACK_FILL;
													if(sendNonPackStartCharacter(wlConn, &repChar))
														if(sendNonPackStartCharacter(wlConn, &repChar))
														{
															taskEXIT_CRITICAL();
															return true;
														}
												}
										}
	taskEXIT_CRITICAL();
	return false;
}



/*!
* \fn static inline void sendNonPackStartCharacter(tUartNr uartNr, uint8_t charToSend)
* \brief Replaces all outgoing PACK_START characters in the stream by "PACK_START" to be able to distinguish them at the receive side.
* \param uartNr: Wireless connection number, where package/char should be sent out.
* \param pCharToSend: Pointer to the char that needs to be sent.
*/
static bool sendNonPackStartCharacter(tUartNr uartNr, uint8_t* pCharToSend)
{
	static char infoBuf[100];
	if ((*pCharToSend) == PACK_START)
	{
		static uint8_t repChar = PACK_REP;
		if(pushToByteQueue(MAX_14830_WIRELESS_SIDE, uartNr, &repChar) == errQUEUE_FULL)
		{
			numberOfDroppedPackages[uartNr]++;
			return false;
		}
		if(pushToByteQueue(MAX_14830_WIRELESS_SIDE, uartNr, pCharToSend) == errQUEUE_FULL)
		{
			numberOfDroppedPackages[uartNr]++;
			return false;
		}
		if(pushToByteQueue(MAX_14830_WIRELESS_SIDE, uartNr, &repChar) == errQUEUE_FULL)
		{
			numberOfDroppedPackages[uartNr]++;
			return false;
		}
	}
	else
	{
		if(pushToByteQueue(MAX_14830_WIRELESS_SIDE, uartNr, pCharToSend) == errQUEUE_FULL)
		{
			numberOfDroppedPackages[uartNr]++;
			return false;
		}
	}
	return true;
}



/*!
* \fn static void readAndExtractWirelessData(uint8_t wirelessConnNr)
* \brief Function that reads the incoming data from the queue and generates receive acknowledges from it plus sends the valid data to the corresponding devices.
* \param wirelessConnNr: Serial connection of wireless interface. Needs to be between 0 and NUMBER_OF_UARTS.
*/
static void readAndExtractWirelessData(uint8_t wlConn)
{
	/* Some static variables in order to save status and data for every wireless connection */
	static uint8_t data[NUMBER_OF_UARTS][PACKAGE_MAX_PAYLOAD_SIZE + sizeof(uint16_t)]; /* space for payload plus crc16 */
	static uint16_t dataCntr[NUMBER_OF_UARTS];
	static tWirelessPackage currentWirelessPackage[NUMBER_OF_UARTS];
	static tWiReceiveHandlerStates currentRecHandlerState[NUMBER_OF_UARTS];
	static uint32_t timestampLastValidPackage[NUMBER_OF_UARTS];
	static uint8_t sessionNumberLastValidPackage[NUMBER_OF_UARTS];
	static uint16_t patternReplaced[NUMBER_OF_UARTS];
	static uint16_t dataCntToAddAfterReadPayload[NUMBER_OF_UARTS];
	uint8_t chr;
	static char infoBuf[128];

	/* check if parameters are valid */
	if (wlConn >= NUMBER_OF_UARTS)
	{
		//showError(__FUNCTION__, "invalid parameter");
	}
	/* read incoming character and react based on the state of the state machine */
	while (popFromByteQueue(MAX_14830_WIRELESS_SIDE, wlConn, &chr))
	{
		switch (currentRecHandlerState[wlConn])
		{
		case STATE_START:
			/* when the state before was the one to read a payload, there is still something in the buffer that needs to be checked */
			while (dataCntr[wlConn] > 0)
			{
				dataCntr[wlConn]--;
				if (data[wlConn][dataCntr[wlConn]] == PACK_START)
				{
					currentRecHandlerState[wlConn] = STATE_READ_HEADER;
					patternReplaced[wlConn] = 0;
					/* check if there is still something left in the buffer to use */
					if ((dataCntToAddAfterReadPayload[wlConn] >= 2) && (dataCntr[wlConn] == 0))
					{
						/* in this case, the first char is already in the buffer -> put it to the first place */
						data[wlConn][0] = data[wlConn][1];
						dataCntr[wlConn] = 1;
						if (checkForPackStartReplacement(&data[wlConn][0], &dataCntr[wlConn], &patternReplaced[wlConn]))
						{
							dataCntr[wlConn] = 0;
							numberOfInvalidPackages[wlConn]++;
							XF1_xsprintf(infoBuf, "Info: Restart state machine in STATE_START 0, start of package detected\r\n");
							pushMsgToShellQueue(infoBuf);
						}
					}
					else
					{
						dataCntr[wlConn] = 0;
					}
					data[wlConn][dataCntr[wlConn]++] = chr;
					if (checkForPackStartReplacement(&data[wlConn][0], &dataCntr[wlConn], &patternReplaced[wlConn]))
					{
						dataCntr[wlConn] = 0;
						numberOfInvalidPackages[wlConn]++;
						XF1_xsprintf(infoBuf, "Info: Restart state machine in STATE_START 1, start of package detected\r\n");
						pushMsgToShellQueue(infoBuf);
					}
					break;
				}
			}
			if (chr == PACK_START)
			{
				currentRecHandlerState[wlConn] = STATE_READ_HEADER;
				dataCntr[wlConn] = 0;
				patternReplaced[wlConn] = 0;
				break;
			}
			break;
		case STATE_READ_HEADER:
			data[wlConn][dataCntr[wlConn]++] = chr;
			if (checkForPackStartReplacement(&data[wlConn][0], &dataCntr[wlConn], &patternReplaced[wlConn]))
			{
				/* found start of package, restart reading header */
				dataCntr[wlConn] = 0;
				numberOfInvalidPackages[wlConn]++;
				XF1_xsprintf(infoBuf, "Info: Restart state machine in STATE_READ_HEADER 0, start of package detected\r\n");
				pushMsgToShellQueue(infoBuf);
			}
			if (dataCntr[wlConn] >= (PACKAGE_HEADER_SIZE - 1 + 2)) /* -1: without PACK_START; +2 to read the first 2 bytes from the payload to check if the replacement pattern is there */
			{
				/* assign header structure. Due to alignement, it's hard to do this directly */
				currentWirelessPackage[wlConn].packType = data[wlConn][0];
				currentWirelessPackage[wlConn].devNum = data[wlConn][1];
				currentWirelessPackage[wlConn].sessionNr = data[wlConn][2];
				currentWirelessPackage[wlConn].sysTime = data[wlConn][6];
				currentWirelessPackage[wlConn].sysTime |= (data[wlConn][5] << 8);
				currentWirelessPackage[wlConn].sysTime |= (data[wlConn][4] << 16);
				currentWirelessPackage[wlConn].sysTime |= (data[wlConn][3] << 24);
				currentWirelessPackage[wlConn].payloadSize = data[wlConn][8];
				currentWirelessPackage[wlConn].payloadSize |= (data[wlConn][7] << 8);
				currentWirelessPackage[wlConn].crc8Header = data[wlConn][9];
				/* the first two bytes from the payload were already read, copy them to the beginning of the buffer */
				dataCntr[wlConn] = 2;
				data[wlConn][0] = data[wlConn][10];
				data[wlConn][1] = data[wlConn][11];
				patternReplaced[wlConn] = 0;
				if (checkForPackStartReplacement(&data[wlConn][0], &dataCntr[wlConn], &patternReplaced[wlConn]) == true)
				{
					/* start of package detected, restart reading header */
					dataCntr[wlConn] = 0;
					currentRecHandlerState[wlConn] = STATE_READ_HEADER;
					numberOfInvalidPackages[wlConn]++;
					XF1_xsprintf(infoBuf, "Info: Restart state machine in STATE_READ_HEADER 1, start of package detected\r\n");
					pushMsgToShellQueue(infoBuf);
					break;
				}
				/* finish reading header. Check if header is valid */
				CRC1_ResetCRC(crcPH);
				CRC1_GetCRC8(crcPH, PACK_START);
				CRC1_GetCRC8(crcPH, currentWirelessPackage[wlConn].packType);
				CRC1_GetCRC8(crcPH, currentWirelessPackage[wlConn].devNum);
				CRC1_GetCRC8(crcPH, currentWirelessPackage[wlConn].sessionNr);
				CRC1_GetCRC8(crcPH, *((uint8_t*)(&currentWirelessPackage[wlConn].sysTime) + 3));
				CRC1_GetCRC8(crcPH, *((uint8_t*)(&currentWirelessPackage[wlConn].sysTime) + 2));
				CRC1_GetCRC8(crcPH, *((uint8_t*)(&currentWirelessPackage[wlConn].sysTime) + 1));
				CRC1_GetCRC8(crcPH, *((uint8_t*)(&currentWirelessPackage[wlConn].sysTime) + 0));
				CRC1_GetCRC8(crcPH, *((uint8_t*)(&currentWirelessPackage[wlConn].payloadSize) + 1));
				uint8_t crc8 = CRC1_GetCRC8(crcPH, *((uint8_t*)(&currentWirelessPackage[wlConn].payloadSize) + 0));
				if(true)//currentWirelessPackage[wlConn].crc8Header == crc8)
				{
					if(currentWirelessPackage[wlConn].crc8Header != crc8) /* in case the above crc check is commented out -> debug info printed that crc wouldnt be correct */
					{
						XF1_xsprintf(infoBuf, "Info: Invalid header CRC received, but continuing anyway (debug)\r\n");
						pushMsgToShellQueue(infoBuf);
					}
					/* CRC is valid - also check if the header parameters are within the valid range */
					if ((currentWirelessPackage[wlConn].packType > PACK_TYPE_REC_ACKNOWLEDGE) ||
						(currentWirelessPackage[wlConn].packType == 0) ||
						(currentWirelessPackage[wlConn].devNum >= NUMBER_OF_UARTS) ||
						(currentWirelessPackage[wlConn].payloadSize > PACKAGE_MAX_PAYLOAD_SIZE))
					{
						/* at least one of the parameters is out of range..reset state machine */
						XF1_xsprintf(infoBuf, "invalid header, but CRC8 was right - implementation error?\r\n");
						pushMsgToShellQueue(infoBuf);
						numberOfInvalidPackages[wlConn]++;
						currentRecHandlerState[wlConn] = STATE_START;
						dataCntr[wlConn] = 0;
					}
					else
					{
						/* valid header. Start reading payload */
						currentRecHandlerState[wlConn] = STATE_READ_PAYLOAD;
					}
				}
				else
				{
					/* invalid header, reset state machine */
					currentRecHandlerState[wlConn] = STATE_START;
					numOfInvalidRecWirelessPack[wlConn]++;
					patternReplaced[wlConn] = 0;
					dataCntr[wlConn] = 0;
					numberOfInvalidPackages[wlConn]++;
					XF1_xsprintf(infoBuf, "Info: Invalid header CRC received, reset state machine\r\n");
					pushMsgToShellQueue(infoBuf);
				}
			}
			break;
		case STATE_READ_PAYLOAD:
			data[wlConn][dataCntr[wlConn]++] = chr;
			if (checkForPackStartReplacement(&data[wlConn][0], &dataCntr[wlConn], &patternReplaced[wlConn]) == true)
			{
				/* start of package detected, restart reading header */
				dataCntr[wlConn] = 0;
				currentRecHandlerState[wlConn] = STATE_READ_HEADER;
				numberOfInvalidPackages[wlConn]++;
				XF1_xsprintf(infoBuf, "Info: Restart state machine in STATE_READ_PAYLOAD 0, start of package detected\r\n");
				pushMsgToShellQueue(infoBuf);
				break;
			}
			/* read payload plus crc */
			if (dataCntr[wlConn] >= (currentWirelessPackage[wlConn].payloadSize + sizeof(currentWirelessPackage[wlConn].crc16payload + 2))) /* +2 because we don't want to miss a replacement pattern at the end */
			{
				uint32_t crc16;
				/* to read the two additional chars after reading the payload */
				dataCntToAddAfterReadPayload[wlConn] = 2;
				dataCntr[wlConn] -= dataCntToAddAfterReadPayload[wlConn];
				/* finish reading payload, check CRC of payload */
				currentWirelessPackage[wlConn].crc16payload = data[wlConn][dataCntr[wlConn] - 1];
				currentWirelessPackage[wlConn].crc16payload |= (data[wlConn][dataCntr[wlConn] - 2] << 8);
				CRC1_ResetCRC(crcPH);
				CRC1_SetCRCStandard(crcPH, LDD_CRC_MODBUS_16); // ToDo: use LDD_CRC_CCITT, MODBUS only for backwards compatibility to old SW
				CRC1_GetBlockCRC(crcPH, data[wlConn], currentWirelessPackage[wlConn].payloadSize, &crc16);
				if(true)//currentWirelessPackage[wlConn].crc16payload == (uint16_t) crc16) /* payload valid? */
				{
					if(currentWirelessPackage[wlConn].crc16payload != (uint16_t) crc16) /* in case the above crc check is commented out -> debug info printed that crc wouldnt be correct */
					{
						XF1_xsprintf(infoBuf, "Info: Invalid payload CRC received, but continuing anyway (debug)\r\n");
						pushMsgToShellQueue(infoBuf);
					}
					/*allocate memory for payload of package and set payload */
					if(currentWirelessPackage[wlConn].payloadSize > 0)
					{
						currentWirelessPackage[wlConn].payload = (uint8_t*) FRTOS_pvPortMalloc(currentWirelessPackage[wlConn].payloadSize*sizeof(int8_t)); /* as payload, the timestamp of the package to be acknowledged is saved */
						if(currentWirelessPackage[wlConn].payload != NULL) /* malloc successful */
						{
							for(int cnt=0; cnt < currentWirelessPackage[wlConn].payloadSize; cnt++) /* set payload of package */
							{
								currentWirelessPackage[wlConn].payload[cnt] = data[wlConn][cnt];
							}
							/* check packet type */
							if (currentWirelessPackage[wlConn].packType == PACK_TYPE_REC_ACKNOWLEDGE)
							{
								/* received acknowledge - send message to queue */
								currentWirelessPackage[wlConn].sysTime = *((uint32_t*)&data[wlConn][dataCntr[wlConn] - 6]);
								numberOfAckReceived[wlConn]++;
								currentWirelessPackage[wlConn].timestampPackageReceived = xTaskGetTickCount();

								if(pushToReceivedPackagesQueue(wlConn, &currentWirelessPackage[wlConn]) != pdTRUE) /* ToDo: handle failure on pushing package to receivedPackages queue , currently it is dropped if unsuccessful */
								{
									vPortFree(currentWirelessPackage[wlConn].payload); /* free payload since it wont be done upon queue pop */
									currentWirelessPackage[wlConn].payload = NULL;
									numberOfDroppedAcks[wlConn]++;
									XF1_xsprintf(infoBuf, "Error: Received acknowledge but unable to push this message to the send handler for wireless queue %u because queue full\r\n", (unsigned int) wlConn);
									LedRed_On();
									pushMsgToShellQueue(infoBuf);
								}
							}
							else if (currentWirelessPackage[wlConn].packType == PACK_TYPE_DATA_PACKAGE)
							{
								/* update throughput printout */
								numberOfPacksReceived[wlConn]++;
								numberOfPayloadBytesExtracted[wlConn] += currentWirelessPackage[wlConn].payloadSize;
								/* received data package - send data to corresponding devices plus inform package generator to prepare a receive acknowledge */
								if(pushToReceivedPackagesQueue(wlConn, &currentWirelessPackage[wlConn]) != pdTRUE) /* ToDo: handle queue full, now package is discarded */
								{
									/* queue full */
									vPortFree(currentWirelessPackage[wlConn].payload); /* free payload since it wont be done upon queue pop */
									currentWirelessPackage[wlConn].payload = NULL;
									numberOfDroppedPackages[wlConn]++;
									XF1_xsprintf(infoBuf, "Error: Received data package but unable to push this message to the send handler for wireless queue %u because queue full\r\n", (unsigned int) wlConn);
									LedRed_On();
									pushMsgToShellQueue(infoBuf);
								}
								/* check if it's the same session number as before */
								if (sessionNumberLastValidPackage[currentWirelessPackage[wlConn].devNum] != currentWirelessPackage[wlConn].sessionNr)
								{
									/* session number changed. Reset timestamp and assign new session number. */
									sessionNumberLastValidPackage[currentWirelessPackage[wlConn].devNum] = currentWirelessPackage[wlConn].sessionNr;
									timestampLastValidPackage[currentWirelessPackage[wlConn].devNum] = 0;
								}

							}
							else
							{
								/* something went wrong - invalid package type. Reset state machine and send out error. */
								numberOfInvalidPackages[wlConn]++;
								XF1_xsprintf(infoBuf, "Error: Invalid package type! There is probably an error in the implementation\r\n");
								LedRed_On();
								pushMsgToShellQueue(infoBuf);
							}
						}
						else /* malloc failed */
						{
							/* malloc failed */
							numberOfInvalidPackages[wlConn]++;
							XF1_xsprintf(infoBuf, "Error: Malloc failed, could not push package to received packages queue\r\n");
							LedRed_On();
							pushMsgToShellQueue(infoBuf);
						}
					}
					else
					{
							numberOfInvalidPackages[wlConn]++;
							XF1_xsprintf(infoBuf, "Error: payloadSize == 0 -> reset state machine\r\n");
							LedRed_On();
							pushMsgToShellQueue(infoBuf);
					}
				}
				else
				{
					/* received invalid payload */
					numOfInvalidRecWirelessPack[wlConn]++;
					numberOfInvalidPackages[wlConn]++;
					XF1_xsprintf(infoBuf, "Info: Received %u invalid payload CRC, reset state machine", (unsigned int) numOfInvalidRecWirelessPack[wlConn]);
					pushMsgToShellQueue(infoBuf);
				}
				/* reset state machine */
				currentRecHandlerState[wlConn] = STATE_START;
				/* copy the two already replaced bytes to the beginning if there was read more data due to check for replacement pattern */
				if (dataCntToAddAfterReadPayload[wlConn] >= 2)
				{
					data[wlConn][0] = data[wlConn][dataCntr[wlConn] - 2 + dataCntToAddAfterReadPayload[wlConn]];
					data[wlConn][1] = data[wlConn][dataCntr[wlConn] - 1 + dataCntToAddAfterReadPayload[wlConn]];
				}
				else if (dataCntToAddAfterReadPayload[wlConn] >= 1)
				{
					data[wlConn][0] = data[wlConn][dataCntr[wlConn] - 1 + dataCntToAddAfterReadPayload[wlConn]];
				}
				dataCntr[wlConn] = dataCntToAddAfterReadPayload[wlConn];
				patternReplaced[wlConn] = 0;
			}
			break;
		default:
			XF1_xsprintf(infoBuf, "Error: Invalid state in state machine\r\n");
			LedRed_On();
			numberOfInvalidPackages[wlConn]++;
			pushMsgToShellQueue(infoBuf);
			break;
		}
	}
}


/*!
* \fn static bool checkForPackStartReplacement(uint8_t* ptrToData, uint16_t* dataCntr, uint16_t* patternReplaced)
* \brief Checks for the PACK_START replacement in the data stream plus the start of a package itself.
*		(PACK_START is replaced by PACK_REP PACK_START PACK_REP in the header plus the data in order to be able to distinguish between the data and the start of a package.)
* \param[in,out] ptrToData: Pointer to the data buffer.
* \param[in,out] dataCntr: Pointer to the data counter.
* \param[in,out] patternReplaced: Pointer to number where the pattern was already replaced - to make sure we don't do it more than once at the same position.
* \return If true is returned, a start of a package has been detected and the state machine should restart to read the header.
*/
static bool checkForPackStartReplacement(uint8_t* ptrToData, uint16_t* dataCntr, uint16_t* patternReplaced)
{
	if ((*dataCntr) >= 3)
	{
		if ((ptrToData[(*dataCntr) - 3] == PACK_REP) && (ptrToData[(*dataCntr) - 2] == PACK_START) && (ptrToData[(*dataCntr) - 1] == PACK_REP))
		{
			/* found pattern, replace it if it wasn't already replaced */
			if (((*dataCntr) - 2) != *patternReplaced)
			{
				ptrToData[(*dataCntr) - 3] = PACK_START;
				*patternReplaced = (*dataCntr) - 3;
				(*dataCntr) -= 2;
			}
		}
		else if((ptrToData[(*dataCntr) - 2] != PACK_REP) && (ptrToData[(*dataCntr) - 1] == PACK_START))
		{
			/* found start of package */
			return true;
		}
	}
	else if ((*dataCntr) >= 2)
	{
		if ((ptrToData[(*dataCntr) - 2] != PACK_REP) && (ptrToData[(*dataCntr) - 1] == PACK_START))
		{
			/* found start of package */
			return true;
		}
	}
	else if ((*dataCntr) >= 1)
	{
		/* only one character in the buffer - if it's already the PACK_START symbol, start again */
		if (ptrToData[(*dataCntr) - 1] == PACK_START)
		{
			/* found start of package */
			return true;
		}
	}
	return false;
}

/*!
* \fn ByseType_t pushToReceivedPackagesQueue(tUartNr wlConn, tWirelessPackage* package)
* \brief Stores the received package in correct queue.
* \param wlConn: UART number where package was received.
* \param package: The package that was received
* \return Status if xQueueSendToBack has been successful, pdFAIL if push unsuccessful
*/
static BaseType_t pushToReceivedPackagesQueue(tUartNr wlConn, tWirelessPackage* pPackage)
{
	if(xQueueSendToBack(ReceivedPackages[wlConn], pPackage, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_PACK_HANDLER_MS) ) == pdTRUE) /* ToDo: handle failure on pushing package to receivedPackages queue , currently it is dropped if unsuccessful */
	{
		if(config.LoggingEnabled)
		{
			pushPackageToLoggerQueue(pPackage, RECEIVED_PACKAGE, wlConn); /* content is only copied in this function, new package generated for logging queue inside this function */
		}
		return pdTRUE;
	}
	return pdFAIL;  /* dont do logging if package wont be sent either */
}


/*!
* \fn ByseType_t popReceivedPackFromQueue(tUartNr uartNr, tWirelessPackage *pPackage)
* \brief Stores a single package from the selected queue in pPackage.
* \param uartNr: UART number the package should be transmitted to.
* \param pPackage: The location where the package should be stored
* \return Status if xQueueReceive has been successful, pdFAIL if uartNr was invalid or pop unsuccessful
*/
BaseType_t popReceivedPackFromQueue(tUartNr uartNr, tWirelessPackage *pPackage)
{
	if(uartNr < NUMBER_OF_UARTS)
	{
		return xQueueReceive(ReceivedPackages[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_PACK_HANDLER_MS) );
	}
	return pdFAIL; /* if uartNr was not in range */
}

/*!
* \fn ByseType_t peekAtReceivedPackQueue(tUartNr uartNr, tWirelessPackage *pPackage)
* \brief Package that will be popped next is stored in pPackage but not removed from queue.
* \param uartNr: UART number the package should be transmitted to.
* \param pPackage: The location where the package should be stored
* \return Status if xQueuePeek has been successful, pdFAIL if uartNr was invalid or pop unsuccessful
*/
BaseType_t peekAtReceivedPackQueue(tUartNr uartNr, tWirelessPackage *pPackage)
{
	if( (uartNr < NUMBER_OF_UARTS) && (uxQueueMessagesWaiting(ReceivedPackages[uartNr]) > 0) )
	{
		return FRTOS_xQueuePeek(ReceivedPackages[uartNr], pPackage, ( TickType_t ) pdMS_TO_TICKS(MAX_DELAY_PACK_HANDLER_MS) );
	}
	return pdFAIL; /* if uartNr was not in range */
}


/*!
* \fn uint16_t numberOfPacksInReceivedPacksQueue(tUartNr uartNr)
* \brief Returns the number of packages stored in the queue that are ready to be received/processed by this program
* \param uartNr: UART number the packages should be read from.
* \return Number of packages waiting to be processed/received
*/
uint16_t numberOfPacksInReceivedPacksQueue(tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
		return  (uint16_t) uxQueueMessagesWaiting(ReceivedPackages[uartNr]);
	return 0; /* if uartNr was not in range */
}


