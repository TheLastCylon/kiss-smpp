// File  : session_manager.cpp
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

#include "session_manager.hpp"

//--------------------------------------------------------------------------------
SessionManager::SessionManager(boost::asio::io_service &io_service,
                               SharedSafeSmppPduQ       recieveQueue) :
  io_service_                (io_service),
  socket_                    (io_service_),
  enquire_link_timer         (io_service_),
  enquire_link_response_timer(io_service_),
  w4rQ_ageing_timer          (io_service_),
  logPduFlag                 (true),                   // TODO: set to false by default after initial testing is completed.
  stopFlag                   (false),
  reconnectFlag              (false),
  rxQ                        (recieveQueue),
  currentState               (SessionManager::CLOSED)
{
  readCount  = 0;
  writeCount = 0;

  start_session();
  setTxq();
  set_w4rQ_ageing_timer();
  connect();
}

//--------------------------------------------------------------------------------
void SessionManager::initiate()
{
  tcp::resolver         resolver(io_service_);
  tcp::resolver::query  query(CFG->get<std::string>("message-centre.host"),
                              CFG->get<std::string>("message-centre.port"));

  endpoint_iterator = resolver.resolve(query);

  setTxq();
  connect();
}

//--------------------------------------------------------------------------------
void SessionManager::setTxq()
{
  std::string qName;
  switch(smppcfg.getTypeOfBind()) {
    case TX : qName = "tx_"; break;
    case RX : qName = "rx_"; break;
    case TRX: qName = "trx"; break;
    default : break;
  }

  qName += "MessageTxq";

  txQ.reset(new TransmitQ(qName, "/tmp", 10));

  // TODO: Make working dir and max items in que configurable.
  // TODO: Make sure the queue name reflects at least a session id, for when there are multiple sessions.
}

//--------------------------------------------------------------------------------
void SessionManager::connect()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  log << "Connecting" << kisscpp::manip::flush;
  boost::asio::async_connect(socket_,
                             endpoint_iterator,
                             boost::bind(&SessionManager::handle_connect, this, boost::asio::placeholders::error));
}

//--------------------------------------------------------------------------------
void SessionManager::setCurrentState(State p)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  currentState = p;

  log << "NEW State: ";
  switch(currentState) {
    case OPEN     : log << "OPEN"     ; break;
    case BOUND_TX : log << "BOUND_TX" ; break;
    case BOUND_RX : log << "BOUND_RX" ; break;
    case BOUND_TRX: log << "BOUND_TRX"; break;
    case UNBOUND  : log << "UNBOUND"  ; break;
    case CLOSED   : log << "CLOSED"   ; break;
    case OUTBOUND : log << "OUTBOUND" ; break;
    default       : log << "WTF"      ; break;
  }
  log << kisscpp::manip::flush;
}

//--------------------------------------------------------------------------------
bool SessionManager::canSend()
{ 
  bool retval = false;

  if(w4rQ.size() < 10) { // TODO: Make this configurable, max Items in w4rQ
      switch(currentState) {
        case BOUND_TX:
        case BOUND_RX:
        case BOUND_TRX:
          retval = true;
          break;
        default:
          retval = false;
          break;
      }
  }

  return retval;
}

//--------------------------------------------------------------------------------
bool SessionManager::writing()
{
  return (!txQ->empty());
}

//--------------------------------------------------------------------------------
void SessionManager::send_pdu(const SharedSmppPdu pdu)
{
  // This is the only method that external classes should be allowed to use to get messages on to the PDU queue.
  // Right now I don't know wither or not I'll be running into concurrency issues, by having multiple
  // external sources access this one entry point. i.e. This is a subjec for extreme testing.
  switch(currentState) {
    case BOUND_TX : send4state_bound_tx (pdu); break;
    case BOUND_RX : send4state_bound_rx (pdu); break;
    case BOUND_TRX: send4state_bound_trx(pdu); break;
    default       : /* throw error         */; break; // can't send without established bind error.
  }
}

//--------------------------------------------------------------------------------
void SessionManager::close_session(bool re_connect /* = false */)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  reconnectFlag = re_connect;

  switch(currentState) {
    case OUTBOUND :
    case BOUND_TX : 
    case BOUND_RX : 
    case BOUND_TRX: 
      do_unbind_request();
      break;
    default       :
      break;
  };

  log << "ASYNC CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
  io_service_.post(boost::bind(&SessionManager::close_connection, this));
}

