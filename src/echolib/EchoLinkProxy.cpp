/**
@file	 EchoLinkProxy.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2010-

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <openssl/md5.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "EchoLinkProxy.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
using namespace Async;
using namespace EchoLink;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

enum
{
  MSG_SYSTEM_BAD_PASSWORD=1, MSG_SYSTEM_ACCESS_DENIED
};


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

void print_hex(const unsigned char *buf, int len, char direction)
{
  printf("%2d%c ", len, direction);
  for (int i=0; i<len; ++i)
  {
    printf("%02x ", buf[i]);
  }
  putchar('\n');
}


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

Proxy *Proxy::the_instance = 0;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Proxy::Proxy(const string &host, uint16_t port, const string &callsign,
             const string &password)
  : con(host, port, recv_buf_size), callsign(callsign), password(password),
    state(STATE_DISCONNECTED), tcp_state(TCP_STATE_DISCONNECTED),
    recv_buf_cnt(0)
{
  delete the_instance;
  the_instance = this;

    // If the password is empty we assume the user want to use a public
    // proxy. The password for public proxies should by convension be
    // set to PUBLIC.
  if (password.empty())
  {
    this->password = "PUBLIC";
  }
  else
  {
    transform(this->password.begin(), this->password.end(),
              this->password.begin(), ::toupper);
  }
  con.connected.connect(mem_fun(*this, &Proxy::onConnected));
  con.dataReceived.connect(mem_fun(*this, &Proxy::onDataReceived));
  con.disconnected.connect(mem_fun(*this, &Proxy::onDisconnected));
} /* Proxy::Proxy */


Proxy::~Proxy(void)
{
  the_instance = 0;
} /* Proxy::~Proxy */


void Proxy::connect(void)
{
  con.connect();
} /* Proxy::connect  */


void Proxy::disconnect(void)
{
  con.disconnect();
  onDisconnected(&con, TcpClient::DR_ORDERED_DISCONNECT);
} /* Proxy::disconnect */


bool Proxy::tcpOpen(const IpAddress &remote_ip)
{
  if (tcp_state >= TCP_STATE_CONNECTING)
  {
    return true;
  }
  if (tcp_state == TCP_STATE_DISCONNECTING)
  {
    return false;
  }
  tcp_state = TCP_STATE_CONNECTING;

  return sendMsgBlock(MSG_TYPE_TCP_OPEN, remote_ip);
} /* Proxy::tcpOpen */


bool Proxy::tcpClose(void)
{
  if (tcp_state <= TCP_STATE_DISCONNECTING)
  {
    return true;
  }
  tcp_state = TCP_STATE_DISCONNECTING;

  return sendMsgBlock(MSG_TYPE_TCP_CLOSE);
} /* Proxy::tcpClose */


bool Proxy::tcpData(const void *data, unsigned len)
{
  if (tcp_state != TCP_STATE_CONNECTED)
  {
    return false;
  }

  return sendMsgBlock(MSG_TYPE_TCP_DATA, IpAddress(), data, len);
} /* Proxy::tcpData */


bool Proxy::udpData(const IpAddress &addr, const void *data, unsigned len)
{
  return sendMsgBlock(MSG_TYPE_UDP_DATA, addr, data, len);
} /* Proxy::udpData */


bool Proxy::udpCtrl(const IpAddress &addr, const void *data, unsigned len)
{
  return sendMsgBlock(MSG_TYPE_UDP_CONTROL, addr, data, len);
} /* Proxy::udpCtrl */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

