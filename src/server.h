#pragma once
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
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
// Helper macros to make it easy to add data request handlers
#define DEBUG_SERVER

#ifdef DEBUG_SERVER

#define RUN_DEBUG_SERVER(port) { Server::initiate(port); }
#define JOIN_DEBUG_SERVER { Server::join(); }
#define KILL_DEBUG_SERVER { Server::kill(); }

#define MAKE_REQUEST_HANDLER_NAME(id) on ## id ## Request
#define MAKE_REQUEST_HANDLER_SIGNATURE(id) bool MAKE_REQUEST_HANDLER_NAME(id)(const std::string& requestedData, std::string& contentOut)

#define DEFINE_ARRAY_REQUEST_HANDLER(ptr, elementCount, id)						\
MAKE_REQUEST_HANDLER_SIGNATURE(id)										\
{																		\
	if (requestedData != #id)											\
			return false;												\
																		\
	contentOut = ServerUtils::toString(ptr, elementCount);	\
	return true;														\
}

#define DEFINE_VECTOR_REQUEST_HANDLER(vector, id)						\
MAKE_REQUEST_HANDLER_SIGNATURE(id)										\
{																		\
	if (requestedData != #id)											\
			return false;												\
																		\
	contentOut = ServerUtils::toString(vector.data(), vector.size());	\
	return true;														\
}

#define DEFINE_VECTOR3_REQUEST_HANDLER(vector, id)			\
MAKE_REQUEST_HANDLER_SIGNATURE(id)							\
{															\
	if (requestedData != #id)								\
			return false;									\
															\
	contentOut = ServerUtils::toString(vector.data(), 3);	\
	return true;											\
}

#define ADD_REQUEST_HANDLER(id) { Server::addDataRequestHandler(MAKE_REQUEST_HANDLER_NAME(id)); }
#else
#define DEFINE_VECTOR_REQUEST_HANDLER(vector, id)
#define DEFINE_VECTOR3_REQUEST_HANDLER(vector, id)
#define ADD_REQUEST_HANDLER(id)
#define RUN_DEBUG_SERVER(port)
#define JOIN_DEBUG_SERVER  
#define KILL_DEBUG_SERVER 
#endif

//////////////////////////////////////////////////////////////////////////////////////////

class Server
{
public:
	typedef bool(*onDataRequestFunc)(const std::string& requestedData, std::string& contentOut);
	typedef std::experimental::filesystem::v1::path FilePath;
private:
	static std::string getObjectNameFromRequestHeader(const char* request, uint32_t length);
	static bool loadRequestedFile(const std::string& filename, char** bufOut, uint64_t& lengthOut);
	static std::string buildResponseHeader(const std::string& contentType, const std::string& content);

	static bool getContentTypeFromFileName(const std::string& filename, std::string& contentType);
	static bool handleRequest(socket_t fd, fd_set& fdSet, timeval& timeout, const sockaddr_in address, socklen_t addrlen);
	
	static void runServer(int port);
	static void closeSocket(socket_t socket);
	static bool onFileRequest(const std::string& requestedFileName, std::string& headerOut);
	static bool onDataRequest(const std::string& requestedFileName, std::string& headerOut);
	static bool isDataRequest(const std::string& requestedObject);
	static void applySettings(const ServerUtils::SettingsVector& settings);
	static void makeIndexHeader(std::string& result);

public:
	static void addDataRequestHandler(onDataRequestFunc handler);
	static void removeDataRequestHandler(onDataRequestFunc handler);
	static void initiate(int port);
	static void kill();
	static void join();

private:
	static volatile bool sRunServer;
	static std::thread sThread;
	static socket_t sSocket;
	static std::vector<onDataRequestFunc> sDataRequestHandlers;
	static std::string sRootWebDir;
	static std::string sUserScripts;
};

//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
