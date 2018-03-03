/*
 * RingBuffer.c
 *
 *  Created on: 20.02.2018
 *      Author: Stefanie
 */

#include "FRTOS.h"
#include "RingBuffer.h"
#include "PackageHandler.h"


/*!
* \fn void rb_init(tRingBuf *rb, size_t maxNumElements, size_t itemSize)
* \brief Initializes a ringbuffer, allocates memory for all its components
* \param rb: pointer to ringbuffer to be initialized
* \param maxNumElements: size of the ringbuffer
* \param itemSize: size of each item that will be stored inside the ringbuffer
*/
void rb_init(tRingBuf* rb, size_t maxNumElements)
{
	rb->itemSize = sizeof(tWirelessPackage*);
    rb->bufferStart = (tWirelessPackage*) FRTOS_pvPortMalloc(maxNumElements * rb->itemSize);
    if(rb->bufferStart == NULL)
        while(1){} /* malloc failed */
    rb->bufferEnd = (tWirelessPackage*)rb->bufferStart + maxNumElements * rb->itemSize;
    rb->maxNumElements = maxNumElements;
    rb->count = 0;

    rb->head = rb->bufferStart;
    rb->tail = rb->bufferStart;
}

/*!
* \fn void rb_free(tRingBuf *rb)
* \brief Deinitializes the ringbuffer, frees memory allocated for ringbuffer
*/
void rb_free(tRingBuf *rb)
{
	FRTOS_vPortFree(rb->bufferStart);
}

/*!
* \fn BaseType_t rb_push_back(tRingBuf *rb, void *item)
* \brief Pushes an element to the back of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element to be pushed to the ringbuffer
* \return Status if push has been successful
*/
BaseType_t rb_pushToBack(tRingBuf *rb, tWirelessPackage* item)
{
    if(rb->count == rb->maxNumElements)
    {
        return pdFAIL;
    }
    rb->head = item; /* head = newest item */
    rb->head = (tWirelessPackage*) (rb->head + rb->itemSize);
    if(rb->head == rb->bufferEnd)
    	rb->head = rb->bufferStart;
    rb->count++;
    return pdTRUE;
}

/*!
* \fn BaseType_t rb_push_back(tRingBuf *rb, void *item)
* \brief Pushes an element to the back of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element to be pushed to the ringbuffer
* \param item: 0...maxNumElements-1
* \return Status if push has been successful
*/
BaseType_t rb_pushToIndex(tRingBuf *rb, tWirelessPackage* item, uint16_t indexNum)
{
	tWirelessPackage* tmp = rb->head; /* save pointer to current free head space */
    if(indexNum >= rb->maxNumElements)
    {
        return pdFAIL;
    }
    if(rb->tail + (indexNum * rb->itemSize) >= rb->bufferEnd) /* item is at beginning of ringbuffer -> start over with counter */
    {
    	indexNum = (((rb->tail + indexNum * rb->itemSize) - rb->bufferEnd) / rb->itemSize); /* find index relative to start of buffer */
    	rb->head = (tWirelessPackage*) (rb->bufferStart + indexNum * rb->itemSize); /* update head pointer to where element will be stored */
    }
    else /* new item added linearely */
    {
    	rb->head = (tWirelessPackage*) (rb->tail + indexNum * rb->itemSize); /* update head pointer to where element will be stored */
    }
    rb->head = item;
    rb->head = (tWirelessPackage*) (rb->head + rb->itemSize); /* head = next free space */
    if(rb->head < tmp) /* is this new item not a new head? */
    {
    	rb->head = (tWirelessPackage*) tmp; /* go back to saving the old item as a head */
    }

    if(rb->head == rb->bufferEnd)
    	rb->head = rb->bufferStart;

    rb->count++;

    return pdTRUE;
}

/*!
* \fn BaseType_t rb_pop_front(tRingBuf *rb, void *item)
* \brief Pops an element from the front of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element where popped element will be stored
* \return Status if pop has been successful
*/
BaseType_t rb_popFromFront(tRingBuf *rb, tWirelessPackage* item)
{
	tWirelessPackage* tmp;
    if(rb->count == 0)
    {
        return pdFAIL;
    }
    item = rb->tail;
    tmp = (tWirelessPackage*) (rb->tail + rb->itemSize); /* pointer to next element to be removed */
    rb->tail = NULL; /* NULL saved as default element when nothing inside buffer at index */
    if(tmp == rb->bufferEnd) /* starting over at beginning of buffer? */
    {
    	rb->tail = rb->bufferStart; /* save beginning as new tail */
    }
    else
    {
    	rb->tail = tmp; /* save pointer to next element as new tail because old one is returned here */
    }
    rb->count--;
    return pdTRUE;
}

/*!
* \fn BaseType_t rb_pop_front(tRingBuf *rb, void *item)
* \brief Peeks at an element inside the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element where peeked element will be copied to
* \param itemNumber: 0...count-1
* \return Status if peek has been successful
*/
BaseType_t rb_peekAtItem(tRingBuf *rb, tWirelessPackage* item, uint16_t itemNumber)
{
    if((rb->maxNumElements < itemNumber) || (rb->count == 0)) /* invalid itemNumber or empty buffer? */
    {
        return pdFAIL;
    }
    if(rb->tail + (itemNumber * rb->itemSize) >= rb->bufferEnd) /* item is at beginning of ringbuffer -> start over with counter */
    {
    	itemNumber = ((rb->tail + itemNumber * rb->itemSize) - rb->bufferEnd) / rb->itemSize;
    	item = (tWirelessPackage*) (rb->bufferStart + (itemNumber * rb->itemSize));
    }
    else /* item is linearly inside array, not over the edge */
    {
    	item = (tWirelessPackage*) (rb->tail + (itemNumber * rb->itemSize));
    }
    return pdTRUE;
}

/*!
* \fn BaseType_t rb_pop_front(tRingBuf *rb, void *item)
* \brief Pops an element from the front of the ringbuffer
* \param rb: pointer to ringbuffer
* \param item: pointer to element where popped element will be stored
* \return Status if pop has been successful
*/
BaseType_t rb_popFromIndex(tRingBuf *rb, tWirelessPackage* item, uint16_t itemNumber)
{
    if((rb->maxNumElements < itemNumber) || (rb->count <= 0)) /* invalid itemNumber or empty buffer? */
    {
        return pdFAIL;
    }
    if(itemNumber == 0) /* Popping from front? */
    {
    	return rb_popFromFront(rb, item); /* updates tail */
    }
    if(rb->tail + (itemNumber * rb->itemSize) >= rb->bufferEnd) /* item is at beginning of ringbuffer (over the edge) -> start over with counter */
    {
    	itemNumber = ((rb->tail + itemNumber * rb->itemSize) - rb->bufferEnd) / rb->itemSize;
    	item = (tWirelessPackage*) (rb->bufferStart + (itemNumber * rb->itemSize));
    }
    else /* item is linearly inside array, not over the edge */
    	item = (tWirelessPackage*) (rb->tail + (itemNumber * rb->itemSize));
    if(item == NULL) /* no element inside at this index */
    	return pdFAIL;
    rb->count--;
    return pdTRUE;
}
