#include "NetworkManager.h"

#include <memory>
#include <iostream>

#define SERVER_PORT 8412
#define SERVER_IP "127.0.0.1"

namespace net
{
	// Move this to a structure for each client
	sockaddr_in addr;
	int addrLen;

	class Buffer
	{
	public:
		Buffer() { }
		~Buffer() { }

		std::vector<uint8_t> data;
	};

	NetworkManager::NetworkManager()
	{
	}

	NetworkManager::~NetworkManager()
	{

	}

	void NetworkManager::Initialize()
	{
		// Initialize WinSock
		WSADATA wsaData;
		int result;

		// Set version 2.2 with MAKEWORD(2,2)
		result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0) {
			printf("WSAStartup failed with error %d\n", result);
			return;
		}
		printf("WSAStartup successfully!\n");


		// Socket
		m_ListenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error %d\n", WSAGetLastError());
			WSACleanup();
			return;
		}
		printf("socket created successfully!\n");

		// using sockaddr_in
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(SERVER_PORT);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		// Bind 
		result = bind(m_ListenSocket, (SOCKADDR*)&addr, sizeof(addr));
		if (result == SOCKET_ERROR) {
			printf("bind failed with error %d\n", WSAGetLastError());
			closesocket(m_ListenSocket);
			WSACleanup();
			return;
		}
		printf("bind was successful!\n");

		unsigned long nonblock = 1;
		result = ioctlsocket(m_ListenSocket, FIONBIO, &nonblock);
		if (result == SOCKET_ERROR) {
			printf("set socket to nonblocking failed with error %d\n", WSAGetLastError());
			closesocket(m_ListenSocket);
			WSACleanup();
			return;
		}
		printf("set socket to nonblocking was successful!\n");

		printf("NetworkManager running...\n");

		m_NextBroadcastTime = std::chrono::high_resolution_clock::now();

		m_Initialized = true;
	}

	void NetworkManager::Destroy()
	{
		if (!m_Initialized)
		{
			return;
		}

		closesocket(m_ListenSocket);
		WSACleanup();

		m_Initialized = false;
	}

	void NetworkManager::Update()
	{
		if (!m_Initialized)
		{
			return;
		}

		// Handle all recv data
		HandleRECV();

		// Process everything

		// Send information/data back to clients
		BroadcastUpdatesToClients();
	}

	void NetworkManager::HandleRECV()
	{
		// Read
		sockaddr_in addr;
		int addrLen = sizeof(addr);

		const int bufLen = 4096;	// recving 2 floats only
		char buffer[bufLen];
		int result = recvfrom(m_ListenSocket, buffer, bufLen, 0, (SOCKADDR*)&addr, &addrLen);
		if (result == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// Not a real error, we expect this.
				// -1 is an error, 0 is disconnected, >0 is a message
				// WSA uses this as a flag to check if it is a real error
				return;
			}
			else
			{
				// TODO: We want to handle this differently.
				printf("recvfrom failed with error %d\n", WSAGetLastError());
				Destroy();
				//closesocket(m_ListenSocket);
				//WSACleanup();
				return;
			}
		}

		// Compare to see if the addr is already registered
		// If it is not registered, we add it
		// If it is registered, we can set the data
		int clientId = -1;
		for (int i = 0; i < m_ConnectedClients.size(); i++)
		{
			ClientInfo& client = m_ConnectedClients[i];
			if (client.addr.sin_addr.s_addr == addr.sin_addr.s_addr
				&& client.addr.sin_port == addr.sin_port)
			{
				clientId = i;
				break;
			}
		}

		if (clientId == -1)
		{
			// Add the client
			ClientInfo newClient;
			newClient.addr = addr;
			newClient.addrLen = sizeof(addr);
			m_ConnectedClients.push_back(newClient);
			clientId = m_ConnectedClients.size() - 1;
		}

		ClientInfo& client = m_ConnectedClients[clientId];


		float amountOfBullets;
		memcpy(&amountOfBullets, (const void*)&(buffer[0]), sizeof(float));
		//std::cout << "num of bullets with client: " << client.bullets.size() << std::endl;
		
		for (int i = 0; i < amountOfBullets; i ++)
		{
			PlayerPosition bullet; 
			memcpy(&bullet.x, (const void*)&(buffer[(i * 8) + 4]), sizeof(float)); 
			memcpy(&bullet.z, (const void*)&(buffer[(i * 8) + 8]), sizeof(float)); 
			//std::cout << "bullet number: " << i << " bulletx: " << bullet.x << " bulletz: " << bullet.z << std::endl;
			
			if (i < client.bullets.size())
			{
				//memcpy(&client.bullets[(i - 4) / 4].x, (const void*)&(buffer[i]), sizeof(float));
				//memcpy(&client.bullets[(i - 4) / 4].z, (const void*)&(buffer[i+4]), sizeof(float));
				client.bullets[i].x = bullet.x; 
				client.bullets[i].z = bullet.z; 
			}
			else
			{
				client.bullets.push_back(bullet);
			}
		}
		
		memcpy(&client.x, (const void*)&(buffer[(int)amountOfBullets * 8 + 4]), sizeof(float));
		memcpy(&client.z, (const void*)&(buffer[(int)amountOfBullets * 8 + 8]), sizeof(float)); 

		//std::cout << "playerx: " << client.x << " playerz: " << client.z << std::endl;

		//printf("From: %s:%d: {%.2f, %.2f}\n", inet_ntoa(client.addr.sin_addr), client.addr.sin_port, client.x, client.z);
	}

	void NetworkManager::BroadcastUpdatesToClients()
	{
		std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
		if (m_NextBroadcastTime > currentTime)
		{
			return;
		}

		//printf("broadcast!\n");

		m_NextBroadcastTime = currentTime + std::chrono::milliseconds(200);

		// Add 20 ms to the next broadcast time from now()
		//m_NextBroadcastTime 

		const int length = 4096;//sizeof(PlayerPosition) * 4;
		char data[length];


		//PlayerPosition positions[4];
		for (int k = 0; k < m_ConnectedClients.size(); k++)
		{
			std::vector<float> message;
			//message.push_back(m_ConnectedClients[0].bullets.size())
			message.push_back(m_ConnectedClients.size());
			int bulletAmount = 0;
			for (int i = 0; i < m_ConnectedClients.size(); i++)
			{
				if (k != i)
					bulletAmount += m_ConnectedClients[i].bullets.size();
			}
			//std::cout << bulletAmount << " bullets" << std::endl;

			message.push_back(bulletAmount);

			for (int i = 0; i < m_ConnectedClients.size(); i++)
			{
				if (k != i)
				{
					for (int j = 0; j < m_ConnectedClients[i].bullets.size(); j++)
					{
						message.push_back(m_ConnectedClients[i].bullets[j].x);
						message.push_back(m_ConnectedClients[i].bullets[j].z);
					}
				}
			}

			for (int i = 0; i < m_ConnectedClients.size(); i++)
			{
				//memcpy(&data[i * sizeof(PlayerPosition)], &m_ConnectedClients[i].x, sizeof(float));
				//memcpy(&data[i * sizeof(PlayerPosition) + sizeof(float)], &m_ConnectedClients[i].z, sizeof(float));
				if (k != i)
				{
					message.push_back(m_ConnectedClients[i].x);
					message.push_back(m_ConnectedClients[i].z);
				}

				//std::cout << "client: " << i << " x: " << message[2 + bulletAmount * 2 + i * 2] << " z: " << message[3 + bulletAmount * 2 + i * 2] << std::endl;

			}

			// Write

			ClientInfo& client = m_ConnectedClients[k]; 
			int result = sendto(m_ListenSocket, reinterpret_cast<const char*>(message.data()), message.size() * sizeof(float), 0, (SOCKADDR*)&client.addr, client.addrLen);
			if (result == SOCKET_ERROR) {
				// TODO: We want to handle this differently.
				printf("send failed with error %d\n", WSAGetLastError());
				closesocket(m_ListenSocket);
				WSACleanup();
				return;
			}
		}
	}


} // namespace net