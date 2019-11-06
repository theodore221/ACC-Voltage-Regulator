/*! @file LEDs.c
 *  @version 1.0
 *
 *  @brief Routines to access the LEDs on the TWR-K70F120M.
 *
 *  This contains the functions for operating the LEDs.
 *
 *  @author 12604120, 12931717
 *  @date LAST EDIT:18/04/2018
 */
/*!
**  @addtogroup LEDs_module LEDs module documentation
**  @{
*/
/* MODULE LEDs */

// new types
#include "types.h"
#include "IO_Map.h"
#include "LEDs.h"

bool LEDs_Init(void)
{
  static bool hasInitialized;
  if (hasInitialized)
  {
    return false;
  }

  //Enable PORT A
  SIM_SCGC5 |=  SIM_SCGC5_PORTA_MASK;

  //Pin Multiplexing
  PORTA_PCR11 = PORT_PCR_MUX(1);
  PORTA_PCR28 = PORT_PCR_MUX(1);
  PORTA_PCR29 = PORT_PCR_MUX(1);
  PORTA_PCR10 = PORT_PCR_MUX(1);

  //Setting Port Direction to Outputs
  GPIOA_PDDR |= GPIO_PDDR_PDD(LED_ORANGE);
  GPIOA_PDDR |= GPIO_PDDR_PDD(LED_YELLOW);
  GPIOA_PDDR |= GPIO_PDDR_PDD(LED_GREEN);
  GPIOA_PDDR |= GPIO_PDDR_PDD(LED_BLUE);

  //Start with all LEDs turned off
  GPIOA_PSOR |= GPIO_PSOR_PTSO(LED_ORANGE);
  GPIOA_PSOR |= GPIO_PSOR_PTSO(LED_YELLOW);
  GPIOA_PSOR |= GPIO_PSOR_PTSO(LED_GREEN);
  GPIOA_PSOR |= GPIO_PSOR_PTSO(LED_BLUE);

  return hasInitialized = true;
}

void LEDs_On(const LED_t color)
{
  GPIOA_PCOR = color;
}

void LEDs_Off(const LED_t color)
{
  GPIOA_PSOR = color;
}

void LEDs_Toggle(const LED_t color)
{
  GPIOA_PTOR = color;
}

/*!
** @}
*/
