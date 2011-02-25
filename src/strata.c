/***************************************************************************
 *   Copyright (C) 2008 by Hilscher GmbH                                   *
 *   cthelen@hilscher.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "strata.h"
#include <string.h>


/* Data bits to read indicating the status of an FLASH operation. */
#define DRV_INTEL_SR7_WRT     (0x80)  /* write ready busy */
#define DRV_INTEL_SR6_ERS     (0x40)  /* erase suspended */
#define DRV_INTEL_SR5_CLR_LCK (0x20)  /* erase and clear lock */
#define DRV_INTEL_SR4_SET_LCK (0x10)  /* Program and set lock */
#define DRV_INTEL_SR3_PRG_VLT (0x08)  /* VPP low */
#define DRV_INTEL_SR2_PRG_SUS (0x04)  /* Program suspended */
#define DRV_INTEL_SR1_DEV_PRT (0x02)  /* Sector locked */
#define DRV_INTEL_SR0_RES     (0x01)  /* reset */

#define MFGCODE_INTEL           0x89  /* Intel's flash manufacturing code. */

/* Define flash memory command set (these are 8bit flash commands). */
#define BLOCK_ERASE                   0x20
#define PROGRAM_BYTE_WORD             0x40
#define CLEAR_STATUS_REGISTER         0x50
#define READ_STATUS_REGISTER          0x70
#define READ_IDENTIFIER_CODE          0x90
#define READ_QUERY                    0x98
#define BLOCK_ERASE_PROGRAM_SUSPEND   0xB0
#define BLOCK_ERASE_PROGRAM_RESUME    0xD0
#define WRITE_TO_BUFFER               0xE8
#define SET_BLOCK_LOCK_BIT            0x60
#define CLEAR_BLOCK_LOCK_BIT          0x60
#define PROTECTION_PROGRAM            0xC0
#define CONFIGURATION                 0xB8
#define READ_ARRAY                    0xFF
#define SET_BLOCK_LOCK_CONFIRM	0x01
#define CLEAR_BLOCK_LOCK_CONFIRM	0xD0


static FLASH_ERRORS_E FlashWaitStatusDone (const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector);
static void           FlashWriteCommand   (const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector, unsigned long ulOffset, unsigned long ulCmd);
static int            FlashIsset          (const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector, unsigned long ulOffset, unsigned long ulCmd);


static FLASH_ERRORS_E FlashReset(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector);
static FLASH_ERRORS_E FlashErase(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector);
static FLASH_ERRORS_E FlashEraseAll(const FLASH_DEVICE_T *ptFlashDev);
static FLASH_ERRORS_E FlashProgram(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulStartOffset, unsigned long ulLength, const void* pvData);
static FLASH_ERRORS_E FlashLock (const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector);
static FLASH_ERRORS_E FlashUnlock(const FLASH_DEVICE_T *ptFlashDev);

static FLASH_FUNCTIONS_T s_tIntelStrataFuncs =
{
  FlashReset,
  FlashErase,
  FlashEraseAll,
  FlashProgram,
  FlashLock,
  FlashUnlock
};

int IntelIdentifyFlash(FLASH_DEVICE_T *ptFlashDev)
{
  int fRet = FALSE;

  /* try to identify Strata Flash */
  FlashWriteCommand(ptFlashDev, 0, 0, READ_IDENT_CMD);

  ptFlashDev->ucManufacturer = ptFlashDev->pucFlashBase[0];
  ptFlashDev->ucDevice       = ptFlashDev->pucFlashBase[1];

  FlashWriteCommand(ptFlashDev, 0 ,0, READ_ARRAY_CMD);

  if(ptFlashDev->ucManufacturer == MFGCODE_INTEL)
  {
    strcpy(ptFlashDev->acIdent, "Intel");
    memcpy(&(ptFlashDev->tFlashFunctions), &s_tIntelStrataFuncs, sizeof(FLASH_FUNCTIONS_T));
    fRet = TRUE;  
  }

  return fRet;
}


/*! Reset the flash sector to read mode
*
*   \param   ptFlashDev       Pointer to the FLASH control Block
*   \param   ulSector         Sector to reset to read mode
*
*   \return  eFLASH_NO_ERROR  on success
*/
static FLASH_ERRORS_E FlashReset(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector)
{
  FlashWriteCommand(ptFlashDev, ulSector, 0, READ_ARRAY);

  return eFLASH_NO_ERROR;
}


