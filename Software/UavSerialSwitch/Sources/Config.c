#include "MINI.h"
#include "Config.h"
#include <stdlib.h> //atoi()
#include <stdbool.h>
#include "Platform.h"

#define TEMP_CSV_SIZE 50
#define DEFAULT_CSV_STRING "0, 0, 0, 0"
#define DEFAULT_BOOL 0
#define DEFAULT_INT 1000


/* prototypes */
void csvToInt(char inputString[], int outputArray[]);
void csvToBool(char inputString[], bool outputArray[]);
void setDefaultConfigValues(void);

/* global variables */
Configuration config;

/*!
* \fn void csvToInt(char inputString[], int outputArray[])
* \brief reads a string with format "2350, 324087, 134098,3407,23407" and returns it as array.
*  Values need to be comma separated, whitespaces are ignored and can be of any length.
* \param inputString: string where numbers should be read from
* \param outputArray: Array where parsed numbers should be stored
*/
void csvToInt(char inputString[], int outputArray[])
{
	int output;
	int indexLastDigit = 0;
	int indexFirstDigit = 0;
	int count = 0;
	for (int i=0; inputString[i]; i++)
	  count += (inputString[i] == ','); // find number of integers saved as csv
	for(int i=0; i<(count+1); i++)
	{
		while((inputString[indexFirstDigit] < '0') || (inputString[indexFirstDigit] > '9')) // fast foreward to the beginning of number
		{
			indexFirstDigit++;
			indexLastDigit++;
		}
		while((inputString[indexLastDigit] >= '0') && (inputString[indexLastDigit] <= '9')) // find the last digit that is part of one number
			indexLastDigit++;
		inputString[indexLastDigit] = '\0';
		output = atoi(&inputString[indexFirstDigit]);
		outputArray[i] = output;
		indexFirstDigit = indexLastDigit+1;
		indexLastDigit = indexLastDigit+1;
	}
}

/*!
* \fn void csvToBool(char inputString[], bool outputArray[])
* \brief reads a string with format "1, 0, 1, 0,0" and returns it as array.
*  Values need to be comma separated, whitespaces are ignored and can be of any length.
* \param inputString: string where numbers should be read from
* \param outputArray: Array where parsed numbers should be stored
*/
void csvToBool(char inputString[], bool outputArray[])
{
	int indexLastDigit = 0;
	int indexFirstDigit = 0;
	int count = 0;
	for (int i=0; inputString[i]; i++)
	  count += (inputString[i] == ','); // find number of integers saved as csv
	for(int i=0; i<(count+1); i++)
	{
		while((inputString[indexFirstDigit] < '0') || (inputString[indexFirstDigit] > '9')) // fast foreward to the beginning of number
		{
			indexFirstDigit++;
			indexLastDigit++;
		}
		while((inputString[indexLastDigit] >= '0') && (inputString[indexLastDigit] <= '9')) // find the last digit that is part of one number
			indexLastDigit++;
		inputString[indexLastDigit] = '\0';
		outputArray[i] = atoi(&inputString[indexFirstDigit]) > 0 ? true : false;
		indexFirstDigit = indexLastDigit+1;
		indexLastDigit = indexLastDigit+1;
	}
}

/*!
* \fn bool readTestConfig(void)
* \brief Reads "TestConfiguration.ini" file -> to check if above functions work correctly
* \return true if test config was read successfully
*/
bool readTestConfig(void)
{
	char sectionName[] = "TestConfiguration";
  	char intKey[] = "myInt";
  	char boolKey[] = "myBool";
  	char arrKey[] = "myArray";
  	char fooValue[] = "foo";
  	int arrSize = 30;
  	char arrStringValue[arrSize];
  	int arrInt[4];
  	char fileName[] = "TestConfig.ini";
  	long int valInt = MINI_ini_getl(sectionName, intKey, 0, fileName);
  	bool valBool = MINI_ini_getbool(sectionName, boolKey, 0, fileName);
  	int numberOfCharsCopied = MINI_ini_gets(sectionName, arrKey, fooValue, arrStringValue, arrSize, fileName);
  	csvToInt(arrStringValue, arrInt);
  	return true;
}

