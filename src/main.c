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
 *   Free Software Foudnation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "flasher_version.h"
#include "netx_consoleapp.h"
#include "rdyrun.h"

#include "board.h"

/* Parallel flash routines. */
#include "flasher_parflash.h"
/* Serial flash on spi. */
#include "flasher_spi.h"


#include "flasher_interface.h"
#include "uprintf.h"

#include "main.h"


/* ------------------------------------- */


static NETX_CONSOLEAPP_RESULT_T opMode_detect(tFlasherInputParameter *ptAppParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	BUS_T tSourceTyp;


	tSourceTyp = ptAppParams->uParameter.tDetect.tSourceTyp;

	/* Clear the result data. */
	memset(ptAppParams->uParameter.tDetect.ptDeviceDescription, 0, sizeof(DEVICE_DESCRIPTION_T));

	uprintf(". Device :");
	switch(tSourceTyp)
	{
	case BUS_ParFlash:
		/*  use parallel flash */
		uprintf("Parallel flash\n");
		tResult = parflash_detect(&(ptAppParams->uParameter.tDetect));
		break;

	case BUS_SPI:
		/*  use SPI flash */
		uprintf("SPI flash\n");
		tResult = spi_detect(&(ptAppParams->uParameter.tDetect));
		break;

	default:
		/*  unknown boot device */
		uprintf("unknown\n");
		uprintf("! illegal device id specified\n");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
		break;
	}

	return tResult;
}

/* ------------------------------------- */

static NETX_CONSOLEAPP_RESULT_T check_device_description(const DEVICE_DESCRIPTION_T *ptDeviceDescription)
{
	NETX_CONSOLEAPP_RESULT_T tResult;


	/* expect error */
	tResult = NETX_CONSOLEAPP_RESULT_ERROR;

	if( ptDeviceDescription==NULL )
	{
		uprintf("! Missing device description!\n");
	}
	else if( ptDeviceDescription->fIsValid==0 )
	{
		uprintf("! The device description is not valid!\n");
	}
	else if( ptDeviceDescription->sizThis!=sizeof(DEVICE_DESCRIPTION_T) )
	{
		uprintf("! The size of the device description differs from the internal representation!\n");
	}
	else if( ptDeviceDescription->ulVersion!=FLASHER_INTERFACE_VERSION )
	{
		uprintf("! The device description has an invalid version!\n");
	}
	else
	{
		uprintf(". The device description seems to be ok.\n");
	}

	return tResult;
}

/* ------------------------------------- */


static NETX_CONSOLEAPP_RESULT_T opMode_flash(tFlasherInputParameter *ptAppParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	const DEVICE_DESCRIPTION_T *ptDeviceDescription;
	BUS_T tSourceTyp;


	/* check the device description */
	ptDeviceDescription = ptAppParams->uParameter.tFlash.ptDeviceDescription;
	tResult = check_device_description(ptDeviceDescription);

	/* get the source typ */
	tSourceTyp = ptDeviceDescription->tSourceTyp;

	uprintf(". Device :");
	switch(tSourceTyp)
	{
	case BUS_ParFlash:
		/*  use parallel flash */
		uprintf("Parallel flash\n");
		tResult = parflash_flash(&(ptAppParams->uParameter.tFlash));
		break;

	case BUS_SPI:
		/*  use SPI flash */
		uprintf("SPI flash\n");
		tResult = spi_flash(&(ptAppParams->uParameter.tFlash));
		break;

	default:
		/*  unknown boot device */
		uprintf("unknown\n");
		uprintf("! illegal device id specified\n");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
		break;
	}

	return tResult;
}


/* ------------------------------------- */


static NETX_CONSOLEAPP_RESULT_T opMode_erase(tFlasherInputParameter *ptAppParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	const DEVICE_DESCRIPTION_T *ptDeviceDescription;
	BUS_T tSourceTyp;


	/* check the device description */
	ptDeviceDescription = ptAppParams->uParameter.tFlash.ptDeviceDescription;
	tResult = check_device_description(ptDeviceDescription);

	/* get the source typ */
	tSourceTyp = ptDeviceDescription->tSourceTyp;

	uprintf(". Device :");
	switch(tSourceTyp)
	{
	case BUS_ParFlash:
		/*  use parallel flash */
		uprintf("Parallel flash\n");
		tResult = parflash_erase(&(ptAppParams->uParameter.tErase));
		break;

	case BUS_SPI:
		/*  use SPI flash */
		uprintf("SPI flash\n");
		tResult = spi_erase(&(ptAppParams->uParameter.tErase));
		break;

	default:
		/*  unknown boot device */
		uprintf("unknown\n");
		uprintf("! illegal device id specified\n");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
		break;
	}

	return tResult;
}


/* ------------------------------------- */


