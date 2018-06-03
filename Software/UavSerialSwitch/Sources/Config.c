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
void validateSwConfiguration(void);

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
* \fn bool readConfig(void)
* \brief Reads config file and stores it in global variable
*/
bool readConfig(void)
{
	int tmpIntArr[NUMBER_OF_UARTS];
	int numberOfCharsCopied;
	char copiedCsv[TEMP_CSV_SIZE];
  	char fileName[] = "serialSwitch_Config.ini";

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
  	/* PRIO_WIRELESS_CONN */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "RESEND_DELAY_WIRELESS_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.ResendDelayWirelessConn);

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
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "PAYLOAD_NUMBERING_PROCESSING_MODE",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, tmpIntArr);
  	for(int i=0; i<NUMBER_OF_UARTS; i++)
  	{
  		switch(tmpIntArr[i])
  		{
  			case PAYLOAD_NUMBER_IGNORED:
  				config.PayloadNumberingProcessingMode[i] = PAYLOAD_NUMBER_IGNORED;
  		  		break;
  			case PAYLOAD_REORDERING:
  				config.PayloadNumberingProcessingMode[i] = PAYLOAD_REORDERING;
  				break;
  			case ONLY_SEND_OUT_NEW_PAYLOAD:
  				config.PayloadNumberingProcessingMode[i] = ONLY_SEND_OUT_NEW_PAYLOAD;
  				break;
  			default:
  				config.PayloadNumberingProcessingMode[i] = PAYLOAD_NUMBER_IGNORED;
  				break;
  		}
  	}

  	/* PACK_REORDERING_TIMEOUT */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "PAYLOAD_REORDERING_TIMEOUT",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToInt(copiedCsv, config.PayloadReorderingTimeout);

  	/* SYNC_MESSAGING_MODE_ENABLED_PER_WL_CONN */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "SYNC_MESSAGING_MODE_ENABLED_PER_WL_CONN",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToBool(copiedCsv, config.SyncMessagingModeEnabledPerWlConn);

  	/* LOAD_BALANCING_MODE */
  	config.LoadBalancingMode = MINI_ini_getl("TransmissionConfiguration", "LOAD_BALANCING_MODE",  DEFAULT_BOOL, "serialSwitch_Config.ini");
  	switch(config.LoadBalancingMode)
  	{
  		case LOAD_BALANCING_AS_CONFIGURED:
  			config.LoadBalancingMode = LOAD_BALANCING_AS_CONFIGURED;
  	  		break;
  	 	case LOAD_BALANCING_SWITCH_WL_CONN_WHEN_ACK_NOT_RECEIVED:
  	  		config.LoadBalancingMode = LOAD_BALANCING_SWITCH_WL_CONN_WHEN_ACK_NOT_RECEIVED;
  	  		break;
  	 	case LOAD_BALANCING_USE_ALGORITHM:
  	  		config.LoadBalancingMode = LOAD_BALANCING_USE_ALGORITHM;
  	  		break;
  	 	default:
  	  		config.LoadBalancingMode = LOAD_BALANCING_AS_CONFIGURED;
  	  		break;

  	 }

  	/* USE_GOLAY_ERROR_CORRECTING_CODE */
  	numberOfCharsCopied = MINI_ini_gets("TransmissionConfiguration", "USE_GOLAY_ERROR_CORRECTING_CODE",  DEFAULT_CSV_STRING, copiedCsv, TEMP_CSV_SIZE, "serialSwitch_Config.ini");
  	csvToBool(copiedCsv, config.UseGolayPerWlConn);

  	/* -------- SoftwareConfiguration -------- */
  	/* TEST_HW_LOOPBACK_ONLY */
  	config.TestHwLoopbackOnly = MINI_ini_getbool("SoftwareConfiguration", "TEST_HW_LOOPBACK_ONLY",  DEFAULT_BOOL, "serialSwitch_Config.ini");

  	/* TEST_HW_LOOPBACK_ONLY */
  	config.EnableStressTest = MINI_ini_getbool("SoftwareConfiguration", "ENABLE_STRESS_TEST",  DEFAULT_BOOL, "serialSwitch_Config.ini");

  	/* GENERATE_DEBUG_OUTPUT */
  	config.GenerateDebugOutput = MINI_ini_getl("SoftwareConfiguration", "GENERATE_DEBUG_OUTPUT",  DEFAULT_BOOL, "serialSwitch_Config.ini");
  	switch(config.GenerateDebugOutput)
  	{
		case DEBUG_OUTPUT_NONE:
		case DEBUG_OUTPUT_SHELL_COMMANDS_ONLY:
		case DEBUG_OUTPUT_FULLLY_ENABLED:
			break; /* no action when config parameter set right */
		default:
			config.GenerateDebugOutput = DEBUG_OUTPUT_FULLLY_ENABLED; /* set debug output to enabled when config parameter faulty */
  	}

  	/* LOGGING_ENABLED */
  	config.LoggingEnabled = MINI_ini_getl("SoftwareConfiguration", "LOGGING_ENABLED",  DEFAULT_BOOL, "serialSwitch_Config.ini");

  	/* SD_CARD_SYNC_INTERVAL */
  	config.SdCardSyncInterval_s = MINI_ini_getl("SoftwareConfiguration", "SD_CARD_SYNC_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* SPI_HANDLER_TASK_INTERVAL */
  	config.SpiHandlerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "SPI_HANDLER_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* PACKAGE_GENERATOR_TASK_INTERVAL */
  	config.PackageHandlerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "PACKAGE_GENERATOR_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* NETWORK_HANDLER_TASK_INTERVAL */
    config.NetworkHandlerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "NETWORK_HANDLER_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

    /* TRANSPORT_HANDLER_TASK_INTERVAL */
    config.TransportHandlerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "TRANSPORT_HANDLER_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* TOGGLE_GREEN_LED_INTERVAL */
  	config.ToggleGreenLedInterval = MINI_ini_getl("SoftwareConfiguration", "TOGGLE_GREEN_LED_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

  	/* THROUGHPUT_PRINTOUT_TASK_INTERVAL */
	config.ThroughputPrintoutTaskInterval_s = MINI_ini_getl("SoftwareConfiguration", "THROUGHPUT_PRINTOUT_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

	/* SHELL_TASK_INTERVAL */
	config.ShellTaskInterval = MINI_ini_getl("SoftwareConfiguration", "SHELL_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

	/* LOGGER_TASK_INTERVAL */
	config.LoggerTaskInterval = MINI_ini_getl("SoftwareConfiguration", "LOGGER_TASK_INTERVAL",  DEFAULT_INT, "serialSwitch_Config.ini");

	validateSwConfiguration();

  	return true;

}

/*!
* \fn void validateSwConfiguration(void)
* \brief Validates config file and makes sure no impossible SW configurations are made
*/
void validateSwConfiguration(void)
{
	for(int devNr=0; devNr < NUMBER_OF_UARTS; devNr++)
	{
		// todo: validate software configuration!
		if(config.SendAckPerWirelessConn[devNr])
		{
			if(config.LoadBalancingMode == LOAD_BALANCING_SWITCH_WL_CONN_WHEN_ACK_NOT_RECEIVED) /* check if there is a WL conn with priority 1 and 2 */
			{
				/* check priorities for load balancing setting */
				for(int prio = 1; prio<=2; prio++)
				{
					uint8_t wlConnectionToUse = 0;
					while ( wlConnectionToUse < NUMBER_OF_UARTS && config.PrioWirelessConnDev[devNr][wlConnectionToUse] != prio )
					{
						++wlConnectionToUse;
					}
					if(wlConnectionToUse >= NUMBER_OF_UARTS)
					{
						config.LoadBalancingMode = LOAD_BALANCING_AS_CONFIGURED;
						// ToDo: print warning in shell
					}
				}
			}
		}
		else
		{
			config.LoadBalancingMode = LOAD_BALANCING_AS_CONFIGURED;
			config.SyncMessagingModeEnabledPerWlConn[devNr] = 0; /* no acknowledge sent -> synchoronous transmission mode disabled */
		}
	}
	/* constrain execution intervals */
	UTIL1_constrain(config.SdCardSyncInterval_s, 1, 1000); /* 1sec...1000sec */
	UTIL1_constrain(config.SpiHandlerTaskInterval, 1, 1000); /* 1ms...1sec */
	UTIL1_constrain(config.PackageHandlerTaskInterval, 1, 1000); /* 1ms...1sec */
	UTIL1_constrain(config.NetworkHandlerTaskInterval, 1, 1000); /* 1ms...1sec */
	UTIL1_constrain(config.TransportHandlerTaskInterval, 1, 1000); /* 1ms...1sec */
	UTIL1_constrain(config.ShellTaskInterval, 1, 1000); /* 1ms...1sec */
	UTIL1_constrain(config.LoggerTaskInterval, 1, 1000); /* 1ms...1sec */
	UTIL1_constrain(config.ThroughputPrintoutTaskInterval_s, 1, 1000); /* 1sec...1000sec */
	UTIL1_constrain(config.ToggleGreenLedInterval, 1, 1000); /* 1ms...1sec */
}
