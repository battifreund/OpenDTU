// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "Configuration.h"
#include "defaults.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

CONFIG_T config;

void ConfigurationClass::init()
{
    memset(&config, 0x0, sizeof(config));
    config.Cfg_SaveCount = 0;
    config.Cfg_Version = CONFIG_VERSION;

    // WiFi Settings
    strlcpy(config.WiFi_Ssid, WIFI_SSID, sizeof(config.WiFi_Ssid));
    strlcpy(config.WiFi_Password, WIFI_PASSWORD, sizeof(config.WiFi_Password));
    config.WiFi_Dhcp = WIFI_DHCP;
    strlcpy(config.WiFi_Hostname, APP_HOSTNAME, sizeof(config.WiFi_Hostname));

    // NTP Settings
    strlcpy(config.Ntp_Server, NTP_SERVER, sizeof(config.Ntp_Server));
    strlcpy(config.Ntp_Timezone, NTP_TIMEZONE, sizeof(config.Ntp_Timezone));
    strlcpy(config.Ntp_TimezoneDescr, NTP_TIMEZONEDESCR, sizeof(config.Ntp_TimezoneDescr));

    // MqTT Settings
    config.Mqtt_Enabled = MQTT_ENABLED;
    strlcpy(config.Mqtt_Hostname, MQTT_HOST, sizeof(config.Mqtt_Hostname));
    config.Mqtt_Port = MQTT_PORT;
    strlcpy(config.Mqtt_Username, MQTT_USER, sizeof(config.Mqtt_Username));
    strlcpy(config.Mqtt_Password, MQTT_PASSWORD, sizeof(config.Mqtt_Password));
    strlcpy(config.Mqtt_Topic, MQTT_TOPIC, sizeof(config.Mqtt_Topic));
    config.Mqtt_Retain = MQTT_RETAIN;
    config.Mqtt_Tls = MQTT_TLS;
    strlcpy(config.Mqtt_RootCaCert, MQTT_ROOT_CA_CERT, sizeof(config.Mqtt_RootCaCert));
    strlcpy(config.Mqtt_LwtTopic, MQTT_LWT_TOPIC, sizeof(config.Mqtt_LwtTopic));
    strlcpy(config.Mqtt_LwtValue_Online, MQTT_LWT_ONLINE, sizeof(config.Mqtt_LwtValue_Online));
    strlcpy(config.Mqtt_LwtValue_Offline, MQTT_LWT_OFFLINE, sizeof(config.Mqtt_LwtValue_Offline));
    config.Mqtt_PublishInterval = MQTT_PUBLISH_INTERVAL;

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        config.Inverter[i].Serial = 0;
        strlcpy(config.Inverter[i].Name, "", 0);
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            config.Inverter[0].MaxChannelPower[c] = 0;
        }
    }

    config.Dtu_Serial = DTU_SERIAL;
    config.Dtu_PollInterval = DTU_POLL_INTERVAL;
    config.Dtu_PaLevel = DTU_PA_LEVEL;

    config.Mqtt_Hass_Enabled = MQTT_HASS_ENABLED;
    config.Mqtt_Hass_Expire = MQTT_HASS_EXPIRE;
    config.Mqtt_Hass_Retain = MQTT_HASS_RETAIN;
    strlcpy(config.Mqtt_Hass_Topic, MQTT_HASS_TOPIC, sizeof(config.Mqtt_Hass_Topic));
    config.Mqtt_Hass_IndividualPanels = MQTT_HASS_INDIVIDUALPANELS;
}

bool ConfigurationClass::write()
{
    File f = LittleFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg_SaveCount++;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&config);
    for (unsigned int i = 0; i < sizeof(CONFIG_T); i++) {
        f.write(bytes[i]);
    }
    f.close();

    JSONwrite();

    return true;
}

bool ConfigurationClass::read()
{
    // if( LittleFS.exists(CONFIG_JSON_FILENAME) ) {
    //     JSONread();
    // } else {
        File f = LittleFS.open(CONFIG_FILENAME, "r");
        if (!f) {
            return false;
        }
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&config);
        for (unsigned int i = 0; i < sizeof(CONFIG_T); i++) {
            bytes[i] = f.read();
        }
        f.close();
    // }
    return true;
}

void ipFromJson(const char* src, byte ip[])
{
    IPAddress addr;
    addr.fromString(src);
    ip[0] = addr[0];
    ip[1] = addr[1];
    ip[2] = addr[2];
    ip[3] = addr[3];

    Serial.printf(" %s IP: %d.%d.%d.%d\n", src, ip[0], ip[1], ip[2], ip[3]);
}

