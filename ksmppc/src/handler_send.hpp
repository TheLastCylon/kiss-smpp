// File  : handler_send.hpp
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

#ifndef _HANDLER_SEND_HPP_
#define _HANDLER_SEND_HPP_

#include <iostream>
#include <string>
#include <smpppdu_all.hpp>

#include <kisscpp/logstream.hpp>
#include <kisscpp/request_handler.hpp>
#include <kisscpp/request_status.hpp>
#include <kisscpp/boost_ptree.hpp>

#include "util.hpp"
#include "cfg.hpp"
#include "smpppdu_queue.hpp"

class SendHandler : public kisscpp::RequestHandler
{
  public:
    SendHandler(SharedSafeSmppPduQ snQ) :
      kisscpp::RequestHandler("send", "Used for sending messages.")
    {
      kisscpp::LogStream log(__PRETTY_FUNCTION__);

      sendingQ = snQ;
    };

    ~SendHandler() {};

    void run(const BoostPtree& request, BoostPtree& response);

  protected:

  private:
    SharedSafeSmppPduQ sendingQ;
};

#endif

