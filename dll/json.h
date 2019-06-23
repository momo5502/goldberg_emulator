/* Copyright (C) 2019 Nemirtingas
   This file is part of the Goldberg Emulator

   Those utils are free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   Those utils are distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.
*/

#ifndef __JSON_INCLUDED__
#define __JSON_INCLUDED__

#include "json_value.h"

namespace nemir
{
    json_value parse_json(std::string const& buffer);
}

#endif//__JSON_INCLUDED__