uint64_t serialFromJson(const char* src)
{
    return strtoll(src, NULL, 16);
}

void serialToJson(const uint64_t& serial, JsonVariant dst)
{
    char buffer[sizeof(uint64_t) * 8 + 1];
    snprintf(buffer, sizeof(buffer), "%0lx%08lx",
        ((uint32_t)((serial >> 32) & 0xFFFFFFFF)),
        ((uint32_t)(serial & 0xFFFFFFFF)));
    dst.set(buffer);

    Serial.printf("convertToJson(uint64_t, JsonVariant) : %s \n", buffer);
}

DynamicJsonDocument ConfigurationClass::JSONserialize()
{
    DynamicJsonDocument doc(6144);

    doc[F("version")] = config.Cfg_Version;
    doc[F("save_count")] = config.Cfg_SaveCount;

    JsonObject dtu = doc.createNestedObject("dtu");
    dtu[F("dtu_serial")] = "";
    serialToJson(config.Dtu_Serial, dtu[F("dtu_serial")]);
    dtu[F("dtu_pollinterval")] = config.Dtu_PollInterval;
    dtu[F("dtu_palevel")] = config.Dtu_PaLevel;

    JsonObject network = doc.createNestedObject("network");
    network[F("hostname")] = config.WiFi_Hostname;
    network[F("dhcp")] = config.WiFi_Dhcp;
    network[F("ipaddress")] = IPAddress(config.WiFi_Ip);
    network[F("netmask")] = IPAddress(config.WiFi_Netmask);
    network[F("gateway")] = IPAddress(config.WiFi_Gateway);
    network[F("dns1")] = IPAddress(config.WiFi_Dns1);
    network[F("dns2")] = IPAddress(config.WiFi_Dns2);
    network[F("ssid")] = config.WiFi_Ssid;
    network[F("password")] = config.WiFi_Password;

    JsonObject ntp = doc.createNestedObject("ntp");
    ntp[F("ntp_server")] = config.Ntp_Server;
    ntp[F("ntp_timezone")] = config.Ntp_Timezone;
    ntp[F("ntp_timezone_descr")] = config.Ntp_TimezoneDescr;

    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt[F("mqtt_enabled")] = config.Mqtt_Enabled;
    mqtt[F("mqtt_hostname")] = config.Mqtt_Hostname;
    mqtt[F("mqtt_port")] = config.Mqtt_Port;
    mqtt[F("mqtt_username")] = config.Mqtt_Username;
    mqtt[F("mqtt_password")] = config.Mqtt_Password;
    mqtt[F("mqtt_topic")] = config.Mqtt_Topic;
    mqtt[F("mqtt_retain")] = config.Mqtt_Retain;
    mqtt[F("mqtt_tls")] = config.Mqtt_Tls;
    mqtt[F("mqtt_root_ca_cert")] = config.Mqtt_RootCaCert;
    mqtt[F("mqtt_lwt_topic")] = config.Mqtt_LwtTopic;
    mqtt[F("mqtt_lwt_online")] = config.Mqtt_LwtValue_Online;
    mqtt[F("mqtt_lwt_offline")] = config.Mqtt_LwtValue_Offline;
    mqtt[F("mqtt_publish_interval")] = config.Mqtt_PublishInterval;
    mqtt[F("mqtt_hass_enabled")] = config.Mqtt_Hass_Enabled;
    mqtt[F("mqtt_hass_expire")] = config.Mqtt_Hass_Expire;
    mqtt[F("mqtt_hass_retain")] = config.Mqtt_Hass_Retain;
    mqtt[F("mqtt_hass_topic")] = config.Mqtt_Hass_Topic;
    mqtt[F("mqtt_hass_individualpanels")] = config.Mqtt_Hass_IndividualPanels;

    JsonArray inverters = doc.createNestedArray("inverter");
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial > 0) {
            JsonObject inverter = inverters.createNestedObject();

            inverter[F("serial")] = "";
            serialToJson(config.Inverter[i].Serial, inverter[F("serial")]);
            inverter[F("name")] = config.Inverter[i].Name;

            for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
                inverter[F("max_power")][c] = config.Inverter[i].MaxChannelPower[c];
            }
        }
    }
   
    return doc;
}

bool ConfigurationClass::JSONserialize(Stream &output)
{
    return serializeJsonPretty(JSONserialize(), output) > 0;
}

bool ConfigurationClass::JSONwrite(const char* filename)
{
    File f = LittleFS.open(filename, "w");
    if (!f) {
        return false;
    }
    config.Cfg_SaveCount++;

    JSONserialize(f);

    f.close();
    return true;
}

