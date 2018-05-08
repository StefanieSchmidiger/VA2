#include <Config.h>
#include <FreeRTOS.h>
#include <NetworkHandler.h>
#include <portmacro.h>
#include <projdefs.h>
#include <PE_Types.h>
#include <queue.h>
#include <stdio.h> // modulo
#include <stdbool.h>
#include <stdint.h>
#include "UTIL1.h" // strcat
#include <string.h> // strlen
#include <Shell.h> // to print out debugInfo
#include <task.h>
#include <ThroughputPrintout.h>
#include "Logger.h"
#include "LedRed.h"
#include "ApplicationHandler.h"
#include "RNG.h"


/* global variables, only used in this file */
static tWirelessPackage unacknowledgedPackages[MAX_NUMBER_OF_UNACK_PACKS_STORED];
static bool unacknowledgedPackagesOccupiedAtIndex[MAX_NUMBER_OF_UNACK_PACKS_STORED];
static int numberOfUnacknowledgedPackages;
static uint16_t sentPackNumTracker[NUMBER_OF_UARTS];
static uint32_t sentAckNumTracker[NUMBER_OF_UARTS];
static volatile bool ackReceived[NUMBER_OF_UARTS];
static uint8_t costFunctionPerWlConn[NUMBER_OF_UARTS];

/* prototypes of local functions */
static void initNetworkHandlerQueues(void);
static void initSempahores(void);
static bool processAssembledPackage(tUartNr wlConn);
static bool sendAndStoreGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr rawDataUartNr);
static bool storeNewPackageInUnacknowledgedPackagesArray(tWirelessPackage* pPackage);
static uint8_t getWlConnConfiguredForPrio(tUartNr uartNr, uint8_t desiredPrio);
static bool generateAckPackage(tWirelessPackage* pReceivedDataPack, tWirelessPackage* pAckPack);
static void handleResendingOfUnacknowledgedPackages(void);
static uint8_t findWlConnForDevice(tUartNr deviceNr, int prio);


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
			/* push generated wireless packages out on wireless side */
			if( nofReadyToSendPackInQueue(deviceNr) > 0)
			{
				/* find wl connection to use for this package */
				uint8_t wlConnToUse = NUMBER_OF_UARTS;
				for(int prio=1; prio < NUMBER_OF_UARTS; prio++)
				{
					wlConnToUse = findWlConnForDevice(deviceNr, prio); /* find wl conn with highest priority that can be used */
					if(wlConnToUse < NUMBER_OF_UARTS) /* found a valid match */
					{
						break;
					}
				}
				if(wlConnToUse < NUMBER_OF_UARTS) /* founda wl connection for this package */
				{
					if( freeSpaceInPackagesToDisassembleQueue(wlConnToUse) ) /* There is space in the queue of next handler? */
					{
						if( ! (config.SyncMessagingModeEnabledPerWlConn[wlConnToUse] && (ackReceived[wlConnToUse] == false)) ) /* sync mode not enabled or last ack received */
						{
							if(popFromReadyToSendPackQueue(deviceNr, &package) == pdTRUE) /* popping package from upper handler successful? */
							{
								sendAndStoreGeneratedWlPackage(&package, wlConnToUse); /* send the generated package down and store it internally if ACK is configured */
							}
						}
					}
				}
			}

			/* extract data from received packages, send ACK and send raw data to corresponding UART interface */
			if(nofAssembledPacksInQueue(deviceNr) > 0)
			{
				processAssembledPackage(deviceNr);
			}

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
}



/*!
* \fn void initNetworkHandlerQueues(void)
* \brief This function initializes the array of queues
*/
static void initNetworkHandlerQueues(void)
{
}

/*!
* \fn static void initSempahores(void)
* \brief This function initializes the array of semaphores
*/
static void initSempahores(void)
{
	for(int uartNr=0; uartNr<NUMBER_OF_UARTS; uartNr++)
	{
		ackReceived[uartNr] = true; /* sending package = taking semaphore, receiving ack = giving it back */
	}
}


