
#include "SPI.h"
#include "LedOrange.h"
#include "SpiHandler.h" // queues, tasks, semaphores
#include "XF1.h" // xsprintf
#include <string.h> // strlen
#include "Config.h" // baudrates
#include "nResetDeviceSide.h" // pin configuration
#include "nResetWirelessSide.h" // pin configuration
#include "nIrqDeviceSide.h" // pin configuration
#include "nIrqWirelessSide.h" // pin configuration
#include "Shell.h" // to print out debug information
#include "ThroughputPrintout.h" //to store debug information
#include "Golay.h"
#include "PackageHandler.h" // for PACK_FILL in golay decoding
#include "Platform.h"
#include "Logger.h"

#if PL_HAS_PERCEPIO
#include "PTRC1.h"
#endif

#define CS_DEVICE 			0
#define CS_WIRELESS 		1

#define WRITE_TRANSFER 		true
#define READ_TRANSFER 		false
#define SINGLE_BYTE			1
#define NUMBER_OF_EVENT_CHANNELS (7)

/* global variables, only used in this file */
#if PL_HAS_PERCEPIO
traceString userEvent[NUMBER_OF_EVENT_CHANNELS];
#endif
LDD_TDeviceData* spiDevice; /* For different functions to access SPI device */
static xQueueHandle TxWirelessBytes[NUMBER_OF_UARTS]; /* Incoming data from wireless side stored here */
static xQueueHandle RxWirelessBytes[NUMBER_OF_UARTS]; /* Outgoing data to wireless side stored here */
static xQueueHandle TxDeviceBytes[NUMBER_OF_UARTS]; /* Incoming data from device side stored here */
static xQueueHandle RxDeviceBytes[NUMBER_OF_UARTS];  /* Outgoing data to device side stored here */
const char* queueNameRxWirelessBytes[] = {"RxWirelessBytes0", "RxWirelessBytes1", "RxWirelessBytes2", "RxWirelessBytes3"};
const char* queueNameRxDeviceBytes[] = {"RxDeviceBytes0", "RxDeviceBytes1", "RxDeviceBytes2", "RxDeviceBytes3"};
const char* queueNameTxWirelessBytes[] = {"TxWirelessBytes0", "TxWirelessBytes1", "TxWirelessBytes2", "TxWirelessBytes3"};
const char* queueNameTxDeviceBytes[] = {"TxDeviceBytes0", "TxDeviceBytes1", "TxDeviceBytes2", "TxDeviceBytes3"};
#if USE_SEMAPHORES_INSTEAD_OF_FLAGS
xSemaphoreHandle spiRxMutex; /* Semaphore given in SPI_OnBlockReceived */
xSemaphoreHandle spiTxMutex; /* Semaphore given in SPI_OnBlockSent */
#else
volatile bool spiRxDone; /* Semaphore given in SPI_OnBlockReceived */
volatile bool spiTxDone; /* Semaphore given in SPI_OnBlockSent */
#endif

/* prototypes, only used in this file */
void spiHandler_TaskInit(void);
bool spiTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg, bool write, uint8_t* pData, uint8_t numOfTransfers);
void spiWriteToAllUartInterfaces(tMax14830Reg reg, uint8_t data);
bool spiSingleWriteTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg, uint8_t data);
uint8_t spiSingleReadTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg);
void configureHwBufBaudrate(tSpiSlaves spiSlave, tUartNr uartNr, unsigned int baudRateToSet);
void initSpiHandlerQueues(void);
static uint16_t readHwBufAndWriteToQueue(tSpiSlaves spiSlave, tUartNr uartNr, xQueueHandle queue);
static uint16_t readQueueAndWriteToHwBuf(tSpiSlaves spiSlave, tUartNr uartNr, xQueueHandle queue, uint16_t numOfBytesToWrite);
static void generateDebugData(xQueueHandle queue, uint8_t uartNr);

/*!
* \fn void spiHandler_TaskEntry(void)
* \brief Task initializes SPI, used queues and MAX14830.
* Periodically reads and writes bytes to and from byte queue.
*/
void spiHandler_TaskEntry(void* p)
{
	const TickType_t taskInterval = pdMS_TO_TICKS(config.SpiHandlerTaskInterval);
	TickType_t lastWakeTime = xTaskGetTickCount(); /* Initialize the xLastWakeTime variable with the current time. */

	for(;;)
	{
		/* Wait for the next cycle */
		vTaskDelayUntil( &lastWakeTime, taskInterval );
		/* read all data and write it to queue */
		for(int uartNr = 0; uartNr < NUMBER_OF_UARTS; uartNr++)
		{
			//vTracePrint(userEvent[0], "6");
			/* read data from device spi interface */
			if(config.EnableStressTest)
			{
				generateDebugData(RxDeviceBytes[uartNr], uartNr);
			}
			else
			{
				numberOfRxBytesHwBuf[MAX_14830_DEVICE_SIDE][uartNr] += readHwBufAndWriteToQueue(MAX_14830_DEVICE_SIDE, uartNr, RxDeviceBytes[uartNr]);
			}

			/* write data from queue to device spi interface */
			if(config.TestHwLoopbackOnly)
			{
				readQueueAndWriteToHwBuf(MAX_14830_DEVICE_SIDE, uartNr, RxDeviceBytes[uartNr], HW_FIFO_SIZE);
			}
			else
			{
				numberOfTxBytesHwBuf[MAX_14830_DEVICE_SIDE][uartNr] += readQueueAndWriteToHwBuf(MAX_14830_DEVICE_SIDE, uartNr, TxDeviceBytes[uartNr], uxQueueMessagesWaiting(TxDeviceBytes[uartNr]) );
			}

			/* read data from wireless spi interface */
			numberOfRxBytesHwBuf[MAX_14830_WIRELESS_SIDE][uartNr] += readHwBufAndWriteToQueue(MAX_14830_WIRELESS_SIDE, uartNr, RxWirelessBytes[uartNr]);
			/* write data from queue to wireless spi interface */
			if(config.TestHwLoopbackOnly)
			{
				readQueueAndWriteToHwBuf(MAX_14830_WIRELESS_SIDE, uartNr, RxWirelessBytes[uartNr], HW_FIFO_SIZE);
			}
			else
			{
				numberOfTxBytesHwBuf[MAX_14830_WIRELESS_SIDE][uartNr] += readQueueAndWriteToHwBuf(MAX_14830_WIRELESS_SIDE, uartNr, TxWirelessBytes[uartNr], uxQueueMessagesWaiting(TxWirelessBytes[uartNr]) );
			}
			//vTracePrint(userEvent[0], "8");
		}
	}
}


