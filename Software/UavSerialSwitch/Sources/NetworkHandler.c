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
#include "TransportHandler.h"
#include "RNG.h"


/* global variables, only used in this file */
static tWirelessPackage unacknowledgedPackages[MAX_NUMBER_OF_UNACK_PACKS_STORED];
static bool unacknowledgedPackagesOccupiedAtIndex[MAX_NUMBER_OF_UNACK_PACKS_STORED];
static int numberOfUnacknowledgedPackages;
static uint16_t sentPackNumTracker[NUMBER_OF_UARTS];
static volatile bool ackReceived[NUMBER_OF_UARTS];
static uint8_t costFunctionPerWlConn[NUMBER_OF_UARTS];

/* prototypes of local functions */
static void initNetworkHandlerQueues(void);
static void initSempahores(void);
static bool processAssembledPackage(tUartNr wlConn);
static bool sendGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr rawDataUartNr);
static bool storeSentPackageInternally(tWirelessPackage* pPackage, bool* wlConnSent);
static bool copyPackageIntoUnacknowledgedPackagesArray(tWirelessPackage* pPackage);
static void getWlConnConfiguredForPrio(tUartNr uartNr, uint8_t desiredPrio, bool* prioFoundOnWlConn);
static void handleResendingOfUnacknowledgedPackages(void);
static void findWlConnForDevice(tUartNr deviceNr, bool* wlConnToUse);
static bool copyPackage(tWirelessPackage* original, tWirelessPackage* copy);


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
			if( nofGeneratedPayloadPacksInQueue(deviceNr) > 0)
			{
				/* find wl connection to use for this package */
				bool wlConnToUse[] = {false, false, false, false};
				bool ackExpected = false;
				bool packSent = false;
				findWlConnForDevice(deviceNr, wlConnToUse); /* find wlConn to use according to configuration file */
				for(int wlConn = 0; wlConn < NUMBER_OF_UARTS; wlConn++)
				{
					/* this wlconn is configured for the desired priority and there is space in the queue of next handler? */
					if( (wlConnToUse[wlConn] == true) && (freeSpaceInPackagesToDisassembleQueue(wlConn)) )
					{
						if( ! (config.SyncMessagingModeEnabledPerWlConn[wlConn] && (ackReceived[wlConn] == false)) ) /* sync mode not enabled or last ack received */
						{
							if(peekAtGeneratedPayloadPackInQueue(deviceNr, &package) == pdTRUE) /* peeking at package from upper handler successful? */
							{

								tWirelessPackage tmpPack;
								copyPackage(&package, &tmpPack);
								if(sendGeneratedWlPackage(&tmpPack, wlConn) == false) /* send the generated package down and store it internally if ACK is configured */
								{
									/* package couldnt be sent and payload was freed! don't access package anymore! */
									break; /* exit innner for loop */
								}
								packSent = true;
								if(config.SendAckPerWirelessConn[wlConn])
								{
									ackExpected = true;
								}
							}
						}
					}
				}
				if(packSent)
				{
					popFromGeneratedPacksQueue(deviceNr, &package); /* this is done here because if two wlConn configured with same priority, package cant be removed twice */
					vPortFree(package.payload);
					package.payload = NULL;
				}
				if(ackExpected)
				{
					storeSentPackageInternally(&package, wlConnToUse);
				}
			}

			/* extract data from received packages, send ACK and send raw data to corresponding UART interface */
			if(nofAssembledPacksInQueue(deviceNr) > 0)
			{
				processAssembledPackage(deviceNr);
			}
		}
		/* handle resend in case acknowledge not received */
		handleResendingOfUnacknowledgedPackages(); /* not done per wlConn, so needs to be outside for-loop */
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
* \fn static bool sendGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr wlConn)
* \brief Sends the generated package to package handler for sending
* \param pPackage: pointer to package that should be sent to out
* \param wlConn: wireless connection where package should be sent out on this first try
* \return true if a package was sent to package handler and stored in buffer successfully
*/
static bool sendGeneratedWlPackage(tWirelessPackage* pPackage, tUartNr wlConn)
{
	char infoBuf[70];

	if(pushToPacksToDisassembleQueue(wlConn, pPackage) != pdTRUE)
	{
		XF1_xsprintf(infoBuf, "%u: Warning: Couldn't push newly generated package from device %u to package queue on wl conn %u \r\n", xTaskGetTickCount(), pPackage->devNum, wlConn);
		pushMsgToShellQueue(infoBuf);
		vPortFree(pPackage->payload);
		pPackage->payload = NULL;
		numberOfDroppedPackages[wlConn]++;
		return false;
	}
	/* update throughput printout */
	numberOfPacksSent[wlConn]++;
	numberOfPayloadBytesSent[wlConn] += pPackage->payloadSize;
	return true; /* package couldnt be stored in unacknowledgedPackagesArray */
}