/*!
* \fn static bool sendAndStoreGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr wlConn)
* \brief Sends the generated package to package handler for sending and stores it in internal buffer if ACK is expected according to config file
* \param pPackage: pointer to package that should be sent to out
* \param wlConn: wireless connection where package should be sent out on this first try
* \return true if a package was sent to package handler and stored in buffer successfully
*/
static bool sendAndStoreGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr wlConn)
{
	char infoBuf[70];

	if(pushToPacksToDisassembleQueue(wlConn, pPackage) != pdTRUE)
	{
		XF1_xsprintf(infoBuf, "%u: Warning: Couldn't push newly generated package from device %u to package queue on wl conn %u \r\n", xTaskGetTickCount(), pPackage->devNum, wlConn);
		pushMsgToShellQueue(infoBuf);
		vPortFree(pPackage->payload); /* free package payload here since it wont be done upon pulling from queue */
		pPackage->payload = NULL;
		numberOfDroppedPackages[wlConn]++;
	}
	/* update throughput printout */
	numberOfPacksSent[wlConn]++;
	numberOfPayloadBytesSent[wlConn] += pPackage->payloadSize;
	/* store generated package in internal array if acknowledge is expected from this WL connection */
	if(config.SendAckPerWirelessConn[wlConn])
	{
		/* set all information about package (re)sending */
		for(int index=0; index < NUMBER_OF_UARTS; index++)
		{
			pPackage->timestampLastSendAttempt[index] = 0;
			pPackage->totalNumberOfSendAttemptsPerWirelessConnection[index] = config.SendCntWirelessConnDev[pPackage->devNum][index];
			pPackage->sendAttemptsLeftPerWirelessConnection[index] = config.SendCntWirelessConnDev[pPackage->devNum][index];
		}
		pPackage->sendAttemptsLeftPerWirelessConnection[wlConn]--;
		pPackage->timestampLastSendAttempt[wlConn] = xTaskGetTickCount();
		pPackage->timestampFirstSendAttempt = xTaskGetTickCount();
		pPackage->wlConnUsedForLastSendAttempt = wlConn;
		ackReceived[wlConn] = false; /* flag for PackNumberProcessingMode == WAIT_FOR_ACK_BEFORE_SENDING_NEXT_PACK */
		if(storeNewPackageInUnacknowledgedPackagesArray(pPackage) == true)
		{
			return true; /* success */
		}
	}
	else /* no ack expected */
	{
		return true; /* success */
	}
	return false; /* package couldnt be stored in unacknowledgedPackagesArray */
}

/*!
* \fn static uint8_t getWlConnConfiguredForPrio(tUartNr uartNr, uint8_t desiredPrio)
* \brief Checks which wireless connection number is configured with the desired priority
* \return wlConnectionToUse: a number between 0 and (NUMBER_OF_UARTS-1). This priority is not configured if NUMBER_OF_UARTS is returned.
*/
static uint8_t getWlConnConfiguredForPrio(tUartNr uartNr, uint8_t desiredPrio)
{
	uint8_t wlConnectionToUse = 0;
	while ( wlConnectionToUse < NUMBER_OF_UARTS && config.PrioWirelessConnDev[uartNr][wlConnectionToUse] != desiredPrio ) ++wlConnectionToUse;
	return wlConnectionToUse;
}


/*!
* \fn static uint8_t  findWlConnForDevice(tUartNr deviceNr, int prio)
* \brief Checks which wireless connection number is configured with the desired priority
* \return wlConnectionToUse: a number between 0 and (NUMBER_OF_UARTS-1). This priority is not configured if NUMBER_OF_UARTS is returned.
*/
static uint8_t  findWlConnForDevice(tUartNr deviceNr, int prio)
{
	switch(config.LoadBalancingMode)
	{
		case LOAD_BALANCING_AS_CONFIGURED:
			return getWlConnConfiguredForPrio(deviceNr, prio);
		case LOAD_BALANCING_SWITCH_WL_CONN_WHEN_ACK_NOT_RECEIVED: /* costFunctionPerWlConn is either 0 or 100 */
			// ToDo: Not implemented yet
			return NUMBER_OF_UARTS;
		case LOAD_BALANCING_USE_ALGORITHM:
			// ToDo: Not implemented yet
			return NUMBER_OF_UARTS;
	}
	return NUMBER_OF_UARTS;
}