/*!
* \fn void spiHandler_TaskInit(void)
* \brief Initializes all components used in spiHandler_TaskEntry(): Queues, SPI, Semaphores and gets MAX14830 ready for usage
*/
void spiHandler_TaskInit(void)
{
	spiDevice = SPI_Init(NULL); /* no auto-init in SPIMaster_LDD used because this variable is needed for ChangeConfiguration */
	initSpiHandlerQueues();
#if USE_SEMAPHORES_INSTEAD_OF_FLAGS
	spiRxMutex = FRTOS_xSemaphoreCreateBinary(); /* Waits on Spi_ReceiveBlock */
	if(spiRxMutex == NULL)
		while(true){} /* malloc for semaphore failed */
	spiTxMutex = FRTOS_xSemaphoreCreateBinary(); /* Waits for previous Spi_SendBlock to finish */
	if(spiTxMutex == NULL)
		while(true){} /* malloc for semaphore failed */

	/* http://www.freertos.org/xSemaphoreCreateBinary.html:
	 * The semaphore is created in the 'empty' state, meaning the semaphore must first be given using the xSemaphoreGive()
	 * before it can subsequently be taken (obtained) using the xSemaphoreTake() function. */
	xSemaphoreGive(spiTxMutex); /* make sure Semaphore is given by default and can be taken for first SPI_send() */

#else
	spiRxDone = true;
	spiTxDone = true;
#endif

	/*
	Initialize MAX14830's:
	- Reset (pull reset signal down)
	- Wait until IRQ is high (after this, MAX14830's communication interface is ready)
	- Set BRGConfig to 0 and CLKSource[CrystalEn] to 1 to select crystal as clock source
	- Wait on STSInt (clock stable)
	- Program UART baud rates
	*/

	/* Reset the two MAX14830 */
	nResetWirelessSide_SetOutput();
	nResetDeviceSide_SetOutput();
	nResetWirelessSide_PutVal(0);
	nResetDeviceSide_PutVal(0);

	vTaskDelay(pdMS_TO_TICKS(200)); /* Wait for the next cycle */

	/* There are external pull ups available, so here we switch them back in high impedance mode */
	nResetWirelessSide_PutVal(1);
	nResetDeviceSide_PutVal(1);
	nResetDeviceSide_SetInput();
	nResetWirelessSide_SetInput();


	/* Wait until both IRQ's are high (means that the MAX14830 are ready to be written to) */
#if PL_WITH_BASEBOARD
	while ((nIrqWirelessSide_GetVal() == false) || (nIrqDeviceSide_GetVal() == false))
	{
		vTaskDelay(pdMS_TO_TICKS(20)); /* Wait for the next cycle */
	}
#endif

	/* Set PLL devider and multiplier */
	spiWriteToAllUartInterfaces(MAX_REG_PLL_CONFIG, 0x06);	/* 0x06: Multiply by 6, factor 6 => freq_in*1 in PLL */

	/* Set devider in baud rate register */
	/* Input is 3.686 MHZ when PLL factor is 1. Baud rate is this clock /16 => /2 = 115200 baud; /4 = 57600; .. */
	configureHwBufBaudrate(MAX_14830_DEVICE_SIDE, MAX_UART_0, config.BaudRatesDeviceConn[0]);
	configureHwBufBaudrate(MAX_14830_DEVICE_SIDE, MAX_UART_1, config.BaudRatesDeviceConn[1]);
	configureHwBufBaudrate(MAX_14830_DEVICE_SIDE, MAX_UART_2, config.BaudRatesDeviceConn[2]);
	configureHwBufBaudrate(MAX_14830_DEVICE_SIDE, MAX_UART_3, config.BaudRatesDeviceConn[3]);
	configureHwBufBaudrate(MAX_14830_WIRELESS_SIDE, MAX_UART_0, config.BaudRatesWirelessConn[0]);
	configureHwBufBaudrate(MAX_14830_WIRELESS_SIDE, MAX_UART_1, config.BaudRatesWirelessConn[1]);
	configureHwBufBaudrate(MAX_14830_WIRELESS_SIDE, MAX_UART_2, config.BaudRatesWirelessConn[2]);
	configureHwBufBaudrate(MAX_14830_WIRELESS_SIDE, MAX_UART_3, config.BaudRatesWirelessConn[3]);

	/* configure hardware flow control (CTS only) if configured */
	spiWriteToAllUartInterfaces(MAX_REG_MODE1, 0x02);	/* TX needs to be disabled before changing anything on the CTS behaviour */
	if (config.UseCtsPerWirelessConn[0] > 0)		spiSingleWriteTransfer(MAX_14830_WIRELESS_SIDE, MAX_UART_0, MAX_REG_FLOW_CTRL, 0x02);
	if (config.UseCtsPerWirelessConn[1] > 0)		spiSingleWriteTransfer(MAX_14830_WIRELESS_SIDE, MAX_UART_1, MAX_REG_FLOW_CTRL, 0x02);
	if (config.UseCtsPerWirelessConn[2] > 0)		spiSingleWriteTransfer(MAX_14830_WIRELESS_SIDE, MAX_UART_2, MAX_REG_FLOW_CTRL, 0x02);
	if (config.UseCtsPerWirelessConn[3] > 0)		spiSingleWriteTransfer(MAX_14830_WIRELESS_SIDE, MAX_UART_3, MAX_REG_FLOW_CTRL, 0x02);
	//spiWriteToAllUartInterfaces(MAX_REG_MODE1, 0x00);	/* enable TX again */

	/* PLL bypass disable, PLL enable, external crystal enable */
	spiWriteToAllUartInterfaces(MAX_REG_CLK_SOURCE, 0x06);

	/* Set word length and number of stop bits */
	spiWriteToAllUartInterfaces(MAX_REG_LCR, 0x03);

#if PL_HAS_PERCEPIO
	userEvent[0] = xTraceRegisterString("dropping bytes in queue");
	userEvent[1] = xTraceRegisterString("readQueueAndWriteToHwBuf");
	userEvent[2] = xTraceRegisterString("readHwBufAndWriteToQueue");
	userEvent[3] = xTraceRegisterString("before for-loop");
	userEvent[4] = xTraceRegisterString("popping one byte");
	userEvent[5] = xTraceRegisterString("queue empty");
#endif
}

