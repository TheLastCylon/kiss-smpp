// File  : ksmppc.hpp
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

#ifndef _KSMPPC_HPP_
#define _KSMPPC_HPP_

#include <boost/thread.hpp>
#include <smpp_pdu_all.hpp>

#include <kisscpp/server.hpp>
#include <kisscpp/client.hpp>
#include <kisscpp/ptree_queue.hpp>
#include <kisscpp/logstream.hpp>

#include "cfg.hpp"
#include "util.hpp"
#include "session_manager.hpp"
#include "handler_send.hpp"
#include "smpppdu_queue.hpp"

// ----------------------- TODO: -----------------------------
//*- Gnu automake implementation
//*- Copyright/left messages at top of files
//*- client transmissions.
//*  -- deadline timer on client. (Timeout)
//*- seperation of application base into library
//*- Logging
//*- Make logging honour SIGHUP
//*- Persistant Queue
//*- Communications Buffer -- equivalent of ViaMedia Queues.
//*- Throtling
//   -- do work for honouring Throtling errors.
//*- Various SMPP Timers
//*  -- Enquire Link
//*  -- Response Timers
//*     -- Awaiting respose map - i.e. Hold Queue
//         -- configured max size
//         -- max age for an item in the list.
//*- Session manager
//*  -- re-connects
//   -- on startup, when there are items already in the queue... how to deal with that?
//   -- Also, how to keep transmission going with items on disk.
//   -- Session manager: more descriptive messages on bind request failures.
// - Allow submit_multi_sm if config sais that MC supports it.
// - Allow data_sm if config sais that MC supports it.
// - Messaging:
//   -- Multi-Part messages. i.e. Messages exceeding 160 characters.
//   -- Binary SMS sending & recieving. 
//*- SMPP_PDU something funky with the message creation. ---: Testing show's it's sorted. Keep it in mind though.
// - Handlers;
//   -- re-process error buffer
//   -- dump error buffer
//*- Run application as Daemon
// - user traceable messages.
// - 
// - Documentation.
// - deb generation
// - rpm generation

typedef boost::shared_ptr<SessionManager>   SharedSession;

class ksmppc : public kisscpp::Server
{
  public:
    ksmppc(const std::string &instance,
           const bool        &runAsDaemon);
    ~ksmppc();

  protected:
    void constructQueues();
    void registerHandlers();
    void startSessions();
    void startThreads();
    void recieveProcessor();
    void sendingProcessor();

    void smpp2ptree     (SharedSmppPdu pdu, BoostPtree &pt);
    void dataSm2Ptree   (SharedSmppPdu pdu, BoostPtree &pt);
    void deliverSm2Ptree(SharedSmppPdu pdu, BoostPtree &pt);

  private:
    SharedSafeSmppPduQ          sendingBuffer;
    SharedSafeSmppPduQ          recieveBuffer;
    SharedSafeSmppPduQ          rcv_errBuffer; //Recieving-error buffer. Perminant comms failures go here
    SharedSession               session;
    bool                        running;
    kisscpp::RequestHandlerPtr  sendHandler;
    boost::asio::io_service     sessionIoService;
    boost::asio::io_service     clientIoService;
    boost::thread_group         threadGroup;
};

#endif //_KSMPPC_HPP_

