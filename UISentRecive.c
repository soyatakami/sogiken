// UIntRecive.c

// 送受信用のモジュールをMainから切り分けて実装したいです．
// モジュール名：UIntRecive
// 関数名：UIntRecive_Sent
// 関数名：UIntRecive_Recive
// 関数名：UIntRecive_ InvalidValueHandling 送受信が出来ない場合にエラーIDの更新

// 送受信（pack/unpack 含む）を Main から分離
// 送受信失敗時は ERR_ID7_SEND_RECV_FAIL をコールバック経由で通知

// UIntRecive.c
// 送受信（pack/unpack 含む）を Main から分離
// 送受信失敗時は ERR_ID7_SEND_RECV_FAIL をコールバック経由で通知

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

// Winsock は .c 側でだけ include（ヘッダは非依存にする）
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <string.h>

#include "coyomi_all.h"  // SentData, ReceiveData, ErrorID, PACKET_WORDS, g_error_id, etc.

// --- 内部ユーティリティ --------------------------------------------------

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
    // 末尾 word に「最後の1件」エラーID（g_error_id）を載せる仕様
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
    g_error_id = (ErrorID)t[5];  // 受信時に g_error_id を更新
}

// --- 公開API -------------------------------------------------------------

int UIntRecive_Sent(COYOMI_SOCKET cs, const SentData* sd, ErrorRaiseCb on_error){
    if (!sd) {
        if (on_error) on_error(ERR_ID7_SEND_RECV_FAIL);
        return -1;
    }
    SOCKET s = (SOCKET)cs;  // COYOMI_SOCKET は UINT_PTR。SOCKET と互換。
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
