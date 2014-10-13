// File  : rawpdu.hpp
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

#ifndef _RAWPDU_HPP_
#define _RAWPDU_HPP_

#include <smpp_pdu_all.hpp>
#include <boost/shared_ptr.hpp>

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

#endif // _RAWPDU_HPP_
