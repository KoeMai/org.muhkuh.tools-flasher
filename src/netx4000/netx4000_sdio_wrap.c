#include "netx4000_sdio.h"
#include "netx4000_sdio_wrap.h"

//#include "cr7_global_timer.h"
//#include "netx_io_areas.h"
#include "portcontrol.h"
#include "uprintf.h"
#include "progress_bar.h"

static const SDIO_OPTIONS_T tSdioOptions =
{
	.ulPowerUpTimeoutTicks = 200000U,    /* The timeout to get the SDIO core out of the power down mode. This is 1ms. */
	.ulResetDelayTicks     = 1000000U,   /* The delay between power enable and reset release. This is 5ms. */
	.ulInitialSpeedKHz = 400,            /* Start with 400kHz. */
	.ulMaximumSpeedKHz = 25000,          /* Allow a maximum of 25MHz. */
	.ulInitialClockGenerationUs = 3032,  /* Delay for 3032us, which is enough for the required 74 clock cycles at 400kHz. */
	.ausPortControl =
	{
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLDOWN50K),    /* SDIO_CLK  */
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLUP50K),      /* SDIO_CMD  */
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLUP50K),      /* SDIO_DAT0 */
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLUP50K),      /* SDIO_DAT1 */
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLUP50K),      /* SDIO_DAT2 */
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLUP50K),      /* SDIO_DAT3 */
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLUP50K),      /* SDIO_CD   */
		PORTCONTROL_CONFIGURATION(REEMUX_0, 0, REEMUX_DRV_08MA, REEMUX_UDC_PULLUP50K)       /* SDIO_WP   */
	},
	.ucHwTimeoutExponentInitialization = 20,
	.ucHwTimeoutExponentTransfer       = 27
};
 
/*-------------------------------------------------------------------------*/

NETX_CONSOLEAPP_RESULT_T sdio_detect_wrap(SDIO_HANDLE_T *ptSdioHandle)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	int iResult;

	uprintf(". Detecting SD/EMMC on SDIO port...\n");

	iResult = sdio_detect(ptSdioHandle, &tSdioOptions);
	if( iResult!=0 )
	{
		uprintf("! failed to detect SD/EMMC!\n");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
	}
	else
	{
		uprintf(". OK, found SD/EMMC\n");
		tResult = NETX_CONSOLEAPP_RESULT_OK;
	}
	return tResult;
}

/* must be a power of two */
#define SD_SECTOR_SIZE  (512)
#define SD_SECTOR_SIZE_LOG (9)
#define SD_SECTOR_SIZE_MSK (0x1ff)


NETX_CONSOLEAPP_RESULT_T sdio_read(CMD_PARAMETER_READ_T *ptParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	int iResult;

	const SDIO_HANDLE_T *ptSdioHandle;
	
	unsigned long ulProgressCnt;
	
	unsigned long ulFlashAdr;
	unsigned long ulFlashEndAdr;
	unsigned char *pucDataAdr;
	
	unsigned long ulSectorId;
	unsigned long ulSectorOffset;
	unsigned long ulChunkLength;
	
	union {
		unsigned long aul[SD_SECTOR_SIZE/4];
		unsigned char auc[SD_SECTOR_SIZE];
	} tSector;
	
	ptSdioHandle = &ptParams->ptDeviceDescription->uInfo.tSdioHandle;
	
	ulFlashAdr = ptParams->ulStartAdr;
	ulFlashEndAdr = ptParams->ulEndAdr;
	pucDataAdr = ptParams->pucData;

	ulProgressCnt = 0;
	progress_bar_init(ulFlashEndAdr-ulFlashAdr);
	
	uprintf(". Reading...\n");
	
	while (ulFlashAdr < ulFlashEndAdr)
	{
		ulSectorId = ulFlashAdr >> SD_SECTOR_SIZE_LOG; /* divide by sector size 512 bytes */
		ulSectorOffset = ulFlashAdr & SD_SECTOR_SIZE_MSK; /* take the offset in a 512 byte sector */
		ulChunkLength = SD_SECTOR_SIZE - ulSectorOffset;
		
		uprintf("addr: 0x%08x  sector ID: %d  sector offset: %d\n", ulFlashAdr, ulSectorId, ulSectorOffset);
		//static int sdio_read_sector(SDIO_HANDLE_T *ptSdioHandle, unsigned long ulSectorId, unsigned long *pulRead)
		iResult = sdio_read_sector(ptSdioHandle, ulSectorId, tSector.aul);
		if( iResult!=0 )
		{
			uprintf("! failed to read sector %d!\n", ulSectorId);
			tResult = NETX_CONSOLEAPP_RESULT_ERROR;
			break;
		}
		
		memcpy(pucDataAdr, tSector.auc + ulSectorOffset, ulChunkLength);
		pucDataAdr += ulChunkLength;
		ulFlashAdr += ulChunkLength;
		
		/* inc progress */
		ulProgressCnt += ulChunkLength;
		progress_bar_set_position(ulProgressCnt);
	}
	
	progress_bar_finalize();
	
	if( iResult!=0 )
	{
		uprintf("! failed to read from SD/EMMC!\n");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
	}
	else
	{
		uprintf(". OK\n");
		tResult = NETX_CONSOLEAPP_RESULT_OK;
	}
	return tResult;
}