bool ConfigurationClass::JSONdeserialize(Stream &input)
{
    DynamicJsonDocument doc(6144);
    bool res = deserializeJson(doc, input);

    memset(&config, 0x0, sizeof(config));

    config.Cfg_Version = doc[F("version")];
    config.Cfg_SaveCount = doc[F("save_count")];

    config.Dtu_Serial = serialFromJson(doc[F("dtu")][F("dtu_serial")]);
    config.Dtu_PollInterval = doc[F("dtu")][F("dtu_pollinterval")];
    config.Dtu_PaLevel = doc[F("dtu")][F("dtu_palevel")];

    strlcpy(config.WiFi_Hostname, doc[F("network")][F("hostname")], sizeof(config.WiFi_Hostname));
    
    config.WiFi_Dhcp = doc[F("network")][F("dhcp")];
    ipFromJson(doc[F("network")][F("ipaddress")], config.WiFi_Ip);
    ipFromJson(doc[F("network")][F("netmask")], config.WiFi_Netmask);
    ipFromJson(doc[F("network")][F("gateway")],config.WiFi_Gateway);
    ipFromJson(doc[F("network")][F("dns1")], config.WiFi_Dns1);
    ipFromJson(doc[F("network")][F("dns2")], config.WiFi_Dns2);
    
    strlcpy(config.WiFi_Ssid, doc[F("network")][F("ssid")], sizeof(config.WiFi_Ssid));
    strlcpy(config.WiFi_Password, doc[F("network")][F("password")], sizeof(config.WiFi_Password));

    strlcpy(config.Ntp_Server, doc[F("ntp")][F("ntp_server")], sizeof(config.Ntp_Server));
    strlcpy(config.Ntp_Timezone, doc[F("ntp")][F("ntp_timezone")], sizeof(config.Ntp_Timezone) );
    strlcpy(config.Ntp_TimezoneDescr, doc[F("ntp")][F("ntp_timezone_descr")], sizeof(config.Ntp_TimezoneDescr));

    config.Mqtt_Enabled = doc[F("mqtt")][F("mqtt_enabled")];
    strlcpy(config.Mqtt_Hostname, doc[F("mqtt")][F("mqtt_hostname")], sizeof(config.Mqtt_Hostname));
    config.Mqtt_Port = doc[F("mqtt")][F("mqtt_port")];
    strlcpy(config.Mqtt_Username, doc[F("mqtt")][F("mqtt_username")], sizeof(config.Mqtt_Username));
    strlcpy(config.Mqtt_Password, doc[F("mqtt")][F("mqtt_password")], sizeof(config.Mqtt_Password));
    strlcpy(config.Mqtt_Topic, doc[F("mqtt")][F("mqtt_topic")], sizeof(config.Mqtt_Topic));
    
    config.Mqtt_Retain = doc[F("mqtt")][F("mqtt_retain")];
    config.Mqtt_Tls = doc[F("mqtt")][F("mqtt_tls")];
    
    strlcpy(config.Mqtt_RootCaCert, doc[F("mqtt")][F("mqtt_root_ca_cert")], sizeof(config.Mqtt_RootCaCert));
    
    strlcpy(config.Mqtt_LwtTopic, doc[F("mqtt")][F("mqtt_lwt_topic")], sizeof(config.Mqtt_LwtTopic));
    strlcpy(config.Mqtt_LwtValue_Online, doc[F("mqtt")][F("mqtt_lwt_online")], sizeof(config.Mqtt_LwtValue_Online));
    strlcpy(config.Mqtt_LwtValue_Offline, doc[F("mqtt")][F("mqtt_lwt_offline")], sizeof(config.Mqtt_LwtValue_Offline));
    
    config.Mqtt_PublishInterval = doc[F("mqtt")][F("mqtt_publish_interval")];
    config.Mqtt_Hass_Enabled = doc[F("mqtt")][F("mqtt_hass_enabled")];
    config.Mqtt_Hass_Expire = doc[F("mqtt")][F("mqtt_hass_expire")];
    config.Mqtt_Hass_Retain = doc[F("mqtt")][F("mqtt_hass_retain")];
    strlcpy(config.Mqtt_Hass_Topic, doc[F("mqtt")][F("mqtt_hass_topic")], sizeof(config.Mqtt_Hass_Topic) );
    config.Mqtt_Hass_IndividualPanels = doc[F("mqtt")][F("mqtt_hass_individualpanels")];

    for (uint8_t i = 0; i < INV_MAX_COUNT && i < doc[F("inverter")].size(); i++) {
        if (doc[F("inverter")][i][F("serial")] > 0) {
            config.Inverter[i].Serial = serialFromJson(doc[F("inverter")][i][F("serial")]);
            strlcpy(config.Inverter[i].Name, doc[F("inverter")][i][F("name")], sizeof(config.Inverter[i].Name));

            for (uint8_t c = 0; c < doc[F("inverter")][i][F("max_power")].size(); c++) {
                config.Inverter[i].MaxChannelPower[c] = doc[F("inverter")][i][F("max_power")][c];
            }
        }
    }

    return res;
}