/*!
* \fn static bool storeSentPackageInternally(tWirelessPackage* pPackage, bool* wlConnSent)
* \brief Stores the sent package internally to remember resending it if no ACK received
* \param pPackage: pointer to package that should be sent to out
* \param wlConnsent: boolean over which wlConn the package was sent out on first try
* \return true if a package was stored in buffer successfully
*/
static bool storeSentPackageInternally(tWirelessPackage* pPackage, bool* wlConnSent)
{
	/* set all information about package (re)sending */
	for(int index=0; index < NUMBER_OF_UARTS; index++)
	{
		pPackage->timestampLastSendAttempt[index] = 0;
		pPackage->totalNumberOfSendAttemptsPerWirelessConnection[index] = config.SendCntWirelessConnDev[pPackage->devNum][index];
		pPackage->sendAttemptsLeftPerWirelessConnection[index] = config.SendCntWirelessConnDev[pPackage->devNum][index];
	}
	for(int wlConn = 0; wlConn < NUMBER_OF_UARTS; wlConn++)
	{
		if(wlConnSent[wlConn]) /* this wireless connection has been used for sending */
		{
			pPackage->sendAttemptsLeftPerWirelessConnection[wlConn]--;
			pPackage->timestampLastSendAttempt[wlConn] = xTaskGetTickCount();
			pPackage->timestampFirstSendAttempt = xTaskGetTickCount();
			pPackage->wlConnUsedForLastSendAttempt = wlConn;
			ackReceived[wlConn] = false; /* flag for PackNumberProcessingMode == WAIT_FOR_ACK_BEFORE_SENDING_NEXT_PACK */
		}
	}
	if(copyPackageIntoUnacknowledgedPackagesArray(pPackage) == true)
	{
		return true; /* success */
	}
}

/*!
* \fn static void getWlConnConfiguredForPrio(tUartNr uartNr, uint8_t desiredPrio, bool* prioFoundOnWlConn)
* \brief Checks which wireless connection number is configured with the desired priority
* \return wlConnectionToUse: a number between 0 and (NUMBER_OF_UARTS-1). This priority is not configured if NUMBER_OF_UARTS is returned.
*/
static void getWlConnConfiguredForPrio(tUartNr uartNr, uint8_t desiredPrio, bool* prioFoundOnWlConn)
{
	for(int wlConn = 0; wlConn<NUMBER_OF_UARTS; wlConn++)
	{
		if(config.PrioWirelessConnDev[uartNr][wlConn] == desiredPrio)
		{
			prioFoundOnWlConn[wlConn] = true;
		}
		else
		{
			prioFoundOnWlConn[wlConn] = false;
		}
	}
}


/*!
* \fn static uint8_t  findWlConnForDevice(tUartNr deviceNr, int prio)
* \brief Checks which wireless connection number is configured with the desired priority
* \return wlConnectionToUse: a number between 0 and (NUMBER_OF_UARTS-1). This priority is not configured if NUMBER_OF_UARTS is returned.
*/
static void findWlConnForDevice(tUartNr deviceNr, bool* wlConnToUse)
{
	for(int i = 0; i<NUMBER_OF_UARTS; i++)
	{
		wlConnToUse[i] = false;
	}
	switch(config.LoadBalancingMode)
	{
		case LOAD_BALANCING_AS_CONFIGURED:
			getWlConnConfiguredForPrio(deviceNr, 1, wlConnToUse);
			break;
		case LOAD_BALANCING_SWITCH_WL_CONN_WHEN_ACK_NOT_RECEIVED: /* costFunctionPerWlConn is either 0 or 100 */
			// ToDo: Not implemented yet
			break;
		case LOAD_BALANCING_USE_ALGORITHM:
			// ToDo: Not implemented yet
			break;
	}
}

