#ifndef BLESKOMAT_CACHE_H
#define BLESKOMAT_CACHE_H

#include <Preferences.h>
#include <string>
#include "util.h"

namespace cache {
	std::string getString(const char* key);
	void save(const char* key, const std::string &value);
	void save(
		const char* key1, const std::string &value1,
		const char* key2, const std::string &value2
	);
	void save(
		const char* key1, const std::string &value1,
		const char* key2, const std::string &value2,
		const char* key3, const std::string &value3
	);
	void remove(const char* key);
	void remove(const char* key1, const char* key2);
}

#endif
