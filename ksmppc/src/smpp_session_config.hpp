// File  : smpp_session_config.hpp
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

#ifndef _SMPP_SESSION_CONFIG_HPP_
#define _SMPP_SESSION_CONFIG_HPP_

#include <string>
#include <smpp_pdu_all.hpp>

#include "cfg.hpp"
#include "bind_type.hpp"
#include "util.hpp"

//--------------------------------------------------------------------------------
class SmppSessionConfiguration
{
  public:
    SmppSessionConfiguration() :
      enquire_link_timeout     (CFG->get<unsigned int>("smpp_session.enquire_link_period")),
      enquire_link_resp_timeout(CFG->get<unsigned int>("smpp_session.enquire_link_response_timeout"))
    {
      systemId                  = (CFG->get<std::string> ("smpp_session.system_id"  )).c_str();
      password                  = (CFG->get<std::string> ("smpp_session.password"   )).c_str();
      systemType                = (CFG->get<std::string> ("smpp_session.system_type")).c_str();
      interfaceVersion          = makeInterfaceVersion(CFG->get<int>("smpp_session.interface_version",34));
      addrTon                   =  CFG->get<uint8_t>     ("smpp_session.default_type_of_number");
      addrNpi                   =  CFG->get<uint8_t>     ("smpp_session.default_number_plan_indicator");
      addressRange              = (CFG->get<std::string> ("smpp_session.address_range")).c_str();
      tx_throttle_limit         =  CFG->get<unsigned int>("smpp_session.tx_throttle_limit");
      typeOfBind                = makeBindType(CFG->get<std::string>("smpp_session.bind_type"));
    }

    ~SmppSessionConfiguration() {}

    smpp_pdu::SystemId         &getSystemId              () {return systemId;                 }
    smpp_pdu::Password         &getPassword              () {return password;                 }
    smpp_pdu::SystemType       &getSystemType            () {return systemType;               }
    smpp_pdu::InterfaceVersion &getInterfaceVersion      () {return interfaceVersion;         }
    smpp_pdu::Ton              &getAddrTon               () {return addrTon;                  }
    smpp_pdu::Npi              &getAddrNpi               () {return addrNpi;                  }
    smpp_pdu::AddressRange     &getAddressRange          () {return addressRange;             }
    boost::posix_time::seconds &getEnquireLinkTimeout    () {return enquire_link_timeout;     }
    boost::posix_time::seconds &getEnquireLinkRespTimeout() {return enquire_link_resp_timeout;}
    unsigned                   &getTxThrottleLimit       () {return tx_throttle_limit;        }
    BindType                   &getTypeOfBind            () {return typeOfBind;               }

  protected:
  private:
    smpp_pdu::SystemId          systemId;
    smpp_pdu::Password          password;
    smpp_pdu::SystemType        systemType;
    smpp_pdu::InterfaceVersion  interfaceVersion;
    smpp_pdu::Ton               addrTon;
    smpp_pdu::Npi               addrNpi;
    smpp_pdu::AddressRange      addressRange;
    boost::posix_time::seconds  enquire_link_timeout;
    boost::posix_time::seconds  enquire_link_resp_timeout;
    unsigned                    tx_throttle_limit;
    BindType                    typeOfBind;
};

#endif // _SMPP_SESSION_CONFIG_HPP_

