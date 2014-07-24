// File  : transmit_queue.hpp
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

#ifndef _TRANSMIT_QUEUE_HPP_
#define _TRANSMIT_QUEUE_HPP_

#include "smpppdu_queue.hpp"

class TransmitQ : public PrioritisedSmppPduQ
{
  public:
    enum {
      SESSION = 0,
      RESPONSE,
      MESSAGE,
      PRIORITY_LEVELS
    };

    TransmitQ(const std::string& queueName,
              const std::string& queueWorkingDir,
              const unsigned     maxItemsPerPage) :
      PrioritisedSmppPduQ(queueName,
                          queueWorkingDir,
                          PRIORITY_LEVELS,
                          maxItemsPerPage)
    {
      // session management and response PDU's are not transferable between sessions
      clearSessionQueues();
    }

    void clearSessionQueues()
    {
      clear(TransmitQ::SESSION);
      clear(TransmitQ::RESPONSE);
    }

};

typedef boost::scoped_ptr<TransmitQ> ScopedTransmitQ;

#endif // _THREADSAFE_PERSISTED_PRIORITY_QUEUE_HPP_

