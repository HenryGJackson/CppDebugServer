#include <algorithm>
#include <chrono>    
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <streambuf>
#include <string>
#include <cstdio>
#include <sstream>

#include "server.h"
#include "serverUtils.h"

#ifdef WIN32
#include <io.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <unistd.h>
#include <netinet/in.h>
#include <poll.h>
#endif

#ifdef _MSC_VER
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

volatile bool
Server::sRunServer = true;

std::thread
Server::sThread;

socket_t
Server::sSocket;

std::vector<Server::onDataRequestFunc>
Server::sDataRequestHandlers;

std::string
Server::sRootWebDir;

std::string
Server::sUserScripts;

std::string 
Server::getObjectNameFromRequestHeader(const char* request, uint32_t length)
{
	std::string str(request);

	// Get the position of the first place of what is being requested
	size_t startIt = str.find("GET /") + 5;
	if (startIt >= str.size())
		return "";

	// Find the end of the object name
	size_t endIt = str.find(" ", startIt);

	// Return the requested object name
	return str.substr(startIt, endIt - startIt);
}

bool
Server::loadRequestedFile(const std::string& filename, char** bufOut, uint64_t& lengthOut)
{
	std::string fullFilePath = sRootWebDir + filename;
#ifdef WIN32
	FILE *f = nullptr;
	fopen_s(&f, fullFilePath.c_str(), "rb");
#else
	FILE *f = fopen(fullFilePath.c_str(), "rb");
#endif
	if (f == nullptr)
	{
		std::cout << "Failed to read file: " << fullFilePath << std::endl;
		return false;
	}

	// get the total length of the file
	fseek(f, 0, SEEK_END);
	lengthOut = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	// Allocate the buffer and write the file contents to it
	*bufOut = new char[(unsigned int)lengthOut];
	fread(*bufOut, (size_t)lengthOut, 1, f);

	fclose(f);
	return true;
}

bool
Server::getContentTypeFromFileName(const std::string& filename, std::string& contentType)
{
	// Given a filename, use the extension to determine the content type
	std::string extension = filename.substr(filename.find_last_of(".") + 1);
	if (extension == "html")
	{
		contentType = "text/html";
		return true;
	}
	else if (extension == "js")
	{
		contentType = "text/javascript";
		return true;
	}
	return false;
}

std::string
Server::buildResponseHeader(const std::string& contentType, const std::string& content)
{
	uint32_t contentLength = content.size();
	std::string header(
		"HTTP/1.1 200 OK\nContent-Type: " + contentType + 
		"\nContent-Length: " + std::to_string(contentLength) + 
		"\n\n" + content);

	return header;
}

void
Server::closeSocket(socket_t socket)
{
#ifdef WIN32
	closesocket(socket);
#else
	close(socket);
#endif
}

bool
Server::isDataRequest(const std::string& requestedObject)
{
	return requestedObject.substr(0, 4) == "data";
}

void
Server::makeIndexHeader(std::string& result)
{
	std::string path = sRootWebDir + "html/indexHeader.html";

	// Read in the index header file.
	std::fstream file;
	file.open(path, std::fstream::in);
	if (!file.is_open())
	{
		perror("Failed to open settings file. Continuing with defaults. Things may not work..");
		return;
	}

	// write the index html file to our result string
	result = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	file.close();

	// Find the point at which we should insert the user scripts
	size_t pos = result.find("</body>");
	if (pos == std::string::npos)
	{
		perror("Failed finding entry point in indexHeader.html");
		return;
	}

	std::string userScript;
	std::string sciptTagStart("<script src=\"");
	std::string scriptTagEnd("\"></script>");

	// Add script tags for each of the user scripts
	std::stringstream ss(sUserScripts);
	while (std::getline(ss, userScript, ','))
	{
		userScript = sciptTagStart + userScript + scriptTagEnd;
		result.insert(pos, userScript);
		pos += userScript.length();
	}
}

bool
Server::onFileRequest(const std::string& requestedFileName, std::string& headerOut)
{
	// Get the requeste file from the client. If there is no file specified, give the startup page
	std::string contentType;
	std::string content;
	if (requestedFileName == "")
	{
		contentType = "html";
		makeIndexHeader(content);
	}
	else
	{
		char* buf;
		uint64_t length;
		if (!loadRequestedFile(requestedFileName, &buf, length))
			return false;

		if (!getContentTypeFromFileName(requestedFileName, contentType))
			return false;

		content = std::string(buf, (unsigned int)length);
	}

	headerOut = buildResponseHeader(contentType, content);
	return true;
}

bool
Server::onDataRequest(const std::string& requestedData, std::string& headerOut)
{
	// We know that the "data/" portion of the request takes up 5 characters so we expect at least 6
	if (requestedData.length() < 6)
	{
		std::cout << "Asked for data without specifying which" << std::endl;
		return false;
	}

	// Call all handlers and expect them to early out if they do not handle the requested data
	std::string content;
	for (onDataRequestFunc handler : sDataRequestHandlers)
	{
		if (!handler(requestedData.substr(5), content))
			continue;

		headerOut = buildResponseHeader("text/plain", content);
		return true;
	}

	perror("Unhandled data request");
	return false;
}