/*!
* \fn uint16_t numberOfBytesInRxByteQueue(tSpiSlaves spiSlave, tUartNr uartNr)
* \brief Reads config file and stores it in global variable
*/
bool readConfig(void)
{
	int tmpIntArr[NUMBER_OF_UARTS];
	int numberOfCharsCopied;
	char copiedCsv[TEMP_CSV_SIZE];
  	char fileName[] = "serialSwitch_Config.ini";

#if PL_HAS_SD_CARD
  	/* -------- BaudRateConfiguration -------- */
  	/* BAUD_RATES_WIRELESS_CONN */
  	numberOfCharsCopied = MINI_ini_gets("BaudRateConfiguration", "BAUD_RATES_WIRELESS_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.BaudRatesWirelessConn);

  	/* BAUD_RATES_DEVICE_CONN */
    numberOfCharsCopied = MINI_ini_gets("BaudRateConfiguration", "BAUD_RATES_DEVICE_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.BaudRatesDeviceConn);


  	/* -------- ConnectionConfiguration -------- */
  	/* PRIO_WIRELESS_CONN_DEV_0 */
  	numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "PRIO_WIRELESS_CONN_DEV_0",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.PrioWirelessConnDev[0]);

  	/* PRIO_WIRELESS_CONN_DEV_1 */
  	numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "PRIO_WIRELESS_CONN_DEV_1",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.PrioWirelessConnDev[1]);

  	/* PRIO_WIRELESS_CONN_DEV_2 */
  	numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "PRIO_WIRELESS_CONN_DEV_2",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.PrioWirelessConnDev[2]);

  	/* PRIO_WIRELESS_CONN_DEV_3 */
    numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "PRIO_WIRELESS_CONN_DEV_3",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.PrioWirelessConnDev[3]);

  	/* SEND_CNT_WIRELESS_CONN_DEV_0 */
  	numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "SEND_CNT_WIRELESS_CONN_DEV_0",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.SendCntWirelessConnDev[0]);

  	/* SEND_CNT_WIRELESS_CONN_DEV_1 */
  	numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "SEND_CNT_WIRELESS_CONN_DEV_1",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.SendCntWirelessConnDev[1]);

  	/* SEND_CNT_WIRELESS_CONN_DEV_2 */
  	numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "SEND_CNT_WIRELESS_CONN_DEV_2",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.SendCntWirelessConnDev[2]);

  	/* SEND_CNT_WIRELESS_CONN_DEV_3 */
    numberOfCharsCopied = MINI_ini_gets("ConnectionConfiguration", "SEND_CNT_WIRELESS_CONN_DEV_3",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
    csvToInt(copiedCsv, config.SendCntWirelessConnDev[3]);

  	/* -------- TransmissionConfiguration -------- */
  	/* PRIO_WIRELESS_CONN_DEV_0 */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "RESEND_DELAY_WIRELESS_CONN_DEV_0",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.ResendDelayWirelessConnDev[0]);

  	/* PRIO_WIRELESS_CONN_DEV_1 */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "RESEND_DELAY_WIRELESS_CONN_DEV_1",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.ResendDelayWirelessConnDev[1]);

  	/* PRIO_WIRELESS_CONN_DEV_2 */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "RESEND_DELAY_WIRELESS_CONN_DEV_2",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.ResendDelayWirelessConnDev[2]);

  	/* PRIO_WIRELESS_CONN_DEV_3 */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "RESEND_DELAY_WIRELESS_CONN_DEV_3",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.ResendDelayWirelessConnDev[3]);

  	/* MAX_THROUGHPUT_WIRELESS_CONN */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "MAX_THROUGHPUT_WIRELESS_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.MaxThroughputWirelessConn);

  	/* USUAL_PACKET_SIZE_DEVICE_CONN */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "USUAL_PACKET_SIZE_DEVICE_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.UsualPacketSizeDeviceConn);

  	/* PACKAGE_GEN_MAX_TIMEOUT */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "PACKAGE_GEN_MAX_TIMEOUT",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.PackageGenMaxTimeout);

  	/* DELAY_DISMISS_OLD_PACK_PER_DEV */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "DELAY_DISMISS_OLD_PACK_PER_DEV",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.DelayDismissOldPackagePerDev);

  	/* SEND_ACK_PER_WIRELESS_CONN */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "SEND_ACK_PER_WIRELESS_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToBool(copiedCsv, config.SendAckPerWirelessConn);

  	/* USE_CTS_PER_WIRELESS_CONN */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "USE_CTS_PER_WIRELESS_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToBool(copiedCsv, config.UseCtsPerWirelessConn);

  	/* PACK_NUMBERING_PROCESSING_MODE */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "PACK_NUMBERING_PROCESSING_MODE",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, tmpIntArr);
  	for(int i=0; i<NUMBER_OF_UARTS; i++)
  	{
  		switch(tmpIntArr[i])
  		{
  			case PACKAGE_NUMBER_IGNORED:
  				config.PackNumberingProcessingMode[i] = PACKAGE_NUMBER_IGNORED;
  		  		break;
  			case WAIT_FOR_ACK_BEFORE_SENDING_NEXT_PACK:
  				config.PackNumberingProcessingMode[i] = WAIT_FOR_ACK_BEFORE_SENDING_NEXT_PACK;
  				break;
  			case PACKAGE_REORDERING:
  				config.PackNumberingProcessingMode[i] = PACKAGE_REORDERING;
  				break;
  			case ONLY_SEND_OUT_NEW_PACKAGES:
  				config.PackNumberingProcessingMode[i] = ONLY_SEND_OUT_NEW_PACKAGES;
  				break;
  			default:
  				config.PackNumberingProcessingMode[i] = PACKAGE_NUMBER_IGNORED;
  				break;
  		}
  	}

  	/* -------- SoftwareConfiguration -------- */
  	/* TEST_HW_LOOPBACK_ONLY */
  	config.TestHwLoopbackOnly = MINI_ini_getbool("SoftwareConfiguration", "TEST_HW_LOOPBACK_ONLY",  DEFAULT_BOOL, "serialSwitch_Config.ini");

  	/* GENERATE_DEBUG_OUTPUT */
  	config.GenerateDebugOutput = MINI_ini_getbool("SoftwareConfiguration", "GENERATE_DEBUG_OUTPUT",  DEFAULT_BOOL, "serialSwitch_Config.ini");

  	/* LOGGING_ENABLED */
  	config.LoggingEnabled = MINI_ini_getl("SoftwareConfiguration", "LOGGING_ENABLED",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* SPI_HANDLER_TASK_INTERVAL */
  	config.SpiHandlerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "SPI_HANDLER_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* PACKAGE_GENERATOR_TASK_INTERVAL */
  	config.PackageHandlerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "PACKAGE_GENERATOR_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* NETWORK_HANDLER_TASK_INTERVAL */
    config.NetworkHandlerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "NETWORK_HANDLER_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* TOGGLE_GREEN_LED_INTERVAL */
  	config.ToggleGreenLedInterval = MINI_ini_getl("SoftwareConfiguration", "TOGGLE_GREEN_LED_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* THROUGHPUT_PRINTOUT_TASK_INTERVAL */
	config.ThroughputPrintoutTaskInterval = MINI_ini_getl("SoftwareConfiguration", "THROUGHPUT_PRINTOUT_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

	/* SHELL_TASK_INTERVAL */
	config.ShellTaskInterval = MINI_ini_getl("SoftwareConfiguration", "SHELL_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

	/* LOGGER_TASK_INTERVAL */
	config.LoggerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "LOGGER_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	return true;
#else
  	setDefaultConfigValues();
  	return true;
#endif
}


void setDefaultConfigValues(void)
{

  	/* -------- BaudRateConfiguration -------- */
  	/* BAUD_RATES_WIRELESS_CONN */
  	config.BaudRatesWirelessConn[0] = 9600;
  	config.BaudRatesWirelessConn[1] = 9600;
  	config.BaudRatesWirelessConn[2] = 9600;
  	config.BaudRatesWirelessConn[3] = 9600;

  	/* BAUD_RATES_DEVICE_CONN */
    config.BaudRatesDeviceConn[0] = 9600;
    config.BaudRatesDeviceConn[1] = 9600;
    config.BaudRatesDeviceConn[2] = 9600;
    config.BaudRatesDeviceConn[3] = 9600;


  	/* -------- ConnectionConfiguration -------- */
  	/* PRIO_WIRELESS_CONN_DEV_0 */
  	config.PrioWirelessConnDev[0][0] = 1;
  	config.PrioWirelessConnDev[0][1] = 0;
  	config.PrioWirelessConnDev[0][2] = 0;
  	config.PrioWirelessConnDev[0][3] = 0;


  	/* PRIO_WIRELESS_CONN_DEV_1 */
  	config.PrioWirelessConnDev[1][0] = 0;
  	config.PrioWirelessConnDev[1][1] = 1;
  	config.PrioWirelessConnDev[1][2] = 0;
  	config.PrioWirelessConnDev[1][3] = 0;

  	/* PRIO_WIRELESS_CONN_DEV_2 */
  	config.PrioWirelessConnDev[2][0] = 0;
  	config.PrioWirelessConnDev[2][1] = 0;
  	config.PrioWirelessConnDev[2][2] = 1;
  	config.PrioWirelessConnDev[2][3] = 0;

  	/* PRIO_WIRELESS_CONN_DEV_3 */
  	config.PrioWirelessConnDev[3][0] = 0;
  	config.PrioWirelessConnDev[3][1] = 0;
  	config.PrioWirelessConnDev[3][2] = 0;
  	config.PrioWirelessConnDev[3][3] = 1;

  	/* SEND_CNT_WIRELESS_CONN_DEV_0 */
  	config.SendCntWirelessConnDev[0][0] = 1;
  	config.SendCntWirelessConnDev[0][1] = 0;
  	config.SendCntWirelessConnDev[0][2] = 0;
  	config.SendCntWirelessConnDev[0][3] = 0;

  	/* SEND_CNT_WIRELESS_CONN_DEV_1 */
  	config.SendCntWirelessConnDev[1][0] = 0;
  	config.SendCntWirelessConnDev[1][1] = 1;
  	config.SendCntWirelessConnDev[1][2] = 0;
  	config.SendCntWirelessConnDev[1][3] = 0;

  	/* SEND_CNT_WIRELESS_CONN_DEV_2 */
  	config.SendCntWirelessConnDev[2][0] = 0;
  	config.SendCntWirelessConnDev[2][1] = 0;
  	config.SendCntWirelessConnDev[2][2] = 1;
  	config.SendCntWirelessConnDev[2][3] = 0;

  	/* SEND_CNT_WIRELESS_CONN_DEV_3 */
  	config.SendCntWirelessConnDev[3][0] = 0;
  	config.SendCntWirelessConnDev[3][1] = 0;
  	config.SendCntWirelessConnDev[3][2] = 0;
  	config.SendCntWirelessConnDev[3][3] = 1;

  	/* -------- TransmissionConfiguration -------- */
  	/* PRIO_WIRELESS_CONN_DEV_0 */
  	config.ResendDelayWirelessConnDev[0][0] = 5;
  	config.ResendDelayWirelessConnDev[0][1] = 5;
  	config.ResendDelayWirelessConnDev[0][2] = 5;
  	config.ResendDelayWirelessConnDev[0][3] = 5;


  	/* PRIO_WIRELESS_CONN_DEV_1 */
  	config.ResendDelayWirelessConnDev[1][0] = 5;
  	config.ResendDelayWirelessConnDev[1][1] = 5;
  	config.ResendDelayWirelessConnDev[1][2] = 5;
  	config.ResendDelayWirelessConnDev[1][3] = 5;

  	/* PRIO_WIRELESS_CONN_DEV_2 */
  	config.ResendDelayWirelessConnDev[2][0] = 5;
  	config.ResendDelayWirelessConnDev[2][1] = 5;
  	config.ResendDelayWirelessConnDev[2][2] = 5;
  	config.ResendDelayWirelessConnDev[2][3] = 5;

  	/* PRIO_WIRELESS_CONN_DEV_3 */
  	config.ResendDelayWirelessConnDev[3][0] = 5;
  	config.ResendDelayWirelessConnDev[3][1] = 5;
  	config.ResendDelayWirelessConnDev[3][2] = 5;
  	config.ResendDelayWirelessConnDev[3][3] = 5;

  	/* MAX_THROUGHPUT_WIRELESS_CONN */
  	config.MaxThroughputWirelessConn[0] = 5000;
  	config.MaxThroughputWirelessConn[1] = 5000;
  	config.MaxThroughputWirelessConn[2] = 5000;
  	config.MaxThroughputWirelessConn[3] = 5000;

  	/* USUAL_PACKET_SIZE_DEVICE_CONN */
  	config.UsualPacketSizeDeviceConn[0] = 20;
  	config.UsualPacketSizeDeviceConn[1] = 20;
  	config.UsualPacketSizeDeviceConn[2] = 20;
  	config.UsualPacketSizeDeviceConn[3] = 20;

  	/* PACKAGE_GEN_MAX_TIMEOUT */
  	config.PackageGenMaxTimeout[0] = 5;
  	config.PackageGenMaxTimeout[1] = 5;
  	config.PackageGenMaxTimeout[2] = 5;
  	config.PackageGenMaxTimeout[3] = 5;

  	/* DELAY_DISMISS_OLD_PACK_PER_DEV */
  	config.DelayDismissOldPackagePerDev[0] = 500;
  	config.DelayDismissOldPackagePerDev[1] = 500;
  	config.DelayDismissOldPackagePerDev[2] = 500;
  	config.DelayDismissOldPackagePerDev[3] = 500;

  	/* SEND_ACK_PER_WIRELESS_CONN */
  	config.SendAckPerWirelessConn[0] = 0;
  	config.SendAckPerWirelessConn[1] = 0;
  	config.SendAckPerWirelessConn[2] = 0;
  	config.SendAckPerWirelessConn[3] = 0;

  	/* USE_CTS_PER_WIRELESS_CONN */
  	config.UseCtsPerWirelessConn[0] = 0;
  	config.UseCtsPerWirelessConn[1] = 0;
  	config.UseCtsPerWirelessConn[2] = 0;
  	config.UseCtsPerWirelessConn[3] = 0;


  	/* -------- SoftwareConfiguration -------- */
  	/* TEST_HW_LOOPBACK_ONLY */
  	config.TestHwLoopbackOnly = 0;

  	/* GENERATE_DEBUG_OUTPUT */
  	config.GenerateDebugOutput = 0;

  	/* LOGGING_ENABLED */
  	config.LoggingEnabled = 0;

  	/* SPI_HANDLER_TASK_INTERVAL */
  	config.SpiHandlerTaskInterval = 5;

  	/* PACKAGE_GENERATOR_TASK_INTERVAL */
  	config.PackageHandlerTaskInterval = 5;

  	/* NETWORK_HANDLER_TASK_INTERVAL */
    config.NetworkHandlerTaskInterval = 5;

  	/* TOGGLE_GREEN_LED_INTERVAL */
  	config.ToggleGreenLedInterval = 500;

  	/* THROUGHPUT_PRINTOUT_TASK_INTERVAL */
	config.ThroughputPrintoutTaskInterval = 5;

	/* SHELL_TASK_INTERVAL */
	config.ShellTaskInterval = 50;

	/* LOGGER_TASK_INTERVAL */
	config.LoggerTaskInterval = 5000;
}
