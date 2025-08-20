// ConnectToCoyomi.c
// �T�[�o�� connect() �ł��邩�������m�F���鏬���ȃ��W���[���B
// ���s���ɂ� ERR_ID1_CONNECT �� on_error_raised �Œʒm����B
// ���ł� InvalidValueHandling ���Ϗ����锖�����b�p���񋟁B

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// **��������**: winsock2.h �� ws2tcpip.h �� windows.h
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "coyomi_all.h"

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

// IPv4 ������̊Ȉ� connect�B���O����(�z�X�g��)�ɂ��Ή��B
SOCKET ConnectToCoyomi_ConnectToCoyomi(
    const char* ip,
    uint16_t    port,
    void (*on_error_raised)(int error_id)
){
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET){
        if (on_error_raised) on_error_raised(ERR_ID1_CONNECT);
        return INVALID_SOCKET;
    }

    // ip �� NULL/��Ȃ� 127.0.0.1 ���g��
    const char* use_ip = (ip && *ip) ? ip : "127.0.0.1";

    // �܂��� inet_addr �� IPv4 ������Ƃ��ĉ��߂����݂�
    struct in_addr ipv4;
    ipv4.s_addr = inet_addr(use_ip);

    int ok = -1;

    if (ipv4.s_addr != INADDR_NONE){
        // �u�_�t��IPv4�v������
        struct sockaddr_in a;
        ZeroMemory(&a, sizeof(a));
        a.sin_family = AF_INET;
        a.sin_port   = htons(port);
        a.sin_addr   = ipv4;

        ok = connect(s, (struct sockaddr*)&a, (int)sizeof(a));
    } else {
        // �z�X�g���������ꍇ: getaddrinfo �� IPv4 �������Đڑ�������
        char port_str[16];
        _snprintf(port_str, _countof(port_str), "%u", (unsigned)port);

        struct addrinfo hints;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family   = AF_INET;       // IPv4
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo* res = NULL;
        if (getaddrinfo(use_ip, port_str, &hints, &res) == 0 && res){
            ok = connect(s, res->ai_addr, (int)res->ai_addrlen);
            freeaddrinfo(res);
        } else {
            ok = -1;
        }
    }

    if (ok == 0){
        // ����
        return s;
    }

    // ���s
    closesocket(s);
    if (on_error_raised) on_error_raised(ERR_ID1_CONNECT);
    return INVALID_SOCKET;
}

// �����Ϗ����b�p�i�K�v�Ȃ���Ύg��Ȃ��Ă�OK�j
void ConnectToCoyomi_InvalidValueHandling(
    const InputData* in,
    SentData*        out_sd,
    bool*            out_set_dtp_today,
    wchar_t*         opt_errbuf, size_t opt_errcap,
    void (*on_error_raised)(int error_id)
){
    InvalidValueHandling_InvalidValueHandling(
        in, out_sd, out_set_dtp_today, opt_errbuf, opt_errcap, on_error_raised
    );
}
