// File  : smpppdu_queue.hpp
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

#ifndef _SMPPPDU_QUEUE_HPP_
#define _SMPPPDU_QUEUE_HPP_

#include <sstream>
#include <smpppdu_all.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <kisscpp/threadsafe_persisted_queue.hpp>
#include <kisscpp/threadsafe_persisted_priority_queue.hpp>
#include <kisscpp/logstream.hpp>

//--------------------------------------------------------------------------------
class SmppPduBase64Bicoder : public kisscpp::Base64BiCoder<smpp_pdu::SMPP_PDU>
{
  public:
    SmppPduBase64Bicoder() {};
    ~SmppPduBase64Bicoder() {};

    //--------------------------------------------------------------------------------
    virtual boost::shared_ptr<std::string> encode(const boost::shared_ptr<smpp_pdu::SMPP_PDU> obj2encode)
    {
      kisscpp::LogStream log(__PRETTY_FUNCTION__);
      std::string        tstr = obj2encode->encode();
      std::stringstream  ss;

      smpp_pdu::hex_dump(reinterpret_cast<const uint8_t*>(tstr.c_str()), tstr.size(), ss);
      log << "Encoding:\n" << ss.str() << kisscpp::manip::endl;

      return encodeToBase64String(tstr);
    }

    //--------------------------------------------------------------------------------
    virtual boost::shared_ptr<smpp_pdu::SMPP_PDU> decode(const std::string& str2decode)
    {
      kisscpp::LogStream log(__PRETTY_FUNCTION__);

      log << "String 2 decode: " << str2decode << kisscpp::manip::endl;

      boost::shared_ptr<smpp_pdu::SMPP_PDU> tSmppPduPtr;
      boost::shared_ptr<std::string>        pduString = decodeFromBase64(str2decode);

      const smpp_pdu::CommandId cmdId(smpp_pdu::get_command_id(reinterpret_cast<const uint8_t*>(pduString->c_str())));
      uint32_t                  cmdlen = smpp_pdu::get_command_length(reinterpret_cast<const uint8_t*>(pduString->c_str()));

      std::stringstream ss;
      smpp_pdu::hex_dump(reinterpret_cast<const uint8_t*>(pduString->c_str()), cmdlen, ss);
      log << "Decoding:\n" << ss.str() << kisscpp::manip::endl;

      try {
        switch(cmdId) {
          case smpp_pdu::CommandId::AlertNotification    : tSmppPduPtr.reset(new smpp_pdu::PDU_alert_notification      (pduString->c_str())); break;
          case smpp_pdu::CommandId::BindReceiver         : tSmppPduPtr.reset(new smpp_pdu::PDU_bind_reciever           (pduString->c_str())); break;
          case smpp_pdu::CommandId::BindReceiverResp     : tSmppPduPtr.reset(new smpp_pdu::PDU_bind_reciever_resp      (pduString->c_str())); break;
          case smpp_pdu::CommandId::BindTransceiver      : tSmppPduPtr.reset(new smpp_pdu::PDU_bind_transceiver        (pduString->c_str())); break;
          case smpp_pdu::CommandId::BindTransceiverResp  : tSmppPduPtr.reset(new smpp_pdu::PDU_bind_transceiver_resp   (pduString->c_str())); break;
          case smpp_pdu::CommandId::BindTransmitter      : tSmppPduPtr.reset(new smpp_pdu::PDU_bind_transmitter        (pduString->c_str())); break;
          case smpp_pdu::CommandId::BindTransmitterResp  : tSmppPduPtr.reset(new smpp_pdu::PDU_bind_transmitter_resp   (pduString->c_str())); break;
          case smpp_pdu::CommandId::BroadcastSm          : tSmppPduPtr.reset(new smpp_pdu::PDU_broadcast_sm            (pduString->c_str())); break;
          case smpp_pdu::CommandId::BroadcastSmResp      : tSmppPduPtr.reset(new smpp_pdu::PDU_broadcast_sm_resp       (pduString->c_str())); break;
          case smpp_pdu::CommandId::CancelBroadcastSm    : tSmppPduPtr.reset(new smpp_pdu::PDU_cancel_broadcast_sm     (pduString->c_str())); break;
          case smpp_pdu::CommandId::CancelBroadcastSmResp: tSmppPduPtr.reset(new smpp_pdu::PDU_cancel_broadcast_sm_resp(pduString->c_str())); break;
          case smpp_pdu::CommandId::CancelSm             : tSmppPduPtr.reset(new smpp_pdu::PDU_cancel_sm               (pduString->c_str())); break;
          case smpp_pdu::CommandId::CancelSmResp         : tSmppPduPtr.reset(new smpp_pdu::PDU_cancel_sm_resp          (pduString->c_str())); break;
          case smpp_pdu::CommandId::DataSm               : tSmppPduPtr.reset(new smpp_pdu::PDU_data_sm                 (pduString->c_str())); break;
          case smpp_pdu::CommandId::DataSmResp           : tSmppPduPtr.reset(new smpp_pdu::PDU_data_sm_resp            (pduString->c_str())); break;
          case smpp_pdu::CommandId::DeliverSm            : tSmppPduPtr.reset(new smpp_pdu::PDU_deliver_sm              (pduString->c_str())); break;
          case smpp_pdu::CommandId::DeliverSmResp        : tSmppPduPtr.reset(new smpp_pdu::PDU_deliver_sm_resp         (pduString->c_str())); break;
          case smpp_pdu::CommandId::EnquireLink          : tSmppPduPtr.reset(new smpp_pdu::PDU_enquire_link            (pduString->c_str())); break;
          case smpp_pdu::CommandId::EnquireLinkResp      : tSmppPduPtr.reset(new smpp_pdu::PDU_enquire_link_resp       (pduString->c_str())); break;
          case smpp_pdu::CommandId::GenericNack          : tSmppPduPtr.reset(new smpp_pdu::PDU_generic_nack            (pduString->c_str())); break;
          case smpp_pdu::CommandId::Outbind              : tSmppPduPtr.reset(new smpp_pdu::PDU_outbind                 (pduString->c_str())); break;
          case smpp_pdu::CommandId::QueryBroadcastSm     : tSmppPduPtr.reset(new smpp_pdu::PDU_query_broadcast_sm      (pduString->c_str())); break;
          case smpp_pdu::CommandId::QueryBroadcastSmResp : tSmppPduPtr.reset(new smpp_pdu::PDU_querybroadcast_sm_resp  (pduString->c_str())); break;
          case smpp_pdu::CommandId::QuerySm              : tSmppPduPtr.reset(new smpp_pdu::PDU_query_sm                (pduString->c_str())); break;
          case smpp_pdu::CommandId::QuerySmResp          : tSmppPduPtr.reset(new smpp_pdu::PDU_query_sm_resp           (pduString->c_str())); break;
          case smpp_pdu::CommandId::ReplaceSm            : tSmppPduPtr.reset(new smpp_pdu::PDU_replace_sm              (pduString->c_str())); break;
          case smpp_pdu::CommandId::ReplaceSmResp        : tSmppPduPtr.reset(new smpp_pdu::PDU_replace_sm_resp         (pduString->c_str())); break;
          case smpp_pdu::CommandId::SubmitMulti          : tSmppPduPtr.reset(new smpp_pdu::PDU_submit_multi            (pduString->c_str())); break;
          case smpp_pdu::CommandId::SubmitMultiResp      : tSmppPduPtr.reset(new smpp_pdu::PDU_submit_multi_resp       (pduString->c_str())); break;
          case smpp_pdu::CommandId::SubmitSm             : tSmppPduPtr.reset(new smpp_pdu::PDU_submit_sm               (pduString->c_str())); break;
          case smpp_pdu::CommandId::SubmitSmResp         : tSmppPduPtr.reset(new smpp_pdu::PDU_submit_sm_resp          (pduString->c_str())); break;
          case smpp_pdu::CommandId::Unbind               : tSmppPduPtr.reset(new smpp_pdu::PDU_unbind                  (pduString->c_str())); break;
          case smpp_pdu::CommandId::UnbindResp           : tSmppPduPtr.reset(new smpp_pdu::PDU_unbind_resp             (pduString->c_str())); break;
          default: /*TODO: Scream Loudly!!!! This should never happen!!!*/ break;
        }
      } catch (smpp_pdu::Error &e) {
        log << "Error while decoding PDU :" << e.what() << kisscpp::manip::endl;
        throw e;
      }

      return tSmppPduPtr;
    }

