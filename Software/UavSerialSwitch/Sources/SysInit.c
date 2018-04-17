#include "PE_Types.h"
#include "FAT1.h"
#include "Config.h"
#include "Shell.h" // to create task
#include "FRTOS.h"
#include "SpiHandler.h" // to create spi handler task, bool
#include "PackageHandler.h"
#include "NetworkHandler.h"
#include "Blinky.h"
#include "ThroughputPrintout.h"
#include "Logger.h"
#include "SysInit.h"
#include "LedGreen.h"
#include "LedRed.h"
#include "LedOrange.h"

/* prototypes for functions that are only within this file scope */
static bool createAllTasks(void);
static void ledInitSequence(void);

/*!
* \fn void SysInit_TaskEntry(void* param)
* \brief Reads config file and creates all other tasks afterwards (because other tasks need config file)
*/
void SysInit_TaskEntry(void* param)
{
  bool cardMounted = false;
  static FAT1_FATFS fileSystemObject;

  ledInitSequence();
  /* mount SD card so config file can be read */
  if(FAT1_Init() != 0)
    {for(;;){}} /* SD Card could not be mounted */
  FAT1_CheckCardPresence(&cardMounted, (uint8_t*)"0" /*volume*/, &fileSystemObject, CLS1_GetStdio());
  readConfig(); /* global config variable is needed for all other tasks -> read it before starting other tasks */
  createAllTasks(); /* start all other tasks */
  vTaskDelete(NULL); /* task deletes itself */
}

