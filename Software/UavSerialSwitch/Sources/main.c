/* ###################################################################
**     Filename    : main.c
**     Project     : Teensy3_5_test
**     Processor   : MK64FX512VMD12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2017-04-24, 15:11, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
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
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "PDD_Includes.h"
#include "Init_Config.h"
/* User includes (#include below this line is not maintained by Processor Expert) */
#include "Application.h"
#include "cau_api.h"

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
  APP_Run();
  cau_des_chk_parity(NULL);
  /* For example: for(;;) { } */

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
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
