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


#include "flasher_header.h"

#include "flasher_version.h"


const tFlasherVersion flasher_version __attribute__ ((section (".version_info")));


const tFlasherVersion flasher_version =
{
	.abMagic = { FLASHER_MAGIC0, FLASHER_MAGIC1, FLASHER_MAGIC2, FLASHER_MAGIC3 },

	.ulVersionMaj = FLASHER_VERSION_MAJ,
	.ulVersionMin = FLASHER_VERSION_MIN,
	.acVersion    = FLASHER_VERSION_ALL,

	.pulLoadAddress = __LOAD_ADDRESS__,
	.pfnExecutionAddress = start,

	.aulChiptyp =
	{
		FLASHER_CHIPTYP0_NETX50,

		0
	},
	.aulIf =
	{
		FLASHER_IF0_NETX50_PFSRAM |
		FLASHER_IF0_NETX50_PFEXT |
		FLASHER_IF0_NETX50_SPI,

		0,

		0,

		0
	}
};

/*-----------------------------------*/

