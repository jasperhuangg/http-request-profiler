#include <string>

void error(const char *msg);
std::string parseHost(std::string url);
std::string parsePath(std::string url);
std::string constructMessage(std::string host, std::string path);
std::pair<std::string, int> get(std::string url);