// File  : stat.hpp
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

#ifndef _STAT_HPP_
#define _STAT_HPP_


#include <kisscpp/statskeeper.hpp>

#define statInc(X)   kisscpp::StatsKeeper::instance()->increment(X);
#define statDec(X)   kisscpp::StatsKeeper::instance()->decrement(X);
#define statSet(X,Y) kisscpp::StatsKeeper::instance()->setStatValue(X,Y);
#define statQue(X,Y) kisscpp::StatsKeeper::instance()->addStatableQueue(X,Y);

#endif
