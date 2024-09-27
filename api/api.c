#include <stdint.h>
#include <windows.h>
#include <winsock2.h>
#include <process.h>

#include "api/api.h"
#include "api/config.h"

#include "dprintf.h"
#include "util.h"

static struct api_config api_cfg;

static DWORD api_socket_thread_proc(LPVOID ctx);

static HANDLE api_socket_thread;
static SOCKET listen_socket = INVALID_SOCKET;
static SOCKET send_socket = INVALID_SOCKET;
static struct sockaddr_in send_addr;
static struct sockaddr_in recv_addr;
static bool threadExitFlag = false;

static bool api_card_state_switch = false;
static bool api_card_reading_state = false;
static bool api_aime_rgb_set = false;
static uint8_t api_aime_rgb[3];
static int api_credits = 0;
static bool api_is_test_pressed = false;
static bool api_is_service_pressed = false;
static bool api_has_card_mifare = false;
static uint8_t api_cardid_mifare[10];
static bool api_has_card_felica = false;
static uint8_t api_cardid_felica[8];
static bool api_has_sequence = false;
static uint8_t api_sequence = 0;
static bool api_has_vfd_string = false;
static uint8_t api_vfd_string[200];
static bool api_card_reader_blocked = false;
static bool api_card_reader_blocked_switch = false;

uint32_t api_get_version(){
    return 0x010101;
}