/*!
* \fn void initQueues(void)
* \brief This function initializes the array of queues
*/
void initSpiHandlerQueues(void)
{

#if configSUPPORT_STATIC_ALLOCATION
	/* The variable used to hold the queue's data structure. */
	static uint8_t /* __attribute__((section (configHEAP_SECTION_NAME_STRING))) */ xStaticQueueRxWlBytes[NUMBER_OF_UARTS][ BYTE_QUEUE_SIZE * sizeof(uint8_t) ];
	static uint8_t /* __attribute__((section (configHEAP_SECTION_NAME_STRING))) */ xStaticQueueTxWlBytes[NUMBER_OF_UARTS][ BYTE_QUEUE_SIZE * sizeof(uint8_t) ];
	static uint8_t xStaticQueueRxDevBytes[NUMBER_OF_UARTS][ BYTE_QUEUE_SIZE * sizeof(uint8_t) ];
	static uint8_t xStaticQueueTxDevBytes[NUMBER_OF_UARTS][ BYTE_QUEUE_SIZE * sizeof(uint8_t) ];
	/* The array to use as the queue's storage area. */
	static StaticQueue_t ucQueueStorageRxWlBytes[NUMBER_OF_UARTS];
	static StaticQueue_t ucQueueStorageRxDevBytes[NUMBER_OF_UARTS];
	static StaticQueue_t ucQueueStorageTxWlBytes[NUMBER_OF_UARTS];
	static StaticQueue_t ucQueueStorageTxDevBytes[NUMBER_OF_UARTS];
#endif

	for(int uartNr=0; uartNr<NUMBER_OF_UARTS; uartNr++)
	{
#if configSUPPORT_STATIC_ALLOCATION
		RxWirelessBytes[uartNr] = xQueueCreateStatic( BYTE_QUEUE_SIZE, sizeof(uint8_t), xStaticQueueRxWlBytes[uartNr], &ucQueueStorageRxWlBytes[uartNr]); /* bytes received on wireless side */
		RxDeviceBytes[uartNr] = xQueueCreateStatic( BYTE_QUEUE_SIZE, sizeof(uint8_t), xStaticQueueRxDevBytes[uartNr], &ucQueueStorageRxDevBytes[uartNr]); /* bytes received on device side */
		TxWirelessBytes[uartNr] = xQueueCreateStatic( BYTE_QUEUE_SIZE, sizeof(uint8_t), xStaticQueueTxWlBytes[uartNr], &ucQueueStorageTxWlBytes[uartNr]); /* bytes sent out on wireless side */
		TxDeviceBytes[uartNr] = xQueueCreateStatic( BYTE_QUEUE_SIZE, sizeof(uint8_t), xStaticQueueTxDevBytes[uartNr], &ucQueueStorageTxDevBytes[uartNr]); /* bytes sent out on device side */
#else
		RxWirelessBytes[uartNr] = xQueueCreate( BYTE_QUEUE_SIZE, sizeof(uint8_t)); /* bytes received on wireless side */
		RxDeviceBytes[uartNr] = xQueueCreate( BYTE_QUEUE_SIZE, sizeof(uint8_t)); /* bytes received on device side */
		TxWirelessBytes[uartNr] = xQueueCreate( BYTE_QUEUE_SIZE, sizeof(uint8_t)); /* bytes sent out on wireless side */
		TxDeviceBytes[uartNr] = xQueueCreate( BYTE_QUEUE_SIZE, sizeof(uint8_t)); /* bytes sent out on device side */
#endif
		if(RxWirelessBytes[uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(RxWirelessBytes[uartNr], queueNameRxWirelessBytes[uartNr]);

		if(RxDeviceBytes[uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(RxDeviceBytes[uartNr], queueNameRxDeviceBytes[uartNr]);

		if(TxWirelessBytes[uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(TxWirelessBytes[uartNr], queueNameTxWirelessBytes[uartNr]);

		if(TxDeviceBytes[uartNr] == NULL)
			while(true){} /* malloc for queue failed */
		vQueueAddToRegistry(TxDeviceBytes[uartNr], queueNameTxDeviceBytes[uartNr]);
	}
}

/*!
* \fn uint8_t spiSingleReadTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg)
* \brief Reads a single byte from the desired register.
* \param spiSlave: SPI slave where the data should be written to.
* \param uartNr: The number of the UART of the MAX14830 chip.
* \param reg: The register the data should be written to.
* \return Data that was read from the desired register.
*/
uint8_t spiSingleReadTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg)
{
	static uint8_t readData[2]; /* spi transfer needs one extra byte for commando */
	spiTransfer(spiSlave, uartNr, reg, READ_TRANSFER, readData, SINGLE_BYTE);
	return readData[1]; /* readData[0] holds answer to commando byte and no data for user */
}



/*!
* \fn bool spiSingleWriteTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg, uint8_t data)
* \brief Writes a single byte to the desired register.
* \param spiSlave: SPI slave where the data should be written to.
* \param uartNr: The number of the UART of the MAX14830 chip.
* \param reg: The register the data should be written to.
* \param data: Data byte that is written to the corresponding register.
* \return true if the data could be written, false otherwise.
*/
bool spiSingleWriteTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg, uint8_t data)
{
	static uint8_t writeData[2];
	writeData[1] = data; /* write[0] will be filled with commando */
	return spiTransfer(spiSlave, uartNr, reg, WRITE_TRANSFER, writeData, SINGLE_BYTE);
}



/*!
* \fn void spiWriteToAllUartInterfaces(bool write, tMax14830Reg reg, uint8_t data)
* \brief SPI transfer that writes the corresponding data to all UART interfaces.
* \param reg: The register the data should be written to.
* \param data: Data that should be written to the corresponding register.
*/
void spiWriteToAllUartInterfaces(tMax14830Reg reg, uint8_t data)
{
	for(int uartNr = 0; uartNr < NUMBER_OF_UARTS; uartNr ++)
	{
		spiSingleWriteTransfer(MAX_14830_WIRELESS_SIDE, uartNr, reg, data);
		spiSingleWriteTransfer(MAX_14830_DEVICE_SIDE, uartNr, reg, data);
	}
}



/*!
* \fn bool spiTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg, bool write, uint8_t* pData, uint8_t numOfTransfers)
* \brief Writes numOfTransfers bytes to the desired register, in the desired SPI slave and UART interface.
* \param spiSlave: SPI slave where the data should be written to.
* \param uartNr: The number of the UART of the MAX14830 chip.
* \param reg: The register the data should be written to.
* \param write: Set to true to write to the SPI slave, false to read.
* \param pData: This array holds read/write data. Important: needs to be 1 byte bigger than numOfTransfers! Leave Byte0 empty! Commando will be stored in that byte!
* \param numOfTransfers: Number of transfers. If bigger than 1, an SPI burst access will be performed. x * 16 Bit! Not in bytes!
* \return true if the data could be written/read, false otherwise.
*/
bool spiTransfer(tSpiSlaves spiSlave, tUartNr uartNr, tMax14830Reg reg, bool write, uint8_t* pData, uint8_t numOfTransfers)
{
	uint8_t transferCnt = 0;
	int maxDelay = 5; // [ms] ToDo: make relative to selected baud rate!
	static uint8_t data[HW_FIFO_SIZE + 1]; /* MAX14830 can hold a maximum of HW_FIFO_SIZE bytes, 1 byte is command. Make static to ensure memory for SPI is still here when SPI master actually sends the data! */
	/* See MAX14830 data sheet; 16 bits per word:
	* 15 (MSB): W/!R
	* 14: UART number, bit 1
	* 13: UART number, bit 0
	* 12: Register address, bit 4
	* 11: Register address, bit 3
	* 10: Register address, bit 2
	* 9: Register address, bit 1
	* 8: Register address, bit 0
	* 7: Data, bit 7
	* 6: Data, bit 6
	* 5: Data, bit 5
	* 4: Data, bit 4
	* 3: Data, bit 3
	* 2: Data, bit 2
	* 1: Data, bit 1
	* 0 (LSB): Data, bit 0 */
	if ((numOfTransfers < 1) || (pData == NULL)) /* faulty function call? */
	{
		return false;
	}

	/* Add 1 byte of overhead for MAX14830 commando */
	if(write)
	{
		pData[0] = 0x80; /* enable write bit */
		pData[0] |= (uartNr << 5);
		pData[0] |= (0x1F & reg);
	}
	else /* read transfer */
	{
		data[0] = 0x0; /* disable write bit */
		data[0] |= (uartNr << 5);
		data[0] |= (0x1F & reg);
	}

	/* Select the correct slave */
	if (spiSlave == MAX_14830_WIRELESS_SIDE)
	{
		SPI_SelectConfiguration(spiDevice, CS_WIRELESS, 0);
	}
	else if (spiSlave == MAX_14830_DEVICE_SIDE)
	{
		SPI_SelectConfiguration(spiDevice, CS_DEVICE, 0);
	}
	else
	{
		/* invalid/not implemented argument */
		return false;
	}

	/* transfer data out over SPI */
	if (write)
	{
		/* write transfer */
#if USE_SEMAPHORES_INSTEAD_OF_FLAGS
		xSemaphoreTake(spiTxMutex, maxDelay / portTICK_PERIOD_MS); /* wait for last write to complete */
#else
		while(spiTxDone != true);
		spiTxDone = false;
#endif
		SPI_SendBlock(spiDevice, pData, numOfTransfers+1); /* wait for SPI to get ready for sending, add 1 byte for command */
	}
	else
	{
		/* read transfer */
#if USE_SEMAPHORES_INSTEAD_OF_FLAGS
		xSemaphoreTake(spiTxMutex, maxDelay / portTICK_PERIOD_MS); /* wait for last write to complete */
#else
		while(spiTxDone != true);
		spiTxDone = false;
#endif
		SPI_ReceiveBlock(spiDevice, pData, numOfTransfers+1); /* add 1 byte for command */
		SPI_SendBlock(spiDevice, data, numOfTransfers+1); /* add 1 byte for command */
#if USE_SEMAPHORES_INSTEAD_OF_FLAGS
		xSemaphoreTake(spiRxMutex, maxDelay / portTICK_PERIOD_MS); /* wait for read to be completed so that variable holds valid information when returning from this function */
#else
		while(spiRxDone != true);
		spiRxDone = false;
#endif
	}
	return true;
}


/*!
* \fn void configureHwBufBaudrate(tSpiSlaves spiSlave, tUartNr uartNr, unsigned int baudRateToSet)
* \brief Configures the desired baud rate at the chosen serial connection.
* \param spiSlave: SPI slave that should be configured.
* \param uartNr: UART number that should be configured.
* \param baudRateToSet: The baud rate to set for this serial interface. See implementation for supported baud rates.
*/
void configureHwBufBaudrate(tSpiSlaves spiSlave, tUartNr uartNr, unsigned int baudRateToSet)
{
	char infoBuf[100];
	switch (baudRateToSet)
	{
	case 115200:
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVLSB, 0x02);
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVMSB, 0x00);
		break;
	case 57600:
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVLSB, 0x04);
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVMSB, 0x00);
		break;
	case 38400:
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVLSB, 0x06);
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVMSB, 0x00);
		break;
	case 9600:
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVLSB, 0x18);
		spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_DIVMSB, 0x00);
		break;
	default:
		XF1_xsprintf(infoBuf, "Warning: Unsupported baud rate on %s side at UART number %u\r\n", spiSlave == MAX_14830_WIRELESS_SIDE ? "wireless" : "device", (unsigned int)uartNr);
		pushMsgToShellQueue(infoBuf);
		LedOrange_On();
		return;
	}
	XF1_xsprintf(infoBuf, "Info: Set baud rate for UART %u on %s side: %u baud\r\n", (unsigned int)uartNr, spiSlave == MAX_14830_WIRELESS_SIDE ? "wireless" : "device", baudRateToSet);
	pushMsgToShellQueue(infoBuf);
}




