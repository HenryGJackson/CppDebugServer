#include "src/server.h"
#include "src/serverUtils.h"
#include <Common/ArgumentParser.h>
#include <iostream>

bool handleRequest(const std::string& requestedData, std::string& contentOut)
{
	contentOut = "This is a test!";
	return true;
}

struct MyStruct : public Server::DataRequestHandler
{
	virtual bool handle(const std::string& requestedData, std::string& contentOut) const override
	{
		contentOut = "This is a test!";
		return true;
	}

	MyStruct() : Server::DataRequestHandler("test") {}
};

int main(int argc, char **argv)
{
	parser::ServerArgParser args;
	args.parseArguments(argc, argv);

	MyStruct m;

	Server::addDataRequestHandler(&m);

	Server::initiate(args.getPort(), args.getSettingsFile().c_str());
	Server::join();
	Server::kill();
	/*RUN_DEBUG_SERVER(8088);
	JOIN_DEBUG_SERVER;*/
}