HRESULT api_init(const char* config_filename) {

    WSADATA wsa;

    if (api_socket_thread != NULL) {
        dprintf("API: already running\n");
        return S_FALSE;
    }

    api_config_load(&api_cfg, config_filename);

    if (!api_cfg.enable) {
        dprintf("API: disabled\n");
        return S_FALSE;
    }
    dprintf("API: Initializing using port %d, group %d, device %d\n", api_cfg.port, api_cfg.groupId, api_cfg.deviceId);

    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err != 0) {
        dprintf("API: Failed to initialize, error %d\n", err);
        return E_FAIL;
    }

    listen_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listen_socket == INVALID_SOCKET) {
        dprintf("API: Failed to open listen socket: %d\n", WSAGetLastError());
        return E_FAIL;
    }

    if (api_cfg.port == 0){
        dprintf("API: port is null??\n");
        return E_FAIL;
    }

    const char opt = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(listen_socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(api_cfg.port);
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_socket, (SOCKADDR *) &recv_addr, sizeof(recv_addr)) == SOCKET_ERROR) {
        dprintf("API: bind (recv) failed with error %d\n", WSAGetLastError());
        return E_FAIL;
    }

    send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (send_socket == INVALID_SOCKET) {
        dprintf("API: Failed to open send socket: %d\n", WSAGetLastError());
        return E_FAIL;
    }
    setsockopt(send_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(send_socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(api_cfg.port);
    send_addr.sin_addr.s_addr = inet_addr(api_cfg.bindAddr);

    threadExitFlag = false;
    api_socket_thread = CreateThread(NULL, 0, api_socket_thread_proc, NULL, 0, NULL);

    return S_OK;
}

DWORD api_socket_thread_proc(__attribute__((unused)) LPVOID ctx) {
    struct sockaddr_in sender_addr;
    int sender_addr_size = sizeof(sender_addr);

    int err = SOCKET_ERROR;
    uint8_t buf[PACKET_MAX_SIZE];

    while (!threadExitFlag) {

        if (recvfrom(listen_socket, (char*)buf, PACKET_MAX_SIZE, 0, (SOCKADDR *) &sender_addr, &sender_addr_size) != SOCKET_ERROR) {
            uint8_t id = buf[PACKET_HEADER_FIELD_ID];
            uint8_t group = buf[PACKET_HEADER_FIELD_GROUPID];
            uint8_t device = buf[PACKET_HEADER_FIELD_MACHINEID];
            uint8_t len = buf[PACKET_HEADER_FIELD_LEN];

            if (group != api_cfg.groupId) {
                if (api_cfg.log) {
                    dprintf("API: Received packet designated for group %d, but we're %d\n", group, api_cfg.groupId);
                }
                continue;
            }

            if (device == api_cfg.deviceId) {
                if (api_cfg.log) {
                    dprintf("API: Received packet from ourselves\n");
                }
                continue;
            }

            len = min(len, PACKET_CONTENT_MAX_SIZE);
            uint8_t data[PACKET_CONTENT_MAX_SIZE];
            memcpy(data, buf + PACKET_HEADER_LEN, len);

            if (api_cfg.log) {
                dprintf("API: Received Packet: %d\n", id);
            }
            api_parse(id, len, data);
        } else {
            err = WSAGetLastError();
            dprintf("API: Receive error: %d\n", err);
            threadExitFlag = true;
        }
    }

    dprintf("API: Exiting\n");
    threadExitFlag = true;

    closesocket(listen_socket);
    closesocket(send_socket);
    WSACleanup();

    return 0;
}

int api_parse(enum API_PACKET id, uint8_t len, const uint8_t *data) {

    uint8_t ack_out = {id};
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
            memcpy(api_cardid_felica, data, min(len, sizeof(api_cardid_felica)));
            api_has_card_felica = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_26_CARD_AIME:
            memcpy(api_cardid_mifare, data, min(len, sizeof(api_cardid_mifare)));
            api_has_card_mifare = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_28_SEQUENCE:
            api_sequence = data[0];
            api_has_sequence = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_30_VFD_SHIFTJIS: {
            int outlen = 200;
            uint8_t utf8str[outlen];
            if (sj2utf8(data, len, utf8str, &outlen)) {
                memcpy(api_vfd_string, data, outlen);
                api_has_vfd_string = true;
            } else {
                dprintf("API: VFD UTF conversion failed\n");
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
            if (api_cfg.log) {
                dprintf("API: Set card read state: %d\n", data[0]);
            }
            api_card_reading_state = data[0];
            api_card_state_switch = true;
            api_send(PACKET_21_ACK, sizeof(ack_out), &ack_out);
            break;
        case PACKET_32_BLOCK_CARD_READER:
            if (api_cfg.log) {
                dprintf("API: Set card reader blocked: %d\n", data[0]);
            }
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
            dprintf("API: Received Exit packet!\n");
            TerminateProcess(GetCurrentProcess(), PACKET_34_EXIT);
            break;
        default:
            return API_PACKET_ID_UNKNOWN;
    }

    return API_COMMAND_OK;
}

int api_send(enum API_PACKET id, uint8_t len, const uint8_t *data) {

    if (!api_cfg.enable) {
        return API_DISABLED;
    }
    if (threadExitFlag) {
        return API_STATE_ERROR;
    }
    if (len > PACKET_CONTENT_MAX_SIZE) {
        return API_PACKET_TOO_LONG;
    }
    if (api_cfg.log) {
        dprintf("API: Sending Packet: %d\n", id);
    }

    int packetLen = PACKET_HEADER_LEN + len;
    uint8_t packet[packetLen];

    packet[PACKET_HEADER_FIELD_ID] = id;
    packet[PACKET_HEADER_FIELD_GROUPID] = api_cfg.groupId;
    packet[PACKET_HEADER_FIELD_MACHINEID] = api_cfg.deviceId;
    packet[PACKET_HEADER_FIELD_LEN] = len;
    memcpy(packet + PACKET_HEADER_LEN, data, len);

    if (sendto(send_socket, (char*)packet, packetLen, 0, (SOCKADDR *) &send_addr, sizeof(send_addr)) == SOCKET_ERROR) {
        dprintf("API: sendto failed with error: %d\n", WSAGetLastError());
        return API_SOCKET_OPERATION_FAIL;
    }

    return API_COMMAND_OK;
}

void api_stop() {
    dprintf("API: shutdown\n");
    threadExitFlag = true;
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
    if (api_aime_rgb_set){
        api_aime_rgb_set = false;
        return api_aime_rgb;
    } else {
        return NULL;
    }
}

void api_block_card_reader(bool b) {
    uint8_t data[1];
    data[0] = b;
    api_send(PACKET_32_BLOCK_CARD_READER, 1, data);
}

int api_get_and_clear_credits(){
   int i = api_credits;
   api_credits = 0;
   return i;
}

bool api_get_and_clear_service(){
    bool b = api_is_service_pressed;
    api_is_service_pressed = false;
    return b;
}

bool api_get_and_clear_test(){
    bool b = api_is_test_pressed;
    api_is_test_pressed = false;
    return b;
}

uint8_t* api_get_and_clear_card_mifare(){
    if (api_has_card_mifare){
        api_has_card_mifare = false;
        return api_cardid_mifare;
    } else {
        return NULL;
    }
}

uint8_t* api_get_and_clear_card_felica(){
    if (api_has_card_felica){
        api_has_card_felica = false;
        return api_cardid_felica;
    } else {
        return NULL;
    }
}

uint8_t api_get_and_clear_sequence(){
    if (api_has_sequence){
        api_has_sequence = false;
        return api_sequence;
    } else {
        return 0xFF;
    }
}

uint8_t* api_get_and_clear_vfd_message(){
    if (api_has_vfd_string){
        api_has_vfd_string = false;
        return api_vfd_string;
    } else {
        return NULL;
    }
}

bool api_get_reader_blocked_switch_state(){
    return api_card_reader_blocked_switch;
}

bool api_get_reader_blocked_and_clear_switch_state(){
    return api_card_reader_blocked;
}

void api_send_vfd(const wchar_t* string){
    char str[1024];
    wcstombs(str, string, 1024);
    api_send(PACKET_29_VFD, strlen(str), (uint8_t*)str);
}

void api_send_vfd_sj(const char* string){
    api_send(PACKET_30_VFD_SHIFTJIS, strlen(string), (uint8_t*)string);
}