/*!
* \fn static void readHwBufAndWriteToQueue(tSpiSlaves spiSlave, tUartNr uartNr, Queue* queuePtr)
* \brief Reads all data from the hardware FIFO of the chosen interface and puts the data into the chosen queue.
* \param spiSlave: SPI slave the data should be read from.
* \param uartNr: UART number the data should be read from.
* \param queuePtr: Pointer to the queue where the read data should be written.
* \return The number of read bytes.
*/
static uint16_t readHwBufAndWriteToQueue(tSpiSlaves spiSlave, tUartNr uartNr, xQueueHandle queue)
{
	static uint32_t timestampLastHwBufRead[NUMBER_OF_UARTS];
	static uint8_t buffer[HW_FIFO_SIZE+1]; /* needs to be one byte bigger just in case we read HW_FIFO_SIZE number of bytes -> one additional byte received for sending command byte */
	static uint8_t encodedBuf[HW_FIFO_SIZE+1];
	uint16_t nofReadBytesToProcess = 0;
	uint16_t totalNofReadBytes = 0;
	uint8_t nofBytesInHwBuf = 0;
	uint8_t nofLoopIterations = 0;
	int freeSpaceInQueue = 0;

	while((totalNofReadBytes < BYTE_QUEUE_SIZE) && (nofLoopIterations < 2))
	{
		/* check how many characters there are to read in the hardware buffer */
		nofBytesInHwBuf = spiSingleReadTransfer(spiSlave, uartNr, MAX_REG_RX_FIFO_LVL);
		if (nofBytesInHwBuf == 0) /* hw buffer empty */
		{
			break; /* nothing left to read, leave this loop */
		}
		else if (nofBytesInHwBuf >= BYTE_QUEUE_SIZE) /* there is more data to read than there is space in the queue */
		{
			nofReadBytesToProcess = BYTE_QUEUE_SIZE;  /* just read as much as can possibly be stored in one read-cycle */
			char infoBuf[100];
			XF1_xsprintf(infoBuf, "SPI Handler read 128 Bytes from %s side, UART number %u, some bytes were probably lost before\r\n",  spiSlave == MAX_14830_WIRELESS_SIDE ? "wireless":"device", (unsigned int)uartNr);
			pushMsgToShellQueue(infoBuf);
		}
		else /* the queue is possibly big enough to hold all read data */
		{
			nofReadBytesToProcess = nofBytesInHwBuf; /* read all the available data */
		}

		/* read byte data from hw buffer */
		if(spiSlave == MAX_14830_WIRELESS_SIDE && config.UseGolayPerWlConn[uartNr]) /* read and decode if Golay enabled */
		{
			/*
			There's a tradeoff here: the number of data to be decoded needs to be a multiple of 6.
			So we can either just read out as many bytes as there is multiple of 6, risking that we delay some of the bytes quite long.
			Or we can read out all chars, fill up with pseudo chars and destroy some of the codewords this way.
			=> decided to read out only multiples of 6
			*/
			if((nofReadBytesToProcess % 6) > 0) /* nof data that will be read is NOT a multiple of six */
			{
				if(xTaskGetTickCount() - timestampLastHwBufRead[uartNr] < MAX_GOLAY_DELAY_TICKS) /* the last read attempt was not too long ago? */
				{
					while((nofReadBytesToProcess % 6) > 0)			nofReadBytesToProcess--; /* read out multiples of 6 */
				}
				else {nofReadBytesToProcess=1;} /* Todo: dont do this here! only debug! dont delay bytes in hw buffer for longer than MAX_DECODING_READ_DELAY_TICKS, instead fill up with pseudo chars */
				if(nofReadBytesToProcess <= 0)
				{
					break; /* leave for-loop, dont update timestampLastHwBufRead because nothing is read */
				}
			}
			timestampLastHwBufRead[uartNr] = xTaskGetTickCount(); /* make sure to not delay last non-multiple of 6 too long */
			/* read the data from the HW buffer */
			spiTransfer(spiSlave, uartNr, MAX_REG_RHR_THR, READ_TRANSFER, encodedBuf, nofReadBytesToProcess);
#if BYTE_LOGGING_ENABLED
			/* log encoded bytes */
			for(int i = 1; i<=nofReadBytesToProcess; i++)
			{
				pushByteToLoggerQueue(encodedBuf[i], RECEIVED_PACKAGE, uartNr);
			}
#endif
			int nofErrors = golay_decode(nofReadBytesToProcess, &encodedBuf[1], &buffer[1]);
			nofReadBytesToProcess = nofReadBytesToProcess / 2; /* Golay doubled the data rate -> after decoding, only half is actual data */
		}
		else /* golay not used on this UART */
		{
			/* read data from HW buffer */
			spiTransfer(spiSlave, uartNr, MAX_REG_RHR_THR, READ_TRANSFER, buffer, nofReadBytesToProcess);
#if BYTE_LOGGING_ENABLED
			if(spiSlave == MAX_14830_WIRELESS_SIDE) /* logging only on WL side */
			{
				/* log encoded bytes */
				for(int i = 1; i<=nofReadBytesToProcess; i++)
				{
					pushByteToLoggerQueue(buffer[i], RECEIVED_PACKAGE, uartNr);
				}
			}
#endif
		}

		/* make space in queue for all bytes that were read from hw buffer */
		freeSpaceInQueue = BYTE_QUEUE_SIZE - uxQueueMessagesWaiting(queue);
		if(freeSpaceInQueue < nofReadBytesToProcess) /* not enough space in queue to save all bytes from HW buffer */
		{
#if PL_HAS_PERCEPIO
			vTracePrint(userEvent[0], "start");
#endif
			char warnBuf[80];
			/* queue is full -> delete enough old bytes to fit new ones in */
			for(int i = 0; i < nofReadBytesToProcess - freeSpaceInQueue; i++)
			{
				uint8_t data;
				if(xQueueReceive(queue, &data, ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) ) != pdTRUE)
				{
					nofReadBytesToProcess = i;
					break; /* leave for-loop and stop emptying queue */
				}
			}
#if PL_HAS_PERCEPIO
			vTracePrint(userEvent[0], "end");
#endif
			numberOfDroppedBytes[spiSlave][uartNr] += (nofReadBytesToProcess - freeSpaceInQueue);
			/* print out warning that bytes have been dropped */
			XF1_xsprintf(warnBuf, "Warning: Cleaning %u bytes on %s side, UART number %u\r\n", (unsigned int) (nofReadBytesToProcess - freeSpaceInQueue), spiSlave == MAX_14830_WIRELESS_SIDE ? "wireless":"device", (unsigned int)uartNr);
			LedOrange_On();
			pushMsgToShellQueue(warnBuf);

		}

		/* send the read data to the corresponding queue */
		/* cnt starts at 1 because buffer[0] is left empty for commando, therefore index needs to start at 1 and count up to nofReadBytesToProcess+1 */
#if PL_HAS_PERCEPIO
		vTracePrint(userEvent[2], "start");
#endif
		for (unsigned int cnt = 1; cnt < nofReadBytesToProcess+1; cnt++)
		{
			if (xQueueSendToBack(queue, &buffer[cnt], ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) ) != pdTRUE)
			{
				char infoBuf[100];
				XF1_xsprintf(infoBuf, "Warning: Not all read bytes could be sent to queue, losing %u bytes on wl %u\r\n", (nofReadBytesToProcess-cnt), uartNr);
				pushMsgToShellQueue(infoBuf);
				break;
			}
		}
