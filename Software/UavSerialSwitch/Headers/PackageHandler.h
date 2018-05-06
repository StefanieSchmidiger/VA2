/*
 * PackageHandler.h
 *
 *  Created on: 07.11.2017
 *      Author: Stefanie
 */

#ifndef HEADERS_PACKAGEHANDLER_H_
#define HEADERS_PACKAGEHANDLER_H_

#include <stdint.h>
#include "SpiHandler.h"

/*! \def QUEUE_NUM_OF_WL_PACK_RECEIVED
*  \brief Number of wireless packages that should have find space within a single queue.
*/
#define QUEUE_NUM_OF_WL_PACK_TO_ASSEMBLE			10 /* about 550 bytes per wireless package, not including the dynamically allocated memory for payload */

/*! \def QUEUE_NUM_OF_WL_PACK_TO_DISASSEMBLE
*  \brief Number of wireless packages that should have find space within a single queue.
*/
#define QUEUE_NUM_OF_WL_PACK_TO_DISASSEMBLE			20 /* about 550 bytes per wireless package, not including the dynamically allocated memory for payload */


/*! \def PACK_START
*  \brief Definition of the mark of a beginn of a wireless package.
*/
#define PACK_START									((uint8_t)'\e')

/*! \def PACK_REP
*  \brief If a PACK_START character is within the data, it will be replaced by PACK_REP PACK_START PACK_REP.
*/
#define PACK_REP									((uint8_t)'"')

/*! \def PACK_FILL
*  \brief Filling character if some is needed.
*/
#define PACK_FILL									((uint8_t)' ')


/*! \def PACKAGE_HEADER_SIZE
*  \brief Size of wireless package header in bytes - see tWirelessPackage without payload stuff plus sizeof PACK_START
*/
#define PACKAGE_HEADER_SIZE							(11)

/*! \def TOTAL_WL_PACKAGE_SIZE
*  \brief Size of wireless package minus payload, +2 because CRC payload is 2 bytes
*/
#define TOTAL_WL_PACKAGE_SIZE						(PACKAGE_HEADER_SIZE + 2)


/*! \def PACKAGE_MAX_PAYLOAD_SIZE
*  \brief Maximal size of payload.
*/
#define PACKAGE_MAX_PAYLOAD_SIZE					(512)

/*! \def MAX_DELAY_PACK_HANDLER_MS
*  \brief Maximal delay on queue operations inside packageHandler task.
*/
#define MAX_DELAY_PACK_HANDLER_MS					(0)

/*! \enum ePackType
*  \brief There are two types of packages: data packages and acknowledges.
*/
typedef enum ePackType
{
	PACK_TYPE_DATA_PACKAGE = 0x01,
	PACK_TYPE_REC_ACKNOWLEDGE = 0x02
} tPackType;




/*! \struct sWirelessPackage
*  \brief Structure that holds all the required information of a wireless package.
*  Acknowledge has the same sysTime & devNum in header as the package it is sent for.
*  The individual sysTime for ACK package is in payload.
*/
typedef struct sWirelessPackage
{
	/* --- payload of package --- */
	// header
	tPackType packType;
	uint8_t devNum;
	uint8_t sessionNr;
	uint16_t packNr;
	uint16_t payloadNr;
	uint16_t payloadSize;
	uint8_t crc8Header;
	// data
	uint8_t* payload;
	uint16_t crc16payload;
	/* internal information, needed for (re)sending package */
	uint8_t currentPrioConnection;
	int8_t sendAttemptsLeftPerWirelessConnection[NUMBER_OF_UARTS];
	uint32_t timestampFirstSendAttempt;
	uint32_t timestampLastSendAttempt[NUMBER_OF_UARTS];	/* holds the timestamp when the packet was sent the last time for every wireless connection */
	uint16_t totalNumberOfSendAttemptsPerWirelessConnection[NUMBER_OF_UARTS];		/* to save the total number of send attempts that were needed for this package */
	uint32_t timestampPackageReceived;
} tWirelessPackage;


/*!
* \fn void packageHandler_TaskEntry(void)
* \brief Task assembles packages from bytes and puts it on ReceivedPackages queue.
* Generated packages are popped from PackagesToSend queue are sent to byte queue for transmission.
*/
void packageHandler_TaskEntry(void* p);

/*!
* \fn void packageHandler_TaskInit(void)
* \brief Initializes queues created by package handler and HW CRC generator
*/
void packageHandler_TaskInit(void);

/*!
* \fn ByseType_t popAssembledPackFromQueue(tUartNr uartNr, tWirelessPackage *pPackage)
* \brief Stores a single byte from the selected queue in pData.
* \param uartNr: UART number the package should be transmitted to.
* \param pPackage: The location where the received package should be stored
* \return Status if xQueueReceive has been successful, pdFAIL if uartNr was invalid or pop unsuccessful
*/
BaseType_t popAssembledPackFromQueue(tUartNr uartNr, tWirelessPackage *pPackage);

/*!
* \fn ByseType_t peekAtAssembledPackQueue(tUartNr uartNr, tWirelessPackage *pPackage)
* \brief Package that will be popped next is stored in pPackage but not removed from queue.
* \param uartNr: UART number the package should be transmitted to.
* \param pPackage: The location where the received package should be stored
* \return Status if xQueuePeek has been successful, pdFAIL if uartNr was invalid or pop unsuccessful
*/
BaseType_t peekAtAssembledPackQueue(tUartNr uartNr, tWirelessPackage *pPackage);

/*!
* \fn uint16_t nofAssembledPacksInQueue(tUartNr uartNr)
* \brief Returns the number of packages stored in the queue that are ready to be received/processed by this program
* \param uartNr: UART number the packages should be read from.
* \return Number of packages waiting to be processed/received
*/
uint16_t nofAssembledPacksInQueue(tUartNr uartNr);


/*!
* \fn ByseType_t pushToPacksToDisassembleQueue(tUartNr wlConn, tWirelessPackage package)
* \brief Stores the sent package in correct queue.
* \param wlConn: UART number where package was received.
* \param pPackage: The package that will be sent
* \return Status if xQueueSendToBack has been successful, pdFAIL if push unsuccessful
*/
BaseType_t pushToPacksToDisassembleQueue(tUartNr wlConn, tWirelessPackage* pPackage);


/*!
* \fn uint16_t freeSpaceInPackagesToDisassembleQueue(tUartNr uartNr)
* \brief Returns the number of packages that can still be stored in this queue
* \param wlConn: WL conn where package should be transmitted to.
* \return Free space in this queue
*/
uint16_t freeSpaceInPackagesToDisassembleQueue(tUartNr wlConn);


#endif /* HEADERS_PACKAGEHANDLER_H_ */
