#ifndef HEADERS_SPIHANDLER_H_
#define HEADERS_SPIHANDLER_H_


#include "SPI.h"
#include <stdbool.h>
#include "FRTOS.h" // queues


/*! \def USE_SEMAPHORES_INSTEAD_OF_FLAGS
*  \brief Semaphores can be used to signal when SPI block received is done, instead of boolean flags, is more costly though
*/
#define USE_SEMAPHORES_INSTEAD_OF_FLAGS (0)

/*! \def HW_FIFO_SIZE
*  \brief Number of elements that fit into the hardware FIFO.
*/
#define HW_FIFO_SIZE				128

/*! \def QUEUE_NUM_OF_CHARS_WL_TX_QUEUE
*  \brief Number of chars that should have find space within a single byte queue.
*/
#define BYTE_QUEUE_SIZE		3000 /* 16 queues of this length will be created */


/*! \def SPI_HANDLER_QUEUE_DELAY
*  \brief Number of ticks to wait on byte queue operations within this task
*/
#define SPI_HANDLER_QUEUE_DELAY				0

/*! \def MAX_DECODING_READ_DELAY_TICKS
*  \brief The golay decoder can only decode when the input is a multiple of 6.
*  This timeout specifies how long the decoder will wait for the 6 bytes to fill up before
*  filling the missing bytes up with PACK_FILL and decoding anyway
*/
#define MAX_GOLAY_DELAY_TICKS				5000



/*! \enum eMax14830Reg
*  \brief Registers of the MAX14830, see MAX14830 data sheet.
*/
typedef enum eMax14830Reg
{
	/* FIFO DATA */
	MAX_REG_RHR_THR = 0x00,
	/* INTERRUPTS */
	MAX_REG_IRQ_EN = 0x01,
	MAX_REG_ISR = 0x02,
	MAX_REG_LSR_INT_EN = 0x03,
	MAX_REG_LSR = 0x04,
	MAX_REG_SPCL_CHR_INT_EN = 0x05,
	MAX_REG_SPCL_CHAR_INT = 0x06,
	MAX_REG_STS_INT_EN = 0x07,
	MAX_REG_STS_INT = 0x08,
	/* UART MODES */
	MAX_REG_MODE1 = 0x09,
	MAX_REG_MODE2 = 0x0A,
	MAX_REG_LCR = 0x0B,
	MAX_REG_RX_TIME_OUT = 0x0C,
	MAX_REG_H_DPLX_DELAY = 0x0D,
	MAX_REG_IR_DA = 0x0E,
	/* FIFOs CONTROL */
	MAX_REG_FLOW_LVL = 0x0F,
	MAX_REG_FIFO_TRG_LVL = 0x10,
	MAX_REG_TX_FIFO_LVL = 0x11,
	MAX_REG_RX_FIFO_LVL = 0x12,
	/* FLOW CONTROL */
	MAX_REG_FLOW_CTRL = 0x13,
	MAX_REG_XON1 = 0x14,
	MAX_REG_XON2 = 0x15,
	MAX_REG_XOFF1 = 0x16,
	MAX_REG_XOFF2 = 0x17,
	/* GPIOs */
	MAX_REG_GPIO_CONFIG = 0x18,
	MAX_REG_GPIO_DATA = 0x19,
	/* CLOCK CONFIGURATION */
	MAX_REG_PLL_CONFIG = 0x1A,
	MAX_REG_BRG_CONFIG = 0x1B,
	MAX_REG_DIVLSB = 0x1C,
	MAX_REG_DIVMSB = 0x1D,
	MAX_REG_CLK_SOURCE = 0x1E,
	/* GLOBAL REGISTERS */
	MAX_REG_GLOBAL_IRQ_COMND = 0x1F,
	/* SYNCHRONIZATION REGISTERS */
	MAX_REG_TX_SYNC = 0x20,
	MAX_REG_SYNCH_DELAY_1 = 0x21,
	MAX_REG_SYNCH_DELAY_2 = 0x22,
	/* TIMER REGISTER */
	MAX_REG_TIMER_1 = 0x23,
	MAX_REG_TIMER_2 = 0x24,
	/* REVISION */
	MAX_REG_REVID = 0x25
} tMax14830Reg;


/*! \enum eUartNr
*  \brief Enumeration of the different UART interfaces within one MAX14830 chip.
*/
typedef enum eUartNr
{
	MAX_UART_0 = 0x0,
	MAX_UART_1 = 0x1,
	MAX_UART_2 = 0x2,
	MAX_UART_3 = 0x3,
	NUMBER_OF_UARTS = 0x4
} tUartNr;


/*! \enum eSpiSlaves
*  \brief Enumeration of the different available SPI slaves.
*/
typedef enum eSpiSlaves
{
	MAX_14830_WIRELESS_SIDE,
	MAX_14830_DEVICE_SIDE,
	NOF_SPI_SLAVES /* needs to be last in this enum list */
} tSpiSlaves;


#if USE_SEMAPHORES_INSTEAD_OF_FLAGS
/* Semaphore for SPI Interface -> given back in SPI_OnBlockReceived/Sent */
extern xSemaphoreHandle spiRxMutex;
extern xSemaphoreHandle spiTxMutex;
#else
extern volatile bool spiRxDone;
extern volatile bool spiTxDone;
#endif


/*!
* \fn void spiHandler_TaskEntry(void)
* \brief Task initializes SPI, used queues and MAX14830.
* Periodically reads and writes bytes to and from byte queue.
*/
void spiHandler_TaskEntry(void* p);

/*!
* \fn void spiHandler_TaskInit(void)
* \brief Initializes all components used in spiHandler_TaskEntry(): Queues, SPI, Semaphores and gets MAX14830 ready for usage
*/
void spiHandler_TaskInit(void);

/*!
* \fn ByseType_t pushToByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t *pData)
* \brief Stores pData in queue
* \param spiSlave: SPI slave the data should be written to.
* \param uartNr: UART number the data should be written to.
* \param pData: The location where the byte should be read
* \return Status if xQueueSendToBack has been successful
*/
BaseType_t pushToByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t* pData);

/*!
* \fn ByseType_t popFromByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t *pData)
* \brief Stores a single byte from the selected queue in pData.
* \param spiSlave: SPI slave the data should be read from.
* \param uartNr: UART number the data should be read from.
* \param pData: The location where the byte should be stored
* \return Status if xQueueReceive has been successful
*/
BaseType_t popFromByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t *pData);

/*!
* \fn uint16_t numberOfBytesInRxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
* \brief Returns the number of bytes stored in the queue that are ready to be received/processed by this program
* \param uartNr: UART number the bytes should be read from.
* \param spiSlave: spiSlave byte should be read from.
* \return Number of packages waiting to be processed/received
*/
uint16_t numberOfBytesInRxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr);

/*!
* \fn uint16_t numberOfBytesInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
* \brief Returns the number of bytes stored in the queue that are ready to be sent to the corresponding MAX14830.
* \param uartNr: UART number the bytes should be transmitted to.
* \param spiSlave: spiSlave byte should be sent to.
* \return Number of bytes waiting to be sent out
*/
uint16_t numberOfBytesInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr);
/*!
* \fn uint16_t freeSpaceInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
* \brief Returns the number of bytes that can still be stored in a queue that will be sent out to the corresponding MAX14830.
* \param uartNr: UART number the bytes should be transmitted to.
* \param spiSlave: spiSlave byte should be sent to.
* \return Free space in the queue in number of bytes
*/
uint16_t freeSpaceInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr);

#endif
