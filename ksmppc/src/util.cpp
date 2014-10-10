// File  : util.hpp
// Author: Dirk J. Botha <bothadj@gmail.com>
//
// This file is part of ksmppcd application. Which is part of the KISS-SMPP
// project.
//
// The ksmppcd application is free software: you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// The ksmppcd application is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with the ksmppcd application.
// If not, see <http://www.gnu.org/licenses/>.

#include "util.hpp"

//--------------------------------------------------------------------------------
uint8_t makeInterfaceVersion(int i)
{
  switch(i) {
    case 33: return 0x33; break;
    case 34: return 0x34; break;
    case 50: return 0x50; break;
    default: return 0x34; break;
  }
}

//--------------------------------------------------------------------------------
BindType makeBindType(std::string s)
{
  if(s == "RX" ) return RX;
  if(s == "TX" ) return TX;
  if(s == "TRX") return TRX;
}
