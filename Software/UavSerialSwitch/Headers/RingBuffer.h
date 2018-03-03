/*
 * RingBuffer.h
 *
 *  Created on: 20.02.2018
 *      Author: Stefanie
 */

#ifndef HEADERS_RINGBUFFER_H_
#define HEADERS_RINGBUFFER_H_

#include "PackageHandler.h"


typedef struct sRingBuf
{
	tWirelessPackage *bufferStart;     // data buffer
	tWirelessPackage *bufferEnd; // end of data buffer
    size_t maxNumElements;// maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t itemSize;        // size of each item in the buffer
    tWirelessPackage *head;       // pointer to head, where next newest package will be stored (is empty still)
    tWirelessPackage *tail;       // pointer to tail, where oldest package is stored
} tRingBuf;

/*!
* \fn void rb_init(tRingBuf *rb, size_t maxNumElements, size_t itemSize)
* \brief Initializes a ringbuffer, allocates memory for all its components
* \param rb: pointer to ringbuffer to be initialized
* \param maxNumElements: size of the ringbuffer
* \param itemSize: size of each item that will be stored inside the ringbuffer
*/
void rb_init(tRingBuf* rb, size_t maxNumElements);

/*!
* \fn BaseType_t rb_free(tRingBuf *rb)
* \brief Deinitializes the ringbuffer, frees memory allocated for ringbuffer
*/
void rb_free(tRingBuf *rb);

/*!
* \fn BaseType_t rb_push_back(tRingBuf *rb, void *item)
* \brief Pushes an element to the back of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element to be pushed to the ringbuffer
* \return Status if push has been successful
*/
BaseType_t rb_pushToBack(tRingBuf *rb, tWirelessPackage* item);

/*!
* \fn BaseType_t rb_push_back(tRingBuf *rb, void *item)
* \brief Pushes an element to the back of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element to be pushed to the ringbuffer
* \param item: 0...maxNumElements-1
* \return Status if push has been successful
*/
BaseType_t rb_pushToIndex(tRingBuf *rb, tWirelessPackage* item, uint16_t indexNum);

/*!
* \fn BaseType_t rb_pop_front(tRingBuf *rb, void *item)
* \brief Pops an element from the front of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element where popped element will be stored
* \return Status if pop has been successful
*/
BaseType_t rb_popFromIndex(tRingBuf *rb, tWirelessPackage* item, uint16_t itemNumber);

/*!
* \fn BaseType_t rb_pop_front(tRingBuf *rb, void *item)
* \brief Pops an element from the front of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element where popped element will be stored
* \return Status if pop has been successful
*/
BaseType_t rb_popFromFront(tRingBuf *rb, tWirelessPackage* item);

/*!
* \fn BaseType_t rb_pop_front(tRingBuf *rb, void *item)
* \brief Peeks at an element inside the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element where peeked element will be copied to
* \param itemNumber: 0...count-1
* \return Status if peek has been successful
*/
BaseType_t rb_peekAtItem(tRingBuf *rb, tWirelessPackage* item, uint16_t itemNumber);




#endif /* HEADERS_RINGBUFFER_H_ */
