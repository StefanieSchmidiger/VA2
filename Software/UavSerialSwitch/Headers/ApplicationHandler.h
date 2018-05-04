/*
 * ApplicationHandler.h
 *
 *  Created on: 02.05.2018
 *      Author: Stefanie
 */

#ifndef HEADERS_APPLICATIONHANDLER_H_
#define HEADERS_APPLICATIONHANDLER_H_

#include "FRTOS.h"
#include "SpiHandler.h"
#include "PackageHandler.h"
#include "NetworkHandler.h"

/*! \def APPLICATION_HANDLER_QUEUE_DELAY
*  \brief Number of ticks to wait on byte queue operations within this task
*/
#define APPLICATION_HANDLER_QUEUE_DELAY				0

/*! \def QUEUE_NUM_OF_READY_TO_SEND_WL_PACKS
*  \brief Number of wireless packages that should have find space within a single queue.
*/
#define QUEUE_NUM_OF_READY_TO_SEND_WL_PACKS			10

/*! \def QUEUE_NUM_OF_RECEIVED_WL_PACKS
*  \brief Number of wireless packages that should have find space within a single queue.
*/
#define QUEUE_NUM_OF_RECEIVED_PAYLOAD_PACKS			10



/*!
* \fn void networkHandler_TaskEntry(void)
* \brief Task generates packages from received bytes (received on device side) and sends those down to
* the NetworkHandler for transmission.
* Payload reordering is done here when configured
*/
void applicationHandler_TaskEntry(void* p);



/*!
* \fn void networkHandler_TaskInit(void)
* \brief Initializes all queues that are declared within application handler
*/
void applicationHandler_TaskInit(void);

/*!
* \fn ByseType_t popToReadyToSendPackFromQueue(tUartNr uartNr, tWirelessPackage* pPackage)
* \brief Pops a package from queue
* \param uartNr: UART number where data was collected.
* \param pPackage: Pointer to empty package
* \return Status if xQueueReceive has been successful
*/
BaseType_t popFromReadyToSendPackQueue(tUartNr uartNr, tWirelessPackage* pPackage);


/*!
* \fn ByseType_t nofReadyToSendPackInQueue(tUartNr uartNr)
* \brief Peeks at queue to see how many elements are inside
* \param uartNr: UART number where data was collected.
* \return Status if xQueueReceive has been successful
*/
BaseType_t nofReadyToSendPackInQueue(tUartNr uartNr);


/*!
* \fn ByseType_t peekAtReadyToSendPackInQueue(tUartNr uartNr)
* \brief Peeks at queue and copies next element into pPackage
* \param uartNr: UART number where data was collected.
* \param pPackage: Pointer to tWirelessPackage element where data will be copied
* \return Status if xQueuePeek has been successful
*/
BaseType_t peekAtReadyToSendPackInQueue(tUartNr uartNr, tWirelessPackage* pPackage);

/*!
* \fn ByseType_t pushToReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage)
* \brief Stores package in queue
* \param uartNr: UART number where data was collected.
* \param pPackage: The location of the package to be pushed to the queue
* \return Status if xQueueSendToBack has been successful
*/
BaseType_t pushToReceivedPayloadPacksQueue(tUartNr uartNr, tWirelessPackage* pPackage);

/*!
* \fn ByseType_t freeSpaceInReceivedPayloadPacksQueue(tUartNr uartNr)
* \brief Returns number of free slots in queue
* \param uartNr: UART number where payload will be pushed out
* \return Number of elements that can still be pushed to this queue
*/
BaseType_t freeSpaceInReceivedPayloadPacksQueue(tUartNr uartNr);


#endif /* HEADERS_APPLICATIONHANDLER_H_ */
