#include "screen.h"

namespace {
	std::string currentScreen = "";

	void setCurrentScreen(const std::string t_currentScreen) {
		currentScreen = t_currentScreen;
		cache::save("lastScreen", t_currentScreen);
	}
}

namespace screen {

	void init() {
		screen_tft::init();
	}

	std::string getCurrentScreen() {
		return currentScreen;
	}

	void showSplashScreen() {
		screen_tft::showSplashScreen();
		setCurrentScreen("splash");
		cache::save("accumulatedValue", "");
	}

	void showInsertFiatScreen(const float &amount) {
		screen_tft::showInsertFiatScreen(amount);
		setCurrentScreen("insertFiat");
		cache::save("accumulatedValue", util::floatToStringWithPrecision(amount, config::getUnsignedShort("fiatPrecision")));
	}

	void showTradeCompleteScreen(const float &amount, const std::string &qrcodeData) {
		screen_tft::showTradeCompleteScreen(amount, qrcodeData);
		setCurrentScreen("tradeComplete");
		cache::save("accumulatedValue", util::floatToStringWithPrecision(amount, config::getUnsignedShort("fiatPrecision")), "qrcodeData", qrcodeData);
	}
}
