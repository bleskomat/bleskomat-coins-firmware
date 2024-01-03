#include "main.h"

unsigned int buttonDelay;
std::string initializeScreen = "";

void setup() {
	Serial.begin(MONITOR_SPEED);
	spiffs::init();
	config::init();
	logger::init();
	logger::write(firmwareName + ": Firmware version = " + firmwareVersion + ", commit hash = " + firmwareCommitHash);
	logger::write(config::getConfigurationsAsString());
	jsonRpc::init();
	screen::init();
	coinAcceptor::init();
	button::init();
	initializeScreen = cache::getString("lastScreen");
	logger::write("Cache loaded lastScreen: " + initializeScreen);
	buttonDelay = config::getUnsignedInt("buttonDelay");
	if (button::isPressedAtStartup()) {
		logger::write("Button pressed during startup, start bluetooth.");
		bluetooth::init();
	}
}

void resetAccumulatedValues() {
	coinAcceptor::resetAccumulatedValue();
}

float amountShown = 0;
unsigned long tradeCompleteTime = 0;
unsigned long lastScreenSwitchTime = 0;

void writeTradeCompleteLog(const float &amount, const std::string &signedUrl) {
	std::string msg = "Trade completed:\n";
	msg += "  Amount  = " + util::floatToStringWithPrecision(amount, config::getUnsignedShort("fiatPrecision")) + " " + config::getString("fiatCurrency") + "\n";
	msg += "  URL     = " + signedUrl;
	logger::write(msg);
}

void runAppLoop() {
	coinAcceptor::loop();
	button::loop();
	const std::string currentScreen = screen::getCurrentScreen();
	if (currentScreen == "") {
		if (initializeScreen == "insertFiat") {
			const std::string cacheAccumulatedValue = cache::getString("accumulatedValue");
			logger::write("Cache loaded accumulatedValue: " + cacheAccumulatedValue);
			if (cacheAccumulatedValue != "") {
				coinAcceptor::setAccumulatedValue(util::stringToFloat(cacheAccumulatedValue));
				screen::showInsertFiatScreen(util::stringToFloat(cacheAccumulatedValue));
			} else {
				screen::showSplashScreen();
			}
		} else if (initializeScreen == "tradeComplete") {
			const std::string cachedQrcodeData = cache::getString("qrcodeData");
			const std::string cachedAccumulatedValue = cache::getString("accumulatedValue");
			logger::write("Cache loaded qrcodeData: " + cachedQrcodeData);
			logger::write("Cache loaded accumulatedValue: " + cachedAccumulatedValue);
			if (cachedQrcodeData != "" && cachedAccumulatedValue != "") {
				coinAcceptor::inhibit();
				screen::showTradeCompleteScreen(util::stringToFloat(cachedAccumulatedValue), cachedQrcodeData);
			} else {
				screen::showSplashScreen();
			}
		} else {
			screen::showSplashScreen();
		}
		initializeScreen = "";
	}
	float accumulatedValue = 0;
	accumulatedValue += coinAcceptor::getAccumulatedValue();
	if (
		accumulatedValue > 0 &&
		currentScreen != "insertFiat" &&
		currentScreen != "tradeComplete"
	) {
		coinAcceptor::disinhibit();
		screen::showInsertFiatScreen(accumulatedValue);
		amountShown = accumulatedValue;
	}
	if (currentScreen == "splash") {
		if (button::wasPushed()) {
			coinAcceptor::disinhibit();
			screen::showInsertFiatScreen(0);
		}
	} else if (currentScreen == "insertFiat") {
		coinAcceptor::disinhibit();
		if (button::wasPushed()) {
			if (accumulatedValue > 0) {
				// Button pushed while insert fiat screen shown and accumulated value greater than 0.
				// Create a withdraw request and render it as a QR code.
				const std::string signedUrl = util::createSignedLnurlWithdraw(accumulatedValue);
				const std::string encoded = util::lnurlEncode(signedUrl);
				std::string qrcodeData = "";
				// Allows upper or lower case URI schema prefix via a configuration option.
				// Some wallet apps might not support uppercase URI prefixes.
				qrcodeData += config::getString("uriSchemaPrefix");
				// QR codes with only uppercase letters are less complex (easier to scan).
				qrcodeData += util::toUpperCase(encoded);
				screen::showTradeCompleteScreen(accumulatedValue, qrcodeData);
				writeTradeCompleteLog(accumulatedValue, signedUrl);
				coinAcceptor::inhibit();
				tradeCompleteTime = millis();
			} else {
				screen::showSplashScreen();
			}
		} else {
			// Button not pressed.
			// Ensure that the amount shown is correct.
			if (amountShown != accumulatedValue) {
				screen::showInsertFiatScreen(accumulatedValue);
				amountShown = accumulatedValue;
			}
		}
	} else if (currentScreen == "tradeComplete") {
		coinAcceptor::inhibit();
		if (button::wasPushed() && millis() - tradeCompleteTime > buttonDelay) {
			// Button pushed while showing the trade complete screen.
			// Reset accumulated values.
			resetAccumulatedValues();
			coinAcceptor::disinhibit();
			screen::showSplashScreen();
			amountShown = 0;
		}
	}
}

void loop() {
	logger::loop();
	jsonRpc::loop();
	if (!jsonRpc::hasPinConflict() || !jsonRpc::inUse()) {
		runAppLoop();
	}
}
