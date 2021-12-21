#pragma once
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include <set>
#ifndef WIN32
#include <sys/socket.h>
#include <csignal>
#include <cstdlib>
#else
#include <winsock2.h>
#endif

#include "serverUtils.h"

struct sockaddr_in;
#ifdef WIN32
struct fd_set;
typedef int socklen_t;
typedef SOCKET socket_t;
#else 
typedef int socket_t;
#endif
struct timeval;

//////////////////////////////////////////////////////////////////////////////////////////

class TCPSocketResponder
{
public:
	bool OpenSocket(int port);
	bool BindSocket();
	bool PollSocket(bool& hasRequest);
	bool AcceptRequest(std::string& requestOut);
	void Respond(const std::string& response);
	void CloseSocket();

	void SetTimeout(timeval to);
	void CreateBuffer(uint32_t size);

private:
	// Possibly only works on windows
	static constexpr socket_t s_invalidSocket = INVALID_SOCKET;

	socket_t m_socket = s_invalidSocket;
	socket_t m_tmpSocket = s_invalidSocket;
	timeval m_timeout = { 0 /*s*/, 100 /*ms*/ };
	fd_set m_set;
	sockaddr_in m_address;
	int m_port;
	std::vector<char> m_buffer;
};

class Server
{
public:
	struct DataRequestHandler
	{
		DataRequestHandler(const char* dataName)
			: dataName(dataName)
		{}
		DataRequestHandler() = delete;

		virtual bool handle(const std::string& requestedData, std::string& contentOut) const = 0;

	private:
		friend Server;
		const char* dataName;
	};

	typedef std::filesystem::path FilePath;
private:
	static std::string getObjectNameFromRequestHeader(const char* request, uint32_t length);
	static bool loadRequestedFile(const std::string& filename, char** bufOut, uint64_t& lengthOut);
	static std::string buildResponseHeader(const std::string& contentType, const std::string& content);

	static bool getContentTypeFromFileName(const std::string& filename, std::string& contentType);
	static bool handleRequest(TCPSocketResponder& responder);
	
	static void runServer(int port);
	static void closeSocket(socket_t socket);
	static bool onFileRequest(const std::string& requestedFileName, std::string& headerOut);
	static bool onDataRequest(const std::string& requestedFileName, std::string& headerOut);
	static bool isDataRequest(const std::string& requestedObject);
	static void applySettings(const ServerUtils::SettingsVector& settings);
	static void makeIndexHeader(std::string& result);

public:
	static void addDataRequestHandler(const DataRequestHandler* handler);
	static void removeDataRequestHandler(const DataRequestHandler* handler);
	static void initiate(int port, const char* path = "");
	static void kill();
	static void join();

private:
	static volatile bool sRunServer;
	static std::thread sThread;
	static socket_t sSocket;
	static std::vector<const DataRequestHandler*> sDataRequestHandlers;
	static std::string sRootWebDir;
	static std::string sSystemFiles;
	static std::string sUserScripts;
};

//////////////////////////////////////////////////////////////////////////////////////////