/* 
    ulStartAddr/ulEndAddr are relative offsets into the flash,
    pucData is an absolute pointer to the RAM buffer.
   
typedef struct CMD_PARAMETER_FLASH_STRUCT
{
	const DEVICE_DESCRIPTION_T *ptDeviceDescription;
	unsigned long ulStartAdr;
	unsigned long ulDataByteSize;
	unsigned char *pucData;
} CMD_PARAMETER_FLASH_T;
*/
 

NETX_CONSOLEAPP_RESULT_T sdio_write(CMD_PARAMETER_FLASH_T *ptParams)
{
	NETX_CONSOLEAPP_RESULT_T tResult;
	int iResult;

	const SDIO_HANDLE_T *ptSdioHandle;
	unsigned long ulProgressCnt;
	
	unsigned long ulFlashAdr;
	unsigned long ulFlashEndAdr;
	unsigned char *pucDataAdr;
	
	unsigned long ulSectorId;
	unsigned long ulSectorOffset;
	unsigned long ulChunkLength;
	
	union {
		unsigned long aul[SD_SECTOR_SIZE/4];
		unsigned char auc[SD_SECTOR_SIZE];
	} tSector;
	
	ptSdioHandle = &ptParams->ptDeviceDescription->uInfo.tSdioHandle;
	
	ulFlashAdr = ptParams->ulStartAdr;
	ulFlashEndAdr = ptParams->ulStartAdr + ptParams->ulDataByteSize;
	pucDataAdr = ptParams->pucData;

	ulProgressCnt = 0;
	progress_bar_init(ulFlashEndAdr-ulFlashAdr);
	
	uprintf(". Writing...\n");
	
	while (ulFlashAdr < ulFlashEndAdr)
	{
		ulSectorId = ulFlashAdr >> SD_SECTOR_SIZE_LOG; /* divide by sector size 512 bytes */
		ulSectorOffset = ulFlashAdr & SD_SECTOR_SIZE_MSK; /* take the offset in a 512 byte sector */
		ulChunkLength = SD_SECTOR_SIZE - ulSectorOffset;
		
		uprintf("addr: 0x%08x  sector ID: %d  sector offset: %d\n", ulFlashAdr, ulSectorId, ulSectorOffset);
		//static int sdio_read_sector(SDIO_HANDLE_T *ptSdioHandle, unsigned long ulSectorId, unsigned long *pulRead)
		
		if ((ulSectorOffset != 0) || (ulChunkLength != SD_SECTOR_SIZE))
		{
			iResult = sdio_read_sector(ptSdioHandle, ulSectorId, tSector.aul);
			if( iResult!=0 )
			{
				uprintf("! failed to read sector %d!\n", ulSectorId);
				tResult = NETX_CONSOLEAPP_RESULT_ERROR;
				break;
			}
		}
		
		memcpy(tSector.auc + ulSectorOffset, pucDataAdr, ulChunkLength);
		
		iResult = sdio_write_sector(ptSdioHandle, ulSectorId, tSector.aul);
		if( iResult!=0 )
		{
			uprintf("! failed to write sector %d!\n", ulSectorId);
			tResult = NETX_CONSOLEAPP_RESULT_ERROR;
			break;
		}
		
		pucDataAdr += ulChunkLength;
		ulFlashAdr += ulChunkLength;
		
		/* inc progress */
		ulProgressCnt += ulChunkLength;
		progress_bar_set_position(ulProgressCnt);
	}
	
	progress_bar_finalize();
	
	if( iResult!=0 )
	{
		uprintf("! failed to read from/write to SD/EMMC!\n");
		tResult = NETX_CONSOLEAPP_RESULT_ERROR;
	}
	else
	{
		uprintf(". OK\n");
		tResult = NETX_CONSOLEAPP_RESULT_OK;
	}
	return tResult;
}