/*! Erase a flash sector
*
*   \param   ptFlashDev       Pointer to the FLASH control Block
*   \param   ulSector         Sector to erase
*
*   \return  eFLASH_NO_ERROR  on success
*/
static FLASH_ERRORS_E FlashErase(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector)
{
  FLASH_ERRORS_E eRet = eFLASH_NO_ERROR;

  FlashWriteCommand(ptFlashDev, ulSector, 0, BLOCK_ERASE);
  FlashWriteCommand(ptFlashDev, ulSector, 0, BLOCK_ERASE_PROGRAM_RESUME);

  eRet = FlashWaitStatusDone(ptFlashDev, ulSector);

  if(eFLASH_NO_ERROR != eRet)
    FlashWriteCommand(ptFlashDev, ulSector, 0, CLEAR_STATUS_REGISTER);

  FlashReset(ptFlashDev, ulSector);

  return eRet;
}


/*! Erase whole flash
*
*   \param   ptFlashDev       Pointer to the FLASH control Block
*
*   \return  eFLASH_NO_ERROR  on success
*/
static FLASH_ERRORS_E FlashEraseAll(const FLASH_DEVICE_T *ptFlashDev)
{
  FLASH_ERRORS_E eRet     = eFLASH_NO_ERROR;
  unsigned long  ulSector = 0;

  for(ulSector = 0; ulSector < ptFlashDev->ulSectorCnt; ++ulSector)
  {
    if(eFLASH_NO_ERROR != (eRet = FlashErase(ptFlashDev, ulSector)))
    {
      FlashWriteCommand(ptFlashDev, ulSector, 0, CLEAR_STATUS_REGISTER);
      break;
    }
  }

  return eRet;
}


/*! Program flash
*
*   \param   ptFlashDev       Pointer to the FLASH control Block
*   \param   ulStartOffset    Offset to start writing at
*   \param   ulLength         Length of data to write
*   \param   pvData           Data pointer
*
*   \return  eFLASH_NO_ERROR  on success
*/
static FLASH_ERRORS_E FlashProgram(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulStartOffset, unsigned long ulLength, const void* pvData)
{
  FLASH_ERRORS_E eRet             = eFLASH_NO_ERROR;
  unsigned long  ulCurrentSector;
  unsigned long  ulCurrentOffset;   
  VADR_T tWriteAdr;
  CADR_T tSrcEndAdr;
  CADR_T tSrcAdr;
  tSrcAdr.puc = (const unsigned char*)pvData;

  /* Determine the start sector and offset inside the sector */
  ulCurrentSector = cfi_find_matching_sector_index(ptFlashDev, ulStartOffset);
  if (ulCurrentSector == 0xffffffffU)
  {
    return eFLASH_INVALID_PARAMETER;
  }
  ulCurrentOffset = ulStartOffset - ptFlashDev->atSectors[ulCurrentSector].ulOffset;

  FlashWriteCommand(ptFlashDev, ulCurrentSector, 0, CLEAR_STATUS_REGISTER);
  FlashReset(ptFlashDev, ulCurrentSector);

  while(ulLength > 0)
  {
    /* determine number of bytes to write */
    unsigned long ulWriteSize    = 0; /* Bufferwrite size this run */
    unsigned char bWriteCountCmd = 0;
    unsigned long ulMaxBuffer    = ptFlashDev->ulMaxBufferWriteSize;

    if(ptFlashDev->fPaired)
      ulMaxBuffer *= 2;

    if(ulLength > ulMaxBuffer)
      ulWriteSize = ulMaxBuffer;
    else
      ulWriteSize = ulLength;

    if((ulCurrentOffset + ulWriteSize) > ptFlashDev->atSectors[ulCurrentSector].ulSize)
    {
      ulWriteSize = ptFlashDev->atSectors[ulCurrentSector].ulSize - ulCurrentOffset;
    }

    /* send write buffer command */
    FlashWriteCommand(ptFlashDev, ulCurrentSector, 0, WRITE_TO_BUFFER);
    FlashWaitStatusDone(ptFlashDev, ulCurrentSector);

    switch(ptFlashDev->tBits)
    {
    case BUS_WIDTH_8Bit:
      bWriteCountCmd = (unsigned char)(ulWriteSize - 1);
      break;

    case BUS_WIDTH_16Bit:
      bWriteCountCmd = (unsigned char)(ulWriteSize / 2 - 1);
      break;

    case BUS_WIDTH_32Bit:
      bWriteCountCmd = (unsigned char)(ulWriteSize / 4 - 1);
      break;
    }    

    FlashWriteCommand(ptFlashDev, ulCurrentSector, 0, bWriteCountCmd);
       
    /* fill the buffer */ 
    tWriteAdr.puc = ptFlashDev->pucFlashBase + 
                                   ptFlashDev->atSectors[ulCurrentSector].ulOffset + 
                                   ulCurrentOffset;
                                                                      
    tSrcEndAdr.puc = tSrcAdr.puc + ulWriteSize;

    
    while(tSrcAdr.puc != tSrcEndAdr.puc) 
      switch(ptFlashDev->tBits)
      {
      case BUS_WIDTH_8Bit:
        *tWriteAdr.puc++ = *tSrcAdr.puc++;
        break;
  
      case BUS_WIDTH_16Bit:
        *tWriteAdr.pus++ = *tSrcAdr.pus++;
        break;
  
      case BUS_WIDTH_32Bit:
        *tWriteAdr.pul++ = *tSrcAdr.pul++;
        break;
      }
    
    ulCurrentOffset += ulWriteSize;
    ulLength -= ulWriteSize;
    
    /* program the buffer contents */
    FlashWriteCommand(ptFlashDev, ulCurrentSector, 0, BLOCK_ERASE_PROGRAM_RESUME);

    /* Full Status Check */
    if(eFLASH_NO_ERROR != (eRet = FlashWaitStatusDone(ptFlashDev, ulCurrentSector)))
    {
      break;
    }

    /* wrap around */
    if(ulCurrentOffset == ptFlashDev->atSectors[ulCurrentSector].ulSize)    
    {
      FlashWriteCommand(ptFlashDev, ulCurrentSector, 0, READ_ARRAY);

      ulCurrentOffset = 0;
      ++ulCurrentSector;
    }
  }

  FlashWriteCommand(ptFlashDev, ulCurrentSector, 0, READ_ARRAY);

  return eRet;
}

