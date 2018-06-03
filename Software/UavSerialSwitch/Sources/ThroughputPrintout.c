#include "ThroughputPrintout.h"
#include "Shell.h"
#include "FRTOS.h"
#include "Config.h"
#include <stdio.h> // sprintf
#include "XF1.h" //xsprintf
#include "SpiHandler.h"

/* global variables */
long unsigned int numberOfAckReceived[NUMBER_OF_UARTS];
long unsigned int numberOfPacksReceived[NUMBER_OF_UARTS];
long unsigned int numberOfPayloadBytesExtracted[NUMBER_OF_UARTS];
long unsigned int numberOfPayloadBytesSent[NUMBER_OF_UARTS];
long unsigned int numberOfPacksSent[NUMBER_OF_UARTS];
long unsigned int numberOfAcksSent[NUMBER_OF_UARTS];
long unsigned int numberOfSendAttempts[NUMBER_OF_UARTS];
long unsigned int numberOfDroppedPackages[NUMBER_OF_UARTS];
long unsigned int numberOfDroppedAcks[NUMBER_OF_UARTS];
long unsigned int numberOfDroppedBytes[NOF_SPI_SLAVES][NUMBER_OF_UARTS];
long unsigned int numberOfInvalidPackages[NUMBER_OF_UARTS];
long unsigned int numberOfRxBytesHwBuf[NOF_SPI_SLAVES][NUMBER_OF_UARTS];
long unsigned int numberOfTxBytesHwBuf[NOF_SPI_SLAVES][NUMBER_OF_UARTS];