//--------------------------------------------------------------------------------
void SessionManager::start_session()
{
  startTime            = boost::posix_time::microsec_clock::local_time();

  tcp::resolver        resolver(io_service_);
  tcp::resolver::query query(CFG->get<std::string>("message-centre.host"),
                             CFG->get<std::string>("message-centre.port"));

  endpoint_iterator    = resolver.resolve(query);
  throttleNextSendTime = boost::posix_time::microsec_clock::local_time();
  timeBetweenSends     = 1000000/smppcfg.getTxThrottleLimit(); //1000000 micro seconds in a second.
}

//--------------------------------------------------------------------------------
void SessionManager::close_connection()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  setCurrentState(SessionManager::CLOSED);

  log << "Canceling timers." << kisscpp::manip::flush;
  enquire_link_timer.cancel();
  enquire_link_response_timer.cancel();
  w4rQ_ageing_timer.cancel();
  txQ->clearSessionQueues();

  log << "Closing Socket with read count: [" << readCount
      << "] and write count ["               << writeCount
      << "] start time : "                   << boost::posix_time::to_iso_string(startTime)
      << kisscpp::manip::flush;

  socket_.close();
  log << "Socket Closed." << kisscpp::manip::flush;

  if(!stopFlag && reconnectFlag) {
    reconnectFlag = false;
    sleep(1); // TODO; make the delay between re-connect attempts configurable.
    connect();
  }
}

//--------------------------------------------------------------------------------
void SessionManager::handle_connect(const boost::system::error_code& error)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if (!error) {
    log << "Connected" << kisscpp::manip::flush;

    setCurrentState(SessionManager::OPEN);

    do_bind_request();

    SharedRawPdu trpdu;
    trpdu.reset(new RawPdu());

    boost::asio::async_read(socket_,
                            boost::asio::buffer(trpdu->headerBuf(), 16),
                            boost::bind(&SessionManager::handle_read_header, this, trpdu, boost::asio::placeholders::error));
  } else {
    log << "Connection failed: [" << error.message() << "]" << kisscpp::manip::flush;
    if(!stopFlag) {
      log << "Next connection attempt in 5 seconds." << kisscpp::manip::flush;
      sleep(5); // TODO: should be configurable.
      connect();
    } else {
      log << "No further connection attempts will be made" << kisscpp::manip::flush;
    }
  }
}

//--------------------------------------------------------------------------------
void SessionManager::handle_read_header(SharedRawPdu rawpdu, const boost::system::error_code& error)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if(!error) {
    log << "Header Read success: Command Length [" << rawpdu->cmd_length()
        << "] body length ["                       << rawpdu->bodyLength()
        << "]" << kisscpp::manip::flush;

    boost::asio::async_read(socket_,
                            boost::asio::buffer(rawpdu->bodyBuf(), rawpdu->bodyLength()),
                            boost::bind(&SessionManager::handle_read_body, this, rawpdu, boost::asio::placeholders::error));
  } else {
    log << "Error - closing. [" << error.message() << "]" << kisscpp::manip::flush;

    if(!stopFlag) {
      if(error == boost::asio::error::eof) {
        setCurrentState(SessionManager::CLOSED);
        reconnectFlag = true;
        log << "ASYNC CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
        io_service_.post(boost::bind(&SessionManager::close_connection, this));
      } else {
        log << "CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
        close_session(true);
      }
    }
  }
}

//--------------------------------------------------------------------------------
void SessionManager::handle_read_body(SharedRawPdu rawpdu, const boost::system::error_code& error)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if(!error) {
    w4rQ_pop     (rawpdu);
    process4state(rawpdu);

    SharedRawPdu nextRawPdu;
    nextRawPdu.reset(new RawPdu());

    readCount++;
    log << "Reading count: " << readCount << kisscpp::manip::flush;

    boost::asio::async_read(socket_,
                            boost::asio::buffer(nextRawPdu->headerBuf(), 16),
                            boost::bind(&SessionManager::handle_read_header, this, nextRawPdu, boost::asio::placeholders::error));
  } else {
    log << "handle_read_body: error - closing. [" << error.message() << "]" << kisscpp::manip::flush;

    if(!stopFlag) {
      if(error == boost::asio::error::eof) {
        setCurrentState(SessionManager::CLOSED);
        reconnectFlag = true;
        log << "ASYNC CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
        io_service_.post(boost::bind(&SessionManager::close_connection, this));
      } else {
        log << "CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
        close_session(true);
      }
    }
  }
}

