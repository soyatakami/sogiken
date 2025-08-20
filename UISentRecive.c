// UIntRecive.c

// ����M�p�̃��W���[����Main����؂蕪���Ď����������ł��D
// ���W���[�����FUIntRecive
// �֐����FUIntRecive_Sent
// �֐����FUIntRecive_Recive
// �֐����FUIntRecive_ InvalidValueHandling ����M���o���Ȃ��ꍇ�ɃG���[ID�̍X�V

// ����M�ipack/unpack �܂ށj�� Main ���番��
// ����M���s���� ERR_ID7_SEND_RECV_FAIL ���R�[���o�b�N�o�R�Œʒm

// UIntRecive.c
// ����M�ipack/unpack �܂ށj�� Main ���番��
// ����M���s���� ERR_ID7_SEND_RECV_FAIL ���R�[���o�b�N�o�R�Œʒm

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// Winsock �� .c ���ł��� include�i�w�b�_�͔�ˑ��ɂ���j
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <string.h>

#include "coyomi_all.h"  // SentData, ReceiveData, ErrorID, PACKET_WORDS, g_error_id, etc.

// --- �������[�e�B���e�B --------------------------------------------------

static int send_all(SOCKET s, const char* buf, int len){
    int sent = 0;
    while (sent < len){
        int r = send(s, buf + sent, len - sent, 0);
        if (r <= 0) return -1;
        sent += r;
    }
    return sent;
}

static int recv_all(SOCKET s, char* buf, int len){
    int recvd = 0;
    while (recvd < len){
        int r = recv(s, buf + recvd, len - recvd, 0);
        if (r <= 0) return -1;
        recvd += r;
    }
    return recvd;
}

static void pack_to_wire_from_sent(const SentData* s, uint32_t netbuf[PACKET_WORDS]){
    // ���� word �Ɂu�Ō��1���v�G���[ID�ig_error_id�j���ڂ���d�l
    uint32_t t[PACKET_WORDS] = {
        s->year, s->month, s->day, s->ndays, s->flags, (uint32_t)g_error_id
    };
    for (int i=0; i<PACKET_WORDS; ++i) netbuf[i] = htonl(t[i]);
}

static void unpack_from_wire_to_recv(ReceiveData* r, const uint32_t netbuf[PACKET_WORDS]){
    uint32_t t[PACKET_WORDS];
    for (int i=0; i<PACKET_WORDS; ++i) t[i] = ntohl(netbuf[i]);
    r->year  = t[0];
    r->month = t[1];
    r->day   = t[2];
    r->ndays = t[3];
    r->flags = t[4];
    g_error_id = (ErrorID)t[5];  // ��M���� g_error_id ���X�V
}

// --- ���JAPI -------------------------------------------------------------

int UIntRecive_Sent(COYOMI_SOCKET cs, const SentData* sd, ErrorRaiseCb on_error){
    if (!sd) {
        if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
        return -1;
    }
    SOCKET s = (SOCKET)cs;  // COYOMI_SOCKET �� UINT_PTR�BSOCKET �ƌ݊��B
    uint32_t netbuf[PACKET_WORDS];
    pack_to_wire_from_sent(sd, netbuf);
    int r = send_all(s, (const char*)netbuf, (int)sizeof(netbuf));
    if (r <= 0){
        if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
        return -1;
    }
    return r;
}

int UIntRecive_Recive(COYOMI_SOCKET cs, ReceiveData* rd, ErrorRaiseCb on_error){
    if (!rd) {
        if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
        return -1;
    }
    SOCKET s = (SOCKET)cs;
    uint32_t netbuf[PACKET_WORDS];
    int r = recv_all(s, (char*)netbuf, (int)sizeof(netbuf));
    if (r <= 0){
        if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
        return -1;
    }
    unpack_from_wire_to_recv(rd, netbuf);
    return r;
}

void UIntRecive_InvalidValueHandling(ErrorRaiseCb on_error){
    if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
}
