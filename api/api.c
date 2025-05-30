#include <stdint.h>
#include <windows.h>
#include <winsock2.h>
#include <process.h>

#include "api/api.h"
#include "api/config.h"

#include "dprintf.h"
#include "util.h"

#define dprintf_if(...) dprintf(__VA_ARGS__)

static struct api_config api_cfg;

static __stdcall DWORD api_socket_thread_proc(LPVOID ctx);

static HANDLE api_socket_thread;
static SOCKET listen_socket = INVALID_SOCKET;
static SOCKET send_socket = INVALID_SOCKET;
static struct sockaddr_in send_address;
static struct sockaddr_in receive_address;
static bool thread_exit_flag = false;

static bool api_card_state_switch = false;
static bool api_card_reading_state = false;
static bool api_aime_rgb_set = false;
static uint8_t api_aime_rgb[3];
static int api_credits = 0;
static bool api_is_test_pressed = false;
static bool api_is_service_pressed = false;
static bool api_has_card_mifare = false;
static uint8_t api_card_id_mifare[10];
static bool api_has_card_felica = false;
static uint8_t api_card_id_felica[8];
static bool api_has_sequence = false;
static uint8_t api_sequence = 0;
static bool api_has_vfd_string = false;
static uint8_t api_vfd_string[200];
static bool api_card_reader_blocked = false;
static bool api_card_reader_blocked_switch = false;

uint32_t api_get_version() {
    return 0x010101;
}