#if PL_HAS_PERCEPIO
		vTracePrint(userEvent[2], "end");
#endif
		totalNofReadBytes += nofReadBytesToProcess;
		nofReadBytesToProcess = 0;
		nofLoopIterations++;
	}
	return totalNofReadBytes;
}

/*!
* \fn static void generateDebugData(xQueueHandle queue)
* \brief Pushed 10 bytes of data onto the queue passed as an argument
* \param queue: queue where debug data should be pushed to
*/
static void generateDebugData(xQueueHandle queue, uint8_t uartNr)
{
	static unsigned char cnt[NUMBER_OF_UARTS]; /* initialized with 0 per default */
	for(char i=0; i<=9; i++)
	{
		xQueueSendToBack(queue, &cnt[uartNr], ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) );
		cnt[uartNr]++;
	}
}




/*!
* \fn static void readQueueAndWriteToHwBuf(tSpiSlaves spiSlave, tUartNr uartNr, xQueueHandle queue)
* \brief Writes up to numOfBytesToWrite bytes to the hardware buffer, ruading the data from the queue.
* \param spiSlave: SPI slave the data should be written to.
* \param uartNr: UART number the data should be written to.
* \param queue: Queue where the data is stored that should be written to the HW buffer.
* \param numOfBytesToWrite: The number of bytes that should be written to the hardware buffer if there is space enough in the buffer.
* \return The number of written bytes.
*/
static uint16_t readQueueAndWriteToHwBuf(tSpiSlaves spiSlave, tUartNr uartNr, xQueueHandle queue, uint16_t numOfBytesToWrite)
{
	static uint32_t lastEncodingTimestamp[NUMBER_OF_UARTS];
	static uint32_t throughputPerWlConn[NUMBER_OF_UARTS];
	static uint32_t lastUpdateThroughput[NUMBER_OF_UARTS];
	static uint8_t buffer[HW_FIFO_SIZE+1];
	static uint8_t encodedBuf[HW_FIFO_SIZE+1];
	uint16_t cnt = 1;
	//vTracePrint(userEvent[1], "0");

	if(numOfBytesToWrite <= 0)
	{
		return 0;
	}

	/* check how much space there is left in hardware buffer */
	uint8_t spaceTakenInHwBuf = spiSingleReadTransfer(spiSlave, uartNr, MAX_REG_TX_FIFO_LVL);
	uint8_t spaceLeftInHwBuf = HW_FIFO_SIZE - spaceTakenInHwBuf;
	if(spiSlave == MAX_14830_WIRELESS_SIDE && config.UseGolayPerWlConn[uartNr]) /* golay enabled for this uart? */
	{
		spaceLeftInHwBuf /= 2; /* golay doubles the data rate */
	}

	/* reset throughput counter for WL slave every second */
	if((spiSlave == MAX_14830_WIRELESS_SIDE) && (xTaskGetTickCount()-lastUpdateThroughput[uartNr] >= pdMS_TO_TICKS(1000))) /* has 1sec passed on WL side? */
	{
		lastUpdateThroughput[uartNr] = xTaskGetTickCount(); /* save time to calculate next update event */
		throughputPerWlConn[uartNr] = 0;
	}

	/* check if there is enough space to write the number of bytes that should be written */
	numOfBytesToWrite = spaceLeftInHwBuf < numOfBytesToWrite ? spaceLeftInHwBuf : numOfBytesToWrite; /* There isn't enough space to write the desired amount of data */

	/* write those bytes */
	if (numOfBytesToWrite > 0)
	{
		if(spiSlave == MAX_14830_WIRELESS_SIDE && config.UseGolayPerWlConn[uartNr]) /* golay encoding required? */
		{
			uint16_t tmpNofBytesToWrite = numOfBytesToWrite;
			if((numOfBytesToWrite % 3) != 0)/* golay needs multiples of 3 for encoding */
			{
				numOfBytesToWrite = numOfBytesToWrite - (numOfBytesToWrite % 3); /* limit bytes to multiple of 3 so fill characters dont have to be used */
			}
			if(numOfBytesToWrite <= 0) /* there are 0, 1 or 2 characters in queue -> not multiple of 3*/
			{
				if((xTaskGetTickCount() - lastEncodingTimestamp[uartNr] < MAX_GOLAY_DELAY_TICKS) || (tmpNofBytesToWrite <= 0)) /* no timeout, we waited less than MAX_GOLAY_DELAY_TICKS for an additional character to get a multiple of 3 */
				{
					return 0;
				} /* else: numOfBytesToWrite == 1, 2 ---> restore the original numOfBytesToWrite*/
				numOfBytesToWrite = tmpNofBytesToWrite;
			}
			lastEncodingTimestamp[uartNr] = xTaskGetTickCount(); /* numOfBytesToWrite will not be multiple of 3, but we waited long enough for new queue byte */
		}
		/* put together an array that can be written to the hardware buffer */
		/* pop bytes from queue and store them in buffer array. cnt starts at 1 because buffer[0] needs to be empty for commando byte */
#if PL_HAS_PERCEPIO
		vTracePrint(userEvent[1], "start");
#endif
		for (cnt = 1; cnt < numOfBytesToWrite+1; cnt++)
		{
			/* check if max throughput reached */
			if((spiSlave == MAX_14830_WIRELESS_SIDE) && (config.MaxThroughputWirelessConn[uartNr] <= throughputPerWlConn[uartNr]))
			{
				char infoBuf[50];
				XF1_xsprintf(infoBuf, "Throughput maximum reached for WL conn %u -> hold off from sending more bytes\r\n", (unsigned int)uartNr);
				break; /* max throughput reached for this second - leave for-loop without popping more data from queue */
			}
			/* try to pop data from queue */
			if (xQueueReceive(queue, &buffer[cnt], ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) ) == pdFAIL)
			{
				//vTracePrint(userEvent[5], "1");
				break; /* queue is empty not empty, but popping failed -> leave for-loop without incrementing cnt */
			}
			throughputPerWlConn[uartNr]++;
		}
#if PL_HAS_PERCEPIO
		vTracePrint(userEvent[1], "end");
#endif

		if(spiSlave == MAX_14830_WIRELESS_SIDE && config.UseGolayPerWlConn[uartNr])
		{
			/* buffer needs to be multiple of 3, add fill chars if necessary */
			while (((cnt-1) % 3) > 0) /* (cnt-1) because cnt starts at 1 and ends at numOfBytesToWrite+1 */
			{
				buffer[cnt++] = PACK_FILL;
			}
			golay_encode(cnt-1, &buffer[1], &encodedBuf[1]); /* (cnt-1) because cnt starts at 1 and ends at numOfBytesToWrite+1 */
			cnt = ((cnt-1) * 2) + 1; /* golay doubles the data rate, cnt = (numberOfBytesToWrite + fill characters because %3 necessary)*2+1 */
		}

		/*
			From the MAX14830 data sheet:
				"Note that an error can occur in the TxFIFO when a character is written into THR at the same time as the transmitter is
				 transmitting out data via TX. In the event of this error condition, the result is that a character will not be transmitted.
				 In order to avoid this, stop the transmitter when writing data to the THR. This can be done via the TxDisable bit in the
				 MODE1 register."
			So it seems as TX needs to be disabled while writing to the FIFO (set TxDisabl to 1 in MODE1 to disable transmission).
			=> Don't do this when hardware flow control is enabled! In this case, transmitting is controlled by the CTS pin.
		*/
		if (spiSlave != MAX_14830_WIRELESS_SIDE)
		{
			spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_MODE1, 0x02);
		}
		else /* WL side */
		{
			if (config.UseCtsPerWirelessConn[uartNr])
			{
				/* When hardware flow control is enabled, disable it temporary */
				spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_FLOW_CTRL, 0x00);
			}
			else
			{
				/* only do it here when hardware flow control is disabled */
				spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_MODE1, 0x02);
			}
		}
		/* transfer data popped from queue. cnt=numberOfTransfers-1 */
		if(spiSlave == MAX_14830_WIRELESS_SIDE && config.UseGolayPerWlConn[uartNr])
		{
#if BYTE_LOGGING_ENABLED
			/* log sent encoded bytes */
			for(int i = 1; i <= cnt; i++)
			{
				pushByteToLoggerQueue(encodedBuf[i], SENT_PACKAGE, uartNr);
			}
#endif
			spiTransfer(spiSlave, uartNr, MAX_REG_RHR_THR, WRITE_TRANSFER, encodedBuf, cnt-1);
		}
		else
		{
#if BYTE_LOGGING_ENABLED
			/* log sent bytes over WL */
			if(spiSlave == MAX_14830_WIRELESS_SIDE)
			{
				for(int i = 1; i <= cnt; i++)
				{
					pushByteToLoggerQueue(buffer[i], SENT_PACKAGE, uartNr);
				}
			}
#endif
			spiTransfer(spiSlave, uartNr, MAX_REG_RHR_THR, WRITE_TRANSFER, buffer, cnt-1);
		}
		/* reenable transmission */
		if (spiSlave == MAX_14830_DEVICE_SIDE)
		{
			spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_MODE1, 0x00);
		}
		else /* MAX_14830_WIRELESS_SIDE */
		{
			if (config.UseCtsPerWirelessConn[(uint8_t)uartNr] > 0)
			{
				/* Enable hardware flow control again */
				spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_FLOW_CTRL, 0x02);
			}
			else
			{
				/* only do it here when hardware flow control is disabled */
				spiSingleWriteTransfer(spiSlave, uartNr, MAX_REG_MODE1, 0x00);
			}
		}
	}
	//vTracePrint(userEvent[1], "1");
	return cnt-1;
}