/*!
* \fn bool createAllTasks(void)
* \brief Initializes and creates all tasks for serial switch
* \return true if all tasks were created successfully
*/
static bool createAllTasks(void)
{
	/* make sure all queues are initialized before being accessed from other tasks */
	Shell_TaskInit(); /* 0.25kB */
	logger_TaskInit(); /* 2x4x(queuelength)x56B */
	spiHandler_TaskInit(); /* 10kB when queuelength = 512, 2x4x2x(queueLength)x1B */
	packageHandler_TaskInit(); /* 2x4x(queueLength)x56B = 3kB */
	networkHandler_TaskInit(); /* 2x4x(queueLength)x56B = 3kB */

#if configSUPPORT_STATIC_ALLOCATION

	/* Buffer that the task being created will use as its stack. */
	static StackType_t puxStackBufferShell[SHELL_STACK_SIZE];
	static StackType_t puxStackBufferSpiHandler[SPI_HANDLER_STACK_SIZE];
	static StackType_t puxStackBufferPackageHandler[PACKAGE_HANDLER_STACK_SIZE];
	static StackType_t puxStackBufferNetworkHandler[NETWORK_HANDLER_STACK_SIZE];
	static StackType_t puxStackBufferThroughputPrintout[THROUGHPUT_PRINTOUT_STACK_SIZE];
	static StackType_t puxStackBufferLogger[LOGGER_STACK_SIZE];
	static StackType_t puxStackBufferBlinky[BLINKY_STACK_SIZE];

	/* Structure that will hold the TCB of the task being created. */
	static StaticTask_t pxTaskBufferShell;
	static StaticTask_t pxTaskBufferSpiHandler;
	static StaticTask_t pxTaskBufferPackageHandler;
	static StaticTask_t pxTaskBufferNetworkHandler;
	static StaticTask_t pxTaskBufferThroughputPrintout;
	static StaticTask_t pxTaskBufferLogger;
	static StaticTask_t pxTaskBufferBlinky;


	/* create Shell task */
	if(config.GenerateDebugOutput != DEBUG_OUTPUT_NONE)
	{
		if (xTaskCreateStatic(Shell_TaskEntry, "Shell", SHELL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, puxStackBufferShell, &pxTaskBufferShell) == NULL) {
				for(;;) {}} /* error */
	}

	/* create SPI handler task */
	if (xTaskCreateStatic(spiHandler_TaskEntry, "SPI_Handler", SPI_HANDLER_STACK_SIZE, NULL, tskIDLE_PRIORITY+4, puxStackBufferSpiHandler, &pxTaskBufferSpiHandler) == NULL) {
	    for(;;) {}} /* error */


	/* create package handler task */
	if (xTaskCreateStatic(packageHandler_TaskEntry, "Package_Handler", PACKAGE_HANDLER_STACK_SIZE, NULL, tskIDLE_PRIORITY+3, puxStackBufferPackageHandler, &pxTaskBufferPackageHandler) == NULL) {
		for(;;) {}} /* error */


	/* create network handler task */
	if (xTaskCreateStatic(networkHandler_TaskEntry, "Network_Handler", NETWORK_HANDLER_STACK_SIZE, NULL, tskIDLE_PRIORITY+2, puxStackBufferNetworkHandler, &pxTaskBufferNetworkHandler) == NULL) {
		for(;;) {}} /* error */

	/* create throughput printout task */
	if(config.GenerateDebugOutput == DEBUG_OUTPUT_FULLLY_ENABLED)
	{
		if (xTaskCreateStatic(throughputPrintout_TaskEntry, "Throughput_Printout", THROUGHPUT_PRINTOUT_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, puxStackBufferThroughputPrintout, &pxTaskBufferThroughputPrintout) == NULL) {
			for(;;) {}} /* error */
	}

	/* create logger task */
	if(config.LoggingEnabled)
	{
		if (xTaskCreateStatic(logger_TaskEntry, "Logger", LOGGER_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, puxStackBufferLogger, &pxTaskBufferLogger) == NULL) {
			for(;;) {}} /* error */
	}

	/* create blinky task last to let user know that all init methods and mallocs were successful when LED blinks */
	if (xTaskCreateStatic(blinky_TaskEntry, "Blinky", BLINKY_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, puxStackBufferBlinky, &pxTaskBufferBlinky) == NULL) {
	    for(;;) {}} /* error */

#else
	/* create Shell task */
	if (xTaskCreate(Shell_TaskEntry, "Shell", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
	  	    for(;;) {}} /* error */


	/* create SPI handler task */
	if (xTaskCreate(spiHandler_TaskEntry, "SPI_Handler", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+3, NULL) != pdPASS) {
	    for(;;) {}} /* error */


	/* create package handler task */
	if (xTaskCreate(packageHandler_TaskEntry, "Package_Handler", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2, NULL) != pdPASS) {
		for(;;) {}} /* error */


	/* create network handler task */
	if (xTaskCreate(networkHandler_TaskEntry, "Network_Handler", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+2, NULL) != pdPASS) {
		for(;;) {}} /* error */

	/* create throughput printout task */
	if(config.GenerateDebugOutput)
		if (xTaskCreate(throughputPrintout_TaskEntry, "Throughput_Printout", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
			for(;;) {}} /* error */

	/* create logger task */
	if(config.LoggingEnabled)
		if (xTaskCreate(logger_TaskEntry, "Logger", 2000/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
			for(;;) {}} /* error */

	/* create blinky task last to let user know that all init methods and mallocs were successful when LED blinks */
	if (xTaskCreate(blinky_TaskEntry, "Blinky", 400/sizeof(StackType_t), NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS) {
	    for(;;) {}} /* error */
#endif
	return true;
}

/*!
* \fn static void ledInitSequence(void)
* \brief Initialization sequence for LED
*/
static void ledInitSequence(void)
{
	LedRed_On();
	LedOrange_On();
	LedGreen_On();
	WAIT1_Waitms(500);
	LedGreen_Off();
	LedOrange_Off();
	LedRed_Off();
	WAIT1_Waitms(500);
	LedRed_On();
	LedOrange_On();
	LedGreen_On();
	WAIT1_Waitms(500);
	LedGreen_Off();
	LedOrange_Off();
	LedRed_Off();
}