/*!
* \fn static bool processAssembledPackage(tUartNr wirelessConnNr)
* \brief Pops received package from queue and checks if it is ACK or Data package.
* ACK package -> deletes the package from the buffer where we wait for ACKS.
* data package -> generates ACK and sends it to package handler queue for packages to send.
* \param wirelessConnNr: The wireless device number where we look for received packages
*/
static bool processAssembledPackage(tUartNr wlConn)
{
	tWirelessPackage package;
	static char infoBuf[150];

	/* if it is a data package -> check if there is enough space on byte queue of device side */
	if(peekAtAssembledPackQueue(wlConn, &package) != pdTRUE) /* peek at package to find out payload size for space on Device Tx Bytes queue */
	{
		return false; /* peek not successful */
	}
	/* no space for package in application handler or no space for acknowledge in package handler */
	if((package.packType == PACK_TYPE_DATA_PACKAGE) && (freeSpaceInReceivedPayloadPacksQueue(package.devNum) <= 0) ||
	   ((package.packType == PACK_TYPE_DATA_PACKAGE) && (config.SendAckPerWirelessConn[wlConn]) && (freeSpaceInPackagesToDisassembleQueue(wlConn) <= 0)) )
	{
		return false; /* not enough space */
	}
	/* pop package from queue to send it out */
	if(popAssembledPackFromQueue(wlConn, &package) != pdTRUE) /* actually remove package from queue */
	{
		return false; /* coun't be removed */
	}
	if(package.packType == PACK_TYPE_DATA_PACKAGE) /* data package received */
	{
		/* check if data is valid */
		if(package.payloadSize > PACKAGE_MAX_PAYLOAD_SIZE)
		{
			package.payloadSize = PACKAGE_MAX_PAYLOAD_SIZE;
		}
		if(package.devNum > NUMBER_OF_UARTS)
		{
			package.devNum = NUMBER_OF_UARTS-1;
		}

		/* push package to application handler for processing payload */
		pushToReceivedPayloadPacksQueue(package.devNum, &package);


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
			if(pushToPacksToDisassembleQueue(wlConn, &ackPackage) != pdTRUE) // ToDo: try sending ACK package out on wireless connection configured (just like data package, iterate through priorities) */
			{
				XF1_xsprintf(infoBuf, "%u: Warning: ACK for wireless number %u could not be pushed to queue\r\n", xTaskGetTickCount(), wlConn);
				pushMsgToShellQueue(infoBuf);
				numberOfDroppedPackages[wlConn]++;
				FRTOS_vPortFree(ackPackage.payload); /* free memory since it wont be done on popping from queue */
				ackPackage.payload = NULL;
			}
			/* memory of ackPackage is freed after package in PackageHandler task, extracted and byte wise pushed to byte queue */
			numberOfAcksSent[wlConn]++;
		}
	}
	else if(package.packType == PACK_TYPE_REC_ACKNOWLEDGE) /* acknowledge package received */
	{
		int tmpNumOfUnackPacks = numberOfUnacknowledgedPackages;
		uint16_t packNr = package.payload[0];
		packNr |= (package.payload[1] << 8);
		/* iterate though unacknowledged packages to find the corresponding one */
		for(int index=0; index < MAX_NUMBER_OF_UNACK_PACKS_STORED; index++)
		{
			if(tmpNumOfUnackPacks <= 0)
			{
				break; /* leave for loop instead of iterating over all packages */
			}
			/* check if this index holds an unacknowledged package */
			if(unacknowledgedPackagesOccupiedAtIndex[index])
			{
				tmpNumOfUnackPacks --;

				/* check if this is the package we got the acknowledge for */
				if(		unacknowledgedPackages[index].devNum == package.devNum &&
						unacknowledgedPackages[index].packNr == packNr   )
				{
					/* free memory of saved package if we got ACK */
					FRTOS_vPortFree(unacknowledgedPackages[index].payload);
					unacknowledgedPackages[index].payload = NULL;
					unacknowledgedPackagesOccupiedAtIndex[index] = false;
					numberOfUnacknowledgedPackages--;
					FRTOS_vPortFree(package.payload); /* free memory for package popped from queue */
					package.payload = NULL;
					ackReceived[wlConn] = true;
					return true; /* unacknowledged package found, leave for-loop */
				}
			}
		}
		XF1_xsprintf(infoBuf, "%u: Warning: Got ACK for packNr %u and payloadNr %u on wireless connection %u but no saved package found -> check ACK configuration on both sides\r\n", xTaskGetTickCount(), packNr, package.payloadNr, wlConn);
		pushMsgToShellQueue(infoBuf);
		FRTOS_vPortFree(package.payload);
		package.payload = NULL;
		return false; /* found no matching data package for this acknowledge */
	}
	else
	{
		XF1_xsprintf(infoBuf, "%u: Error: invalid package type received on wireless connection %u\r\n", xTaskGetTickCount(), wlConn);
		pushMsgToShellQueue(infoBuf);
		FRTOS_vPortFree(package.payload);
	}
	return true;
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
	/* default header = { PACK_START, PACK_TYPE_REC_ACKNOWLEDGE, 0, 0, 0, 0, 0, 0, 0, 0 } */
	/* prepare wireless package */
	pAckPack->packType = PACK_TYPE_REC_ACKNOWLEDGE;
	pAckPack->devNum = pReceivedDataPack->devNum;
	pAckPack->packNr = ++sentAckNumTracker[pReceivedDataPack->devNum];
	pAckPack->payloadNr = 0;
	pAckPack->payloadSize = sizeof(pAckPack->packNr);	/* as payload, the timestamp of the package to be acknowledged is saved */
	/* get space for acknowladge payload (which consists of packNr of datapackage*/
	pAckPack->payload = (uint8_t*) FRTOS_pvPortMalloc(pAckPack->payloadSize*sizeof(int8_t));
	if(pAckPack->payload == NULL) /* malloc failed */
		return false;
	/* generate payload */
	for (uint16_t cnt = 0; cnt < pAckPack->payloadSize; cnt++)
	{
		pAckPack->payload[cnt] = *((uint8_t*)(&pReceivedDataPack->packNr) + cnt);
	}
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
	tWirelessPackage resendPack;

	for (int index = 0; index < MAX_NUMBER_OF_UNACK_PACKS_STORED; index++)
	{
		if(unackPackagesLeft <= 0) /* leave iteration through array when there are no more packages in there */
		{
			return;
		}
		if(unacknowledgedPackagesOccupiedAtIndex[index]) /* there is a package stored at this index */
		{
			tWirelessPackage* pPack = &unacknowledgedPackages[index];
			unackPackagesLeft --;
			int prio = 4;
			int maxPrio = 1;
			/* find maximum wl connection priority value configured on this device (=highest number) */
			while(maxPrio != NUMBER_OF_UARTS)
			{
				maxPrio = getWlConnConfiguredForPrio(pPack->devNum, prio);
				prio --;
			}
			prio = 1;
			while(prio <= NUMBER_OF_UARTS) /* iterate though all priorities (starting at highest priority) to see if resend is required */
			{
				int wlConn = getWlConnConfiguredForPrio(pPack->devNum, prio);
				uint32_t tickCount = xTaskGetTickCount();
				uint32_t keepPackAliveTimeout = pdMS_TO_TICKS(config.DelayDismissOldPackagePerDev[pPack->devNum]);
				uint32_t resendDelayInTicks = pdMS_TO_TICKS(config.ResendDelayWirelessConnDev[wlConn][pPack->devNum]);
				/* max number of resends done for all connections or maximum delay in config reached for this package     OR
				 * no response on last resend received during resend timeout */
				if(((wlConn >= NUMBER_OF_UARTS) || ((pPack->timestampFirstSendAttempt + keepPackAliveTimeout) < tickCount))     ||
					((prio == maxPrio) && (pPack->sendAttemptsLeftPerWirelessConnection[wlConn] == 0) &&  (tickCount - pPack->timestampLastSendAttempt[wlConn] > resendDelayInTicks)))
				{
					XF1_xsprintf(infoBuf, "%u: Warning: Max number of retries reached and no ACK received -> discard package with packNr %u and payloadNr %u for device %u\r\n", xTaskGetTickCount(), pPack->packNr, pPack->payloadNr, pPack->devNum);
					pushMsgToShellQueue(infoBuf);
					FRTOS_vPortFree(pPack->payload); /* free allocated memory when package dropped*/
					pPack->payload = NULL;
					unacknowledgedPackagesOccupiedAtIndex[index] = false;
					numberOfUnacknowledgedPackages--;
					ackReceived[wlConn] = true;
					prio = NUMBER_OF_UARTS; /* leave iteration over priorities */
					numberOfDroppedPackages[pPack->devNum]++; /* update throughput printout */
				}
				else if(pPack->sendAttemptsLeftPerWirelessConnection[wlConn] > 0) /* there are send attempts left on this wl conn */
				{
					/* is timeout for ACK done?*/
					if(tickCount - pPack->timestampLastSendAttempt[wlConn] > pdMS_TO_TICKS(config.ResendDelayWirelessConnDev[wlConn][pPack->devNum]))
					{
						/* create new package for queue because memory is freed once package is pulled from queue */
						resendPack = *pPack;
						resendPack.payload = (uint8_t*) FRTOS_pvPortMalloc(resendPack.payloadSize*sizeof(int8_t)); // ToDo: check queue state before allocating memory for resendPack
						if(resendPack.payload == NULL)
						{
							return; /* leave this entire function if malloc fails! */
						}
						/* fill package.payload with payload data */
						for(int cnt = 0; cnt < resendPack.payloadSize; cnt++)
						{
							resendPack.payload[cnt] = pPack->payload[cnt];
						}
						/* send resendPack */
						if(pushToPacksToDisassembleQueue(wlConn, &resendPack) == pdTRUE)
						{
							pPack->sendAttemptsLeftPerWirelessConnection[wlConn]--;
							pPack->timestampLastSendAttempt[wlConn] = tickCount;
							prio = NUMBER_OF_UARTS; /* leave priority-while loop because package will be sent, now to wait on acknowledge */
							/* update throughput printout */
							numberOfPayloadBytesSent[wlConn] += pPack->payloadSize;
						}
						else
						{
							numberOfDroppedPackages[wlConn]++;
							FRTOS_vPortFree(resendPack.payload); /* free memory since it wont be popped from queue and freed there */
							resendPack.payload = NULL;
						}
					}
					else /* package has already been sent, now waiting on acknowledge -> leave loop for this package, find next package to check */
						prio = NUMBER_OF_UARTS; /* leave priority-while loop */
				}
				/* no send attempt left but we are waiting for ACK of very last send attempt on one wireless connection */
				else if((pPack->sendAttemptsLeftPerWirelessConnection[wlConn] == 0) &&  (tickCount - pPack->timestampLastSendAttempt[wlConn] < resendDelayInTicks))
				{
					prio = NUMBER_OF_UARTS;
				}
				else if(pPack->sendAttemptsLeftPerWirelessConnection[wlConn] == 0) /* no ack received on this wl connection */
				{
					ackReceived[wlConn] = true; /* no resending on this wl conn -> make sure next package can be sent */
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
		if(unacknowledgedPackagesOccupiedAtIndex[index] != true) /* there is no package stored at this index */
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



