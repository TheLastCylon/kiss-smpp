// File  : configuration_manager.hpp
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

#ifndef _CONFIGURATION_MANAGER_HPP_
#define _CONFIGURATION_MANAGER_HPP_

#include <stdint.h>
#include <smpp_pdu_all.hpp>
#include <kisscpp/boost_ptree.hpp>
#include "bind_type.hpp"

class ConfigurationManager
{
  public:
    ConfigurationManager(std::string fileName)
    {
      boost::property_tree::read_ini(fileName, _data);

      _enquirelink_time = _data.get             ("smppgw.enquire-link-time", 30);
      _mchost           = _data.get<std::string>("smppgw.mc-host").c_str();
      _mcport           = _data.get<std::string>("smppgw.mc-port").c_str();
      _server_host      = _data.get<std::string>("smppgw.server-host").c_str();
      _server_port      = _data.get<std::string>("smppgw.server-port").c_str();
      _systemId         = _data.get<std::string>("smppgw.system-id").c_str();
      _password         = _data.get<std::string>("smppgw.password").c_str();
      _systemType       = _data.get<std::string>("smppgw.system-type").c_str();

      _interfaceVersion = makeInterfaceVersion(_data.get("smppgw.interface-version", 34));

      _ton              = _data.get             ("smppgw.type-of-number", 0);
      _npi              = _data.get             ("smppgw.number-plan-indicator", 1);
      _addressRange     = _data.get<std::string>("smppgw.address-range").c_str();

      _bindType         = makeBindType(_data.get<std::string>("smppgw.bind-type"));
    }

    ~ConfigurationManager() {};

    unsigned                   enquirelinkTime () { return _enquirelink_time; }
    std::string                mchost          () { return _mchost;           }
    std::string                mcport          () { return _mcport;           }
    std::string                server_host     () { return _server_host;      }
    std::string                server_port     () { return _server_port;      }
    smpp_pdu::SystemId         systemId        () { return _systemId;         }
    smpp_pdu::Password         password        () { return _password;         }
    smpp_pdu::SystemType       systemType      () { return _systemType;       }
    smpp_pdu::InterfaceVersion interfaceVersion() { return _interfaceVersion; }
    smpp_pdu::Ton              addrTon         () { return _ton;              }
    smpp_pdu::Npi              addrNpi         () { return _npi;              }
    smpp_pdu::AddressRange     addressRange    () { return _addressRange;     }
    BindType                   bindType        () { return _bindType;         }

  protected:

  private:
    BoostPtree                  _data;
    unsigned                    _enquirelink_time;
    std::string                 _mchost;
    std::string                 _mcport;
    std::string                 _server_host;
    std::string                 _server_port;
    smpp_pdu::SystemId          _systemId;
    smpp_pdu::Password          _password;
    smpp_pdu::SystemType        _systemType;
    smpp_pdu::InterfaceVersion  _interfaceVersion;
    smpp_pdu::Ton               _ton;
    smpp_pdu::Npi               _npi;
    smpp_pdu::AddressRange      _addressRange;
    BindType                    _bindType; 

    //--------------------------------------------------------------------------------
    uint8_t makeInterfaceVersion(int i)
    {
      switch(i) {
        case 33: return 0x33; break;
        case 34: return 0x34; break;
        case 50: return 0x50; break;
        default: return 0x34; break;
      }
    }

    //--------------------------------------------------------------------------------
    BindType makeBindType(std::string s)
    {
      if(s == "RX" ) return RX;
      if(s == "TX" ) return TX;
      if(s == "TRX") return TRX;
    }
};

typedef boost::shared_ptr<ConfigurationManager> SharedConfig;

#endif

