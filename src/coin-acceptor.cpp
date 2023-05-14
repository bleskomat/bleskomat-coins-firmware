#include "coin-acceptor.h"

namespace {
	bool inhibited = false;
}

namespace coinAcceptor {

	void init() {
		coinAcceptor_dg600f::init();
	}

	void loop() {
		coinAcceptor_dg600f::loop();
	}

	float getAccumulatedValue() {
		return coinAcceptor_dg600f::getAccumulatedValue();
	}

	void setAccumulatedValue(float value) {
		coinAcceptor_dg600f::setAccumulatedValue(value);
	}

	void resetAccumulatedValue() {
		logger::write("Resetting coin acceptor accumulated value");
		coinAcceptor_dg600f::resetAccumulatedValue();
	}

	bool isInhibited() {
		return inhibited;
	}

	void inhibit() {
		logger::write("Inhibiting coin acceptor");
		coinAcceptor_dg600f::inhibit();
		inhibited = true;
	}

	void disinhibit() {
		logger::write("Disinhibiting coin acceptor");
		coinAcceptor_dg600f::disinhibit();
		inhibited = false;
	}
}
