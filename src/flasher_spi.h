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


#ifndef __FLASHER_SPI_H__
#define __FLASHER_SPI_H__

#include "netx_consoleapp.h"
#include "flasher_interface.h"
#include "sha1.h"


NETX_CONSOLEAPP_RESULT_T spi_flash(CMD_PARAMETER_FLASH_T *ptParameter);
NETX_CONSOLEAPP_RESULT_T spi_erase(CMD_PARAMETER_ERASE_T *ptParameter);
NETX_CONSOLEAPP_RESULT_T spi_smart_erase(CMD_PARAMETER_SMART_ERASE_T *ptParameter);
void analyzeMap(unsigned char * cHexMap, int cHexMapLen, CMD_PARAMETER_SMART_ERASE_T *ptParameter);
void performErase(int EraseMode, unsigned long startSector, CMD_PARAMETER_SMART_ERASE_T *ptParameter);


NETX_CONSOLEAPP_RESULT_T spi_read(CMD_PARAMETER_READ_T *ptParameter);
NETX_CONSOLEAPP_RESULT_T spi_sha1(CMD_PARAMETER_CHECKSUM_T *ptParameter, SHA_CTX *ptSha1Context);
NETX_CONSOLEAPP_RESULT_T spi_verify(CMD_PARAMETER_VERIFY_T *ptParameter, NETX_CONSOLEAPP_PARAMETER_T *ptConsoleParams);

NETX_CONSOLEAPP_RESULT_T spi_detect(CMD_PARAMETER_DETECT_T *ptParameter);
NETX_CONSOLEAPP_RESULT_T spi_isErased(CMD_PARAMETER_ISERASED_T *ptParameter, NETX_CONSOLEAPP_PARAMETER_T *ptConsoleParams);
NETX_CONSOLEAPP_RESULT_T spi_getEraseArea(CMD_PARAMETER_GETERASEAREA_T *ptParameter);


void newArray(unsigned char ** boolArray, long long int dimension);
int setValue(unsigned char * array, long long index, unsigned char val);
unsigned char getValue(unsigned char * array, long long index);
void dumpBoolArray16(unsigned char * map, int len, const char * description);

void initMemory(void);
unsigned char * getMemory(long long int sizeByte);
long long int totalMemory;
unsigned char * memStarPtr;
unsigned char * memCurrentPtr;
unsigned char * memEndPtr;
long long int freeMem;


void setSFDPData(char isValid, char eraseOperation1, char eraseInstruction1, char eraseOperation2, char eraseInstruction2, char eraseOperation3, char eraseInstruction3, char eraseOperation4, char eraseInstruction4);


// some init things?
struct SFDP_Data{
	char isValid;
	int eraseOperation1;
	char eraseInstruction1;
	int eraseOperation2;
	char eraseInstruction2;
	int eraseOperation3;
	char eraseInstruction3;
	int eraseOperation4;
	char eraseInstruction4;
};

struct SFDP_Data * myData;

#endif	/* __FLASHER_SPI_H__ */