  protected:
  private:
    void ppdustr(const char *tmp_buff, unsigned tmp_len)
    {
      kisscpp::LogStream  log(__PRETTY_FUNCTION__);
      unsigned            c1       = 0;
      unsigned            c2       = 0;
      log << kisscpp::manip::hex;
      for(unsigned i = 0; i < tmp_len; ++i) {
        if(c1 > 3) {
          if(c2 > 2) {
            log << "\n--------------- : ";
            c1 = 0;
            c2 = 0;
          } else {
            log << ":";
            c1 = 0;
            ++c2;
          }
        }

        if((unsigned)tmp_buff[i] < 16) {
          log << "0";
        }

        log << tmp_buff[i];
        ++c1;

      }
      log << kisscpp::manip::endl;
    }
};

typedef kisscpp::PersistedQueue<smpp_pdu::SMPP_PDU, SmppPduBase64Bicoder> SmppPduQ;
typedef boost::shared_ptr<SmppPduQ>                                       SharedSmppPduQ;
typedef boost::scoped_ptr<SmppPduQ>                                       ScopedSmppPduQ;

typedef kisscpp::ThreadsafePersistedQueue<smpp_pdu::SMPP_PDU, SmppPduBase64Bicoder> SafeSmppPduQ;
typedef boost::shared_ptr<SafeSmppPduQ>                                             SharedSafeSmppPduQ;
typedef boost::scoped_ptr<SafeSmppPduQ>                                             ScopedSafeSmppPduQ;

