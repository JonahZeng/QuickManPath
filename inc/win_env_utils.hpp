#ifndef _GET_PATH_ENV_
#define _GET_PATH_ENV_
#include <vector>
#include <string>
#include "path_tabel.hpp"

std::vector<EnvPathItem_t> getSystemPath();
std::vector<EnvPathItem_t> getUserPath();
bool setSystemPath(const std::vector<EnvPathItem_t>& systemPaths);
bool setUserPath(const std::vector<EnvPathItem_t>& userPaths);
int getTitleBarHeight();

#endif