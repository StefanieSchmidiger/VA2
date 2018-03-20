#include "ThroughputPrintout.h"
#include "Shell.h"
#include "FRTOS.h"
#include "Config.h"
#include <stdio.h> // sprintf
#include "XF1.h" //xsprintf

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
long unsigned int numberOfDroppedBytes[NUMBER_OF_UARTS];
long unsigned int numberOfInvalidPackages[NUMBER_OF_UARTS];

void throughputPrintout_TaskEntry(void* p)
{
	const TickType_t taskInterval = pdMS_TO_TICKS(config.ThroughputPrintoutTaskInterval*1000); /* task interval in seconds, but configured in milliseconds */
	TickType_t lastWakeTime = xTaskGetTickCount(); /* Initialize the xLastWakeTime variable with the current time. */
	static char buf[150];
	int res;
	/* to calculate averages */
	static float averagePayloadReceived[NUMBER_OF_UARTS];
	static float averagePayloadSent[NUMBER_OF_UARTS];
	static float averagePacksSent[NUMBER_OF_UARTS];
	static float averagePacksReceived[NUMBER_OF_UARTS];
	static float averageAcksSent[NUMBER_OF_UARTS];
	static float averageAcksReceived[NUMBER_OF_UARTS];
	/* so the global variables do not have to be reset */
	static long unsigned int lastNumberOfPacksReceived[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfPacksSent[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfAckReceived[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfAcksSent[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfPayloadBytesExtracted[NUMBER_OF_UARTS];
	static long unsigned int lastNumberOfPayloadBytesSent[NUMBER_OF_UARTS];

	for(;;)
	{
		vTaskDelayUntil( &lastWakeTime, taskInterval ); /* Wait for the next cycle */
		/* calculate throughput in byte per sec */
		for(int cnt = 0; cnt < NUMBER_OF_UARTS; cnt++)
		{
			averagePacksReceived[cnt] = (numberOfPacksReceived[cnt]-lastNumberOfPacksReceived[cnt])/config.ThroughputPrintoutTaskInterval;
			averagePacksSent[cnt] = (numberOfPacksSent[cnt]-lastNumberOfPacksSent[cnt]) / config.ThroughputPrintoutTaskInterval;
			averageAcksReceived[cnt] = (numberOfAckReceived[cnt]-lastNumberOfAckReceived[cnt])/config.ThroughputPrintoutTaskInterval;
			averageAcksSent[cnt] = (numberOfAcksSent[cnt]-lastNumberOfAcksSent[cnt])/config.ThroughputPrintoutTaskInterval;
			averagePayloadReceived[cnt] = (numberOfPayloadBytesExtracted[cnt]-lastNumberOfPayloadBytesExtracted[cnt])/(numberOfPacksReceived[cnt]-lastNumberOfPacksReceived[cnt]);
			averagePayloadSent[cnt] = (numberOfPayloadBytesSent[cnt]-lastNumberOfPayloadBytesSent[cnt])/(numberOfPacksSent[cnt]-lastNumberOfPacksSent[cnt]);
		}

		res = XF1_xsprintf(buf, "-----------------------------------------------------------\r\n");
		res = pushMsgToShellQueue(buf);
		/* print throughput information */
		res = XF1_xsprintf(buf, "Wireless: Sent packages [packages/s]: %.1f,%.1f,%.1f,%.1f; Received packages: [packages/s] %.1f,%.1f,%.1f,%.1f\r\n",
				averagePacksSent[0], averagePacksSent[1], averagePacksSent[2], averagePacksSent[3],
				averagePacksReceived[0], averagePacksReceived[1], averagePacksReceived[2], averagePacksReceived[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Wireless: Average payload sent [bytes/pack]: %.1f,%.1f,%.1f,%.1f; Average payload received: [bytes/pack] %.1f,%.1f,%.1f,%.1f\r\n",
				averagePayloadSent[0], averagePayloadSent[1], averagePayloadSent[2], averagePayloadSent[3],
				averagePayloadReceived[0], averagePayloadReceived[1], averagePayloadReceived[2], averagePayloadReceived[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Wireless: Sent acknowledges [acks/s]: %.1f,%.1f,%.1f,%.2f; Received acknowledges: [acks/s] %.1f,%.1f,%.1f,%.1f\r\n",
				averageAcksSent[0], averageAcksSent[1], averageAcksSent[2], averageAcksSent[3],
				averageAcksReceived[0], averageAcksReceived[1], averageAcksReceived[2], averageAcksReceived[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Wireless: Total number of dropped packages per device input: %lu,%lu,%lu,%lu\r\n",
				numberOfDroppedPackages[0], numberOfDroppedPackages[1], numberOfDroppedPackages[2], numberOfDroppedPackages[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Wireless: Total number of dropped acknowledges per wireless input: %lu,%lu,%lu,%lu\r\n",
				numberOfDroppedAcks[0], numberOfDroppedAcks[1], numberOfDroppedAcks[2], numberOfDroppedAcks[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Wireless: Total number of invalid packages per wireless input: %lu,%lu,%lu,%lu\r\n",
				numberOfInvalidPackages[0], numberOfInvalidPackages[1], numberOfInvalidPackages[2], numberOfInvalidPackages[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Device: Total number of dropped bytes per device input: %lu,%lu,%lu,%lu\r\n",
				numberOfDroppedBytes[0], numberOfDroppedBytes[1], numberOfDroppedBytes[2], numberOfDroppedBytes[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Wireless: Total number of sent bytes per connection: %lu,%lu,%lu,%lu\r\n",
				numberOfPayloadBytesSent[0], numberOfPayloadBytesSent[1], numberOfPayloadBytesSent[2], numberOfPayloadBytesSent[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "Wireless: Total number of received bytes per connection: %lu,%lu,%lu,%lu\r\n",
				numberOfPayloadBytesExtracted[0], numberOfPayloadBytesExtracted[1], numberOfPayloadBytesExtracted[2], numberOfPayloadBytesExtracted[3]);
		res = pushMsgToShellQueue(buf);

		res = XF1_xsprintf(buf, "-----------------------------------------------------------\r\n");
		res = pushMsgToShellQueue(buf);

		/* reset measurement */
		for(int cnt = 0; cnt < NUMBER_OF_UARTS; cnt++)
		{
			lastNumberOfPacksReceived[cnt] = numberOfPacksReceived[cnt];
			lastNumberOfPacksSent[cnt] = numberOfPacksSent[cnt];
			lastNumberOfAckReceived[cnt] = numberOfAckReceived[cnt];
			lastNumberOfAcksSent[cnt] = numberOfAcksSent[cnt];
			lastNumberOfPayloadBytesExtracted[cnt] = numberOfPayloadBytesExtracted[cnt];
			lastNumberOfPayloadBytesSent[cnt] = numberOfPayloadBytesSent[cnt];
		}
	}
}
