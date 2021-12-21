#define ASYNC_SERVER

#include "src/server.h"
#include "src/serverUtils.h"
#include <Common/ArgumentParser.h>
#include <iostream>

struct MyTestHandler : public Server::DataRequestHandler
{
	virtual bool handle(const std::string& requestedData, std::string& contentOut) const override
	{
		contentOut = "This is a test!";
		return true;
	}

	MyTestHandler() : Server::DataRequestHandler("test") {}
};

int main(int argc, char **argv)
{
	parser::ServerArgParser args;
	args.parseArguments(argc, argv);

	MyTestHandler m;

	Server::addDataRequestHandler(&m);

	Server::initiate(args.getPort(), args.getSettingsFile().c_str());
	Server::join();
	Server::kill();
}