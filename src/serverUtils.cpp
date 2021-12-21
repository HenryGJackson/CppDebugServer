#include <filesystem>
#include <fstream>

#include "serverUtils.h"
#include "server.h"

bool ServerUtils::readSettings(SettingsVector& settingsOut, const char* pathCStr)
{
	std::string path(pathCStr);
#ifdef TEST_CONFIG
	path.concat("\\..\\..\\server_config.cfg");
#else
	path += "\\server_config.cfg";
#endif
	std::fstream file;
	file.open(path, std::fstream::in);
	if (!file.is_open())
	{
		perror("Failed to open settings file. Continuing with defaults. Things may not work..");
		return false;
	}

	std::string line;
	while (std::getline(file, line))
	{
		std::pair<std::string, std::string> settingValuePair;
		size_t space = line.find(" ");
		std::string setting = line.substr(0, space);
		std::string value = line.substr(space + 1);
		if (value == "{")
		{
			value = "";
			unsigned short count = 0;
			while (std::getline(file, line) && line.find('}') == std::string::npos)
			{
				if (count++ > 0)
					value += ",";

				// Remove spaces or tabs
				line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return c == ' ' || c == '\t'; }));
				value += line;
			}
		}

		settingsOut.push_back(SettingValuePair(setting, value));
	}

	file.close();
	return true;
}


#ifndef WIN32
void ServerUtils::quitHandler(int signal)
{
	KILL_DEBUG_SERVER;
	std::cout << "signal: " << signal << std::endl;
	exit(0);
}

void ServerUtils::setupQuitHandler()
{
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = quitHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);
}
#endif

namespace parser
{
	int ServerArgParser::getPort() const
	{
		const std::string& portString = getArgValue(ServerArguments::Argument_Port);

		if (!portString.empty())
			return std::stoi(portString);

		return 8080;
	}

	const std::string& ServerArgParser::getSettingsFile() const
	{
		return argValues[ServerArguments::Argument_SettingsPath];
	}
}