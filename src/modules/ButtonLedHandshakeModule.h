#pragma once

#include <Arduino.h>

// IMPORTANT: this is Meshtastic's module base, NOT RadioLib's Module.h
#include "mesh/ProtobufModule.h"

// Needed for meshtastic_MeshPacket and PortNum values
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "mesh/generated/meshtastic/portnums.pb.h"

class ButtonLedHandshakeModule : public ProtobufModule {
public:
    explicit ButtonLedHandshakeModule(bool isMaster);

    int32_t runOnce() override;
    bool handleReceived(const meshtastic_MeshPacket &mp) override;

private:
    // helpers
    void initPins();
    void setLed(bool on);
    bool buttonPressedEdge();

    void sendTextBroadcast(const String &txt);
    bool getTextPayload(const meshtastic_MeshPacket &mp, String &out) const;

    // state machine
    enum State : uint8_t {
        IDLE = 0,
        WAIT_ACK_S_ON,
        WAIT_ACK_M_OFF
    };

    State st = IDLE;

    // role
    bool isMaster = false;

    // debounce / edge detect
    bool lastBtn = true;            // INPUT_PULLUP default HIGH
    uint32_t lastEdgeMs = 0;
};
