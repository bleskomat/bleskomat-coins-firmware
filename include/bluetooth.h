#ifndef BLESKOMAT_BLE_H
#define BLESKOMAT_BLE_H

#include "logger.h"
#include "json-rpc.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

namespace bluetooth {
    void init();
    void loop();
    bool isConnected();
    bool sendMessage(const std::string &message);
}

#endif
