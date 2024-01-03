#include "bluetooth.h"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
std::string receivedMessage;

class BleskomatServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer *pServer) {
		deviceConnected = true;
	}

	void onDisconnect(BLEServer *pServer) {
		deviceConnected = false;
	}
};

class BleskomatCharacteristicCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		std::string value = pCharacteristic->getValue();
		if (value.length() > 0) {
			receivedMessage = value;
			logger::write("Received message in Bluetooth characteristic: ");
			logger::write(receivedMessage.c_str());
			jsonRpc::onBleMessage(receivedMessage.c_str());
		}
	}
};

namespace {
	#define SERVICE_UUID        "8771ff1c-4074-4bc0-9468-d9357cd7026b"
	#define CHARACTERISTIC_UUID "af889f94-5be4-438c-a977-499c55f07a7b"

	void sendChunk(BLECharacteristic* characteristic, const std::string& chunk) {
		characteristic->setValue(chunk);  // Set the value of the characteristic with the chunk
		characteristic->notify();         // Notify connected devices of the new value
	}

	void sendLongMessage(BLECharacteristic* characteristic, const std::string& message) {
		// Bluetooth only allow sending 20 bytes at a time.
		// https://stackoverflow.com/questions/38913743/maximum-packet-length-for-bluetooth-le
		// https://stackoverflow.com/questions/24135682/android-sending-data-20-bytes-by-ble/24178409#24178409
		size_t chunkSize = 20;
		size_t chunkDelay = 100;
		size_t messageLength = message.length();
		// Calculate the number of chunks needed to send the message.
		size_t numChunks = (messageLength + chunkSize - 1) / chunkSize;
		for (size_t i = 0; i < numChunks; i++) {
			size_t startIndex = i * chunkSize;
			size_t endIndex = std::min(startIndex + chunkSize, messageLength);
			std::string chunk = message.substr(startIndex, endIndex - startIndex);

			logger::write(std::string("Sending chunk: ") + chunk.c_str(), "debug");
			// Send each chunk of the message with a small delay between each chunk.
			sendChunk(characteristic, chunk);
			delay(chunkDelay);
		}
	}

}

namespace bluetooth {
	void init() {
		BLEDevice::init("Bleskomat");
		logger::write("Bluetooth initialized");
		logger::write(SERVICE_UUID);
		logger::write(CHARACTERISTIC_UUID);
		BLEServer *pServer = BLEDevice::createServer();
		pServer->setCallbacks(new BleskomatServerCallbacks());
		BLEService *pService = pServer->createService(SERVICE_UUID);
		pCharacteristic = pService->createCharacteristic(
			CHARACTERISTIC_UUID,
			BLECharacteristic::PROPERTY_READ |
			BLECharacteristic::PROPERTY_WRITE |
			BLECharacteristic::PROPERTY_NOTIFY |
			BLECharacteristic::PROPERTY_INDICATE
		);
		pCharacteristic->addDescriptor(new BLE2902());
		pCharacteristic->setCallbacks(new BleskomatCharacteristicCallbacks());
		pService->start();
		BLEAdvertising *pAdvertising = pServer->getAdvertising();
		pAdvertising->start();
		logger::write("Bluetooth server started");
	}

	bool isConnected() {
		return deviceConnected;
	}

	bool sendMessage(const std::string &message) {
		logger::write("Sending message over Bluetooth: ");
		logger::write(message.c_str());
		if (deviceConnected) {
			sendLongMessage(pCharacteristic, message.c_str());
			return true;
		}
		return false;
	}

	void loop() {
		// Do nothing.
	}
}