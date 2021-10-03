// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
#include "ArduinoJson.h"
#include "globalConfig.h"
#include "logger.h"
#include "mbComm.h"
#include "powerfox.h"
#include "webSocket.h"
#include <WebSocketsServer.h>

const uint8_t m = 10;

#define CYCLE_TIME	 1000	
#define JSON_LEN      256

WebSocketsServer webSocket = WebSocketsServer(81);
static uint32_t lastHandleCall       = 0;


void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
	if(type == WStype_TEXT) {
		LOG(m, "Payload %s", (char *)payload)
		if (!strncmp((char *)payload, "currLim", 7)) {
			char * pch;
			pch = strtok((char *)payload, "=");
			pch = strtok(NULL, "=");
			mb_writeReg(0, REG_CURR_LIMIT, atoi(pch));
#ifndef WEMOS_D1
		} else if (strstr_P((char *)payload, PSTR("PV_OFF"))) {
			pf_setMode(PV_OFF);
		} else if (strstr_P((char *)payload, PSTR("PV_ACTIVE"))) {
			pf_setMode(PV_ACTIVE);
		} else if (strstr_P((char *)payload, PSTR("PV_MIN_PV"))) {
			pf_setMode(PV_MIN_PV);
#endif /* WEMOS_D1 */
		}
	} 
}


void webSocket_begin() {
	// start the WebSocket connection
	webSocket.begin();
	webSocket.onEvent(webSocketEvent);
}


void webSocket_handle() {
	webSocket.loop();
	if ((millis() - lastHandleCall < CYCLE_TIME)) {
		return;
	}
	lastHandleCall = millis();

	StaticJsonDocument<JSON_LEN> data;
	uint8_t id = 0;
	data[F("chgStat")]  = content[id][1];
	data[F("power")]    = content[id][10];
	data[F("energyI")]  = (float)((uint32_t) content[id][13] << 16 | (uint32_t)content[id][14]) / 1000.0;
	data[F("currLim")]  = (float)content[id][53]/10.0;
#ifndef WEMOS_D1
	data[F("watt")]     = pf_getWatt();
	data[F("pvMode")]   = pf_getMode();
#endif /* WEMOS_D1 */
	data[F("timeNow")]   = log_time();
	char response[JSON_LEN];
	serializeJson(data, response, JSON_LEN);
	webSocket.broadcastTXT(response);
}
