#include "modules/ButtonLedHandshakeModule.h"

#include <Arduino.h>
#include <algorithm>
#include <cstring>

// Falls in deinem Tree bereits ein Broadcast-Konstant existiert, ist das ok.
// (Viele Trees nutzen 0xFFFFFFFF als Broadcast NodeNum)
#ifndef NODENUM_BROADCAST
#define NODENUM_BROADCAST 0xFFFFFFFFu
#endif

ButtonLedHandshakeModule::ButtonLedHandshakeModule(bool isMaster_)
    : SinglePortModule("ButtonLedHandshake", meshtastic_PortNum_TEXT_MESSAGE_APP), isMaster(isMaster_)
{
    initPins();
}

void ButtonLedHandshakeModule::initPins()
{
    pinMode(PIN_BTN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    lastBtn = digitalRead(PIN_BTN);
    lastEdgeMs = millis();
}

void ButtonLedHandshakeModule::setLed(bool on)
{
    digitalWrite(PIN_LED, on ? HIGH : LOW);
}

bool ButtonLedHandshakeModule::buttonPressedEdge()
{
    // INPUT_PULLUP: ungedrückt=HIGH, gedrückt=LOW
    const uint32_t DEBOUNCE_MS = 30;
    bool cur = digitalRead(PIN_BTN);
    uint32_t now = millis();

    if (cur != lastBtn && (now - lastEdgeMs) > DEBOUNCE_MS) {
        lastEdgeMs = now;
        bool pressed = (lastBtn == HIGH && cur == LOW); // HIGH -> LOW
        lastBtn = cur;
        return pressed;
    }
    return false;
}

void ButtonLedHandshakeModule::sendTextBroadcast(const String &txt)
{
    meshtastic_MeshPacket *p = allocDataPacket();
    if (!p)
        return;

    p->to = NODENUM_BROADCAST;
    p->want_ack = false;
    p->channel = 0;

    // allocDataPacket() setzt i.d.R. decoded bereits korrekt auf.
    // Aber wir setzen PortNum sicherheitshalber:
    p->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;

    auto &pl = p->decoded.payload;
    pl.size = std::min((size_t)txt.length(), sizeof(pl.bytes));
    memcpy(pl.bytes, txt.c_str(), pl.size);

    sendPacket(p);
}

bool ButtonLedHandshakeModule::getTextPayload(const meshtastic_MeshPacket &mp, String &out) const
{
    // MeshPacket hat oneof payload_variant: decoded / encrypted
    if (mp.which_payload_variant != meshtastic_MeshPacket_decoded_tag)
        return false;

    if (mp.decoded.portnum != meshtastic_PortNum_TEXT_MESSAGE_APP)
        return false;

    if (mp.decoded.payload.size == 0)
        return false;

    out = String((const char *)mp.decoded.payload.bytes, mp.decoded.payload.size);
    out.trim();
    return true;
}

bool ButtonLedHandshakeModule::handleReceived(const meshtastic_MeshPacket &mp)
{
    String txt;
    if (!getTextPayload(mp, txt))
        return false;

    // MASTER-Logik:
    // - bekommt ACK_S_ON => Master LED AN
    // - bekommt LED_M_OFF => Master LED AUS + ACK_M_OFF senden
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
        return false;
    }

    // SLAVE-Logik:
    // - bekommt LED_S_ON => Slave LED AN + ACK_S_ON senden
    // - bekommt ACK_M_OFF => Slave LED AUS
    if (txt == "LED_S_ON") {
        setLed(true);
        sendTextBroadcast("ACK_S_ON");
        return true;
    }
    if (txt == "ACK_M_OFF") {
        setLed(false);
        st = IDLE;
        return true;
    }

    return false;
}

int32_t ButtonLedHandshakeModule::runOnce()
{
    if (buttonPressedEdge()) {
        if (isMaster) {
            // Master Button => Slave LED AN
            sendTextBroadcast("LED_S_ON");
            st = WAIT_ACK_S_ON;
        } else {
            // Slave Button => Master LED AUS
            sendTextBroadcast("LED_M_OFF");
            st = WAIT_ACK_M_OFF;
        }
    }

    return 200; // alle 200ms
}