static NETX_CONSOLEAPP_RESULT_T opMode_read(tFlasherInputParameter *ptAppParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	const DEVICE_DESCRIPTION_T *ptDeviceDescription;
	BUS_T tSourceTyp;


	/* check the device description */
	ptDeviceDescription = ptAppParams->uParameter.tFlash.ptDeviceDescription;
	tResult = check_device_description(ptDeviceDescription);

	/* get the source typ */
	tSourceTyp = ptDeviceDescription->tSourceTyp;

	uprintf(". Device :");
	switch(tSourceTyp)
	{
	case BUS_SPI:
		/*  use SPI flash */
		uprintf("SPI flash\n");
		tResult = spi_read(&(ptAppParams->uParameter.tRead));
		break;

	default:
		/*  unknown boot device */
		uprintf("unknown\n");
		uprintf("! illegal device id specified\n");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
		break;
	}

	return tResult;
}


/* ------------------------------------- */

#if 0
static NETX_CONSOLEAPP_RESULT_T opMode_verify(ptFlasherInputParameter ptAppParams)
{
        NETX_CONSOLEAPP_RESULT_T tResult;
        tBootBlockSrcType tBBSrcType;


        tBBSrcType = (tBootBlockSrcType)ptAppParams->ulBootBlockSrcType;

        uprintf(". Device :");
        switch(tBBSrcType)
        {
        case BootBlockSrcType_OldStyle:
                /*  old style bootblock, default to SPI */
                uprintf("old style, fallback to SPI flash\n");
                tResult = spi_verify(ptAppParams->pbData, ptAppParams->ulDataByteSize);
                break;

        case BootBlockSrcType_SRamBus:
                /*  use parallel flash on SRam bus */
                uprintf("SRam Bus parflash\n");
                tResult = srb_verify(ptAppParams->pbData, ptAppParams->ulDataByteSize);
                break;

        case BootBlockSrcType_SPI:
                /*  use SPI flash */
                uprintf("SPI flash\n");
                tResult = spi_verify(ptAppParams->pbData, ptAppParams->ulDataByteSize);
                break;

        case BootBlockSrcType_I2C:
                /*  use I2C eeprom */
                uprintf("I2C eeprom\n");
                tResult = i2c_verify(ptAppParams->pbData, ptAppParams->ulDataByteSize);
                break;

        case BootBlockSrcType_MMC:
                /*  use MMC/SD card */
                uprintf("MMC / SD card\n");

                /*  not yet... */
                uprintf("! MMC / SD card is not supported yet...\n");
                tResult = NETX_CONSOLEAPP_RESULT_ERROR;

                break;

        case BootBlockSrcType_DPM:
                /*  DPM can't be flashed */
                uprintf("DPM\n");

                uprintf("! DPM is not supported\n");
                tResult = NETX_CONSOLEAPP_RESULT_ERROR;
                break;

        case BootBlockSrcType_DPE:
                uprintf("DPM extended\n");

                /*  DPM extended can't be flashed */
                uprintf("! DPM extented not supported\n");
                tResult = NETX_CONSOLEAPP_RESULT_ERROR;
                break;

        case BootBlockSrcType_ExtBus:
                /*  use parallel flash on Extension bus */
                uprintf("extension bus parflash\n");
                tResult = ext_verify(ptAppParams->pbData, ptAppParams->ulDataByteSize);
                break;

        default:
                /*  unknown boot device */
                uprintf("unknown\n");
                uprintf("! illegal device id specified\n");
                tResult = NETX_CONSOLEAPP_RESULT_ERROR;
                break;
        }

        return tResult;
}


/* ------------------------------------- */

#endif


static NETX_CONSOLEAPP_RESULT_T opMode_isErased(tFlasherInputParameter *ptAppParams, NETX_CONSOLEAPP_PARAMETER_T *ptConsoleParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	BUS_T tSrcType;
	unsigned long ulStartAdr;
	unsigned long ulEndAdr;


	tSrcType = ptAppParams->uParameter.tIsErased.ptDeviceDescription->tSourceTyp;
	ulStartAdr = ptAppParams->uParameter.tIsErased.ulStartAdr;
	ulEndAdr = ptAppParams->uParameter.tIsErased.ulEndAdr;

	if( ulStartAdr>=ulEndAdr )
	{
		uprintf("! first address is greater or equal than last address.");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
	}
	else
	{
		uprintf(". Device :");
		switch(tSrcType)
		{
		case BUS_ParFlash:
			/*  use parallel flash */
			uprintf("Parallel flash\n");
			tResult = parflash_isErased(&(ptAppParams->uParameter.tIsErased), ptConsoleParams);
			break;

		case BUS_SPI:
			/*  use SPI flash */
			uprintf("SPI flash\n");
			tResult = spi_isErased(&(ptAppParams->uParameter.tIsErased), ptConsoleParams);
			break;

		default:
			/*  unknown boot device */
			uprintf("unknown\n");
			uprintf("! illegal device id specified\n");
			tResult = NETX_CONSOLEAPP_RESULT_ERROR;
			break;
		}
	}

	return tResult;
}


/* ------------------------------------- */