void
Server::addDataRequestHandler(onDataRequestFunc handler)
{
	if (std::find(sDataRequestHandlers.begin(), sDataRequestHandlers.end(), handler) != sDataRequestHandlers.end())
		return;

	sDataRequestHandlers.push_back(handler);
}

void
Server::removeDataRequestHandler(onDataRequestFunc handler)
{
	auto it = std::find(sDataRequestHandlers.begin(), sDataRequestHandlers.end(), handler);
	if (it == sDataRequestHandlers.end())
		return;

	sDataRequestHandlers.erase(it);
}

bool
Server::handleRequest(socket_t fd, fd_set& fdSet, timeval& timeout, const sockaddr_in address, socklen_t addrlen)
{
#ifndef WIN32
	static constexpr int flags = (POLLRDNORM | POLLWRNORM | POLLWRBAND | POLLIN | POLLRDBAND | POLLPRI);
	pollfd pfd = { fd, flags, 0 };
	int pollResult = poll(&pfd, 1, 1 /*ms*/);
#else
	static constexpr int flags = (POLLIN | POLLOUT);
	pollfd pfd = { fd, flags, 0 };
	int pollResult = WSAPoll(&pfd, 1, 1 /*ms*/);
#endif
	  
	if (pfd.revents)
	{
		if (pfd.revents & (POLLERR | POLLHUP))
		{
			return false;
		}
		else if (pfd.revents & POLLNVAL)
		{
			return false;
		}
	}

	if (pollResult < 0)
	{
		int lastError = WSAGetLastError();
		std::cout << "Error: " << lastError << std::endl;
		perror("In poll");
		return false;
	}
	else if (pollResult == 0)
		return true;

	int new_socket = accept(fd, (struct sockaddr *)&address, &addrlen);
	if (new_socket < 0)
	{
		perror("In accept");
		return false;
	}

	char buffer[30000] = { 0 };
	long valread = recv(new_socket, buffer, 30000, 0);

	std::string requestedObject = getObjectNameFromRequestHeader((const char*)buffer, 30000);

	std::string header;
	bool success = false;
	if (isDataRequest(requestedObject))
		success = onDataRequest(requestedObject, header);
	else
		success = onFileRequest(requestedObject, header);

	if (success)
		send(new_socket, header.c_str(), header.length(), 0);
	else
		perror("Could not send data");
	
	closeSocket(new_socket);
	
	return true;
}

void
Server::runServer(int port)
{
#ifdef WIN32
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);
#endif

	sSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sSocket < 0)
	{
		perror("In socket");
		return;
	}

	unsigned long val = 10000;
	setsockopt(sSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&val, sizeof(unsigned long));

	struct sockaddr_in address;
	int addrlen = sizeof(address);

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	memset(address.sin_zero, '\0', sizeof address.sin_zero);

	if (bind(sSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("In bind"); 
		return;
	}
	if (listen(sSocket, 10) < 0)
	{
		perror("In listen");
		return;
	}

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sSocket, &set);

	timeval timeout = { 0 /*s*/, 100 /*ms*/ };

	while (handleRequest(sSocket, set, timeout, address, addrlen) && sRunServer)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(1000));
	}

	closeSocket(sSocket);
	std::cout << "Server exiting" << std::endl;
#ifdef WIN32
	WSACleanup();
#endif
}

void
Server::initiate(int port)
{
	ServerUtils::SettingsVector settings;
	ServerUtils::readSettings(settings);
	applySettings(settings);

	sThread = std::thread(runServer, port);
#ifndef WIN32
	ServerUtils::setupQuitHandler();
#endif
}

void
Server::kill()
{
	sRunServer = false;

	// Wait for the server thread to fully shut down
	if (sThread.joinable())
		sThread.join();

	// Reinitialise sRunServer now that the server is no longer running
	// so that if we start it up again, it doesn't immediately exit.
	sRunServer = true;
}

void
Server::join()
{
	sThread.join();
}

template <typename T>
static const T getSetting(const std::string& setting, const T defaultValue, const ServerUtils::SettingsVector& settings)
{	
	auto it = std::find_if(settings.begin(), settings.end(), [&](const ServerUtils::SettingValuePair& settingPair) { return settingPair.first == setting; });
	if (it != settings.end())
		return it->second;

	return defaultValue;
}

void
Server::applySettings(const ServerUtils::SettingsVector& settings)
{
	sRootWebDir = getSetting("WEB_FILES_ROOT", std::experimental::filesystem::current_path().string(), settings);
	sUserScripts = getSetting("USER_SCRIPTS", std::experimental::filesystem::current_path().string(), settings);
}

