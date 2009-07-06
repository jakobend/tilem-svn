/*
 * libtilemcore - Graphing calculator emulation library
 *
 * Copyright (C) 2001 Solignac Julien
 * Copyright (C) 2004-2009 Benjamin Moody
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <tilem.h>

#include "x3.h"

void x3_reset(TilemCalc* calc)
{
	calc->hwregs[PORT2] = 0xF8;
	calc->hwregs[PORT3] = 0x0B;
	calc->hwregs[PORT4] = 0x00;
	calc->hwregs[ROM_BANK] = 0x00;

	calc->mempagemap[0] = 0x00;
	calc->mempagemap[1] = 0x00;
	calc->mempagemap[2] = 0x11;
	calc->mempagemap[3] = 0x10;

	tilem_z80_set_speed(calc, 6000);

	tilem_z80_set_timer(calc, TIMER_INT1, 1600, 9259, 1);
	tilem_z80_set_timer(calc, TIMER_INT2A, 1300, 9259, 1);
	tilem_z80_set_timer(calc, TIMER_INT2B, 1000, 9259, 1);
}

int x3_checkrom(FILE* romfile)
{
	return tilem_rom_find_string("TI82", romfile, 0x10 * 0x4000);
}
