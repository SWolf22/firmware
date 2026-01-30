#pragma once

#include <Arduino.h>

// Meshtastic core
#include "mesh/ProtobufModule.h"
#include "mesh/generated/meshtastic/mesh.pb.h"

class ButtonLedHandshakeModule : public ProtobufModule {
public:
    explicit ButtonLedHandshakeModule(bool isMaster);

    // Called periodically by Meshtastic scheduler
    int32_t runOnce() override;

    // Called when a packet is received
    bool handleReceived(const meshtastic_MeshPacket &mp) override;

private:
    bool isMaster;

    void sendTextBroadcast(const String &text);
    bool getTextPayload(const meshtastic_MeshPacket &mp, String &out) const;
};