//--------------------------------------------------------------------------------
void SessionManager::handle_write(const boost::system::error_code& error)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if(!error) {
    boost::lock_guard<boost::mutex> guard(writeMutex);
    w4rQ_put(txQ->last_pop_object()); // here, because it's the only point at wich we know that a PDU was successfully sent.

    if(canSend() && !txQ->empty()) {
      write_pdu();
    }
  } else {
    log << "Handle Write error. [" << error.message() << "]" << kisscpp::manip::flush;
    txQ->push_back_last_pop();

    if(!stopFlag) {
      setCurrentState(SessionManager::CLOSED);
      reconnectFlag = true;
      log << "ASYNC CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
      io_service_.post(boost::bind(&SessionManager::close_connection, this));
    }
  }
}

//--------------------------------------------------------------------------------
void SessionManager::throttle_check()
{
  kisscpp::LogStream      log(__PRETTY_FUNCTION__);
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
  if(now < throttleNextSendTime) {
    boost::posix_time::time_duration td = throttleNextSendTime - now;
    log << "Sleeping for " << td.total_microseconds() << " microseconds" << kisscpp::manip::flush;
    usleep(td.total_microseconds());
  } else {
    log << "Not sleeping." << kisscpp::manip::flush;
  }
  throttleNextSendTime = now + boost::posix_time::microseconds(timeBetweenSends);
}

//--------------------------------------------------------------------------------
void SessionManager::do_write(const SharedSmppPdu pdu, unsigned priority /*= TransmitQ::MESSAGE*/)
{
  kisscpp::LogStream             log(__PRETTY_FUNCTION__);
  boost::lock_guard<boost::mutex> guard(writeMutex);

  bool write_in_progress = writing(); // if the queue is not empty, we are busy writing.

  txQ->push(pdu, priority);

  if(!write_in_progress) {
    write_pdu();
  }
}

//--------------------------------------------------------------------------------
void SessionManager::write_pdu()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  SharedSmppPdu       pdu2send = txQ->pop();

  if(pdu2send->sequence_number <= smpp_pdu::SequenceNumber::Min) {
    pdu2send->sequence_number = seqNumGen.next();
  }

  //w4rQ_put(pdu2send); only once a pdu is sent does it go into the "waiting for response" queue
  print_pdu(pdu2send);
  std::string tbuf = pdu2send->encode();
  throttle_check();
  writeCount++;
  boost::asio::async_write(socket_,
                           boost::asio::buffer(tbuf.c_str(), tbuf.size()),
                           boost::bind(&SessionManager::handle_write, this, boost::asio::placeholders::error));
}

//--------------------------------------------------------------------------------
void SessionManager::do_bind_request()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  SharedPduBindType   bindRequestPDU;

  switch(smppcfg.getTypeOfBind()) {
    case TX : bindRequestPDU.reset(new smpp_pdu::PDU_bind_transmitter()); break;
    case RX : bindRequestPDU.reset(new smpp_pdu::PDU_bind_reciever   ()); break;
    case TRX: bindRequestPDU.reset(new smpp_pdu::PDU_bind_transceiver()); break;
    default : /*TODO: throw bind type not supported error.*/  break;
  }

  bindRequestPDU->sequence_number   = seqNumGen.next();

  bindRequestPDU->system_id_        = smppcfg.getSystemId        ();
  bindRequestPDU->password_         = smppcfg.getPassword        ();
  bindRequestPDU->system_type_      = smppcfg.getSystemType      ();
  bindRequestPDU->interface_version_= smppcfg.getInterfaceVersion();
  bindRequestPDU->ton_              = smppcfg.getAddrTon         ();
  bindRequestPDU->npi_              = smppcfg.getAddrNpi         ();
  bindRequestPDU->address_range_    = smppcfg.getAddressRange    ();

  do_write(bindRequestPDU, TransmitQ::SESSION);
}

