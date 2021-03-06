/* Mulator - An extensible {ARM} {e,si}mulator
 * Copyright 2011-2013  Pat Pannuto <pat.pannuto@gmail.com>
 *
 * This file is part of Mulator.
 *
 * Mulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mulator.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STATIC_ROM_H
#define STATIC_ROM_H

// bintoarray.sh; "echo.bin"
#include <stdint.h>
#define STATIC_ROM_NUM_BYTES (107 * 4)
uint32_t static_rom[107];

#define STATIC_ROM_BLURB "\
\t\t\techo.bin: Listens for a character on the polling UART\n\
\t\t\t          device and echoes it back\n"

#endif // STATIC_ROM_H