bool Proxy::sendMsgBlock(MsgBlockType type, const IpAddress &remote_ip,
                         const void *data, unsigned len)
{
  //cout << "> type=" << type << " remote_ip=" << remote_ip
  //     << " len=" << len << endl;

  if (!con.isConnected() || (state != STATE_CONNECTED))
  {
    return false;
  }

  int msg_len = MSG_HEADER_SIZE + len;
  uint8_t msg_buf[msg_len];
  uint8_t *msg_ptr = msg_buf;

    // Store the message type
  *msg_ptr++ = static_cast<uint8_t>(type);

    // Store the proxied remote IP address
  in_addr_t remote_ip_addr = 0;
  if (!remote_ip.isEmpty())
  {
    remote_ip_addr = remote_ip.ip4Addr().s_addr;
  }
  *msg_ptr++ = remote_ip_addr & 0xff;
  *msg_ptr++ = (remote_ip_addr >> 8) & 0xff;
  *msg_ptr++ = (remote_ip_addr >> 16) & 0xff;
  *msg_ptr++ = (remote_ip_addr >> 24) & 0xff;

    // Store the message data length
  *msg_ptr++ = len & 0xff;
  *msg_ptr++ = (len >> 8) & 0xff;
  *msg_ptr++ = (len >> 16) & 0xff;
  *msg_ptr++ = (len >> 24) & 0xff;

    // Store the message data
  memcpy(msg_ptr, data, len);

    // Send the message
  //print_hex(msg_buf, msg_len, '>');
  //FIXME: We probably need a message FIFO here?
  int ret = con.write(msg_buf, msg_len);
  if (ret == -1)
  {
    cerr << "*** ERROR: Error while writing message to proxy\n";
    disconnect();
  }
  else if (static_cast<unsigned>(ret) != msg_len)
  {
    cerr << "*** ERROR: Could not write all data to proxy\n";
    disconnect();
  }
  return true;
} /* Proxy::sendMsgBlock */


void Proxy::onConnected(void)
{
  state = STATE_WAITING_FOR_DIGEST;
  cout << "Connected to EchoLink proxy "
       << con.remoteHost() << ":" << con.remotePort() << endl;
} /* Proxy::onConnected */


int Proxy::onDataReceived(TcpConnection *con, void *data, int len)
{
  unsigned char *buf = reinterpret_cast<unsigned char*>(data);

  //print_hex(buf, len, '<');
  //cout << "### raw data len=" << len << endl;

  switch (state)
  {
    case STATE_WAITING_FOR_DIGEST:
      return handleAuthentication(buf, len);
      break;

    case STATE_WAITING_FOR_FIRST_MSG:
    case STATE_CONNECTED:
      return parseProxyMessageBlock(buf, len);
      break;

    case STATE_DISCONNECTED:
      cerr << "*** ERROR: Data received in disconnected state\n";
      disconnect();
      break;

    default:
      cerr << "*** ERROR: Data received in unknown state\n";
      disconnect();
      break;
  }

  return 0;

} /* Proxy::onDataReceived */


void Proxy::onDisconnected(TcpConnection *con,
                           Async::TcpClient::DisconnectReason reason)
{
  cout << "Disconnected from EchoLink proxy "
       << con->remoteHost() << ":" << con->remotePort() << endl;
  state = STATE_DISCONNECTED;
  proxyReady(false);

  if (tcp_state != TCP_STATE_DISCONNECTED)
  {
    tcp_state = TCP_STATE_DISCONNECTED;
    recv_buf_cnt = 0;
    tcpDisconnected();
  }
} /* Proxy::onDisconnected */


/**
 * @brief   Create and send an authentication message
 * @param   con The connection that the message was received on
 * @param   buf The buffer containing the data
 * @param   len The number of bytes in the buffer
 *
 * This function will create and send an authentication message when an eight
 * byte nonce has been received from the proxy server. The authentication
 * message is constructed by first putting the callsign and a newline character
 * into the send buffer. Then an MD5 sum is calculated over the password
 * concatenated with the received nonce. The binary MD5 sum is appended to the
 * authentication message and then sent to the proxy server.
 */
int Proxy::handleAuthentication(const unsigned char *buf, int len)
{
  if (len >= NONCE_SIZE)
  {
    unsigned auth_msg_len = callsign.size() + 1 + MD5_DIGEST_LENGTH;
    unsigned char auth_msg[auth_msg_len+1];
    unsigned char *auth_msg_ptr = auth_msg;
    memcpy(auth_msg_ptr, callsign.c_str(), callsign.size());
    auth_msg_ptr += callsign.size();
    *auth_msg_ptr = '\n';
    auth_msg_ptr += 1;

    unsigned auth_str_len = password.size() + NONCE_SIZE;
    unsigned char auth_str[auth_str_len + 1];
    unsigned char *auth_str_ptr = auth_str;
    memcpy(auth_str_ptr, password.c_str(), password.size());
    auth_str_ptr += password.size();
    memcpy(auth_str_ptr, buf, NONCE_SIZE);
    auth_str[auth_str_len] = 0;
    //cout << auth_str << endl;
    MD5(auth_str, auth_str_len, auth_msg_ptr);
    auth_msg[auth_msg_len] = 0;

    //print_hex(auth_msg, auth_msg_len, '>');

    con.write(auth_msg, auth_msg_len);
    //state = STATE_WAITING_FOR_FIRST_MSG;
    state = STATE_CONNECTED;
    proxyReady(true);
    return NONCE_SIZE;
  }

  return 0;

} /* Proxy::handleAuthentication */


