/**
 * \file
 * \brief Shell (command line) interface.
 * \author Erich Styger, erich.styger@hslu.ch
 *
 * This module implements a command line interface to the application.
 */

#ifndef HEADERS_SHELL_H_
#define HEADERS_SHELL_H_

#include "FRTOS.h"


/*!
* \fn void Shell_TaskEntry (void *pvParameters)
* \brief Parses commands in shell and prints debug information if configured in ini file.
*/
void Shell_TaskEntry (void *pvParameters);

/*!
* \fn void Shell_TaskInit(void)
* \brief Initializes the queue used for debug message storage
*/
void Shell_TaskInit(void);

/*!
* \fn BaseType_t pushMsgToShellQueue(unsigned char* msg)
* \brief Stores pData in queue
* \param pMsg: The location where the string is stored
* \return Status if xQueueSendToBack has been successful
*/
BaseType_t pushMsgToShellQueue(char* pMsg);

/*! \def MAX_NUMBER_OF_MESSAGES_STORED
*  \brief Size of queue for messages to be printed in shell.
*/
#define MAX_NUMBER_OF_MESSAGES_STORED		100


/*! \def MAX_NUMBER_OF_CHARS_PER_MESSAGE
*  \brief Size of queue element per messages to be printed in shell.
*/
#define MAX_NUMBER_OF_CHARS_PER_MESSAGE		300


/*! \def MAX_DELAY_SHELL_MS
*  \brief Maximal delay on queue operations inside Shell task.
*/
#define MAX_DELAY_SHELL_MS					(0)


#endif /* SHELL_H_ */