/*!
* \fn ByseType_t popFromByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t *pData)
* \brief Stores a single byte from the selected queue in pData.
* \param spiSlave: SPI slave the data should be read from.
* \param uartNr: UART number the data should be read from.
* \param pData: The location where the byte should be stored
* \return Status if xQueueReceive has been successful
*/
BaseType_t popFromByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t* pData)
{
	if(spiSlave == MAX_14830_WIRELESS_SIDE)
	{
		if(uartNr < NUMBER_OF_UARTS)
		{
			return xQueueReceive(RxWirelessBytes[uartNr], pData, ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) );
		}
	}
	else
	{
		if(uartNr < NUMBER_OF_UARTS)
		{
			return xQueueReceive(RxDeviceBytes[uartNr], pData, ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) );
		}
	}
	return pdFAIL; /* if uartNr was not in range */
}


/*!
* \fn ByseType_t pushToByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t *pData)
* \brief Stores pData in queue
* \param spiSlave: SPI slave the data should be written to.
* \param uartNr: UART number the data should be written to.
* \param pData: The location where the byte should be read
* \return Status if xQueueSendToBack has been successful
*/
BaseType_t pushToByteQueue(tSpiSlaves spiSlave, tUartNr uartNr, uint8_t* pData)
{
	if(spiSlave == MAX_14830_WIRELESS_SIDE)
	{
		if(uartNr < NUMBER_OF_UARTS)
		{
			return xQueueSendToBack(TxWirelessBytes[uartNr], pData, ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) );
		}
	}
	else
	{
		if(uartNr < NUMBER_OF_UARTS)
		{
			return xQueueSendToBack(TxDeviceBytes[uartNr], pData, ( TickType_t ) pdMS_TO_TICKS(SPI_HANDLER_QUEUE_DELAY) );
		}
	}
	return pdFAIL; /* if uartNr was not in range */
}

