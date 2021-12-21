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
#include <Common/ErrorHandling.h>

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

std::vector<const Server::DataRequestHandler*>
Server::sDataRequestHandlers;

std::string
Server::sRootWebDir;

std::string
Server::sSystemFiles;

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
		"HTTP/1.1 200 OK\r\nContent-Type: " + contentType + 
		"\r\nContent-Length: " + std::to_string(contentLength) + 
		"\r\n\n" + content);
	std::cout << content << std::endl;
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
	std::string path = sSystemFiles + "\\html\\indexHeader.html";

	// Read in the index header file.
	std::fstream file;
	file.open(path, std::fstream::in);
	COM_ASSERT(file.is_open(), "Failed to open settings file. Continuing with defaults. Things may not work..");

	// write the index html file to our result string
	result = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	file.close();

	// Find the point at which we should insert the user scripts
	size_t pos = result.find("</body>");
	COM_ASSERT(pos != std::string::npos, "Failed finding entry point in indexHeader.html");

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
		contentType = "text/html";
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
	std::string dataToBeHandled = requestedData.substr(5);

	std::string content;
	for (const DataRequestHandler* handler : sDataRequestHandlers)
	{
		if (handler->dataName != dataToBeHandled)
			continue;

		handler->handle(dataToBeHandled, content);
		
		headerOut = buildResponseHeader("text/plain", content);
		return true;
	}

	COM_THROW("Unhandled data request");
	return false;
}

void
Server::addDataRequestHandler(const DataRequestHandler* toAdd)
{
	auto it = std::find(sDataRequestHandlers.begin(), sDataRequestHandlers.end(), toAdd);
	if (it != sDataRequestHandlers.end())
		return;

	sDataRequestHandlers.push_back(toAdd);
}

void
Server::removeDataRequestHandler(const DataRequestHandler* toRemove)
{
	auto it = std::find(sDataRequestHandlers.begin(), sDataRequestHandlers.end(), toRemove);
	if (it == sDataRequestHandlers.end())
		return;

	sDataRequestHandlers.erase(it);
}

bool
TCPSocketResponder::OpenSocket(int port)
{
	m_port = port;
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	COM_ASSERT(m_socket != s_invalidSocket, "Failed to open socket");

	unsigned long val = 10000;
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&val, sizeof(unsigned long));

	return true;
}

bool
TCPSocketResponder::BindSocket()
{
	COM_ASSERT(m_socket != s_invalidSocket, "Socket not openned");

	m_address.sin_family = AF_INET;
	m_address.sin_addr.s_addr = INADDR_ANY;
	m_address.sin_port = htons(m_port);

	memset(m_address.sin_zero, '\0', sizeof m_address.sin_zero);

	if (bind(m_socket, (struct sockaddr*)&m_address, sizeof(m_address)) < 0)
	{
		COM_THROW("Failed to bind socket");
	}

	if (listen(m_socket, 10) < 0)
	{
		COM_THROW("Error listening to socket");
	}

	FD_ZERO(&m_set);
	FD_SET(m_socket, &m_set);

	return true;
}

void
TCPSocketResponder::SetTimeout(timeval to)
{
	m_timeout = to;
}

bool 
TCPSocketResponder::PollSocket(bool& hasRequest)
{
	COM_ASSERT(m_socket != s_invalidSocket, "Socket not openned");

#ifndef WIN32
	static constexpr int flags = (POLLRDNORM | POLLWRNORM | POLLWRBAND | POLLIN | POLLRDBAND | POLLPRI);
	pollfd pfd = { m_socket, flags, 0 };
	int pollResult = poll(&pfd, 1, 1 /*ms*/);
#else
	static constexpr int flags = (POLLIN | POLLOUT);
	pollfd pfd = { m_socket, flags, 0 };
	int pollResult = WSAPoll(&pfd, 1, 1 /*ms*/);
#endif

	// Check we didn't get any socket errors
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


	COM_ASSERT_LAST_ERROR((pollResult >= 0), "Failed to poll socket");
	
	hasRequest = pollResult > 0;
	return true;
}

