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

#include "netx_consoleapp.h"


#ifndef __FLASHER_I2C_H__
#define __FLASHER_I2C_H__

NETX_CONSOLEAPP_RESULT_T i2c_flash (const unsigned char *pbData, unsigned long ulDataByteLen);
NETX_CONSOLEAPP_RESULT_T i2c_erase (unsigned long       ulDataByteLen);
NETX_CONSOLEAPP_RESULT_T i2c_read  (unsigned char       *pbData, unsigned long ulDataByteLen);
NETX_CONSOLEAPP_RESULT_T i2c_verify(const unsigned char *pbData, unsigned long ulDataByteLen);

#endif  // __FLASHER_I2C_H__
