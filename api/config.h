#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Configuration for segapi.
 */
struct api_config {
	/**
	 * Whether or not the API is enabled.
	 */
	uint8_t enable;
	/**
	 * The API group ID. See https://gmg.hopto.org:82/gmg/wiki/index.php/SegAPI#Groups_and_Devices
	 */
	uint8_t groupId;
	/**
	 * The API device ID. See https://gmg.hopto.org:82/gmg/wiki/index.php/SegAPI#Groups_and_Devices
	 */
    uint8_t deviceId;
	/**
	 * Whether or not log messages about received/sent packets are written to the debugger.
	 */
	uint8_t log;
	/**
	 * The port the API listens on. (default: 5364)
	 */
	uint16_t port;
	/**
	 * The socket bind address for the API. (default: 255.255.255.255)
	 */
	char bindAddr[16];
};

/**
 * Loads the API configuration from a file.
 * @param cfg The api_config struct to fill
 * @param filename The .ini file to load the config from
 */
void api_config_load(
        struct api_config *cfg,
        const char *filename);
