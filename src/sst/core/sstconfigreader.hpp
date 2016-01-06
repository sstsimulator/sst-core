
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace SST {
namespace Core {

static void configReadLine(FILE* config, char* buffer, const size_t bufferLen) {
	// Empty the string
	for(size_t i = 0; i < bufferLen; i++) {
		buffer[i] = '\0';
	}

	// Start at index zero and move out from there
	int bufferIndex = 0;

	while( ! feof(config) ) {
		buffer[bufferIndex] = (char) fgetc(config);

		if( buffer[bufferIndex] == '\n' || buffer[bufferIndex] == '\r') {
			buffer[bufferIndex] = '\0';
			break;
		}

		bufferIndex++;
	}
}

static void populateConfigMap(FILE* conf_file,
	std::map<std::string, std::string>& confMap) {

	const size_t bufferLength = 4096;

	char* lineBuffer    = (char*) malloc(sizeof(char) * bufferLength);
	char* variableName  = (char*) malloc(sizeof(char) * bufferLength);
	char* variableValue = (char*) malloc(sizeof(char) * bufferLength);

	while( ! feof(conf_file) ) {
		configReadLine(conf_file, lineBuffer, bufferLength);

		variableName[0] = '\0';
		variableValue[0] = '\0';

		if(strlen(lineBuffer) == 0) {
			// Do nothing and move on
		} else if(lineBuffer[0] == '#') {
			// Comment line, do nothing move on
		} else if(lineBuffer[0] == '[') {
			// Section line, do nothing move on
		} else {
			const int lineLength = strlen(lineBuffer);
			bool foundAssign = false;

			for(int i = 0; i < lineLength; i++) {
				if(lineBuffer[i] == '=') {
					foundAssign = true;
					break;
				}
			}

			int variableIndex = 0;

			if(foundAssign) {
				foundAssign = false;

				for(int i = 0; i < lineLength; i++) {
					if(lineBuffer[variableIndex] != '\n' &&
						lineBuffer[variableIndex] != '\r') {

						if(foundAssign) {
							variableValue[variableIndex] = lineBuffer[i];
							variableIndex++;
						} else {
							if(lineBuffer[i] == '=') {
								variableName[variableIndex] = '\0';
								variableIndex = 0;
								foundAssign = true;
							} else {
								variableName[variableIndex] = lineBuffer[i];
								variableIndex++;
							}
						}
					}
				}

				variableValue[variableIndex] = '\0';

				std::string varNameStr(variableName);
				std::string varValueStr(variableValue);

				if(confMap.find(varNameStr) != confMap.end()) {
					confMap.erase(varNameStr);
				}

				confMap.insert( std::pair<std::string, std::string>(varNameStr, varValueStr) );
			} else {
				// No = in the line, so don't process it
			}
		}
	}

	free(lineBuffer);
}

}
}
