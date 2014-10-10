// File  : ksmppc.cpp
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
ksmppc::ksmppc(const std::string &instance,
               const bool        &runAsDaemon) :
  Server(1, "ksmppc", instance, runAsDaemon),
  running(true)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  constructQueues();
  startSessions();
  registerHandlers();

  threadGroup.create_thread(boost::bind(&ksmppc::recieveProcessor, this));
  threadGroup.create_thread(boost::bind(&ksmppc::sendingProcessor, this));
}

//--------------------------------------------------------------------------------
ksmppc::~ksmppc()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  running = false;
  stop();
  session->stop();
  threadGroup.join_all();
}

//--------------------------------------------------------------------------------
void ksmppc::constructQueues()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  sendingBuffer.reset(new SafeSmppPduQ("sendingBuffer", "/tmp", 10)); // TODO: The working direcory needs to be obtained from the Config file.
  recieveBuffer.reset(new SafeSmppPduQ("recieveBuffer", "/tmp", 10));
  rcv_errBuffer.reset(new SafeSmppPduQ("rcv_errBuffer", "/tmp", 10));
}

//--------------------------------------------------------------------------------
void ksmppc::registerHandlers()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  if(makeBindType(CFG->get<std::string>("smpp_session.bind_type")) != RX) { // conditional creation of send handler. i.e. If we only recieve, no sending can take place.
    sendHandler.reset(new SendHandler(sendingBuffer));
    register_handler(sendHandler);
  }
}

//--------------------------------------------------------------------------------
void ksmppc::startSessions()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  session.reset(new SessionManager(sessionIoService, recieveBuffer));
  threadGroup.create_thread(boost::bind(&boost::asio::io_service::run, &sessionIoService));
}

//--------------------------------------------------------------------------------
void ksmppc::recieveProcessor()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  while(running) {
    if(recieveBuffer->empty()) {
      sleep(1); // yes, sleep(1), not yield or sleep(0). sleep(1)!
                // spinning threads that do nothing but consume CPU are bad in the real world.
                // Any suggestions around avoiding this would be welcomed.
    } else {
      SharedSmppPdu pdu = recieveBuffer->pop();

      try {
        BoostPtree request;
        BoostPtree response;

        smpp2ptree(pdu, request);

        request.put("kcm-cmd", "smppin");
        request.put("kcm-hst", "localhost");
        request.put("kcm-prt", "9100");

        kisscpp::client requestSender(request, response, 5); // Instantiation of the kisscpp::client class, sends the message.

      } catch(kisscpp::RetryableCommsFailure &e) {
        log << "Retryable comms failure: " << e.what() << kisscpp::manip::endl;
        recieveBuffer->push(pdu);
      } catch(kisscpp::PerminantCommsFailure &e) {
        log << "Perminant comms failure: " << e.what() << kisscpp::manip::endl;
        rcv_errBuffer->push(pdu);
      }
    }
  }
}

//--------------------------------------------------------------------------------
void ksmppc::sendingProcessor()
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  while(running) {
    if(sendingBuffer->empty()) {
      sleep(1); // yes, sleep(1), not yield or sleep(0); sleep(1)!
                // spinning threads that do nothing but consume CPU are bad in the real world.
                // Any suggestions around avoiding this would be welcomed.
    } else {
      boost::shared_ptr<smpp_pdu::SMPP_PDU> pdu = sendingBuffer->pop();
      if(pdu) {
        session->send_pdu(pdu);
      }
    }
  }
}

//--------------------------------------------------------------------------------
void ksmppc::smpp2ptree(SharedSmppPdu pdu, BoostPtree &pt)
{
  kisscpp::LogStream     log(__PRETTY_FUNCTION__);
  smpp_pdu::CommandId cmdid = pdu->command_id;

  switch(cmdid.value()) {
    case smpp_pdu::CommandId::DataSm               : dataSm2Ptree   (pdu, pt); break;
    case smpp_pdu::CommandId::DeliverSm            : deliverSm2Ptree(pdu, pt); break;
    default                                            : /*TODO: Throw some kind of error*/ break;
  }

}

//--------------------------------------------------------------------------------
void ksmppc::dataSm2Ptree(SharedSmppPdu pdu, BoostPtree &pt)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);
  /*not implemented yet*/
}

//--------------------------------------------------------------------------------
void ksmppc::deliverSm2Ptree(SharedSmppPdu pdu, BoostPtree &pt)
{
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  SharedPduDeliverSm tpdu = boost::dynamic_pointer_cast<smpp_pdu::PDU_deliver_sm>(pdu);

  pt.put("service_type"           , tpdu->service_type            .data());
  pt.put("source_addr"            , tpdu->source_addr.address     .data());
  pt.put("destination_addr"       , tpdu->destination_addr.address.data());
  pt.put("esm_class"              , tpdu->esm_class               .data());
  pt.put("protocol_id"            , tpdu->protocol_id             .data());
  pt.put("priority_flag"          , tpdu->priority_flag           .data());
  pt.put("schedule_delivery_time" , tpdu->schedule_delivery_time  .data());
  pt.put("validity_period"        , tpdu->validity_period         .data());
  pt.put("registered_delivery"    , tpdu->registered_delivery     .data());
  pt.put("replace_if_present_flag", tpdu->replace_if_present_flag .data());
  pt.put("data_coding"            , tpdu->data_coding             .data());
  pt.put("sm_default_msg_id"      , tpdu->sm_default_msg_id       .data());
  pt.put("short_message"          , tpdu->short_message           .data());
}

