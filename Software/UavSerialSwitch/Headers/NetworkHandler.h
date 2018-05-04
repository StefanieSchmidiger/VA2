#ifndef HEADERS_NETWORKHANDLER_H_
#define HEADERS_NETWORKHANDLER_H_

#include "SpiHandler.h"
#include "PackageHandler.h"
#include "FRTOS.h"


/*! \def MAX_NUMBER_OF_UNACK_PACKS_STORED
*  \brief Size of internal array for unacknowledged packages.
*/
#define MAX_NUMBER_OF_UNACK_PACKS_STORED		(25)

/*! \def MAX_DELAY_NETW_HANDLER_MS
*  \brief Maximal delay on queue operations inside networkHandler task.
*/
#define MAX_DELAY_NETW_HANDLER_MS				(0)

/*! \def REORDERING_PACKAGES_BUFFER_SIZE
*  \brief Size of ringbuffer for reordering received packages
*/
#define REORDERING_PACKAGES_BUFFER_SIZE				(10)

/*!
* \fn void networkHandler_TaskEntry(void)
* \brief Task generates packages from received bytes (received on device side) and sends those down to
* the packageHandler for transmission.
* When acknowledges are configured, resending is handled here.
*/
void networkHandler_TaskEntry(void* p);

/*!
* \fn void networkHandler_TaskInit(void)
* \brief Initializes all queues that are declared within network handler
* and generates random session number
*/
void networkHandler_TaskInit(void);


#endif