FLASH_ERRORS_E FlashLock(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector)
{
	FLASH_ERRORS_E eRet = eFLASH_NO_ERROR;


	FlashWriteCommand(ptFlashDev, ulSector, 0, SET_BLOCK_LOCK_BIT);
	FlashWriteCommand(ptFlashDev, ulSector, 0, SET_BLOCK_LOCK_CONFIRM);

	eRet = FlashWaitStatusDone(ptFlashDev, ulSector);
	if( eFLASH_NO_ERROR!=eRet )
	{
		FlashWriteCommand(ptFlashDev, ulSector, 0, CLEAR_STATUS_REGISTER);
	}

	FlashReset(ptFlashDev, ulSector);

	return eRet;
}


FLASH_ERRORS_E FlashUnlock(const FLASH_DEVICE_T *ptFlashDev)
{
	FLASH_ERRORS_E eRet = eFLASH_NO_ERROR;


	FlashWriteCommand(ptFlashDev, 0, 0, CLEAR_BLOCK_LOCK_BIT);
	FlashWriteCommand(ptFlashDev, 0, 0, CLEAR_BLOCK_LOCK_CONFIRM);

	eRet = FlashWaitStatusDone(ptFlashDev, 0);
	if( eRet!=eFLASH_NO_ERROR )
	{
		FlashWriteCommand(ptFlashDev, 0, 0, CLEAR_STATUS_REGISTER);
	}

	FlashReset(ptFlashDev, 0);

	return eRet;
}


/*! Write a command to the FLASH
*
*   \param   ptFlashDev  Pointer to the FLASH control Block
*   \param   ulSector    FLASH sector number
*   \param   ulOffset    Offset address in the actual FLASH sector
*   \param   bCmd        Command to execute
*/
void FlashWriteCommand(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector, unsigned long ulOffset, unsigned long ulCmd)
{
	union
	{
		volatile unsigned char *puc;
		volatile unsigned short *pus;
		volatile unsigned long *pul;
	} uAdr;


	uAdr.puc = ptFlashDev->pucFlashBase + ptFlashDev->atSectors[ulSector].ulOffset + ulOffset;

	switch( ptFlashDev->tBits )
	{
	case BUS_WIDTH_8Bit:
		/* 8bits cannot be paired */
		*(uAdr.puc) = (unsigned char)ulCmd;
		break;

	case BUS_WIDTH_16Bit:
		if( ptFlashDev->fPaired )
		{
			ulCmd |= ulCmd << 8;
		}

		*(uAdr.pus) = (unsigned short)ulCmd;
		break;

	case BUS_WIDTH_32Bit:
		if( ptFlashDev->fPaired )
		{
			ulCmd |= ulCmd << 16;
		}

		*(uAdr.pul) = ulCmd;
		break;
	}
}


