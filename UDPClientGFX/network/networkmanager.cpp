#include <network/networkmanager.h>

#include <memory>

#define SERVER_PORT 8412
#define SERVER_IP "127.0.0.1"

gdpNamespaceBegin

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
		m_ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (m_ServerSocket == INVALID_SOCKET) {
			printf("socket failed with error %d\n", WSAGetLastError());
			WSACleanup();
			return;
		}
		printf("socket created successfully!\n");

		unsigned long nonblock = 1;
		result = ioctlsocket(m_ServerSocket, FIONBIO, &nonblock);
		if (result == SOCKET_ERROR) {
			printf("set nonblocking failed with error %d\n", result);
			return;
		}
		printf("set nonblocking successfully!\n");

		m_ServerAddr.sin_family = AF_INET;
		m_ServerAddr.sin_port = htons(SERVER_PORT);
		m_ServerAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
		m_ServerAddrLen = sizeof(m_ServerAddr);

		printf("NetworkManager running...\n");

		m_NextSendTime = std::chrono::high_resolution_clock::now();

		m_NetworkedPositions.resize(NUM_PLAYERS);

		m_Initialized = true;
	}

	void NetworkManager::Destroy()
	{
		if (!m_Initialized)
		{
			return;
		}

		closesocket(m_ServerSocket);
		WSACleanup();

		m_Initialized = false;
	}

	void NetworkManager::Update()
	{
		if (!m_Initialized)
		{
			return;
		}

		// Send information/data back to clients
		SendDataToServer();

		// Handle all recv data
		HandleRECV();

		// Process everything
	}

	void NetworkManager::SendPlayerPositionToServer(float x, float z)
	{
		m_PlayerPosition.x = x;
		m_PlayerPosition.z = z;
	}

	void NetworkManager::SendBulletPositionsToServer(std::vector<float> x, std::vector<float> z)
	{
		for (int i = 0; i < m_BulletPositions.size(); i++)
		{
			m_BulletPositions[i].x = x[i];
			m_BulletPositions[i].z = z[i];

		}

		if (m_BulletPositions.size() < x.size())
		{
			for (int i = m_BulletPositions.size(); i < x.size(); i++)
			{
				PlayerPosition temp;
				temp.x = x[i];
				temp.z = z[i];
				m_BulletPositions.push_back(temp);
			}
		}
	}

	void NetworkManager::HandleRECV()
	{
		// Read
		const int bufLen = 4096;//sizeof(float) * 2 * NUM_PLAYERS;
		char buffer[bufLen];
		int result = recvfrom(m_ServerSocket, buffer, bufLen, 0, (SOCKADDR*)&m_ServerAddr, &m_ServerAddrLen);
		if (result == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// Not a real error, we expect this.
				// -1 is an error, 0 is disconnected, >0 is a message
				// WSA uses this as a flag to check if it is a real error
				return;
			}
			// TODO: We want to handle this differently.
			printf("recvfrom failed with error %d\n", WSAGetLastError());
			Destroy();
			return;
		}

		float amountOfPlayers;
		float amountOfBullets;
		memcpy(&amountOfPlayers, (const void*)&(buffer[0]), sizeof(float)); 
		memcpy(&amountOfBullets, (const void*)&(buffer[4]), sizeof(float));
		//std::cout << "fromserver: " << amountOfBullets << std::endl;

		for (int i = 0; i < amountOfBullets; i ++)
		{
			PlayerPosition bullet; 
			memcpy(&bullet.x, (const void*)&(buffer[8 + i * 8]), sizeof(float));
			memcpy(&bullet.z, (const void*)&(buffer[12 + i * 8]), sizeof(float)); 
			 
			if (i < m_NetworkedPositions.size() - 4)
			{
				/*memcpy(&m_NetworkedPositions[4 + i].x, (const void*)&(buffer[8 + i * 8]), sizeof(float));
				memcpy(&m_NetworkedPositions[4 + i].z, (const void*)&(buffer[12 + i * 8]), sizeof(float));*/

				m_NetworkedPositions[4 + i].x = bullet.x;
				m_NetworkedPositions[4 + i].z = bullet.z;

			}
			else
			{
				m_NetworkedPositions.push_back(bullet);
			}

			//std::cout << "bulletx: " << bullet.x << " bulletz: " << bullet.z << std::endl;

		}

		/*for (int i = 0; i < 4; i++)
		{
			memcpy(&m_NetworkedPositions[i].x, (const void*)&(buffer[4 + (int)amountOfBullets * 8]), sizeof(float));
			memcpy(&m_NetworkedPositions[i].z, (const void*)&(buffer[8 + (int)amountOfBullets * 8]), sizeof(float)); 

		}*/

		//std::cout << "number of players: " << amountOfPlayers << std::endl;

		
		memcpy(&m_NetworkedPositions[0].x, (const void*)&(buffer[8 + (int)amountOfBullets * 8]), sizeof(float));
		memcpy(&m_NetworkedPositions[0].z, (const void*)&(buffer[12 + (int)amountOfBullets * 8]), sizeof(float));
		//std::cout << "playerx: " << m_NetworkedPositions[0].x << " playerz: " << m_NetworkedPositions[0].z << std::endl;
		if (amountOfPlayers > 1)
		{
			memcpy(&m_NetworkedPositions[1].x, (const void*)&(buffer[16 + (int)amountOfBullets * 8]), sizeof(float));
			memcpy(&m_NetworkedPositions[1].z, (const void*)&(buffer[20 + (int)amountOfBullets * 8]), sizeof(float));
		}
		if (amountOfPlayers > 2)
		{
			memcpy(&m_NetworkedPositions[2].x, (const void*)&(buffer[24 + (int)amountOfBullets * 8]), sizeof(float));
			memcpy(&m_NetworkedPositions[2].z, (const void*)&(buffer[28 + (int)amountOfBullets * 8]), sizeof(float));
		}
		if (amountOfPlayers > 3)
		{
			memcpy(&m_NetworkedPositions[3].x, (const void*)&(buffer[32 + (int)amountOfBullets * 8]), sizeof(float));
			memcpy(&m_NetworkedPositions[3].z, (const void*)&(buffer[36 + (int)amountOfBullets * 8]), sizeof(float));
		}
	}

	void NetworkManager::SendDataToServer()
	{
		std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
		if (m_NextSendTime > currentTime)
		{
			return;
		}

		m_NextSendTime = currentTime + std::chrono::milliseconds(200);
		
		// Add 20 ms to the next broadcast time from now()
		//m_NextBroadcastTime 
		//float* message = new float[1 /* number of bullets */ + m_BulletPositions.size() + 2/* player position */];
		std::vector<float> message;
		message.push_back(m_BulletPositions.size());
		for (int i = 0; i < m_BulletPositions.size(); i++)
		{
			message.push_back(m_BulletPositions[i].x);
			message.push_back(m_BulletPositions[i].z);

			//std::cout << " x: " << message[1 + i * 2] << " z: " << message[2 + i * 2] << std::endl;
		}
		//std::cout << "numofbullets: " << message[0] << std::endl;
		//std::cout << "previous x: " << m_PlayerPosition.x << " z: " << m_PlayerPosition.z << std::endl; 
		message.push_back(m_PlayerPosition.x); 
		message.push_back(m_PlayerPosition.z); 
		//std::cout << "playerx: " << message[m_BulletPositions.size() * 2 + 1] << "playerz: " << message[m_BulletPositions.size() * 2 + 2] << std::endl;
		//message[0] = 
		// MessageQueue, loop through and send all messages
		// You may multiple servers, you are sending data to
		int result = sendto(m_ServerSocket, reinterpret_cast<const char*>(message.data()), //(const char*)&m_PlayerPosition, 
			message.size() * sizeof(float), 0, (SOCKADDR*)&m_ServerAddr, m_ServerAddrLen); 
		if (result == SOCKET_ERROR) {
			// TODO: We want to handle this differently.
			printf("send failed with error %d\n", WSAGetLastError());
			Destroy();
			return;
		}
	}


} // namespace net

gdpNamespaceEnd