/*
 * libtilemcore - Graphing calculator emulation library
 *
 * Copyright (C) 2011 Benjamin Moody
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
#include <string.h>
#include "tilem.h"

void tilem_usbctl_reset(TilemCalc* calc TILEM_ATTR_UNUSED)
{
}

void tilem_usb_frame_timer(TilemCalc* calc TILEM_ATTR_UNUSED,
                           void* data TILEM_ATTR_UNUSED)
{
}
