/*!
 * @file Flash.c
 * @version 1.0
 *
 * @brief Routines for erasing and writing to the Flash.
 *
 * contains functions required to access & modify internal flash storage
 *
 * @author Ben Essam, Theodore Xavier
 * @date 2018-04-18
 *
 */
/*!
**  @addtogroup flash_module flash module documentation
**  @{
*/
/* MODULE flash */

#include <string.h>

#include "types.h"
#include "MK70F12.h"
#include "Flash.h"

//Macros define Flash write/erase commands
#define COMMAND_WRITE_SECTOR 0x07
#define COMMAND_ERASE_SECTOR 0x09
#define ACCESS_ERROR_VIOLATION 0x03

//indicates total size of flash storage
#define FLASH_SIZE_BYTES 8

//struct for FCCOB registers, includes unions for address & data bytes
typedef struct
{
  uint8_t commandByte;
  union
  {
    uint8_t addressBytes[4];
    uint32_t address;
  };
  union
  {
    uint8_t dataBytes[8];
    uint64_t data;
  };
} FCCOB_t;

// Map of used locations in flash memory - TRUE if used, FALSE if free
static bool UsedFlashMap[FLASH_SIZE_BYTES];

/*! @brief Enables the Flash module.
 *
 *  @return bool - TRUE if the Flash was setup successfully.
 */
bool Flash_Init(void)
{
  //clears memory map of used flash spaces
  memset(UsedFlashMap, false, FLASH_SIZE_BYTES);
  return true;
}

/*!
 * @brief Configures the FCCOB register with required parameter bytes.
 * @param command - FCCOB struct.
 *
 * @return bool - True if FCCOB configured and command completed.
 */
static bool LaunchCommand(FCCOB_t* command)
{
  //SEE FLOWCHART ON PAGE 806 OF K70 MANUAL
  //loop the FSTAT Register Command Complete Interupt Flag is 1

  //Wait for the CCIF to be set
  while (~FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK);

  //w1c the access error flag
  FTFE_FSTAT = FTFE_FSTAT_ACCERR_MASK;

  //w1c the flash protection violation flag
  FTFE_FSTAT = FTFE_FSTAT_FPVIOL_MASK;

  //write to the FCCOB registers
  FTFE_FCCOB0 = command->commandByte;
  FTFE_FCCOB1 = command->addressBytes[2];
  FTFE_FCCOB2 = command->addressBytes[1];
  FTFE_FCCOB3 = command->addressBytes[0];
  FTFE_FCCOB4 = command->dataBytes[7];
  FTFE_FCCOB5 = command->dataBytes[6];
  FTFE_FCCOB6 = command->dataBytes[5];
  FTFE_FCCOB7 = command->dataBytes[4];
  FTFE_FCCOB8 = command->dataBytes[3];
  FTFE_FCCOB9 = command->dataBytes[2];
  FTFE_FCCOBA = command->dataBytes[1];
  FTFE_FCCOBB = command->dataBytes[0];

  //Write to CCIF sets the flag until command is completed
  FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;
  while (~FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK);

  return true;
}

/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *         The pointer will be allocated to a relevant address:
 *         If the variable is a byte, then any address.
 *         If the variable is a half-word, then an even address.
 *         If the variable is a word, then an address divisible by 4.
 *         This allows the resulting variable to be used with the relevant Flash_Write function which assumes a certain memory address.
 *         e.g. a 16-bit variable will be on an even address
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  //points to the next available memory address
  void * flashPtr = (void *) FLASH_DATA_START;

  switch (size)
  {
    //if variable size is a byte
    case 1:
    {
      //loops through memory map to identify used locations in flash memory
      for (int i = 0; i < FLASH_SIZE_BYTES; i++)
      {
        if (!UsedFlashMap[i])
	{
          //points to available address
	  flashPtr += i;
	  UsedFlashMap[i] = true;
	  *variable = flashPtr;

	  return true;
	}
      }
      //if no available memory space is found then returns false
      return false;
    }
    //if variable size is half-word
    case 2:
    {
      for (int i = 0; i < FLASH_SIZE_BYTES; i += 2)
      {
      	if (!UsedFlashMap[i] && !UsedFlashMap[i + 1])
      	{
      	  flashPtr += i;
      	  UsedFlashMap[i]     = true;
      	  UsedFlashMap[i + 1] = true;
      	  *variable = flashPtr;

      	  return true;
      	}
      }

      return false;
    }
    //if variable size is word
    case 4:
    {
      for (int i = 0; i < FLASH_SIZE_BYTES; i += 4)
      {
	if (!UsedFlashMap[i] && !UsedFlashMap[i + 1] && !UsedFlashMap[i + 2] && !UsedFlashMap[i + 3])
	{
	  flashPtr += i;
      	  UsedFlashMap[i]     = true;
      	  UsedFlashMap[i + 1] = true;
      	  UsedFlashMap[i + 2] = true;
      	  UsedFlashMap[i + 3] = true;
      	  *variable = flashPtr;

	  return true;
	}
      }

      return false;
    }
    default:
    {
      return false;
    }
  }
}