int Proxy::parseProxyMessageBlock(unsigned char *buf, int len)
{
  int total_processed = 0;
  while (len >= MSG_HEADER_SIZE)
  {
      // Extract message type
    MsgBlockType msg_type = static_cast<MsgBlockType>(*buf++);

      // Extract ip address of proxied remote host
    IpAddress::Ip4Addr remote_ip_addr;
    remote_ip_addr.s_addr = *buf++;
    remote_ip_addr.s_addr |= static_cast<uint32_t>(*buf++) << 8;
    remote_ip_addr.s_addr |= static_cast<uint32_t>(*buf++) << 16;
    remote_ip_addr.s_addr |= static_cast<uint32_t>(*buf++) << 24;
    IpAddress remote_ip(remote_ip_addr);

      // Extract the length of the data field
    uint32_t msg_len = *buf++;
    msg_len |= static_cast<uint32_t>(*buf++) << 8;
    msg_len |= static_cast<uint32_t>(*buf++) << 16;
    msg_len |= static_cast<uint32_t>(*buf++) << 24;

    int total_msg_size = MSG_HEADER_SIZE + msg_len;
    if (len < total_msg_size)
    {
      break;
    }

    handleProxyMessageBlock(msg_type, remote_ip, msg_len, buf);
    buf += msg_len;
    len -= total_msg_size;
    total_processed += total_msg_size;
  }
  
  return total_processed;
} /* Proxy::parseProxyMessageBlock */


void Proxy::handleProxyMessageBlock(MsgBlockType type,
    const IpAddress &remote_ip, uint32_t len, unsigned char *data)
{
  //cout << "< type=" << (unsigned)type << " len=" << len
  //     << " remote_ip=" << remote_ip << endl;

  switch (state)
  {
    case STATE_WAITING_FOR_DIGEST:
      cerr << "*** ERROR: Received message block while waiting for nonce\n";
      disconnect();
      return;

    case STATE_WAITING_FOR_FIRST_MSG:
      if (type != MSG_TYPE_SYSTEM)
      {
        state = STATE_CONNECTED;
      }

    default:
      break;
  }

  switch (type)
  {
    case MSG_TYPE_TCP_OPEN:
      cerr << "*** ERROR: TCP_OPEN EchoLink proxy message received. "
              "This is not a message that the proxy should send.\n";
      disconnect();
      break;

    case MSG_TYPE_TCP_DATA:
      handleTcpDataMsg(data, len);
      break;
     
    case MSG_TYPE_TCP_CLOSE:
      handleTcpCloseMsg(data, len);
      break;

    case MSG_TYPE_TCP_STATUS:
      handleTcpStatusMsg(data, len);
      break;
     
    case MSG_TYPE_UDP_DATA:
      handleUdpDataMsg(remote_ip, data, len);
      break;

    case MSG_TYPE_UDP_CONTROL:
      handleUdpCtrlMsg(remote_ip, data, len);
      break;

    case MSG_TYPE_SYSTEM:
      handleSystemMsg(data, len);
      break;

    default:
      cerr << "*** ERROR: Unknown EchoLink proxy message type received: "
           << type << "\n";
      disconnect();
      break;
  }
} /* handleProxyMessageBlock */


