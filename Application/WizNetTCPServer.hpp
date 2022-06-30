/*
 * TCPServer.hpp
 *
 *  Created on: 2022年6月10日
 *      Author: GeneKong
 */

#ifndef IO6LIBRARY_APPLICATION_TCPSERVER_HPP_
#define IO6LIBRARY_APPLICATION_TCPSERVER_HPP_

#include "socket.h"
#include "w6100.h"
#include "WizNetTCPClient.hpp"

#define W6100_TCP_SERVER_CONN_NUM						(4)

#ifdef  LOG_TAG
    #undef  LOG_TAG
#endif
#define LOG_TAG                             "TCPServer"

namespace WizNet {

class TCPServer {
public:
	TCPServer(uint8_t max = W6100_TCP_SERVER_CONN_NUM)
	{
		m_max_conn = max;
		for(int i = 0; i< m_max_conn; i++)
		{
			m_socket_fd[i] = -1;
		}
		m_port = 0;
	}

	TCPServer(uint16_t port)
		: TCPServer()
	{
		this->establish(port);
	}

	virtual ~TCPServer()
	{
		for(int j = 0; j < m_max_conn; j++)
		{
			if(m_socket_fd[j] != -1)
				close(m_socket_fd[j]);
			m_socket_fd[j] = -1;
		}
	}

	/**
	 * @fn int establish(uint16_t, uint32_t=INADDR_ANY)
	 * @brief server will bind port and start listening.
	 *
	 * @param port	port used for bind.
	 *
	 * @return establish ok or not.
	 */
	virtual bool establish(uint16_t port)
	{
		uint8_t status;
		for(int j = 0; j < m_max_conn; j++)
		{
			this->establish(port, j);
		}
		m_port = port;
		return true;
	}

	/**
     * @fn bool isAvailable(int)
	 * @brief Check sub-connection is available.
	 *
	 * @param index which sub-connection need check.
	 * @return is connected or not.
	 */
	bool isAvailable(int index)
	{
		uint8_t status;
		if(index < 0 || index > m_max_conn)
			return false;

		if(m_socket_fd[index] == -1)
		{
			if(!this->establish(m_port, index))
			{
				log_w("Server sub-conn %d have no valid sockets.", index);
			}
		}

		if(getsockopt(m_socket_fd[index], SO_STATUS, &status) == SOCK_OK)
		{
			switch(status)
			{
			case SOCK_ESTABLISHED:
			{
				if(getSn_IR(m_socket_fd[index]) & Sn_IR_CON)
				{
					setSn_IR(m_socket_fd[index], Sn_IR_CON);
				}
				return true;
			}
			case SOCK_CLOSED:
				m_socket_fd[index] = -1;
				break;
			default:;
			}
		}
		return false;
	}

	/**
     * @fn int available(int)
	 * @brief check special index have available byte to receive.
	 *
	 * @param index check special sockets.
	 * @return 0 or available bytes.
	 */
	int available(int index)
	{
		if(this->isAvailable(index))
		{
			return  getSn_RX_RSR(m_socket_fd[index]);
		}

		if(m_socket_fd[index] != -1)
			close(m_socket_fd[index]);
		m_socket_fd[index] = -1;
		return 0;
	}

	int read(int index, uint8_t *buf, size_t size)
	{
		if(m_socket_fd[index] != -1)
		{
			return recv((uint8_t)m_socket_fd[index], buf, size);
		}
		return 0;
	}

	size_t write(int index, const uint8_t *buf, size_t size)
	{
		uint16_t ret = 0;
		if(m_socket_fd[index] != -1)
		{
			ret = send((uint8_t)m_socket_fd[index], (uint8_t *)buf, (uint16_t)size);
		}
		return ret;
	}

private:
	bool establish(uint16_t port, int index)
	{
		uint8_t status;
		FE_TICKS_TYPE start = fe_get_ticks();
		FE_TICKS_TYPE timeout = 5000;

		if(index < 0 || index > m_max_conn)
			return false;

		if(m_socket_fd[index] != -1)
		{
			close(m_socket_fd[index]);
			m_socket_fd[index] = -1;
		}

		for(int i = 0; i< 8; i++)
		{
			if(getsockopt(i, SO_STATUS, &status) == SOCK_OK)
			{
				int8_t ret = 0;
				if(status == SOCK_CLOSED)
				{
					m_socket_fd[index] = i;
					if((ret = socket(m_socket_fd[index], Sn_MR_TCPD, port, 0x0)) != m_socket_fd[index])
					{
						log_w("socket:%d init error:%d", m_socket_fd[index], ret);
						close(m_socket_fd[index]);
						m_socket_fd[index] = -1;
					}
					else
					{
						// Wait timeout until listen
						do {
							bool setup = false;
							if((getsockopt(i, SO_STATUS, &status) == SOCK_OK))
							{
								switch(status)
								{
								case SOCK_INIT:
									ret = listen((uint8_t)i);
									break;
								case SOCK_LISTEN:
								case SOCK_ESTABLISHED:
									log_d("socket:%d listening.", i);
									setup = true;
									break;
								case SOCK_CLOSED:
									m_socket_fd[index] = -1;
									break;
								default:;
								}
							}
							if(setup) break;
						} while(fe_ticks_istimeout(start, timeout) == FALSE);
					}
				}
			}
		}
		return m_socket_fd[index] != -1;
	}

	int8_t m_socket_fd[W6100_TCP_SERVER_CONN_NUM];
	uint8_t m_max_conn;
	uint16_t m_port;
};

} /* namespace WizNet */

#undef  LOG_TAG
#endif /* IO6LIBRARY_APPLICATION_WIZNETTCPSERVER_HPP_ */
