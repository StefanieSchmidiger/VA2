/* ###################################################################
**     Filename    : Events.h
**     Project     : SimpleThroughputTeensy
**     Processor   : MK64FX512VMD12
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2017-10-25, 12:17, # CodeGen: 0
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Contents    :
**         SPI_OnBlockSent                      - void SPI_OnBlockSent(LDD_TUserData *UserDataPtr);
**         SPI_OnBlockReceived                  - void SPI_OnBlockReceived(LDD_TUserData *UserDataPtr);
**         FRTOS_vApplicationStackOverflowHook - void FRTOS_vApplicationStackOverflowHook(TaskHandle_t pxTask, char...
**         FRTOS_vApplicationTickHook          - void FRTOS_vApplicationTickHook(void);
**         FRTOS_vApplicationIdleHook          - void FRTOS_vApplicationIdleHook(void);
**         FRTOS_vApplicationMallocFailedHook  - void FRTOS_vApplicationMallocFailedHook(void);
**         Cpu_OnNMI                            - void Cpu_OnNMI(void);
**
** ###################################################################*/
/*!
** @file Events.h
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/         
/*!
**  @addtogroup Events_module Events module documentation
**  @{
*/         

#ifndef __Events_H
#define __Events_H
/* MODULE Events */

#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "Pins1.h"
#include "WAIT1.h"
#include "MCUC1.h"
#include "FRTOS.h"
#include "CLS1.h"
#include "UTIL1.h"
#include "CS1.h"
#include "XF1.h"
#include "RTT1.h"
#include "KIN1.h"
#include "FAT1.h"
#include "MINI.h"
#include "TmDt1.h"
#include "FATM1.h"
#include "SS1.h"
#include "SPI2.h"
#include "TMOUT1.h"
#include "RTC1.h"
#include "SPI.h"
#include "LedGreen.h"
#include "LEDpin2.h"
#include "BitIoLdd2.h"
#include "LedOrange.h"
#include "LEDpin3.h"
#include "BitIoLdd3.h"
#include "LedRed.h"
#include "LEDpin4.h"
#include "BitIoLdd4.h"
#include "nIrqWirelessSide.h"
#include "BitIoLdd5.h"
#include "nIrqDeviceSide.h"
#include "BitIoLdd6.h"
#include "nResetDeviceSide.h"
#include "BitIoLdd7.h"
#include "nResetWirelessSide.h"
#include "BitIoLdd8.h"
#include "HF1.h"
#include "CRC1.h"
#include "RNG.h"
#include "PTRC1.h"
#include "Pin33.h"
#include "BitIoLdd9.h"

#ifdef __cplusplus
extern "C" {
#endif 

/*
** ===================================================================
**     Event       :  SPI_OnBlockSent (module Events)
**
**     Component   :  SPI [SPIMaster_LDD]
*/
/*!
**     @brief
**         This event is called after the last character from the
**         output buffer is moved to the transmitter. This event is
**         available only if the SendBlock method is enabled.
**     @param
**         UserDataPtr     - Pointer to the user or
**                           RTOS specific data. The pointer is passed
**                           as the parameter of Init method. 
*/
/* ===================================================================*/
void SPI_OnBlockSent(LDD_TUserData *UserDataPtr);

/*
** ===================================================================
**     Event       :  SPI_OnBlockReceived (module Events)
**
**     Component   :  SPI [SPIMaster_LDD]
*/
/*!
**     @brief
**         This event is called when the requested number of data is
**         moved to the input buffer. This method is available only if
**         the ReceiveBlock method is enabled.
**     @param
**         UserDataPtr     - Pointer to the user or
**                           RTOS specific data. The pointer is passed
**                           as the parameter of Init method. 
*/
/* ===================================================================*/
void SPI_OnBlockReceived(LDD_TUserData *UserDataPtr);

void FRTOS_vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
/*
** ===================================================================
**     Event       :  FRTOS_vApplicationStackOverflowHook (module Events)
**
**     Component   :  FRTOS [FreeRTOS]
**     Description :
**         if enabled, this hook will be called in case of a stack
**         overflow.
**     Parameters  :
**         NAME            - DESCRIPTION
**         pxTask          - Task handle
**       * pcTaskName      - Pointer to task name
**     Returns     : Nothing
** ===================================================================
*/

void FRTOS_vApplicationTickHook(void);
/*
** ===================================================================
**     Event       :  FRTOS_vApplicationTickHook (module Events)
**
**     Component   :  FRTOS [FreeRTOS]
**     Description :
**         If enabled, this hook will be called by the RTOS for every
**         tick increment.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

void FRTOS_vApplicationIdleHook(void);
/*
** ===================================================================
**     Event       :  FRTOS_vApplicationIdleHook (module Events)
**
**     Component   :  FRTOS [FreeRTOS]
**     Description :
**         If enabled, this hook will be called when the RTOS is idle.
**         This might be a good place to go into low power mode.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

void FRTOS_vApplicationMallocFailedHook(void);
/*
** ===================================================================
**     Event       :  FRTOS_vApplicationMallocFailedHook (module Events)
**
**     Component   :  FRTOS [FreeRTOS]
**     Description :
**         If enabled, the RTOS will call this hook in case memory
**         allocation failed.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

/*
** ===================================================================
**     Event       :  Cpu_OnNMI (module Events)
**
**     Component   :  Cpu [MK64FN1M0MD12]
*/
/*!
**     @brief
**         This event is called when the Non maskable interrupt had
**         occurred. This event is automatically enabled when the [NMI
**         interrupt] property is set to 'Enabled'.
*/
/* ===================================================================*/
void Cpu_OnNMI(void);


/*
** ===================================================================
**     Event       :  RNG_OnError (module Events)
**
**     Component   :  RNG [RNG_LDD]
*/
/*!
**     @brief
**         Called when empty FIFO is read (interrupt must be enabled
**         and no mask applied). See SetEventMask() and GetEventMask()
**         methods.
**     @param
**         UserDataPtr     - Pointer to the user or
**                           RTOS specific data. This pointer is passed
**                           as the parameter of Init method.
*/
/* ===================================================================*/
void RNG_OnError(LDD_TUserData *UserDataPtr);

void PTRC1_OnTraceWrap(void);
/*
** ===================================================================
**     Event       :  PTRC1_OnTraceWrap (module Events)
**
**     Component   :  PTRC1 [PercepioTrace]
**     Description :
**         Called for trace ring buffer wrap around. This gives the
**         application a chance to dump the trace buffer.
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/

/* END Events */

#ifdef __cplusplus
}  /* extern "C" */
#endif 

#endif 
/* ifndef __Events_H*/
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
