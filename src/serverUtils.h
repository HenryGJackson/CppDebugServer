#pragma once
#include <string>
#include <vector>
#include <Common/ArgumentParser.h>

struct ServerUtils
{
	template <typename T>
	static std::string toString(T* data, uint32_t elementCount)
	{
		std::string result = "";
		for (uint32_t i = 0; i < elementCount; ++i)
		{
			result += std::to_string(data[i]);
			if (i < elementCount - 1)
				result += ",";
		}
		return result;
	}

	template <typename T>
	static std::string toString(T data)
	{
		return std::string(data);
	}

	typedef std::pair<std::string, std::string> SettingValuePair;
	typedef std::vector<std::pair<std::string, std::string>> SettingsVector;

	static bool readSettings(SettingsVector& settingsOut, const char* path);

#ifndef WIN32
	static void quitHandler(int signal);

	static void setupQuitHandler();
#endif
};

namespace parser
{
	enum ServerArguments : unsigned int
	{
		Argument_SettingsPath = 0,
		Argument_Port,
		Argument_Count
	};

	struct ServerArgParser : public ArgumentsParser<ServerArguments>
	{
		int getPort() const;
		const std::string& getSettingsFile() const;
	};

	const std::string ServerArgParser::s_argNames[ServerArguments::Argument_Count][ArgForm::ArgForm_Count] =
	{
		{"-settings_path", "-s"}
		, {"-port", "-p"}
	};
};
