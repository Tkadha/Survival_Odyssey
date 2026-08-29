#pragma once

#include <Ws2tcpip.h>


class Endpoint
{
public:
	Endpoint();
	Endpoint(const char* address, int port);
	~Endpoint();

	sockaddr_in m_ipv4Endpoint;

	static Endpoint Any;
	std::string ToString();
};

