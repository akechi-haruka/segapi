/**
 * SegApi / Segatools API header file
 * 2024 Haruka
 *
 * https://gmg.hopto.org:82/gmg/wiki/index.php/Segatools_API
 */

enum API_PACKET {
    PACKET_20_PING = 20,
	PACKET_21_ACK = 21,
	PACKET_22_TEST = 22,
	PACKET_23_SERVICE = 23,
	PACKET_24_CREDIT = 24,
	PACKET_25_CARD_FELICA = 25,
	PACKET_26_CARD_AIME = 26,
	PACKET_28_SEQUENCE = 28,
	PACKET_29_VFD = 29,
	PACKET_30_VFD_SHIFTJIS = 30,
	PACKET_31_SET_CARD_READING_STATE = 31,
	PACKET_32_BLOCK_CARD_READER = 32,
    PACKET_33_AIME_RGB = 33
};

// Length of the packet header
#define PACKET_HEADER_LEN 4
// Packet index of the packet ID
#define PACKET_HEADER_FIELD_ID 0
// Packet index of the group ID
#define PACKET_HEADER_FIELD_GROUPID 1
// Packet index of the machine ID
#define PACKET_HEADER_FIELD_MACHINEID 2
// Packet index of the payload length
#define PACKET_HEADER_FIELD_LEN 3

// Maximum payload size
#define PACKET_CONTENT_MAX_SIZE 128
// Maximum packet size
#define PACKET_MAX_SIZE (PACKET_CONTENT_MAX_SIZE + PACKET_HEADER_LEN)

// Return values of api_* functions:

// API is disabled (success)
#define API_DISABLED 0
// OK
#define API_COMMAND_OK 1

#define API_IS_ERROR(x) x < API_DISABLED

// Payload is too long
#define API_PACKET_TOO_LONG (-1)
// Invalid state
#define API_STATE_ERROR (-2)
// Unknown packet ID
#define API_PACKET_ID_UNKNOWN (-3)
// Socket error
#define API_SOCKET_OPERATION_FAIL (-4)

/*
 * Get the current API version.
 *
 * @return 0x010101
 */
uint32_t api_get_version();

/**
 * Initializes the API, sockets and threads.
 *
 * @param config_filename config_filename: Path to an .ini file with an "[api]" section.
 * @return HRESULT indicating status.
 */
HRESULT api_init(const char* config_filename);

/*
 * Stops the API, threads and sockets.
 */
void api_stop();

/**
 * Handles a received packet.
 *
 * @param id The packet ID.
 * @param len length of data.
 * @param data The payload bytes.
 * @return API_* constant indicating success or failure.
 */
int api_parse(enum API_PACKET id, uint8_t len, const uint8_t* data);

/**
 * Sends a packet.
 *
 * @param id The packet ID.
 * @param len length of data.
 * @param data The payload bytes.
 * @return API_* constant indicating success or failure.
 */
int api_send(enum API_PACKET id, uint8_t len, const uint8_t* data);

/**
 * 
 * @return if a PACKET_31_SET_CARD_READING_STATE was received.
 */
bool api_get_card_switch_state();

/**
 * 
 * @return Returns the value of PACKET_31_SET_CARD_READING_STATE and clears its receive flag.
 */
bool api_get_card_reading_state_and_clear_switch_state();

/**
 * Sends a PACKET_32_BLOCK_CARD_READER.
 * @param b 1 to block, 0 to unblock
 */
void api_block_card_reader(bool b);

/**
 * 
 * @return Returns the value (3 bytes) of a PACKET_33_AIME_RGB and clears its receive flag. NULL if nothing is received.
 */
uint8_t* api_get_aime_rgb_and_clear();

/**
 * 
 * @return Returns the value of a PACKET_24_CREDIT and clears its receive flag. 0 if nothing is received.
 */
int api_get_and_clear_credits();

/**
 * 
 * @return Returns true if a PACKET_23_SERVICE was received and clears its receive flag.
 */
bool api_get_and_clear_service();

/**
 * 
 * @return Returns true if a PACKET_22_TEST was received and clears its receive flag.
 */
bool api_get_and_clear_test();

/**
 * 
 * @return Returns a MIFARE card ID if a PACKET_26_CARD_AIME was received and clears it. NULL otherwise.
 */
uint8_t* api_get_and_clear_card_mifare();

/**
 * 
 * @return Returns a MIFARE card ID if a PACKET_25_CARD_FELICA was received and clears it. NULL otherwise.
 */
uint8_t* api_get_and_clear_card_felica();

/**
 * 
 * @return Returns the sequence state if a PACKET_28_SEQUENCE was received and clears it. 0xFF otherwise.
 */
uint8_t api_get_and_clear_sequence();

/**
 * 
 * @return Returns the VFD message if any VFD packet was received. This string is always UTF-8. NULL otherwise.
 */
uint8_t* api_get_and_clear_vfd_message();

/**
 * 
 * @return Returns true if a PACKET_32_BLOCK_CARD_READER was received.
 */
bool api_get_reader_blocked_switch_state();

/**
 * 
 * @return Returns the value of PACKET_32_BLOCK_CARD_READER if one was received and clears its receive flag.
 */
bool api_get_reader_blocked_and_clear_switch_state();

/**
 * Sends a PACKET_29_VFD (UTF-8).
 * @param string The VFD text to send.
 */
void api_send_vfd(const wchar_t* string);

/**
 * Sends a PACKET_30_VFD_SHIFTJIS (UTF-8).
 * @param string The VFD text to send.
 */
void api_send_vfd_sj(const char* string);