/*!
* \fn uint16_t numberOfBytesInRxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
* \brief Returns the number of bytes stored in the queue that are ready to be received/processed by this program
* \param uartNr: UART number the bytes should be read from.
* \param spiSlave: spiSlave byte should be read from.
* \return Number of packages waiting to be processed/received
*/
uint16_t numberOfBytesInRxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
		return (spiSlave == MAX_14830_WIRELESS_SIDE)? (uint16_t) uxQueueMessagesWaiting(RxWirelessBytes[uartNr]) :  (uint16_t) uxQueueMessagesWaiting(RxDeviceBytes[uartNr]);
	return 0; /* if uartNr was not in range */
}

/*!
* \fn uint16_t numberOfBytesInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
* \brief Returns the number of bytes stored in the queue that are ready to be sent to the corresponding MAX14830.
* \param uartNr: UART number the bytes should be transmitted to.
* \param spiSlave: spiSlave byte should be sent to.
* \return Number of bytes waiting to be sent out
*/
uint16_t numberOfBytesInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
		return (spiSlave == MAX_14830_WIRELESS_SIDE)? (uint16_t) uxQueueMessagesWaiting(TxWirelessBytes[uartNr]) :  (uint16_t) uxQueueMessagesWaiting(TxDeviceBytes[uartNr]);
	return 0; /* if uartNr was not in range */
}

/*!
* \fn uint16_t freeSpaceInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
* \brief Returns the number of bytes that can still be stored in a queue that will be sent out to the corresponding MAX14830.
* \param uartNr: UART number the bytes should be transmitted to.
* \param spiSlave: spiSlave byte should be sent to.
* \return Free space in the queue in number of bytes
*/
uint16_t freeSpaceInTxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
{
	if(uartNr < NUMBER_OF_UARTS)
		return (spiSlave == MAX_14830_WIRELESS_SIDE)? (BYTE_QUEUE_SIZE - ((uint16_t) uxQueueMessagesWaiting(TxWirelessBytes[uartNr]))) :  (BYTE_QUEUE_SIZE - ((uint16_t) uxQueueMessagesWaiting(TxDeviceBytes[uartNr])));
	return 0; /* if uartNr was not in range */
}
