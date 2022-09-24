// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "WebApi_static.h"
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "Hoymiles.h"
#include <ctime>
#include <LittleFS.h>

void WebApiStaticClass::init(AsyncWebServer* server)
{
    using std::placeholders::_1;

    _server = server;

    _server->serveStatic("/config.json", LittleFS, "/config.json");
    _server->serveStatic("/backup", LittleFS, "/config.json");
    _server->serveStatic("/factory", LittleFS, "/factory.json");
}

void WebApiStaticClass::loop()
{
}