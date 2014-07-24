// File  : main.cpp
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

#include "ksmppc.hpp"

//--------------------------------------------------------------------------------
void usage()
{
  std::cerr << "Usage: smppgw <config-file>\n";
}

// https://www.youtube.com/watch?v=hornvnjML88

//--------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  if(argc < 2) { usage(); return 1; }

  try {
    //std::cout << "SMPPGW: Starting" << std::endl;

    SharedConfig config;

    config.reset(new ConfigurationManager(argv[1]));
    ksmppc app(config);

    app.run();

    //std::cout << "SMPPGW: Stopped cleanly." << std::endl;
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