void Proxy::handleTcpDataMsg(uint8_t *buf, int len)
{
  if (tcp_state != TCP_STATE_CONNECTED)
  {
    cerr << "*** ERROR: TCP data received from EchoLink proxy but no TCP "
            "connection should be open at the moment.\n";
    return;
  }

  if (len > 0)
  {
    //cout << "### len=" << len;
    if (recv_buf_cnt > 0)
    {
      if (recv_buf_cnt + len > recv_buf_size)
      {
        //cerr << "*** ERROR: Data buffer overflow in EchoLink proxy for "
        //        "TCP data\n";
        disconnect();
        return;
      }
      memcpy(recv_buf + recv_buf_cnt, buf, len);
      recv_buf_cnt += len;
      int processed = tcpDataReceived(recv_buf, recv_buf_cnt);
      //cout << "  processed from buffer=" << processed;
      if (processed >= recv_buf_cnt)
      {
        recv_buf_cnt = 0;
      }
      else
      {
        recv_buf_cnt = recv_buf_cnt - processed;
        memmove(recv_buf, recv_buf + processed, recv_buf_cnt);
      }
    }
    else
    {
      int processed = tcpDataReceived(buf, len);
      //cout << "  processed directly=" << processed;
      if (processed < len)
      {
        recv_buf_cnt = len - processed;
        memcpy(recv_buf, buf + processed, recv_buf_cnt);
      }
    }
    //cout << "  recv_buf_cnt=" << recv_buf_cnt << endl;
  }
} /* Proxy::handleTcpDataMsg */


void Proxy::handleTcpCloseMsg(const uint8_t *buf, int len)
{
  if (len != 0)
  {
    cerr << "*** ERROR: Wrong size for TCP_CLOSE message\n";
    disconnect();
    return;
  }

  tcpCloseReceived();

  if (tcp_state == TCP_STATE_DISCONNECTED)
  {
    return;
  }
  tcp_state = TCP_STATE_DISCONNECTED;

  tcpDisconnected();
} /* Proxy::handleTcpCloseMsg */


void Proxy::handleTcpStatusMsg(const uint8_t *buf, int len)
{
  if (len != 4)
  {
    cerr << "*** ERROR: Wrong size for TCP_STATUS message\n";
    disconnect();
    return;
  }

  uint32_t status = *buf++;
  status |= static_cast<uint32_t>(*buf++) << 8;
  status |= static_cast<uint32_t>(*buf++) << 16;
  status |= static_cast<uint32_t>(*buf++) << 24;

  tcpStatusReceived(status);

  if (tcp_state == TCP_STATE_CONNECTING)
  {
    if (status == 0)
    {
      tcp_state = TCP_STATE_CONNECTED;
      recv_buf_cnt = 0;
      tcpConnected();
    }
    else
    {
      cerr << "*** ERROR: The directory connection through the EchoLink "
              "proxy could not be established\n";
      tcp_state = TCP_STATE_DISCONNECTED;
      tcpDisconnected();
    }
  }
} /* Proxy::handleTcpStatusMsg */


void Proxy::handleUdpDataMsg(const IpAddress &remote_ip, uint8_t *buf, int len)
{
  if (len > 0)
  {
    udpDataReceived(remote_ip, buf, len);
  }
} /* Proxy::handleUdpDataMsg */


void Proxy::handleUdpCtrlMsg(const IpAddress &remote_ip, uint8_t *buf, int len)
{
  if (len > 0)
  {
    udpCtrlReceived(remote_ip, buf, len);
  }
} /* Proxy::handleUdpCtrlMsg */


void Proxy::handleSystemMsg(const unsigned char *buf, int len)
{
  //if (state != STATE_WAITING_FOR_FIRST_MSG)
  if (state != STATE_CONNECTED)
  {
    cerr << "*** ERROR: SYSTEM message received in wrong state\n";
    disconnect();
    return;
  }

  if (len != 1)
  {
    cerr << "*** ERROR: Malformed SYSTEM message block\n";
    disconnect();
    return;
  }

  switch (*buf)
  {
    case MSG_SYSTEM_BAD_PASSWORD:
      cerr << "*** ERROR: Bad EchoLink proxy password\n";
      disconnect();
      break;

    case MSG_SYSTEM_ACCESS_DENIED:
      cerr << "*** ERROR: Access denied to EchoLink proxy\n";
      disconnect();
      break;

    default:
      cerr << "*** ERROR: Unknown SYSTEM message: "
           << (unsigned)*buf << "\n";
      disconnect();
      break;
  }
} /* Proxy::handleSystemMsgBlock */



/*
 * This file has not been truncated
 */

