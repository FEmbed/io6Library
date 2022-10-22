/*
 * TCPClient.h
 *
 *  Created on: Jun 8, 2022
 *      Author: GeneKong
 */

#ifndef IO6LIBRARY_ETHERNET_TCPCLIENT_H_
#define IO6LIBRARY_ETHERNET_TCPCLIENT_H_

#include <Client.h>
#include "socket.h"

#ifdef  LOG_TAG
	#define STASH_TAG						LOG_TAG
    #undef  LOG_TAG
#endif
#define LOG_TAG                             "TCPClient"

namespace WizNet {

class TCPClient  : public Client {
public:
	TCPClient()
	{
		m_socket_fd = -1;
		_connected = false;
	}

	virtual ~TCPClient()
	{
		// Disconnect socket
		if(m_socket_fd != -1)
			wiz_close(m_socket_fd);
		m_socket_fd = -1;
	}

	int connectV4(uint32_t ip, uint16_t port, int32_t timeout = 5000)
	{
		uint8_t status;
		FE_TICKS_TYPE start = fe_get_ticks();
		for(int i = 0; i< 8; i++)
		{
			if(getsockopt(i, SO_STATUS, &status) == SOCK_OK)
			{
				int8_t ret = 0;
				if(status == SOCK_CLOSED)
				{
					m_socket_fd = i;
					if((ret = wiz_socket(m_socket_fd, Sn_MR_TCPD, port, 0x0)) != m_socket_fd)
					{
						log_w("socket:%d init error:%d", m_socket_fd, ret);
						wiz_close(m_socket_fd);
						m_socket_fd = -1;
					}
					else
					{
						// Wait timeout until connected
						do {
							if((getsockopt(i, SO_STATUS, &status) == SOCK_OK))
							{
								switch(status)
								{
								case SOCK_INIT:
									ret = ::wiz_connect((uint8_t)i, (uint8_t *)&ip, port, (uint8_t)4);
									break;
								case SOCK_ESTABLISHED:
									log_d("socket:%d connected.", i);
									return m_socket_fd;
								case SOCK_CLOSED:
									m_socket_fd = -1;
									break;
								default:;
								}
							}
						} while(fe_ticks_istimeout(start, _timeout) == 0);
					}
					break;
				}
			}
		}
		return m_socket_fd;
	}

#if defined(CONFIG_LWIP_TCP_MSS) || defined(CONFIG_TCP_MSS)
	int connect(IPAddress ip, uint16_t port) override
	{
		if(ip.isV4())
		{
			uint32_t addr = ip.v4();
			this->connectV4(addr, port, this->_timeout);
		}
		else
		{
			log_w("Current wrap can not support ipv6");
			wiz_close(m_socket_fd);
			m_socket_fd = -1;
		}
		return m_socket_fd;
	}
#endif
#if BOOTLOADER == 0
	int connect(const char *host, uint16_t port) override
	{
		IPAddress ip;
		if(ip.fromString(host))
		{
			return this->connect(ip, port);
		}
		log_w("Current version just support ip string host.");
		return -1;
	}

	int connect(const char *host, uint16_t port, int32_t timeout) override
	{
		this->setTimeout(timeout);
		return this->connect(host, port);
	}
#endif

	size_t write(uint8_t byte) override
	{
		if(m_socket_fd != -1)
		{
			wiz_send(m_socket_fd, &byte, 1);
		}
		return 0;
	}

	size_t write(const uint8_t *buf, size_t size) override
	{
		uint16_t ret = 0;
		if(m_socket_fd != -1)
		{
			ret = wiz_send((uint8_t)m_socket_fd, (uint8_t *)buf, (uint16_t)size);
		}
		return ret;
	}

	int available() override
	{
		if(m_socket_fd != -1)
		{
			return getSn_RX_RSR(m_socket_fd);
		}
		return -1;
	}

	int read() override
	{
		uint8_t byte;
		if(m_socket_fd != -1)
		{
			wiz_recv(m_socket_fd, &byte, 1);
		}
		return byte;
	}

	int read(uint8_t *buf, size_t size) override
	{
		if(m_socket_fd != -1)
		{
			return wiz_recv((uint8_t)m_socket_fd, buf, size);
		}
		return 0;
	}

	int peek() override
	{
		return 0;
	}

	void flush() override
	{
		// Do nothing for flush operate.
	}

	void stop() override
	{
		if(m_socket_fd != -1)
		{
			wiz_close(m_socket_fd);
		}
		m_socket_fd = -1;
	}

	uint8_t connected() override
	{
		uint8_t status;
		if(m_socket_fd != -1)
		{
			if((getsockopt(m_socket_fd, SO_STATUS, &status) == SOCK_OK))
			{
				return status == SOCK_ESTABLISHED;
			}
		}
		return 0;
	}

	operator bool() override
	{
		if(m_socket_fd >= 0 && m_socket_fd < 8) return true;
		return false;
	}

	int8_t m_socket_fd;
	timeval m_so_timeout;
	bool _connected;
};

} /* namespace WizNet */

#ifdef STASH_TAG
#undef LOG_TAG
#define  LOG_TAG	STASH_TAG
#undef STASH_TAG
#endif
#endif /* IO6LIBRARY_ETHERNET_WIZNETTCP_H_ */