/*! Checks if a given flag (bCmd) is set on the FLASH device
*
*   \param   ptFlashDev  Pointer to the FLASH control Block
*   \param   ulSector    FLASH sector number
*   \param   ulOffset    Offset address in the actual FLASH sector
*   \param   bCmd        Flag value to be checked
*
*   \return  TRUE        on success
*/
int FlashIsset(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector, unsigned long ulOffset, unsigned long ulCmd)
{
  int iRet = FALSE;
  volatile void* pvReadAddr = ptFlashDev->pucFlashBase + ptFlashDev->atSectors[ulSector].ulOffset + ulOffset;
  
  switch(ptFlashDev->tBits)
  {
  case BUS_WIDTH_8Bit:
    {
      unsigned char ucValue       = *(volatile unsigned char*)pvReadAddr;
      if (ucValue & (unsigned char) ulCmd)
        iRet = TRUE;
    }
    break;
  case BUS_WIDTH_16Bit:
    {
      unsigned short usValue    = *(volatile unsigned short*)pvReadAddr;
      unsigned short usCheckCmd = (unsigned short) ulCmd;

      if(ptFlashDev->fPaired)
        usCheckCmd |= (unsigned short)(ulCmd << 8);

      if((usValue & usCheckCmd) == usCheckCmd)
        iRet = TRUE;
    }
    break;

  case BUS_WIDTH_32Bit:
    {
      unsigned long ulValue    = *(volatile unsigned long*)pvReadAddr;
      unsigned long ulCheckCmd = ulCmd;

      if(ptFlashDev->fPaired)
        ulCheckCmd |= ulCmd << 16;

      if((ulValue & ulCheckCmd) == ulCheckCmd)
        iRet = TRUE;
    }
    break;
  }

  return iRet;
}

/*! Wait until FLASH has accepted a state change
*
*   \param   ptFlashDev  Pointer to the FLASH control Block
*   \param   ulSector    FLASH sector number
*
*/
static FLASH_ERRORS_E FlashWaitStatusDone(const FLASH_DEVICE_T *ptFlashDev, unsigned long ulSector)
{
  FLASH_ERRORS_E eRet = eFLASH_NO_ERROR;

  while(!FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR7_WRT)) 
    ;

  if(FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR6_ERS))
  {
    eRet = eFLASH_GENERAL_ERROR;
/*    EdbgOutputDebugString("ERROR: Status done. Erase suspended!\r\n"); */

  } else if(FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR5_CLR_LCK))
  {
    eRet = eFLASH_LOCKED;
/*    EdbgOutputDebugString("ERROR: Status done. Erase and clear Lock!\r\n"); */
  } else if(FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR4_SET_LCK))
  {

    eRet = eFLASH_LOCKED;
/*    EdbgOutputDebugString("ERROR: Status done. Program and Set Lock!\r\n"); */
  } else if(FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR3_PRG_VLT))
  {
    eRet = eFLASH_VPP_LOW;
/*    EdbgOutputDebugString("ERROR: Status done. VPP low!\r\n"); */
  } else if(FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR2_PRG_SUS))
  {
    eRet = eFLASH_GENERAL_ERROR;
/*    EdbgOutputDebugString("ERROR: Status done. Program suspended!\r\n"); */
  } else if(FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR1_DEV_PRT))
  {
    eRet = eFLASH_LOCKED;
/*    EdbgOutputDebugString("ERROR: Status done. Sector locked!\r\n"); */
  } else if(FlashIsset(ptFlashDev, ulSector, 0, DRV_INTEL_SR0_RES))
  {
    eRet = eFLASH_BUSY;
/*    EdbgOutputDebugString("ERROR: Status done. Reset!\r\n"); */
  }

  return eRet;
}

