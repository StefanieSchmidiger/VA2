#ifndef HEADERS_SYSINIT_H_
#define HEADERS_SYSINIT_H_

#include "FRTOS.h"

/*!
* \fn void SysInit_TaskEntry(void* param)
* \brief Reads config file and creates all other tasks afterwards (because other tasks need config file)
*/
void SysInit_TaskEntry(void* param);

#define SHELL_STACK_SIZE 				(2000/sizeof(StackType_t))
#define SPI_HANDLER_STACK_SIZE 			(2000/sizeof(StackType_t))
#define PACKAGE_HANDLER_STACK_SIZE		(2000/sizeof(StackType_t))
#define NETWORK_HANDLER_STACK_SIZE		(2000/sizeof(StackType_t))
#define THROUGHPUT_PRINTOUT_STACK_SIZE	(2000/sizeof(StackType_t))
#define LOGGER_STACK_SIZE				(2000/sizeof(StackType_t))
#define BLINKY_STACK_SIZE				(400/sizeof(StackType_t))

#endif
