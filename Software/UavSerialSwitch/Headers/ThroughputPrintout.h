/**
 * \file
 * \brief ThroughputPrintout on command line
 * \author Stefanie Schmidiger, stefanie.schmidiger@stud.hslu.ch
 *
 * This module prints out the throughput of the UAV serial switch in a task
 */

#ifndef HEADERS_THROUGHPUTPRINTOUT_H_
#define HEADERS_THROUGHPUTPRINTOUT_H_

#include "SpiHandler.h" // NUMBER_OF_UARTS


extern long unsigned int numberOfAckReceived[NUMBER_OF_UARTS];
extern long unsigned int numberOfPacksReceived[NUMBER_OF_UARTS];
extern long unsigned int numberOfPacksSent[NUMBER_OF_UARTS];
extern long unsigned int numberOfAcksSent[NUMBER_OF_UARTS];
extern long unsigned int numberOfPayloadBytesExtracted[NUMBER_OF_UARTS];
extern long unsigned int numberOfPayloadBytesSent[NUMBER_OF_UARTS];
extern long unsigned int numberOfSendAttempts[NUMBER_OF_UARTS];
extern long unsigned int numberOfDroppedPackages[NUMBER_OF_UARTS];
extern long unsigned int numberOfDroppedAcks[NUMBER_OF_UARTS];
extern long unsigned int numberOfDroppedBytes[NUMBER_OF_UARTS];
extern long unsigned int numberOfInvalidPackages[NUMBER_OF_UARTS];

void throughputPrintout_TaskEntry(void* p);


#endif /* HEADERS_THROUGHPUTPRINTOUT_H_ */
