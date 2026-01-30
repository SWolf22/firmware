#include "ButtonLedHandshakeModule.h"

#include <algorithm>

ButtonLedHandshakeModule::ButtonLedHandshakeModule(bool isMaster_)
    : ProtobufModule("ButtonLedHandshake"), isMaster(isMaster_) {
    initPins();
}

void ButtonLedHandshakeModule::initPins() {
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
}

void ButtonLedHandshakeModule::setLed(bool on) {
    digitalWrite(PIN_LED, on ? HIGH : LOW);
}

bool ButtonLedHandshakeModule::buttonPressedEdge() {
    const uint32_t DEBOUNCE_MS = 30;
    bool cur = digitalRead(PIN_BTN);
    uint32_t now = millis();

    if (cur != lastBtn && (now - lastEdgeMs) > DEBOUNCE_MS) {
        lastEdgeMs = now;
        bool pressed = (lastBtn && !cur); // HIGH -> LOW (Pullup -> gedrückt)
        lastBtn = cur;
        return pressed;
    }
    return false;
}

void ButtonLedHandshakeModule::sendTextBroadcast(const String &txt) {
    // allocate a DATA packet using Meshtastic helper (avoids Router API differences)
    meshtastic_MeshPacket *p = allocDataPacket();
    if (!p)
        return;

    p->to = 0xFFFFFFFF; // Broadcast
    p->want_ack = false;
    p->channel = 0;

    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    auto &d = p->decoded;
    d.payload.size = std::min((size_t)txt.length(), sizeof(d.payload.bytes) - 1);
    memcpy(d.payload.bytes, txt.c_str(), d.payload.size);

    // send using ProtobufModule API (stable across branches)
    sendPacket(p);
}

bool ButtonLedHandshakeModule::getTextPayload(const meshtastic_MeshPacket &mp, String &out) const {
    if (!mp.has_decoded)
        return false;
    if (mp.decoded.portnum != meshtastic_PortNum_TEXT_MESSAGE_APP)
        return false;
    if (mp.decoded.payload.size == 0)
        return false;

    out = String((const char *)mp.decoded.payload.bytes, mp.decoded.payload.size);
    out.trim();
    return true;
}

bool ButtonLedHandshakeModule::handleReceived(const meshtastic_MeshPacket &mp) {
    String txt;
    if (!getTextPayload(mp, txt))
        return false;

    if (isMaster) {
        if (st == WAIT_ACK_S_ON && txt == "ACK_S_ON") {
            setLed(true);
            st = IDLE;
            return true;
        }
        if (txt == "LED_M_OFF") {
            setLed(false);
            sendTextBroadcast("ACK_M_OFF");
            return true;
        }
    } else {
        if (txt == "LED_S_ON") {
            setLed(true);
            sendTextBroadcast("ACK_S_ON");
            return true;
        }
        if (txt == "ACK_M_OFF") {
            setLed(false);
            return true;
        }
    }

    return false;
}

int32_t ButtonLedHandshakeModule::runOnce() {
    if (buttonPressedEdge()) {
        if (isMaster) {
            sendTextBroadcast("LED_S_ON");
            st = WAIT_ACK_S_ON;
        } else {
            sendTextBroadcast("LED_M_OFF");
            st = WAIT_ACK_M_OFF;
        }
    }
    return 200; // run every 200ms
}