//--------------------------------------------------------------------------------
void SessionManager::do_unbind_request()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  SharedPduUnbind     unbindRequestPDU;
  unbindRequestPDU.reset(new smpp_pdu::PDU_unbind());
  //unbindRequestPDU->sequence_number = seqNumGen.next();
  do_write(unbindRequestPDU, TransmitQ::SESSION);
}

//--------------------------------------------------------------------------------
void SessionManager::send4state_bound_tx(const SharedSmppPdu pdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(pdu->command_id) {
    case smpp_pdu::CommandId::BroadcastSm      :
    case smpp_pdu::CommandId::CancelBroadcastSm:
    case smpp_pdu::CommandId::CancelSm         :
    case smpp_pdu::CommandId::DataSm           :
    case smpp_pdu::CommandId::QueryBroadcastSm :
    case smpp_pdu::CommandId::QuerySm          :
    case smpp_pdu::CommandId::ReplaceSm        :
    case smpp_pdu::CommandId::SubmitMulti      :
    case smpp_pdu::CommandId::SubmitSm         :
      log << "posting PDU to txQ." << kisscpp::manip::flush;
      do_write(pdu, TransmitQ::MESSAGE);
      break;
    default:
      log << "unsupported PDU." << kisscpp::manip::flush;
      break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::send4state_bound_rx(const SharedSmppPdu pdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(pdu->command_id) {
    case smpp_pdu::CommandId::DataSmResp     :
    case smpp_pdu::CommandId::DeliverSmResp  :
      log << "posting PDU to txQ." << kisscpp::manip::flush;
      do_write(pdu, TransmitQ::MESSAGE);
      break;
  default :
      log << "unsupported PDU" << kisscpp::manip::flush;
      break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::send4state_bound_trx(const SharedSmppPdu pdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(pdu->command_id) {
    case smpp_pdu::CommandId::BroadcastSm      :
    case smpp_pdu::CommandId::CancelBroadcastSm:
    case smpp_pdu::CommandId::CancelSm         :
    case smpp_pdu::CommandId::DataSm           :
    case smpp_pdu::CommandId::DataSmResp       :
    case smpp_pdu::CommandId::DeliverSmResp    :
    case smpp_pdu::CommandId::QueryBroadcastSm :
    case smpp_pdu::CommandId::QuerySm          :
    case smpp_pdu::CommandId::ReplaceSm        :
    case smpp_pdu::CommandId::SubmitMulti      :
    case smpp_pdu::CommandId::SubmitSm         :
      log << "posting PDU to txQ." << kisscpp::manip::flush;
      do_write(pdu, TransmitQ::MESSAGE);
      break;
    default :
      log << "unsupported PDU"     << kisscpp::manip::flush;
      break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::process4state(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  reschedule_enquire_link(); // we have a full SMPP PDU. the next enquire link should be delayed.

  try {
    switch(currentState) {
      case BOUND_TX : process4state_bound_tx (rawpdu); break;
      case BOUND_RX : process4state_bound_rx (rawpdu); break;
      case BOUND_TRX: process4state_bound_trx(rawpdu); break;
      case OPEN     : process4state_open     (rawpdu); break;
      case OUTBOUND : process4state_outbound (rawpdu); break;
      case UNBOUND  : process4state_unbound  (rawpdu); break;
      case CLOSED   : process4state_closed   (rawpdu); break;
      default       :
                      // This should NEVER Happen
                      std::stringstream ss;
                      ss << "Session manager - Fatal Exceptoin: Unrecognised State: [" << __PRETTY_FUNCTION__ << "]";
                      throw std::runtime_error(ss.str());
                      break;
    }
  } catch(std::runtime_error &e) {
    log << "Command Length: "         << rawpdu->cmd_length() << kisscpp::manip::flush;
    log << "Failure to process PDU: " << e.what()             << kisscpp::manip::flush;
    std::stringstream ss;
    smpp_pdu::hex_dump(rawpdu->data(), rawpdu->cmd_length(), ss);
    log << "ERROR PDU:\n" << ss.str() << kisscpp::manip::flush;
    log << "CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
    close_session(true);
  }
}

//--------------------------------------------------------------------------------
void SessionManager::process4state_open(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(rawpdu->cmd_id()) {
    case smpp_pdu::CommandId::BindReceiverResp   : procpdu_bind_resp        (rawpdu, BOUND_RX ); break;
    case smpp_pdu::CommandId::BindTransmitterResp: procpdu_bind_resp        (rawpdu, BOUND_TX ); break;
    case smpp_pdu::CommandId::BindTransceiverResp: procpdu_bind_resp        (rawpdu, BOUND_TRX); break;
    case smpp_pdu::CommandId::EnquireLink        : procpdu_enquire_link     (rawpdu);            break;
    case smpp_pdu::CommandId::EnquireLinkResp    : procpdu_enquire_link_resp(rawpdu);            break;
    case smpp_pdu::CommandId::GenericNack        : procpdu_generic_nack     (rawpdu);            break;
    case smpp_pdu::CommandId::Outbind            : do_bind_request(); break;
    default                                      : break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::process4state_bound_tx(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(rawpdu->cmd_id()) {
    case smpp_pdu::CommandId::BroadcastSmResp      : procpdu_broadcast_sm_resp       (rawpdu); break;
    case smpp_pdu::CommandId::CancelBroadcastSmResp: procpdu_cancel_broadcast_sm_resp(rawpdu); break;
    case smpp_pdu::CommandId::CancelSmResp         : procpdu_cancel_sm_resp          (rawpdu); break;
    case smpp_pdu::CommandId::DataSmResp           : procpdu_data_sm_resp            (rawpdu); break;
    case smpp_pdu::CommandId::QueryBroadcastSmResp : procpdu_query_broadcast_sm_resp (rawpdu); break;
    case smpp_pdu::CommandId::QuerySmResp          : procpdu_query_sm_resp           (rawpdu); break;
    case smpp_pdu::CommandId::ReplaceSmResp        : procpdu_replace_sm_resp         (rawpdu); break;
    case smpp_pdu::CommandId::SubmitMultiResp      : procpdu_submit_multi_resp       (rawpdu); break;
    case smpp_pdu::CommandId::SubmitSmResp         : procpdu_submit_sm_resp          (rawpdu); break;
    case smpp_pdu::CommandId::Unbind               : procpdu_unbind                  (rawpdu); break;
    case smpp_pdu::CommandId::UnbindResp           : procpdu_unbind_resp             (rawpdu); break;
    case smpp_pdu::CommandId::EnquireLink          : procpdu_enquire_link            (rawpdu); break;
    case smpp_pdu::CommandId::EnquireLinkResp      : procpdu_enquire_link_resp       (rawpdu); break;
    case smpp_pdu::CommandId::GenericNack          : procpdu_generic_nack            (rawpdu); break;
    default                                        : break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::process4state_bound_rx(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(rawpdu->cmd_id()) {
    case smpp_pdu::CommandId::AlertNotification: procpdu_alert_notification(rawpdu); break;
    case smpp_pdu::CommandId::DataSm           : procpdu_data_sm           (rawpdu); break;
    case smpp_pdu::CommandId::DeliverSm        : procpdu_deliver_sm        (rawpdu); break;
    case smpp_pdu::CommandId::EnquireLink      : procpdu_enquire_link      (rawpdu); break;
    case smpp_pdu::CommandId::EnquireLinkResp  : procpdu_enquire_link_resp (rawpdu); break;
    case smpp_pdu::CommandId::GenericNack      : procpdu_generic_nack      (rawpdu); break;
    case smpp_pdu::CommandId::Unbind           : procpdu_unbind            (rawpdu); break;
    case smpp_pdu::CommandId::UnbindResp       : procpdu_unbind_resp       (rawpdu); break;
    default                                    : break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::process4state_bound_trx(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(rawpdu->cmd_id()) {
    case smpp_pdu::CommandId::AlertNotification    : procpdu_alert_notification      (rawpdu); break;
    case smpp_pdu::CommandId::BroadcastSmResp      : procpdu_broadcast_sm_resp       (rawpdu); break;
    case smpp_pdu::CommandId::CancelBroadcastSmResp: procpdu_cancel_broadcast_sm_resp(rawpdu); break;
    case smpp_pdu::CommandId::CancelSmResp         : procpdu_cancel_sm_resp          (rawpdu); break;
    case smpp_pdu::CommandId::DataSm               : procpdu_data_sm                 (rawpdu); break;
    case smpp_pdu::CommandId::DataSmResp           : procpdu_data_sm_resp            (rawpdu); break;
    case smpp_pdu::CommandId::DeliverSm            : procpdu_deliver_sm              (rawpdu); break;
    case smpp_pdu::CommandId::QueryBroadcastSmResp : procpdu_query_broadcast_sm_resp (rawpdu); break;
    case smpp_pdu::CommandId::QuerySmResp          : procpdu_query_sm_resp           (rawpdu); break;
    case smpp_pdu::CommandId::ReplaceSmResp        : procpdu_replace_sm_resp         (rawpdu); break;
    case smpp_pdu::CommandId::SubmitMultiResp      : procpdu_submit_multi_resp       (rawpdu); break;
    case smpp_pdu::CommandId::SubmitSmResp         : procpdu_submit_sm_resp          (rawpdu); break;
    case smpp_pdu::CommandId::EnquireLink          : procpdu_enquire_link            (rawpdu); break;
    case smpp_pdu::CommandId::EnquireLinkResp      : procpdu_enquire_link_resp       (rawpdu); break;
    case smpp_pdu::CommandId::GenericNack          : procpdu_generic_nack            (rawpdu); break;
    case smpp_pdu::CommandId::Unbind               : procpdu_unbind                  (rawpdu); break;
    case smpp_pdu::CommandId::UnbindResp           : procpdu_unbind_resp             (rawpdu); break;
    default                                        : break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::process4state_unbound(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(rawpdu->cmd_id()) {
    case smpp_pdu::CommandId::EnquireLink    : procpdu_enquire_link     (rawpdu); break;
    case smpp_pdu::CommandId::EnquireLinkResp: procpdu_enquire_link_resp(rawpdu); break;
    case smpp_pdu::CommandId::GenericNack    : procpdu_generic_nack     (rawpdu); break;
    default                                  : /*TODO: throw & log error */       break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::process4state_closed(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  // one should not be able to recieve during a closed state!!!
}

//--------------------------------------------------------------------------------
void SessionManager::process4state_outbound(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  switch(rawpdu->cmd_id()) {
    case smpp_pdu::CommandId::BindReceiverResp   : procpdu_bind_resp        (rawpdu, BOUND_RX ); break;
    case smpp_pdu::CommandId::BindTransmitterResp: procpdu_bind_resp        (rawpdu, BOUND_TX ); break;
    case smpp_pdu::CommandId::BindTransceiverResp: procpdu_bind_resp        (rawpdu, BOUND_TRX); break;
    case smpp_pdu::CommandId::EnquireLink        : procpdu_enquire_link     (rawpdu);            break;
    case smpp_pdu::CommandId::EnquireLinkResp    : procpdu_enquire_link_resp(rawpdu);            break;
    case smpp_pdu::CommandId::GenericNack        : procpdu_generic_nack     (rawpdu);            break;
    default                                      : /*TODO: throw & log error */                  break;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_bind_resp(SharedRawPdu rawpdu, State stateAferSuccess)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if(rawpdu->cmd_status() == smpp_pdu::CommandStatus::ESME_ROK) {
    setCurrentState(stateAferSuccess);
  } else {
    //TODO: some form of error processing needed here.
  }
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_alert_notification(SharedRawPdu rawpdu)
{
  // not supported yet;

  // No error status possible here. see Spec 4.8.4.52
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_broadcast_sm_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_cancel_broadcast_sm_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_cancel_sm_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_data_sm(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_data_sm_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_deliver_sm(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  std::stringstream   ss;
  SharedSmppPdu       recievedPDU;
  SharedPduDeliverSm  tpdu;

  smpp_pdu::hex_dump(rawpdu->data(), rawpdu->cmd_length(), ss);
  log << "Recieved DeliverSM:\n" << ss.str() << kisscpp::manip::flush;

  recievedPDU.reset(new smpp_pdu::PDU_deliver_sm(rawpdu->c_str()));

  tpdu = boost::dynamic_pointer_cast<smpp_pdu::PDU_deliver_sm>(recievedPDU);

  rxQ->push(recievedPDU);

  if((tpdu->esm_class).bits_set(smpp_pdu::spEsmClass::MSG_TYPE_DLR)) {
    log << "DLR >";
  } else if((tpdu->esm_class).bits_set(smpp_pdu::spEsmClass::MSG_TYPE_DEFAULT)) {
    log << "MSG >";
  }

  log << (std::string)tpdu->short_message << "<" << kisscpp::manip::flush;

  SharedPduDeliverSmResp responsePDU;

  responsePDU.reset(new smpp_pdu::PDU_deliver_sm_resp());

  responsePDU->command_status  = smpp_pdu::CommandStatus::ESME_ROK;
  responsePDU->sequence_number = tpdu->sequence_number;

  do_write(responsePDU, TransmitQ::RESPONSE);
}

// What if we recieve an enquire link, but there are items in txQ?
// That would mean that we recieved an enquire link, while we are seemingly transmitting.
// -- Could be indicative of a problem we aren't aware of.
// TODO: work on loging/logic to indicate/prevent this.
//--------------------------------------------------------------------------------
void SessionManager::procpdu_enquire_link(SharedRawPdu rawpdu)
{
  kisscpp::LogStream         log(__PRETTY_FUNCTION__);
  smpp_pdu::PDU_enquire_link recieved_pdu(rawpdu->c_str());
  SharedPduEnquireLinkResp   responsePDU;

  responsePDU.reset(new smpp_pdu::PDU_enquire_link_resp());

  responsePDU->command_status  = smpp_pdu::CommandStatus::ESME_ROK;
  responsePDU->sequence_number = recieved_pdu.sequence_number;

  do_write(responsePDU, TransmitQ::RESPONSE);
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_enquire_link_resp(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  // we have recieved a response to an enquire_link.
  enquire_link_response_timer.cancel();
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_generic_nack(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_query_broadcast_sm_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_query_sm_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_replace_sm_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_submit_multi_resp(SharedRawPdu rawpdu)
{
  // not supported yet;
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_submit_sm_resp(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  log << "Command Length: " << rawpdu->cmd_length() << kisscpp::manip::flush;
  std::stringstream ss;
  smpp_pdu::hex_dump(rawpdu->data(), rawpdu->cmd_length(), ss);
  log << "PDU:\n" << ss.str() << kisscpp::manip::flush;

  if(rawpdu->cmd_status() == smpp_pdu::CommandStatus::ESME_ROK) {
    smpp_pdu::PDU_submit_sm_resp recieved_pdu(rawpdu->c_str());

    log << "seqnum = " << recieved_pdu.sequence_number << kisscpp::manip::flush;

    // TODO: message was delivered. Send notification to internal application.
  } else {
    smpp_pdu::CommandStatus cmd_err(rawpdu->cmd_status());
    log << "ERROR: " << cmd_err.long_description(cmd_err) << kisscpp::manip::flush;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_unbind(SharedRawPdu rawpdu)
{
  kisscpp::LogStream  log(__PRETTY_FUNCTION__);
  smpp_pdu::PDU_unbind recieved_pdu(rawpdu->c_str());
  SharedSmppPdu        responsePDU;

  responsePDU.reset(new smpp_pdu::PDU_unbind_resp());

  responsePDU->command_status  = smpp_pdu::CommandStatus::ESME_ROK;
  responsePDU->sequence_number = recieved_pdu.sequence_number;

  setCurrentState(SessionManager::UNBOUND);

  do_write(responsePDU, TransmitQ::RESPONSE);

  stopFlag      = false;
  reconnectFlag = true;
  log << "UNBIND CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
  io_service_.post(boost::bind(&SessionManager::close_connection, this));
}

//--------------------------------------------------------------------------------
void SessionManager::procpdu_unbind_resp(SharedRawPdu rawpdu)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  setCurrentState(SessionManager::OPEN);
  log << "CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
  close_session(false);
}

//--------------------------------------------------------------------------------
void SessionManager::reschedule_enquire_link()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  enquire_link_timer.cancel();
  enquire_link_response_timer.cancel();
  enquire_link_timer.expires_from_now(smppcfg.getEnquireLinkTimeout());
  enquire_link_timer.async_wait(boost::bind(&SessionManager::do_enquire_link, this, boost::asio::placeholders::error));
}

//--------------------------------------------------------------------------------
void SessionManager::do_enquire_link(const boost::system::error_code& e)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if(e != boost::asio::error::operation_aborted) {  // the enquire link timer expired, send an enquire_link pdu to the message centre.

    SharedSmppPdu requestPDU;
    requestPDU.reset(new smpp_pdu::PDU_enquire_link());
    requestPDU->sequence_number = seqNumGen.next();

    do_write(requestPDU, TransmitQ::SESSION);

    enquire_link_response_timer.expires_from_now(smppcfg.getEnquireLinkRespTimeout());
    enquire_link_response_timer.async_wait(boost::bind(&SessionManager::do_enquire_link_failure, this, boost::asio::placeholders::error));

  } else {                                         // the enquire link timer was aborted, we don't have to do anything.
  }
}

//--------------------------------------------------------------------------------
void SessionManager::do_enquire_link_failure(const boost::system::error_code& e)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if(e != boost::asio::error::operation_aborted) { // the enquire link response wasn't recieved,
                                                   // the bind is broken and most probably the connection as well.
    if(!stopFlag) {                                // Restart everything.
      reconnectFlag = true;
      log << "CLOSE: " << __PRETTY_FUNCTION__ << kisscpp::manip::flush;
      close_session(false);
    }
  } else {                                         // the enquire link response was recieved, we don't have to do anything.
    log << "abort detected." << kisscpp::manip::flush;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::print_pdu(SharedSmppPdu pdu)
{
  kisscpp::LogStream  log(__PRETTY_FUNCTION__);
  std::string          encodedPdu = pdu->encode();

  std::stringstream ss;
  smpp_pdu::hex_dump(reinterpret_cast<const uint8_t*>(encodedPdu.c_str()), encodedPdu.size(), ss);
  log << "Print PDU:\n" << ss.str() << kisscpp::manip::flush;
}

//--------------------------------------------------------------------------------
void SessionManager::w4rQ_put(SharedSmppPdu pdu)
{
  if(pdu->command_id < smpp_pdu::CommandId::BindReceiverResp) { // i.e. This IS NOT a response PDU
    kisscpp::LogStream             log(__PRETTY_FUNCTION__);
    boost::lock_guard<boost::mutex> guard(w4rQMutex);
    SharedTimeStampedPdu            stsp;
    stsp.reset(new timeStampedPdu(pdu));
    w4rQ[pdu->sequence_number] = stsp;
  }
}

//--------------------------------------------------------------------------------
void SessionManager::w4rQ_pop(SharedRawPdu rawpdu)
{
  if(rawpdu->cmd_id() > smpp_pdu::CommandId::GenericNack) { // i.e. this IS a response pdu
    kisscpp::LogStream             log(__PRETTY_FUNCTION__);
    boost::lock_guard<boost::mutex> guard(w4rQMutex);
    AwaitingResponseMapTypeItr      itr = w4rQ.find(rawpdu->seq_num());
    if(itr != w4rQ.end()) {
      w4rQ.erase(itr);
    }
  }
}

//--------------------------------------------------------------------------------
void SessionManager::w4rQ_age_cleanup(const boost::system::error_code& e)
{
  if(e != boost::asio::error::operation_aborted && w4rQ.size() > 0) {
    kisscpp::LogStream             log(__PRETTY_FUNCTION__);
    boost::lock_guard<boost::mutex> guard(w4rQMutex);
    for(AwaitingResponseMapTypeItr i = w4rQ.begin(); i != w4rQ.end(); ++i) {
      if((i->second)->expired(30)) {                         // TODO: Make this time configurable.
        do_write((i->second)->getObj(), TransmitQ::MESSAGE); // perhaps we'll need to be more specific about the priority here,
                                                             // the message could be a session level messsage;
        w4rQ.erase(w4rQ.find((i->second)->pduSeqNum()));
      }
    }
    set_w4rQ_ageing_timer();
  }
}

//--------------------------------------------------------------------------------
void SessionManager::set_w4rQ_ageing_timer()
{
  // TODO: the timer should expire at half the max age of a sent PDU
  // TODO: this is so that the true max age will never be more than maxage+maxage/2
  // TODO: Note in the documentation that this should never be set to more than 60 seconds, use 30 seconds as good default.
  w4rQ_ageing_timer.expires_from_now(smppcfg.getEnquireLinkTimeout()); // TODO: Make this time properly configurable.
  w4rQ_ageing_timer.async_wait(boost::bind(&SessionManager::w4rQ_age_cleanup, this, boost::asio::placeholders::error));
}

