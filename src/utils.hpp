#include <string>
#include <vector>
#pragma once


namespace Orchestrator {

	enum STATUS {
		UNKNOWN,
		FETCHING,
		PULLING,
		DEPLOYING,
		PENDING
	};

	std::string get_string_from_status(STATUS stat) {
		switch(stat){
			case (UNKNOWN):   return "UNKNOWN";
			case (FETCHING):  return "FETCHING";
			case (DEPLOYING): return "DEPLOYING";
			case (PENDING):   return "PENDING";
			case (PULLING):   return "PULLING";
			default: return "ERORR";
		}
	}
}

std::string format_string(const std::string& format, const std::vector<std::string>& values) {
	std::string result;
	size_t last_pos = 0;

	for (size_t i = 0; i < values.size(); ++i) {
		size_t pos = format.find("%s", last_pos);

		if (pos == std::string::npos) {
			break;
		}

		result += format.substr(last_pos, pos - last_pos);
		result += values[i];

		last_pos = pos + 2;
	}

	result += format.substr(last_pos);

	return result;
}
