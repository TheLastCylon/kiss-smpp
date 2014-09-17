// File  : handler_send.cpp
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

#include "handler_send.hpp"

void SendHandler::run(const BoostPtree& request, BoostPtree& response)
{
  // TODO: basic validation on the various parts of the request
  kisscpp::LogStream log(__PRETTY_FUNCTION__);

  try {

    SharedPduSubmitSm submitPDU;

    submitPDU.reset(new smpp_pdu::PDU_submit_sm());

    submitPDU->service_type             = request.get<std::string>("service-type"          ,"");
    submitPDU->source_addr     .ton     = request.get<uint8_t>    ("source-addr-ton"       ,(uint8_t)config->addrTon());
    submitPDU->source_addr     .npi     = request.get<uint8_t>    ("source-addr-npi"       ,(uint8_t)config->addrNpi());
    submitPDU->source_addr     .address = request.get<std::string>("source-addr");
    submitPDU->destination_addr.ton     = request.get<uint8_t>    ("destination-addr-ton"  ,(uint8_t)config->addrTon());
    submitPDU->destination_addr.npi     = request.get<uint8_t>    ("destination-addr-npi"  ,(uint8_t)config->addrNpi());
    submitPDU->destination_addr.address = request.get<std::string>("destination-addr");
    submitPDU->esm_class                = request.get<uint8_t>    ("esm-class"             ,0);
    submitPDU->protocol_id              = request.get<uint8_t>    ("protocol-id"           ,0);
    submitPDU->priority_flag            = request.get<uint8_t>    ("priority-flag"         ,0);
    submitPDU->schedule_delivery_time   = request.get<std::string>("schedule-delivery-time",""); // TODO: Deal with propper encoding of time here.
    submitPDU->validity_period          = request.get<std::string>("validity-period"       ,"");
    submitPDU->registered_delivery      = request.get<uint8_t>    ("registered-delivery"   ,3);
    submitPDU->replace_if_present_flag  = request.get<uint8_t>    ("replace-if-presentFlag",0);
    submitPDU->data_coding              = request.get<uint8_t>    ("data-coding"           ,3);
    submitPDU->sm_default_msg_id        = request.get<uint8_t>    ("sm-default-msg-id"     ,0);
    submitPDU->short_message            = request.get<std::string>("short-message");

    // TODO: deal with support for TLV that overrides short-message parameter:
    // message-payload TLV. in spec: 4.8.4.36

    sendingQ->push(submitPDU);

  } catch (std::exception& e) {
    log << "Exception: " << e.what() << kisscpp::manip::endl;
    response.put("kcm-sts", kisscpp::RQST_UNKNOWN);
    response.put("kcm-erm", e.what());
  }

  response.put("kcm-sts", kisscpp::RQST_SUCCESS);
}

