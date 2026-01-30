#pragma once

#include <Arduino.h>

// Wichtig: NICHT "Module.h" (RadioLib-Falle!), sondern Meshtastic ProtobufModule:
#include "mesh/ProtobufModule.h"
#include "mesh/generated/meshtastic/mesh.pb.h"
#include "mesh/generated/meshtastic/portnums.pb.h"

#ifndef HLH_PIN_BTN
// Vorschlag: Button an GPIO21 (gegen GND), interner Pullup
#define HLH_PIN_BTN 21
#endif

#ifndef HLH_PIN_LED
// Du hattest dich schon für GPIO20 entschieden
#define HLH_PIN_LED 20
#endif

class ButtonLedHandshakeModule : public ProtobufModule {
public:
    explicit ButtonLedHandshakeModule(bool isMaster);

    int32_t runOnce() override;
    bool handleReceived(const meshtastic_MeshPacket &mp) override;

private:
    enum State : uint8_t { IDLE, WAIT_ACK_S_ON, WAIT_ACK_M_OFF };

    const bool isMaster;
    State st = IDLE;

    bool lastBtn = true;          // INPUT_PULLUP: true = nicht gedrückt
    uint32_t lastEdgeMs = 0;

    void initPins();
    void setLed(bool on);
    bool buttonPressedEdge();

    void sendTextBroadcast(const String &txt);
    bool getTextPayload(const meshtastic_MeshPacket &mp, String &out) const;
};
