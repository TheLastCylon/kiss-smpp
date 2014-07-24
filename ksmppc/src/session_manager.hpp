// File  : session_manager.hpp
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

#ifndef _SESSION_MANAGER_HPP_
#define _SESSION_MANAGER_HPP_

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <string>
#include <deque>
#include <map>
#include <ctime>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <smpp_pdu_all.hpp>
#include <kisscpp/logstream.hpp>

#include "configuration_manager.hpp"
#include "smpppdu_queue.hpp"
#include "transmit_queue.hpp"
#include "bind_type.hpp"
#include "sharedsmpppdu.hpp"

using boost::asio::ip::tcp;


//--------------------------------------------------------------------------------
class RawPdu
{

  public:
    RawPdu() :
      header    (NULL),
      fullBuffer(NULL),
      body      (NULL)
    {
      header = new uint8_t[16];
    }

    ~RawPdu()
    {
      if(header && fullBuffer) {
        delete [] fullBuffer;
      } else if(header) {
        delete [] header;
      }
    }

    uint8_t *bodyBuf()
    {
      if(header) {
        if(fullBuffer) delete fullBuffer;
        fullBuffer = new uint8_t[cmd_length()];
        memcpy(fullBuffer, header, 16);
        delete [] header;
        header     = fullBuffer;
        body       = fullBuffer + 16;
      }
      return body;
    }

    uint8_t    *data      () { return fullBuffer; }
    uint8_t    *headerBuf () { return header; }
    const char *c_str     () { return (fullBuffer)?reinterpret_cast<const char *>(fullBuffer):NULL; }

    uint32_t    bodyLength() { return (cmd_length() - 16); }
    uint32_t    cmd_length() { return (header)?smpp_pdu::get_command_length (header):0; }
    uint32_t    cmd_id    () { return (header)?smpp_pdu::get_command_id     (header):0; }
    uint32_t    cmd_status() { return (header)?smpp_pdu::get_command_status (header):0; }
    uint32_t    seq_num   () { return (header)?smpp_pdu::get_sequence_number(header):0; }
      
  private:
    uint8_t  *header;
    uint8_t  *fullBuffer;
    uint8_t  *body;
    uint32_t  cmdLength;
};

typedef boost::shared_ptr<RawPdu> SharedRawPdu;

//--------------------------------------------------------------------------------
class SequinceNumberGenerator
{
  public:
    SequinceNumberGenerator() : mVal(smpp_pdu::SequenceNumber::Min) {}

    uint32_t next() // Returns the next number in the uniformly incrementing sequence of numbers.
    {
      boost::lock_guard<boost::mutex> l(mMtx);
      if(mVal < smpp_pdu::SequenceNumber::Max) {
        return mVal++;
      } else {
        mVal = smpp_pdu::SequenceNumber::Min;
        return smpp_pdu::SequenceNumber::Max;
      }
    }

  private:
    uint32_t     mVal;
    boost::mutex mMtx;
};

//--------------------------------------------------------------------------------
class timeStampedPdu
{
  public:
    timeStampedPdu(SharedSmppPdu o)
    {
      obj       = o;
      timestamp = time(NULL);
    }

    ~timeStampedPdu() {};

    SharedSmppPdu getObj()       { return obj; };
    time_t        getTimestamp() { return timestamp; };
    uint32_t      pduSeqNum()    { return (uint32_t)obj->sequence_number; }

    bool expired(time_t seconds)
    {
      return ((time(NULL) - timestamp) >= seconds);
    }

  private:
    time_t        timestamp;
    SharedSmppPdu obj;
};

//--------------------------------------------------------------------------------
typedef boost::shared_ptr<timeStampedPdu>        SharedTimeStampedPdu;
typedef std::map<uint32_t, SharedTimeStampedPdu> AwaitingResponseMapType;
typedef AwaitingResponseMapType::iterator        AwaitingResponseMapTypeItr;

//--------------------------------------------------------------------------------
class SessionManager
{
  public:
    SessionManager(boost::asio::io_service &io_service,
                   SharedConfig             cfg,
                   SharedSafeSmppPduQ       recieveQueue);

    ~SessionManager() {};

    void send_pdu(const SharedSmppPdu pdu);

    void stop() { stopFlag = true; close_session(false); };

    enum State { OPEN, BOUND_TX, BOUND_RX, BOUND_TRX, UNBOUND, CLOSED, OUTBOUND }; // Session States

    void setSystemId        (smpp_pdu::SystemId         &p) { systemId         = p; }
    void setPassword        (smpp_pdu::Password         &p) { password         = p; }
    void setSystemType      (smpp_pdu::SystemType       &p) { systemType       = p; }
    void setInterfaceVersion(smpp_pdu::InterfaceVersion &p) { interfaceVersion = p; }
    void setAddrTon         (smpp_pdu::Ton              &p) { addrTon          = p; }
    void setAddrNpi         (smpp_pdu::Npi              &p) { addrNpi          = p; }
    void setAddressRange    (smpp_pdu::AddressRange     &p) { addressRange     = p; }
    void associateRXQ       (SharedSafeSmppPduQ              p) { rxQ              = p; }

    State                      &getCurrentState    () { return currentState    ; }
    smpp_pdu::SystemId         &getSystemId        () { return systemId        ; }
    smpp_pdu::Password         &getPassword        () { return password        ; }
    smpp_pdu::SystemType       &getSystemType      () { return systemType      ; }
    smpp_pdu::InterfaceVersion &getInterfaceVersion() { return interfaceVersion; }
    smpp_pdu::Ton              &getAddrTon         () { return addrTon         ; }
    smpp_pdu::Npi              &getAddrNpi         () { return addrNpi         ; }
    smpp_pdu::AddressRange     &getAddressRange    () { return addressRange    ; }

