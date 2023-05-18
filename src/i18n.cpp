#include "i18n.h"

namespace {

	const std::map<const char*, const i18n::Locale*, util::MapCharPointerComparator> locales {
		{ "cs", &locale_cs },
		{ "en", &locale_en },
		{ "es", &locale_es }
	};

	const char* defaultLocale = "en";
}

namespace i18n {

	std::string t(const char* key) {
		return t(key, config::getString("locale").c_str());
	}

	std::string t(const char* key, const char* locale) {
		if (locales.count(locale) > 0) {
			const i18n::Locale* strings = locales.at(locale);
			if (strings->count(key) > 0) {
				// Locale has key defined.
				return std::string(strings->at(key));
			} else if (locale != defaultLocale) {
				// Try the default locale.
				return t(key, defaultLocale);
			}
		}
		return std::string(key);
	}

	std::string getSupportedLocales() {
		std::string supportedLocales;
		for (auto const& locale: locales) {
			supportedLocales += std::string(locale.first) + ",";
		}
		if (supportedLocales != "") {
			supportedLocales.pop_back();// remove last comma
		}
		return supportedLocales;
	}
}