void throughputPrintout_TaskEntry(void* p)
{
	const TickType_t taskInterval = pdMS_TO_TICKS(config.ThroughputPrintoutTaskInterval_s*1000); /* task interval in seconds, but configured in milliseconds */
	TickType_t lastWakeTime = xTaskGetTickCount(); /* Initialize the xLastWakeTime variable with the current time. */
	static char buf[150];
	int res;
	/* to calculate averages */
	static unsigned int averagePayloadReceived[NUMBER_OF_UARTS];
	static unsigned int averagePayloadSent[NUMBER_OF_UARTS];
	static unsigned int averagePacksSent[NUMBER_OF_UARTS];
	static unsigned int averagePacksReceived[NUMBER_OF_UARTS];
	static unsigned int averageAcksSent[NUMBER_OF_UARTS];
	static unsigned int averageAcksReceived[NUMBER_OF_UARTS];
	static unsigned int averageUartBytesSent[NOF_SPI_SLAVES][NUMBER_OF_UARTS];
	static unsigned int averageUartBytesReceived[NOF_SPI_SLAVES][NUMBER_OF_UARTS];
	/* so the global variables do not have to be reset */
	static long unsigned int lastNumberOfPacksReceived[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfPacksSent[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfAckReceived[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfAcksSent[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfPayloadBytesExtracted[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfPayloadBytesSent[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfUartBytesSent[NOF_SPI_SLAVES][NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfUartBytesReceived[NOF_SPI_SLAVES][NUMBER_OF_UARTS];

	for(;;)
	{
		vTaskDelayUntil( &lastWakeTime, taskInterval ); /* Wait for the next cycle */
		/* calculate throughput in byte per sec */
		for(int cnt = 0; cnt < NUMBER_OF_UARTS; cnt++)
		{
			averagePacksReceived[cnt] = (numberOfPacksReceived[cnt]-lastNumberOfPacksReceived[cnt])/config.ThroughputPrintoutTaskInterval_s;
			averagePacksSent[cnt] = (numberOfPacksSent[cnt]-lastNumberOfPacksSent[cnt]) / config.ThroughputPrintoutTaskInterval_s;
			averageAcksReceived[cnt] = (numberOfAckReceived[cnt]-lastNumberOfAckReceived[cnt])/config.ThroughputPrintoutTaskInterval_s;
			averageAcksSent[cnt] = (numberOfAcksSent[cnt]-lastNumberOfAcksSent[cnt])/config.ThroughputPrintoutTaskInterval_s;
			averageUartBytesSent[MAX_14830_DEVICE_SIDE][cnt] = (unsigned int) ((numberOfTxBytesHwBuf[MAX_14830_DEVICE_SIDE][cnt] - lastNumberOfUartBytesSent[MAX_14830_DEVICE_SIDE][cnt])/config.ThroughputPrintoutTaskInterval_s);
			averageUartBytesSent[MAX_14830_WIRELESS_SIDE][cnt] = (unsigned int) ((numberOfTxBytesHwBuf[MAX_14830_WIRELESS_SIDE][cnt] - lastNumberOfUartBytesSent[MAX_14830_WIRELESS_SIDE][cnt])/config.ThroughputPrintoutTaskInterval_s);
			averageUartBytesReceived[MAX_14830_DEVICE_SIDE][cnt] = (unsigned int) ((numberOfRxBytesHwBuf[MAX_14830_DEVICE_SIDE][cnt] - lastNumberOfUartBytesReceived[MAX_14830_DEVICE_SIDE][cnt])/config.ThroughputPrintoutTaskInterval_s);
			averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][cnt] = (unsigned int) ((numberOfRxBytesHwBuf[MAX_14830_WIRELESS_SIDE][cnt] - lastNumberOfUartBytesReceived[MAX_14830_WIRELESS_SIDE][cnt])/config.ThroughputPrintoutTaskInterval_s);
			averagePayloadReceived[cnt] = (numberOfPayloadBytesExtracted[cnt]-lastNumberOfPayloadBytesExtracted[cnt])/(numberOfPacksReceived[cnt]-lastNumberOfPacksReceived[cnt]);
			averagePayloadSent[cnt] = (numberOfPayloadBytesSent[cnt]-lastNumberOfPayloadBytesSent[cnt])/(numberOfPacksSent[cnt]-lastNumberOfPacksSent[cnt]);

		}

		res = XF1_xsprintf(buf, "***************************************************************************************************** \r\n");
		res = pushMsgToShellQueue(buf);
		/* print throughput information */

		res = XF1_xsprintf(buf, "Device 0 ------>\t %lu B/s\t-------> ================= -------> \t %lu B/s\t -------> Wireless 0 \r\n",
				averageUartBytesReceived[MAX_14830_DEVICE_SIDE][0], averageUartBytesSent[MAX_14830_WIRELESS_SIDE][0]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t ||\t\t||\t\t %lu Datapack/s \r\n",
				averagePacksSent[0]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Device 1 ------>\t %lu B/s\t-------> ||             || -------> \t %lu B/s\t -------> Wireless 1 \r\n",
				averageUartBytesReceived[MAX_14830_DEVICE_SIDE][1], averageUartBytesSent[MAX_14830_WIRELESS_SIDE][1]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t ||\t\t||\t\t %lu Datapack/s \r\n",
				averagePacksSent[1]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Device 2 ------>\t %lu B/s\t-------> ||             || -------> \t %lu B/s\t -------> Wireless 2 \r\n",
				averageUartBytesReceived[MAX_14830_DEVICE_SIDE][2], averageUartBytesSent[MAX_14830_WIRELESS_SIDE][2]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t ||\t\t||\t\t %lu Datapack/s \r\n",
				averagePacksSent[2]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Device 3 ------>\t %lu B/s\t-------> ================= -------> \t %lu B/s\t -------> Wireless 3 \r\n",
				averageUartBytesReceived[MAX_14830_DEVICE_SIDE][3], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t\t\t\t\t %lu Datapack/s \r\n\r\n\r\n",
				averagePacksSent[3]);
		res = pushMsgToShellQueue(buf);


		/*******************************/
		/* print throughput information */

		res = XF1_xsprintf(buf, "Device 0 <------\t %lu B/s\t<------- ================= <------- \t %lu B/s\t <------- Wireless 0 \r\n",
				averageUartBytesSent[MAX_14830_DEVICE_SIDE][0], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][0]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t ||\t\t||\t\t %lu Datapack/s \r\n",
				averagePacksReceived[0]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Device 1 <------\t %lu B/s\t<------- ||             || <------- \t %lu B/s\t <------- Wireless 1 \r\n",
				averageUartBytesSent[MAX_14830_DEVICE_SIDE][1], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][1]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t ||\t\t||\t\t %lu Datapack/s \r\n",
				averagePacksReceived[1]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Device 2 <------\t %lu B/s\t<------- ||             || <------- \t %lu B/s\t <------- Wireless 2 \r\n",
				averageUartBytesSent[MAX_14830_DEVICE_SIDE][2], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][2]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t ||\t\t||\t\t %lu Datapack/s \r\n",
				averagePacksReceived[2]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Device 3 <------\t %lu B/s\t<------- ================= <------- \t %lu B/s\t <------- Wireless 3 \r\n",
				averageUartBytesSent[MAX_14830_DEVICE_SIDE][3], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "\t\t\t\t\t\t\t\t\t %lu Datapack/s \r\n\r\n",
				averagePacksReceived[3]);
		res = pushMsgToShellQueue(buf);


		res = XF1_xsprintf(buf, "NetworkHandler: Total number of dropped packages per device input: %lu,%lu,%lu,%lu \r\n",
				numberOfDroppedPackages[0], numberOfDroppedPackages[1], numberOfDroppedPackages[2], numberOfDroppedPackages[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "NetworkHandler: Total number of dropped acknowledges per wireless input: %lu,%lu,%lu,%lu \r\n",
				numberOfDroppedAcks[0], numberOfDroppedAcks[1], numberOfDroppedAcks[2], numberOfDroppedAcks[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "PackageHandler: Total number of invalid packages per wireless input: %lu,%lu,%lu,%lu \r\n",
				numberOfInvalidPackages[0], numberOfInvalidPackages[1], numberOfInvalidPackages[2], numberOfInvalidPackages[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SPI Handler: Total number of dropped bytes per device byte input: %lu,%lu,%lu,%lu \r\n",
				numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][0], numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][1], numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][2], numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SpiHandler: Total number of dropped bytes per wireless byte input: %lu,%lu,%lu,%lu \r\n\r\n",
				numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][0], numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][1], numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][2], numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "***************************************************************************************************** \r\n");
		res = pushMsgToShellQueue(buf);

#if 0
		res = XF1_xsprintf(buf, "NetworkHandler: Sent packages [packages/s]: %lu,%lu,%lu,%lu; Received packages: [packages/s] %lu,%lu,%lu,%lu \r\n",
				averagePacksSent[0], averagePacksSent[1], averagePacksSent[2], averagePacksSent[3],
				averagePacksReceived[0], averagePacksReceived[1], averagePacksReceived[2], averagePacksReceived[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "NetworkHandler: Average payload sent [bytes/pack]: %lu,%lu,%lu,%lu; Average payload received: [bytes/pack] %lu,%lu,%lu,%lu \r\n",
				averagePayloadSent[0], averagePayloadSent[1], averagePayloadSent[2], averagePayloadSent[3],
				averagePayloadReceived[0], averagePayloadReceived[1], averagePayloadReceived[2], averagePayloadReceived[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "NetworkHandler: Sent acknowledges [acks/s]: %lu,%lu,%lu,%lu; Received acknowledges: [acks/s] %lu,%lu,%lu,%lu \r\n",
				averageAcksSent[0], averageAcksSent[1], averageAcksSent[2], averageAcksSent[3],
				averageAcksReceived[0], averageAcksReceived[1], averageAcksReceived[2], averageAcksReceived[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SpiHandler: Average bytes read from device side[bytes/s]: %lu,%lu,%lu,%lu; Average bytes sent to device side: [bytes/s] %lu,%lu,%lu,%lu \r\n",
				averageUartBytesReceived[MAX_14830_DEVICE_SIDE][0], averageUartBytesReceived[MAX_14830_DEVICE_SIDE][1], averageUartBytesReceived[MAX_14830_DEVICE_SIDE][2], averageUartBytesReceived[MAX_14830_DEVICE_SIDE][3],
				averageUartBytesSent[MAX_14830_DEVICE_SIDE][0], averageUartBytesSent[MAX_14830_DEVICE_SIDE][1], averageUartBytesSent[MAX_14830_DEVICE_SIDE][2], averageUartBytesSent[MAX_14830_DEVICE_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SpiHandler: Average bytes read from wireless side[bytes/s]: %lu,%lu,%lu,%lu; Average bytes sent to wireless side: [bytes/s] %lu,%lu,%lu,%lu \r\n",
				averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][0], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][1], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][2], averageUartBytesReceived[MAX_14830_WIRELESS_SIDE][3],
				averageUartBytesSent[MAX_14830_WIRELESS_SIDE][0], averageUartBytesSent[MAX_14830_WIRELESS_SIDE][1], averageUartBytesSent[MAX_14830_WIRELESS_SIDE][2], averageUartBytesSent[MAX_14830_WIRELESS_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "NetworkHandler: Total number of dropped packages per device input: %lu,%lu,%lu,%lu \r\n",
				numberOfDroppedPackages[0], numberOfDroppedPackages[1], numberOfDroppedPackages[2], numberOfDroppedPackages[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "NetworkHandler: Total number of dropped acknowledges per wireless input: %lu,%lu,%lu,%lu \r\n",
				numberOfDroppedAcks[0], numberOfDroppedAcks[1], numberOfDroppedAcks[2], numberOfDroppedAcks[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "PackageHandler: Total number of invalid packages per wireless input: %lu,%lu,%lu,%lu \r\n",
				numberOfInvalidPackages[0], numberOfInvalidPackages[1], numberOfInvalidPackages[2], numberOfInvalidPackages[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SPI Handler: Total number of dropped bytes per device byte input: %lu,%lu,%lu,%lu \r\n",
				numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][0], numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][1], numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][2], numberOfDroppedBytes[MAX_14830_DEVICE_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SpiHandler: Total number of dropped bytes per wireless byte input: %lu,%lu,%lu,%lu \r\n",
				numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][0], numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][1], numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][2], numberOfDroppedBytes[MAX_14830_WIRELESS_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SPI Handler: Total number of received bytes per device byte input (x2 if golay enabled): %lu,%lu,%lu,%lu \r\n",
				numberOfRxBytesHwBuf[MAX_14830_DEVICE_SIDE][0], numberOfRxBytesHwBuf[MAX_14830_DEVICE_SIDE][1], numberOfRxBytesHwBuf[MAX_14830_DEVICE_SIDE][2], numberOfRxBytesHwBuf[MAX_14830_DEVICE_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SpiHandler: Total number of received bytes per wireless byte input (x2 if golay enabled): %lu,%lu,%lu,%lu \r\n",
				numberOfRxBytesHwBuf[MAX_14830_WIRELESS_SIDE][0], numberOfRxBytesHwBuf[MAX_14830_WIRELESS_SIDE][1], numberOfRxBytesHwBuf[MAX_14830_WIRELESS_SIDE][2], numberOfRxBytesHwBuf[MAX_14830_WIRELESS_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SPI Handler: Total number of sent bytes per device byte input (x2 if golay enabled): %lu,%lu,%lu,%lu \r\n",
				numberOfTxBytesHwBuf[MAX_14830_DEVICE_SIDE][0], numberOfTxBytesHwBuf[MAX_14830_DEVICE_SIDE][1], numberOfTxBytesHwBuf[MAX_14830_DEVICE_SIDE][2], numberOfTxBytesHwBuf[MAX_14830_DEVICE_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "SpiHandler: Total number of sent bytes per wireless byte input (x2 if golay enabled): %lu,%lu,%lu,%lu \r\n",
				numberOfTxBytesHwBuf[MAX_14830_WIRELESS_SIDE][0], numberOfTxBytesHwBuf[MAX_14830_WIRELESS_SIDE][1], numberOfTxBytesHwBuf[MAX_14830_WIRELESS_SIDE][2], numberOfTxBytesHwBuf[MAX_14830_WIRELESS_SIDE][3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "NetworkHandler: Total number of payload bytes sent over wireless: %lu,%lu,%lu,%lu \r\n",
				numberOfPayloadBytesSent[0], numberOfPayloadBytesSent[1], numberOfPayloadBytesSent[2], numberOfPayloadBytesSent[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "NetworkHandler: Total number of payload bytes received over wireless: %lu,%lu,%lu,%lu \r\n",
				numberOfPayloadBytesExtracted[0], numberOfPayloadBytesExtracted[1], numberOfPayloadBytesExtracted[2], numberOfPayloadBytesExtracted[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "----------------------------------------------------------- \r\n");
		res = pushMsgToShellQueue(buf);
#endif
		/* reset measurement */
		for(int cnt = 0; cnt < NUMBER_OF_UARTS; cnt++)
		{
			lastNumberOfPacksReceived[cnt] = numberOfPacksReceived[cnt];
			lastNumberOfPacksSent[cnt] = numberOfPacksSent[cnt];
			lastNumberOfAckReceived[cnt] = numberOfAckReceived[cnt];
			lastNumberOfAcksSent[cnt] = numberOfAcksSent[cnt];
			lastNumberOfPayloadBytesExtracted[cnt] = numberOfPayloadBytesExtracted[cnt];
			lastNumberOfPayloadBytesSent[cnt] = numberOfPayloadBytesSent[cnt];
			lastNumberOfUartBytesReceived[MAX_14830_DEVICE_SIDE][cnt] = numberOfRxBytesHwBuf[MAX_14830_DEVICE_SIDE][cnt];
			lastNumberOfUartBytesSent[MAX_14830_DEVICE_SIDE][cnt] = numberOfTxBytesHwBuf[MAX_14830_DEVICE_SIDE][cnt];
			lastNumberOfUartBytesReceived[MAX_14830_WIRELESS_SIDE][cnt] = numberOfRxBytesHwBuf[MAX_14830_WIRELESS_SIDE][cnt];
			lastNumberOfUartBytesSent[MAX_14830_WIRELESS_SIDE][cnt] = numberOfTxBytesHwBuf[MAX_14830_WIRELESS_SIDE][cnt];
		}
	}
}