/*!
 * @brief Writes a new phrase of data to flash.
 * @param address - the address of the data.
 * @param phrase - The 64bit data to write.
 *
 * @return bool - True if Phrase sucessfully written.
 */
static bool WritePhrase(const uint64_t * const address, const uint64union_t phrase) {
  FCCOB_t reg;
  reg.commandByte = COMMAND_WRITE_SECTOR;
  reg.address = (uint32_t) address;
  reg.data = phrase.l;

  return LaunchCommand(&reg);
}

/*!
 * @brief Erases a sector of data in flash.
 * @param address - The address of the data.
 *
 * @return bool - True if Sector successfully erased.
 */
static bool EraseSector(const uint64_t * const address) {
  FCCOB_t reg;
  reg.commandByte = COMMAND_ERASE_SECTOR;
  reg.address = (uint32_t) address;

  return LaunchCommand(&reg);
}

/*!
 * @brief Modifies a phrase of data in flash.
 * @param address - The address of the data.
 * @param phrase - 64bit data to be write.
 *
 * @return bool - True if new phrase successfully written.
 */
static bool ModifyPhrase(uint64_t * address, uint64union_t phrase) {
  bool erasedSector = EraseSector(address);
  bool wrotePhrase = false;
  //checks that previous data has been erased before writing to flash
  if (erasedSector)
    wrotePhrase =  WritePhrase(address, phrase);

  //returns whether the sector was erased and new phrase written
  return erasedSector && wrotePhrase;
}

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
  if((uint32_t)address < FLASH_DATA_START && (uint32_t)address > FLASH_DATA_END)
  {
    return false;
  }
  //if address is not divisible by 4, returns false
  if (((uint32_t) address) % 4 != 0)
    return false;

  uint64union_t newPhrase;

  void* wordStart   = (void*) address;
  void* phraseStart = (void*) ((uint32_t) address - ((uint32_t) address % 8));

  if (wordStart == phraseStart)
  {
    newPhrase.s.Hi = data;
    newPhrase.s.Lo = _FW(address + 1);
  }
  else
  {
    newPhrase.s.Hi = _FW(address - 1);
    newPhrase.s.Lo = data;
  }

  return ModifyPhrase((uint64_t *) phraseStart, newPhrase);
}

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
  if((uint32_t)address < FLASH_DATA_START && (uint32_t)address > FLASH_DATA_END)
  {
    return false;
  }
  if (((uint32_t) address) % 2 != 0)
  {
    return false;
  }

  uint32union_t newWord;

  void* halfWordStart = (void*) address;
  void* wordStart     = (void*) ((uint32_t) address - ((uint32_t) address % 4));

  if (halfWordStart == wordStart)
  {
    newWord.s.Lo = data;
    newWord.s.Hi = _FH(address + 1);
  }
  else
  {
    newWord.s.Lo = _FH(address - 1);
    newWord.s.Hi = data;
  }

  return Flash_Write32((uint32_t *) wordStart, newWord.l);
}

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
  if((uint32_t)address < FLASH_DATA_START && (uint32_t)address > FLASH_DATA_END)
    {
      return false;
    }
  uint16union_t newHalfWord;

  void* byteStart = (void*) address;
  void* halfWordStart = (void*) ((uint32_t) address - ((uint32_t) address % 2));

  if (byteStart == halfWordStart)
  {
    newHalfWord.s.Lo = data;
    newHalfWord.s.Hi = _FB(address + 1);
  }
  else
  {
    newHalfWord.s.Lo = _FB(address - 1);
    newHalfWord.s.Hi = data;
  }

  return Flash_Write16((uint16_t *) halfWordStart, newHalfWord.l);
}

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void)
{
  return EraseSector((uint64_t *) FLASH_DATA_START);
}

/*!
** @}
*/