typedef kisscpp::ThreadsafePersistedPriorityQueue<smpp_pdu::SMPP_PDU, SmppPduBase64Bicoder> PrioritisedSmppPduQ;
typedef boost::shared_ptr<PrioritisedSmppPduQ>                                              SharedPrioritisedSmppPduQ;
typedef boost::scoped_ptr<PrioritisedSmppPduQ>                                              ScopedPrioritisedSmppPduQ;

typedef boost::shared_ptr<smpp_pdu::PDU_alert_notification>       SharedPduAlertNotification;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_type>                SharedPduBindType;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_type_resp>           SharedPduBindTypeResp;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_reciever>            SharedPduBindReciever;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_reciever_resp>       SharedPduBindRecieverResp;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_transceiver>         SharedPduBindTransceiver;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_transceiver_resp>    SharedPduBindTransceiverResp;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_transmitter>         SharedPduBindTransmitter;
typedef boost::shared_ptr<smpp_pdu::PDU_bind_transmitter_resp>    SharedPduBindTransmitterResp;
typedef boost::shared_ptr<smpp_pdu::PDU_broadcast_sm>             SharedPduBroadcastSm;
typedef boost::shared_ptr<smpp_pdu::PDU_broadcast_sm_resp>        SharedPduBroadcastSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_cancel_broadcast_sm>      SharedPduCancelBroadcastSm;
typedef boost::shared_ptr<smpp_pdu::PDU_cancel_broadcast_sm_resp> SharedPduCancelBroadcastSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_cancel_sm>                SharedPduCancelSm;
typedef boost::shared_ptr<smpp_pdu::PDU_cancel_sm_resp>           SharedPduCancelSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_data_sm>                  SharedPduDataSm;
typedef boost::shared_ptr<smpp_pdu::PDU_data_sm_resp>             SharedPduDataSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_deliver_sm>               SharedPduDeliverSm;
typedef boost::shared_ptr<smpp_pdu::PDU_deliver_sm_resp>          SharedPduDeliverSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_enquire_link>             SharedPduEnquireLink;
typedef boost::shared_ptr<smpp_pdu::PDU_enquire_link_resp>        SharedPduEnquireLinkResp;
typedef boost::shared_ptr<smpp_pdu::PDU_generic_nack>             SharedPduGenericNack;
typedef boost::shared_ptr<smpp_pdu::PDU_outbind>                  SharedPduOutbind;
typedef boost::shared_ptr<smpp_pdu::PDU_query_broadcast_sm>       SharedPduQueryBroadcastSm;
typedef boost::shared_ptr<smpp_pdu::PDU_querybroadcast_sm_resp>   SharedPduQueryBroadcastSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_query_sm>                 SharedPduQuerySm;
typedef boost::shared_ptr<smpp_pdu::PDU_query_sm_resp>            SharedPduQuerySmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_replace_sm>               SharedPduReplaceSm;
typedef boost::shared_ptr<smpp_pdu::PDU_replace_sm_resp>          SharedPduReplaceSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_submit_multi>             SharedPduSubmitMulti;
typedef boost::shared_ptr<smpp_pdu::PDU_submit_multi_resp>        SharedPduSubmitMultiResp;
typedef boost::shared_ptr<smpp_pdu::PDU_submit_sm>                SharedPduSubmitSm;
typedef boost::shared_ptr<smpp_pdu::PDU_submit_sm_resp>           SharedPduSubmitSmResp;
typedef boost::shared_ptr<smpp_pdu::PDU_unbind>                   SharedPduUnbind;
typedef boost::shared_ptr<smpp_pdu::PDU_unbind_resp>              SharedPduUnbindResp;

#endif

