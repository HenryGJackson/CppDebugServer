#pragma once
#include <string>
#include <vector>

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

	static bool readSettings(SettingsVector& settingsOut);

#ifndef WIN32
	static void quitHandler(int signal);

	static void setupQuitHandler();
#endif
};