  private:
    void close_session                   (bool re_connect = false);
    void initiate                        ();
    void setTxq                          ();
    void connect                         ();
    void setCurrentState                 (State p);
    bool canSend                         ();
    bool writing                         ();

    void handle_connect                  (const boost::system::error_code& error);
    void handle_read_header              (SharedRawPdu rawpdu, const boost::system::error_code& error);
    void handle_read_body                (SharedRawPdu rawpdu, const boost::system::error_code& error);
    void handle_write                    (const boost::system::error_code& error);
    void throttle_check                  ();
    void do_write                        (const SharedSmppPdu pdu, unsigned priority = TransmitQ::MESSAGE);
    void write_pdu                       ();
    void close_connection                ();

    void do_bind_request                 ();
    void do_unbind_request               ();

    void send4state_bound_tx             (const SharedSmppPdu pdu);
    void send4state_bound_rx             (const SharedSmppPdu pdu);
    void send4state_bound_trx            (const SharedSmppPdu pdu);

    void process4state                   (SharedRawPdu rawpdu);
    void process4state_open              (SharedRawPdu rawpdu);
    void process4state_bound_tx          (SharedRawPdu rawpdu);
    void process4state_bound_rx          (SharedRawPdu rawpdu);
    void process4state_bound_trx         (SharedRawPdu rawpdu);
    void process4state_unbound           (SharedRawPdu rawpdu);
    void process4state_closed            (SharedRawPdu rawpdu);
    void process4state_outbound          (SharedRawPdu rawpdu);

    void procpdu_bind_resp               (SharedRawPdu rawpdu, State stateAferSuccess);
    void procpdu_alert_notification      (SharedRawPdu rawpdu);
    void procpdu_broadcast_sm_resp       (SharedRawPdu rawpdu);
    void procpdu_cancel_broadcast_sm_resp(SharedRawPdu rawpdu);
    void procpdu_cancel_sm_resp          (SharedRawPdu rawpdu);
    void procpdu_data_sm                 (SharedRawPdu rawpdu);
    void procpdu_data_sm_resp            (SharedRawPdu rawpdu);
    void procpdu_deliver_sm              (SharedRawPdu rawpdu);
    void procpdu_enquire_link            (SharedRawPdu rawpdu);
    void procpdu_enquire_link_resp       (SharedRawPdu rawpdu);
    void procpdu_generic_nack            (SharedRawPdu rawpdu);
    void procpdu_query_broadcast_sm_resp (SharedRawPdu rawpdu);
    void procpdu_query_sm_resp           (SharedRawPdu rawpdu);
    void procpdu_replace_sm_resp         (SharedRawPdu rawpdu);
    void procpdu_submit_multi_resp       (SharedRawPdu rawpdu);
    void procpdu_submit_sm_resp          (SharedRawPdu rawpdu);
    void procpdu_unbind                  (SharedRawPdu rawpdu);
    void procpdu_unbind_resp             (SharedRawPdu rawpdu);

    void reschedule_enquire_link         ();
    void do_enquire_link                 (const boost::system::error_code& e);
    void do_enquire_link_failure         (const boost::system::error_code& e);

    void print_pdu                       (SharedSmppPdu pdu); // this method exists for debug purposes only, don't use it if you don't need to.

    void w4rQ_put                        (SharedSmppPdu                    pdu);
    void w4rQ_pop                        (SharedRawPdu                     rawpdu);
    void w4rQ_age_cleanup                (const boost::system::error_code &e);
    void set_w4rQ_ageing_timer           ();

    // vars
    boost::asio::io_service             &io_service_;
    tcp::socket                          socket_;
    tcp::resolver::iterator              endpoint_iterator;

    std::string                          data_buffer;
    char                                 header_buffer[16];
    char                                *body_buffer;
    uint32_t                             body_length;

    smpp_pdu::SystemId                   systemId;
    smpp_pdu::Password                   password;
    smpp_pdu::SystemType                 systemType;
    smpp_pdu::InterfaceVersion           interfaceVersion;
    smpp_pdu::Ton                        addrTon;
    smpp_pdu::Npi                        addrNpi;
    smpp_pdu::AddressRange               addressRange;

    bool                                 logPduFlag;
    bool                                 stopFlag;
    bool                                 reconnectFlag;
    BindType                             typeOfBind;
    SharedSafeSmppPduQ                   rxQ;
    ScopedTransmitQ                      txQ;
    State                                currentState;
    SequinceNumberGenerator              seqNumGen;

    boost::posix_time::seconds           enquire_link_timeout;
    boost::posix_time::seconds           enquire_link_resp_timeout;
    boost::asio::deadline_timer          enquire_link_timer;
    boost::asio::deadline_timer          enquire_link_response_timer;

    boost::posix_time::ptime             throttleNextSendTime;
    unsigned                             timeBetweenSends; // microseconds between sends

    SharedConfig                         config;

    AwaitingResponseMapType              w4rQ; // a map of sent PDUs that are (W)aiting 4 (R)esponses.
    boost::asio::deadline_timer          w4rQ_ageing_timer;

    boost::mutex                         writeMutex;
    boost::mutex                         w4rQMutex;

    unsigned                             readCount;  // microseconds between sends
    unsigned                             writeCount; // microseconds between sends
    boost::posix_time::ptime             startTime;
};

#endif