/*!
* \fn static bool copyPackage(tWirelessPackage* original, tWirelessPackage* copy)
* \brief Copies the content of the original package into the copy, allocates memory for payload of copy
* \return bool: true if successful, false otherwise
*/
static bool copyPackage(tWirelessPackage* original, tWirelessPackage* copy)
{
	*copy = *original;
	copy->payload = FRTOS_pvPortMalloc(original->payloadSize*sizeof(int8_t));
	if(copy->payload == NULL)
	{
		return false;
	}
	for(int cnt = 0; cnt < copy->payloadSize; cnt++)
	{
		copy->payload[cnt] = original->payload[cnt];
	}
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
	/* no space for package in transport handler or no space for acknowledge in package handler */
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
			bool sendPackOnWlConn[NUMBER_OF_UARTS];
			unackPackagesLeft --;
			int prio;
			int maxPrio = NUMBER_OF_UARTS;
			/* find maximum wl connection priority value configured on this device (=highest number) */
			for(prio = 1; prio <= NUMBER_OF_UARTS; prio++)
			{
				getWlConnConfiguredForPrio(pPack->devNum, prio, sendPackOnWlConn);
				for(int i = 0; i<NUMBER_OF_UARTS; i++)
				{
					if(sendPackOnWlConn[i]) /* found a wlConn configured with this priority */
					{
						maxPrio = prio;
					}
				}
				if(maxPrio != prio) /* no priority found for this iteration -> last priority was maximum configured */
				{
					break; /* leave for-loop */
				}
			}
			prio = 1;

			while(prio <= NUMBER_OF_UARTS) /* iterate though all priorities (starting at highest priority = lowest number) to see if resend is required */
			{
				int wlConn = 0;
				getWlConnConfiguredForPrio(pPack->devNum, prio, sendPackOnWlConn);
				for(int i = 0; i > NUMBER_OF_UARTS; i++)
				{
					if(sendPackOnWlConn[i])
					{
						wlConn = i;
						break;
					}
				}
				uint32_t tickCount = xTaskGetTickCount();
				uint32_t keepPackAliveTimeout = pdMS_TO_TICKS(config.DelayDismissOldPackagePerDev[pPack->devNum]);
				uint32_t resendDelayInTicks = pdMS_TO_TICKS(config.ResendDelayWirelessConn[wlConn]);
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
					if(tickCount - pPack->timestampLastSendAttempt[wlConn] > pdMS_TO_TICKS(config.ResendDelayWirelessConn[wlConn]))
					{
						/* create new package for queue because memory is freed once package is pulled from queue */
						copyPackage(pPack, &resendPack);
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
* \fn bool copyPackageIntoUnacknowledgedPackagesArray(tWirelessPackage* pPackage)
* \brief Finds free space in array and stores package in array for unacknowledged packages.
* \param pPackage: Pointer to package that should be stored
* \return true if successful, false if array is full
*/
static bool copyPackageIntoUnacknowledgedPackagesArray(tWirelessPackage* pPackage)
{
	for(int index=0; index < MAX_NUMBER_OF_UNACK_PACKS_STORED; index++)
	{
		if(unacknowledgedPackagesOccupiedAtIndex[index] != true) /* there is no package stored at this index */
		{
			copyPackage(pPackage, &unacknowledgedPackages[index]);
			unacknowledgedPackagesOccupiedAtIndex[index] = 1;
			numberOfUnacknowledgedPackages++;
			return true;
		}
	}
	return false;
}


/*!
* \fn void resetAllNetworkPackageCounter(void)
* \brief Resets packageNr counter when sessionNr changed on other side
*/
void resetAllNetworkPackageCounter(void)
{
	for (int index = 0; index < MAX_NUMBER_OF_UNACK_PACKS_STORED; index++)
	{
		unacknowledgedPackagesOccupiedAtIndex[index] = 0;
		vPortFree(unacknowledgedPackages[index].payload);
		unacknowledgedPackages[index].payload = NULL;
	}
	numberOfUnacknowledgedPackages = 0;
	for(int index = 0; index < NUMBER_OF_UARTS; index++)
	{
		/* reset any arrays */
	}
}



