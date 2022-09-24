// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>

class WebApiStaticClass {
public:
    void init(AsyncWebServer* server);
    void loop();

    AsyncWebServer* _server;
};