static NETX_CONSOLEAPP_RESULT_T opMode_getEraseArea(tFlasherInputParameter *ptAppParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	BUS_T tSrcType;
	unsigned long ulStartAdr;
	unsigned long ulEndAdr;


	tSrcType = ptAppParams->uParameter.tGetEraseArea.ptDeviceDescription->tSourceTyp;
	ulStartAdr = ptAppParams->uParameter.tGetEraseArea.ulStartAdr;
	ulEndAdr = ptAppParams->uParameter.tGetEraseArea.ulEndAdr;

	if( ulStartAdr>=ulEndAdr )
	{
		uprintf("! first address is greater or equal than last address.");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
	}
	else
	{
		uprintf(". Device :");
		switch(tSrcType)
		{
		case BUS_ParFlash:
			/*  use parallel flash */
			uprintf("Parallel flash\n");
			tResult = parflash_getEraseArea(&(ptAppParams->uParameter.tGetEraseArea));
			break;

		case BUS_SPI:
			/*  use SPI flash */
			uprintf("SPI flash\n");
			tResult = spi_getEraseArea(&(ptAppParams->uParameter.tGetEraseArea));
			break;

		default:
			/*  unknown boot device */
			uprintf("unknown\n");
			uprintf("! illegal device id specified\n");
			tResult = NETX_CONSOLEAPP_RESULT_ERROR;
			break;
		}
	}

	return tResult;
}


/* ------------------------------------- */


NETX_CONSOLEAPP_RESULT_T netx_consoleapp_main(NETX_CONSOLEAPP_PARAMETER_T *ptTestParam)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	tFlasherInputParameter *ptAppParams;
	unsigned long ulParamVersion;
	OPERATION_MODE_T tOpMode;


	/* init the board */
	tResult = board_init();
	if( tResult!=NETX_CONSOLEAPP_RESULT_OK )
	{
		/* failed to init board, can not continue */
		setRdyRunLed(RDYRUN_LED_RED);
	}
	else
	{
		/* switch off sys led */
		setRdyRunLed(RDYRUN_LED_OFF);

		/* say hi */
		uprintf("\f\n\n\n\nFlasher v" FLASHER_VERSION_ALL "\n\n");
		uprintf("Copyright (C) 2005-2010 C.Thelen (cthelen@hilscher.com) and M.Trensch.\n");
		uprintf("There is NO warranty.  You may redistribute this software\n");
		uprintf("under the terms of the GNU Library General Public License.\n");
		uprintf("For more information about these matters, see the files named COPYING.\n");

		uprintf("\n");
		uprintf(". Data pointer:    0x%08x\n", (unsigned long)ptTestParam);
		uprintf(". Init parameter:  0x%08x\n", (unsigned long)ptTestParam->pvInitParams);
		uprintf("\n");

		/*  get application parameters */
		ptAppParams = (tFlasherInputParameter*)ptTestParam->pvInitParams;

		/*  check parameter version */
		ulParamVersion = ptAppParams->ulParamVersion;
		if( ulParamVersion!=0x00020000 )
		{
			uprintf("! unknown parameter version: %04x.%04x. Expected 0002.0000!\n", ulParamVersion>>16, ulParamVersion&0xffff);
			setRdyRunLed(RDYRUN_LED_RED);
			tResult = NETX_CONSOLEAPP_RESULT_ERROR;
		}
		else
		{
			/*  run operation */
			tOpMode = ptAppParams->tOperationMode;
			switch( tOpMode )
			{
			case OPERATION_MODE_Detect:
				uprintf(". Operation Mode: Detect\n");
				tResult = opMode_detect(ptAppParams);
				break;

			case OPERATION_MODE_Flash:
				uprintf(". Operation Mode: Flash\n");
				tResult = opMode_flash(ptAppParams);
				break;

			case OPERATION_MODE_Erase:
				uprintf(". Operation Mode: Erase\n");
				tResult = opMode_erase(ptAppParams);
				break;

			case OPERATION_MODE_Read:
				uprintf(". Operation Mode: Read\n");
				tResult = opMode_read(ptAppParams);
				break;
/*
			case OperationMode_Verify:
				uprintf(". Operation Mode: Verify\n");
				tResult = opMode_verify(ptAppParams);
				break;
*/
			case OPERATION_MODE_IsErased:
				uprintf(". Operation Mode: IsErased\n");
				tResult = opMode_isErased(ptAppParams, ptTestParam);
				break;

			case OPERATION_MODE_GetEraseArea:
				uprintf(". Operation Mode: Get Erase Area\n");
				tResult = opMode_getEraseArea(ptAppParams);
				break;

			default:
				uprintf("! unknown operation mode: %d\n", tOpMode);
				setRdyRunLed(RDYRUN_LED_RED);
				tResult = NETX_CONSOLEAPP_RESULT_ERROR;
				break;
			}
		}
	}

	if( tResult==NETX_CONSOLEAPP_RESULT_OK )
	{
		/*  operation ok */
		uprintf("* OK *\n");
		setRdyRunLed(RDYRUN_LED_GREEN);
	}
	else
	{
		/*  operation failed */
		setRdyRunLed(RDYRUN_LED_RED);
	}


	return tResult;
}

