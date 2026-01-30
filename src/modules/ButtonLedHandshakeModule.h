#pragma once

#include "Module.h"
#include <Arduino.h>

class ButtonLedHandshakeModule : public Module {
public:
    explicit ButtonLedHandshakeModule(bool isMaster);

    int32_t runOnce() override;
    bool handleReceived(const meshtastic_MeshPacket &mp) override;

private:
    const bool isMaster;

    static constexpr int PIN_BTN = 4;    // Taste
    static constexpr int PIN_LED = 20;   // LED (wie festgelegt)

    bool lastBtn = true;
    uint32_t lastEdgeMs = 0;

    enum State { IDLE, WAIT_ACK_S_ON, WAIT_ACK_M_OFF };
    State st = IDLE;

    void initPins();
    void setLed(bool on);
    bool buttonPressedEdge();

    void sendTextBroadcast(const String &txt);
    bool getTextPayload(const meshtastic_MeshPacket &mp, String &out) const;
};