bool ConfigurationClass::JSONread(const char* filename)
{
    bool res = true;
    File f = LittleFS.open(filename, "r");
    if (!f) {
        return false;
    }

    res = JSONdeserialize(f);
    f.close();

    return res;
}

void ConfigurationClass::migrate()
{
    if (config.Cfg_Version < 0x00010400) {
        strlcpy(config.Ntp_Server, NTP_SERVER, sizeof(config.Ntp_Server));
        strlcpy(config.Ntp_Timezone, NTP_TIMEZONE, sizeof(config.Ntp_Timezone));
        strlcpy(config.Ntp_TimezoneDescr, NTP_TIMEZONEDESCR, sizeof(config.Ntp_TimezoneDescr));
    }

    if (config.Cfg_Version < 0x00010500) {
        config.Mqtt_Enabled = MQTT_ENABLED;
        strlcpy(config.Mqtt_Hostname, MQTT_HOST, sizeof(config.Mqtt_Hostname));
        config.Mqtt_Port = MQTT_PORT;
        strlcpy(config.Mqtt_Username, MQTT_USER, sizeof(config.Mqtt_Username));
        strlcpy(config.Mqtt_Password, MQTT_PASSWORD, sizeof(config.Mqtt_Password));
        strlcpy(config.Mqtt_Topic, MQTT_TOPIC, sizeof(config.Mqtt_Topic));
    }

    if (config.Cfg_Version < 0x00010600) {
        config.Mqtt_Retain = MQTT_RETAIN;
    }

    if (config.Cfg_Version < 0x00010700) {
        strlcpy(config.Mqtt_LwtTopic, MQTT_LWT_TOPIC, sizeof(config.Mqtt_LwtTopic));
        strlcpy(config.Mqtt_LwtValue_Online, MQTT_LWT_ONLINE, sizeof(config.Mqtt_LwtValue_Online));
        strlcpy(config.Mqtt_LwtValue_Offline, MQTT_LWT_OFFLINE, sizeof(config.Mqtt_LwtValue_Offline));
    }

    if (config.Cfg_Version < 0x00010800) {
        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            config.Inverter[i].Serial = 0;
            strlcpy(config.Inverter[i].Name, "", 0);
        }
    }

    if (config.Cfg_Version < 0x00010900) {
        config.Dtu_Serial = DTU_SERIAL;
        config.Dtu_PollInterval = DTU_POLL_INTERVAL;
        config.Dtu_PaLevel = DTU_PA_LEVEL;
    }

    if (config.Cfg_Version < 0x00011000) {
        config.Mqtt_PublishInterval = MQTT_PUBLISH_INTERVAL;
    }

    if (config.Cfg_Version < 0x00011100) {
        init(); // Config will be completly incompatible after this update
    }

    if (config.Cfg_Version < 0x00011200) {
        config.Mqtt_Hass_Enabled = MQTT_HASS_ENABLED;
        config.Mqtt_Hass_Retain = MQTT_HASS_RETAIN;
        strlcpy(config.Mqtt_Hass_Topic, MQTT_HASS_TOPIC, sizeof(config.Mqtt_Hass_Topic));
        config.Mqtt_Hass_IndividualPanels = MQTT_HASS_INDIVIDUALPANELS;
    }

    if (config.Cfg_Version < 0x00011300) {
        config.Mqtt_Tls = MQTT_TLS;
        strlcpy(config.Mqtt_RootCaCert, MQTT_ROOT_CA_CERT, sizeof(config.Mqtt_RootCaCert));
    }

    if (config.Cfg_Version < 0x00011400) {
        strlcpy(config.Mqtt_Hostname, config.Mqtt_Hostname_Short, sizeof(config.Mqtt_Hostname_Short));
    }

    if (config.Cfg_Version < 0x00011500) {
        config.Mqtt_Hass_Expire = MQTT_HASS_EXPIRE;
    }

    config.Cfg_Version = CONFIG_VERSION;
    write();
}

CONFIG_T& ConfigurationClass::get()
{
    return config;
}

INVERTER_CONFIG_T* ConfigurationClass::getFreeInverterSlot()
{
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        if (config.Inverter[i].Serial == 0) {
            return &config.Inverter[i];
        }
    }

    return NULL;
}

ConfigurationClass Configuration;