void
TCPSocketResponder::CreateBuffer(uint32_t size)
{
	m_buffer.resize(size);
	std::memset(m_buffer.data(), 0, size);
}

bool
TCPSocketResponder::AcceptRequest(std::string& requestOut)
{
	COM_ASSERT(m_socket != s_invalidSocket, "Socket not openned");
	COM_ASSERT(m_tmpSocket == s_invalidSocket, "Temp socket already valid");

	int addrlen = sizeof(m_address);
	int new_socket = accept(m_socket, (struct sockaddr*)&m_address, &addrlen);
	COM_ASSERT_LAST_ERROR((new_socket != s_invalidSocket), "Failed to accept socket");

	long valread = recv(new_socket, m_buffer.data(), m_buffer.size(), 0);
	requestOut = m_buffer.data();

	m_tmpSocket = new_socket;
	return true;
}

void
TCPSocketResponder::Respond(const std::string& response)
{
	COM_ASSERT(m_socket != s_invalidSocket, "Socket not openned");

	int result = send(m_tmpSocket, response.c_str(), response.length(), 0);
	COM_ASSERT_LAST_ERROR(result != SOCKET_ERROR, "Failed responding to request");

#ifdef WIN32
	result = closesocket(m_tmpSocket);
	COM_ASSERT_LAST_ERROR(result != SOCKET_ERROR, "Failed closing socket");
#else
	close(m_tmpSocket);
#endif
	m_tmpSocket = s_invalidSocket;
}

void
TCPSocketResponder::CloseSocket()
{
#ifdef WIN32
	int result = closesocket(m_socket);
	COM_ASSERT_LAST_ERROR(result != SOCKET_ERROR, "Failed closing socket");
#else
	close(m_socket);
#endif
	m_socket = -1;
}

bool
Server::handleRequest(TCPSocketResponder& responder)
{
	bool hasRequest;
	if (!responder.PollSocket(hasRequest))
		return false;

	if (!hasRequest)
		return true;

	std::string request;
	responder.AcceptRequest(request);

	std::string requestedObject = getObjectNameFromRequestHeader(request.c_str(), request.length());

	std::string header;
	bool success = false;
	if (isDataRequest(requestedObject))
		success = onDataRequest(requestedObject, header);
	else
		success = onFileRequest(requestedObject, header);

	if (success)
		responder.Respond(header);
	else
		COM_THROW("Could not send data");

	return true;
}

void
Server::runServer(int port)
{
#ifdef WIN32
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	if (WSAStartup(versionWanted, &wsaData))
		COM_THROW_LAST_ERROR("Failed in WSAStartup");
#endif

	TCPSocketResponder responder;
	if (!responder.OpenSocket(port))
		return;

	if (!responder.BindSocket())
		return;

	responder.SetTimeout({ 0 /*s*/, 100 /*ms*/ });
	responder.CreateBuffer(2048);

	while (handleRequest(responder) && sRunServer)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(1000));
	}

	responder.CloseSocket();

	std::cout << "Server exiting" << std::endl;
#ifdef WIN32
	if (WSACleanup())
		COM_THROW_LAST_ERROR("Failed in WSACleanup");
#endif
}

void
Server::initiate(int port, const char* settingsPath)
{
	try
	{
		ServerUtils::SettingsVector settings;
		if (!ServerUtils::readSettings(settings, settingsPath))
			return;

		applySettings(settings);
		//runServer(port);
		sThread = std::thread(runServer, port);
#ifndef WIN32
		ServerUtils::setupQuitHandler();
#endif
	}
	catch (com::Exception* e)
	{
#ifdef WIN32
		MessageBoxA(NULL, e->what(), "Server Error", MB_ICONERROR | MB_OK);
#endif
	}
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
	sRootWebDir = getSetting("WEB_FILES_ROOT", std::filesystem::current_path().string(), settings);
	sSystemFiles = getSetting("SYSTEM_FILES", std::filesystem::current_path().string(), settings);
	sUserScripts = getSetting("USER_SCRIPTS", std::filesystem::current_path().string(), settings);
}