HRESULT api_init(const char* config_filename) {
    WSADATA wsa;

    if (api_socket_thread != NULL) {
        dprintf("segapi: already running\n");
        return S_FALSE;
    }

    api_config_load(&api_cfg, config_filename);

    if (!api_cfg.enable) {
        dprintf("segapi: disabled\n");
        return S_FALSE;
    }
    dprintf("segapi: Initializing using port %d, group %d, device %d\n", api_cfg.port, api_cfg.groupId,
            api_cfg.deviceId);

    const int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err != 0) {
        dprintf("segapi: Failed to initialize, error %d\n", err);
        return E_FAIL;
    }

    listen_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listen_socket == INVALID_SOCKET) {
        dprintf("segapi: Failed to open listen socket: %d\n", WSAGetLastError());
        return E_FAIL;
    }

    if (api_cfg.port == 0) {
        dprintf("segapi: port is null??\n");
        return E_FAIL;
    }

    const char opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(listen_socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    receive_address.sin_family = AF_INET;
    receive_address.sin_port = htons(api_cfg.port);
    receive_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_socket, (SOCKADDR *) &receive_address, sizeof(receive_address)) == SOCKET_ERROR) {
        dprintf("segapi: bind (recv) failed with error %d\n", WSAGetLastError());
        return E_FAIL;
    }

    send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (send_socket == INVALID_SOCKET) {
        dprintf("segapi: Failed to open send socket: %d\n", WSAGetLastError());
        return E_FAIL;
    }
    setsockopt(send_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(send_socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    send_address.sin_family = AF_INET;
    send_address.sin_port = htons(api_cfg.port);
    send_address.sin_addr.s_addr = inet_addr(api_cfg.bindAddr);

    thread_exit_flag = false;
    api_socket_thread = CreateThread(NULL, 0, api_socket_thread_proc, NULL, 0, NULL);

    return S_OK;
}

bool api_is_initialized() {
    if (!api_cfg.enable || api_socket_thread == NULL) {
        return false;
    }
    DWORD ec = 0;
    if (!GetExitCodeThread(api_socket_thread, &ec)) {
        return false;
    }
    return ec == STILL_ACTIVE;
}

DWORD __stdcall api_socket_thread_proc(__attribute__((unused)) LPVOID ctx) {
    struct sockaddr_in sender_address;
    int sender_addr_size = sizeof(sender_address);

    int err = SOCKET_ERROR;
    uint8_t buf[PACKET_MAX_SIZE];

    while (!thread_exit_flag) {
        if (recvfrom(listen_socket, buf, PACKET_MAX_SIZE, 0, (SOCKADDR *) &sender_address, &sender_addr_size) != SOCKET_ERROR) {
            const uint8_t id = buf[PACKET_HEADER_FIELD_ID];
            const uint8_t group = buf[PACKET_HEADER_FIELD_GROUPID];
            const uint8_t device = buf[PACKET_HEADER_FIELD_MACHINEID];
            uint8_t len = buf[PACKET_HEADER_FIELD_LEN];

            if (group != api_cfg.groupId) {
                dprintf_if("segapi: Received packet designated for group %d, but we're %d\n", group, api_cfg.groupId);
                continue;
            }

            if (device == api_cfg.deviceId) {
                dprintf_if("segapi: Received packet from ourselves\n");
                continue;
            }

            len = min(len, PACKET_CONTENT_MAX_SIZE);
            uint8_t data[PACKET_CONTENT_MAX_SIZE];
            memcpy(data, buf + PACKET_HEADER_LEN, len);

            dprintf_if("segapi: Received packet: %d\n", id);
            api_parse(id, len, data);
        } else {
            err = WSAGetLastError();
            dprintf("segapi: Receive error: %d\n", err);
            thread_exit_flag = true;
        }
    }

    dprintf("segapi: Exiting\n");
    thread_exit_flag = true;

    closesocket(listen_socket);
    closesocket(send_socket);
    WSACleanup();

    return err;
}

int api_parse(const enum API_PACKET id, const uint8_t len, const uint8_t* data) {
    const uint8_t ack_out = {id};
    switch (id) {
        case PACKET_20_PING:
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_21_ACK:
            break;
        case PACKET_22_TEST:
            api_is_test_pressed = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_23_SERVICE:
            api_is_service_pressed = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_24_CREDIT:
            if (len > 0) {
                api_credits += data[0];
            } else {
                api_credits += 1;
            }
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_25_CARD_FELICA:
            memcpy(api_card_id_felica, data, min(len, sizeof(api_card_id_felica)));
            api_has_card_felica = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_26_CARD_AIME:
            memcpy(api_card_id_mifare, data, min(len, sizeof(api_card_id_mifare)));
            api_has_card_mifare = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_28_SEQUENCE:
            api_sequence = data[0];
            api_has_sequence = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_30_VFD_SHIFTJIS: {
            int out_len = 200;
            uint8_t utf8str[out_len];
            if (sj2utf8(data, len, utf8str, &out_len)) {
                memcpy(api_vfd_string, data, out_len);
                api_has_vfd_string = true;
            } else {
                dprintf("segapi: VFD UTF conversion failed\n");
            }
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        }
        case PACKET_29_VFD:
            memcpy(api_vfd_string, data, len);
            api_has_vfd_string = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_31_SET_CARD_READING_STATE:
            dprintf_if("segapi: Set card read state: %d\n", data[0]);
            api_card_reading_state = data[0];
            api_card_state_switch = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_32_BLOCK_CARD_READER:
            dprintf_if("segapi: Set card reader blocked: %d\n", data[0]);
            api_card_reader_blocked = data[0];
            api_card_reader_blocked_switch = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_33_AIME_RGB:
            api_aime_rgb_set = true;
            api_aime_rgb[0] = data[0];
            api_aime_rgb[1] = data[1];
            api_aime_rgb[2] = data[2];
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_34_EXIT:
            dprintf("segapi: Received Exit packet!\n");
            TerminateProcess(GetCurrentProcess(), PACKET_34_EXIT);
            break;
        default:
            return API_PACKET_ID_UNKNOWN;
    }

    return API_COMMAND_OK;
}

int api_send(const enum API_PACKET id, const uint8_t len, const uint8_t* data) {
    if (!api_cfg.enable) {
        return API_DISABLED;
    }
    if (thread_exit_flag) {
        return API_STATE_ERROR;
    }
    if (len > PACKET_CONTENT_MAX_SIZE) {
        return API_PACKET_TOO_LONG;
    }
    dprintf_if("segapi: Sending Packet: %d\n", id);

    const int packetLen = PACKET_HEADER_LEN + len;
    uint8_t packet[packetLen];

    packet[PACKET_HEADER_FIELD_ID] = id;
    packet[PACKET_HEADER_FIELD_GROUPID] = api_cfg.groupId;
    packet[PACKET_HEADER_FIELD_MACHINEID] = api_cfg.deviceId;
    packet[PACKET_HEADER_FIELD_LEN] = len;
    memcpy(packet + PACKET_HEADER_LEN, data, len);

    if (sendto(send_socket, packet, packetLen, 0, (SOCKADDR *) &send_address, sizeof(send_address)) ==
        SOCKET_ERROR) {
        dprintf("segapi: sendto failed with error: %d\n", WSAGetLastError());
        return API_SOCKET_OPERATION_FAIL;
    }

    return API_COMMAND_OK;
}

void api_stop() {
    dprintf("segapi: shutdown\n");
    thread_exit_flag = true;
    closesocket(listen_socket);
    closesocket(send_socket);
    WaitForSingleObject(api_socket_thread, INFINITE);
    CloseHandle(api_socket_thread);
    api_socket_thread = NULL;
}

bool api_get_card_switch_state() {
    return api_card_state_switch;
}

bool api_get_card_reading_state_and_clear_switch_state() {
    api_card_state_switch = false;
    return api_card_reading_state;
}

uint8_t* api_get_aime_rgb_and_clear() {
    if (api_aime_rgb_set) {
        api_aime_rgb_set = false;
        return api_aime_rgb;
    }
    return NULL;
}

void api_block_card_reader(const bool b) {
    uint8_t data[1];
    data[0] = b;
    api_send(PACKET_32_BLOCK_CARD_READER, 1, data);
}

int api_get_and_clear_credits() {
    const int i = api_credits;
    api_credits = 0;
    return i;
}

bool api_get_and_clear_service() {
    const bool b = api_is_service_pressed;
    api_is_service_pressed = false;
    return b;
}

bool api_get_and_clear_test() {
    const bool b = api_is_test_pressed;
    api_is_test_pressed = false;
    return b;
}

uint8_t* api_get_and_clear_card_mifare() {
    if (api_has_card_mifare) {
        api_has_card_mifare = false;
        return api_card_id_mifare;
    }
    return NULL;
}

uint8_t* api_get_and_clear_card_felica() {
    if (api_has_card_felica) {
        api_has_card_felica = false;
        return api_card_id_felica;
    }
    return NULL;
}

uint8_t api_get_and_clear_sequence() {
    if (api_has_sequence) {
        api_has_sequence = false;
        return api_sequence;
    }
    return 0xFF;
}

uint8_t* api_get_and_clear_vfd_message() {
    if (api_has_vfd_string) {
        api_has_vfd_string = false;
        return api_vfd_string;
    }
    return NULL;
}

bool api_get_reader_blocked_switch_state() {
    return api_card_reader_blocked_switch;
}

bool api_get_reader_blocked_and_clear_switch_state() {
    return api_card_reader_blocked;
}

void api_send_vfd(const wchar_t* string, const int len) {
    char str[1024];
    wcstombs(str, string, 1024);
    api_send(PACKET_29_VFD, len, str);
}

void api_send_vfd_sj(const char* string, const int len) {
    api_send(PACKET_30_VFD_SHIFTJIS, len, (uint8_t *) string);
}
