/* =====================================================================
 * Coyomi_Server_v01.c  ?  単純TCPサーバ（クライアント仕様に対応）
 *
 *  プロトコル（32bit big-endian = network byte order）6語：
 *   [0]=year, [1]=month, [2]=day, [3]=ndays, [4]=flags(bit0:dir_after, bit1:flag01), [5]=0
 *  応答も同形式で、[0..2] を「基準日±日数」結果に置換して返します。
 * =====================================================================*/

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#pragma comment(lib, "ws2_32.lib")

/* ====== サーバ設定 ====== */
static const char*    BIND_ADDR   = "127.0.0.1";
static const unsigned short PORT  = 50000;
static const int      BACKLOG     = 8;
static const DWORD    IO_TIMEOUT_MS = 15000;

/* ====== プロトコル定数 ====== */
enum {
    PACKET_WORDS = 6
};
enum {
    FLAG_DIR_MASK    = 0x00000001u, /* bit0: dir_after（1=後, 0=前） */
    FLAG_FLAG01_MASK = 0x00000002u  /* bit1: 1日目フラグ（今回の計算では未使用） */
};

/* ====== ユーティリティ（送受信） ====== */
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

/* ====== 日付加算（クライアント側に合わせて：ローカル→UTC→100ns加算→ローカル） ====== */
static SYSTEMTIME AddDays(SYSTEMTIME base, unsigned int ndays, bool dirAfter){
    /* ローカル→UTC */
    SYSTEMTIME utc;
    TzSpecificLocalTimeToSystemTime(NULL, &base, &utc);

    /* UTC→FILETIME */
    FILETIME ft;
    SystemTimeToFileTime(&utc, &ft);

    /* 64bit化 */
    ULARGE_INTEGER u;
    u.LowPart  = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;

    /* 1日 = 864000000000 (100ns単位) */
    const LONGLONG oneDay100ns = 864000000000LL;
    LONGLONG delta = (LONGLONG)ndays * oneDay100ns * (dirAfter ? +1 : -1);
    u.QuadPart += (ULONGLONG)delta;

    /* 戻す */
    ft.dwLowDateTime  = u.LowPart;
    ft.dwHighDateTime = u.HighPart;

    SYSTEMTIME utc2, local2;
    FileTimeToSystemTime(&ft, &utc2);
    SystemTimeToTzSpecificLocalTime(NULL, &utc2, &local2);
    return local2;
}

/* ====== 1接続処理スレッド ====== */
static unsigned __stdcall client_thread(void* arg){
    SOCKET cs = (SOCKET)(uintptr_t)arg;

    /* タイムアウト（送受信） */
    setsockopt(cs, SOL_SOCKET, SO_RCVTIMEO, (const char*)&IO_TIMEOUT_MS, sizeof(DWORD));
    setsockopt(cs, SOL_SOCKET, SO_SNDTIMEO, (const char*)&IO_TIMEOUT_MS, sizeof(DWORD));

    for(;;){
        uint32_t words_net[PACKET_WORDS];

        if (recv_all(cs, (char*)words_net, sizeof(words_net)) <= 0){
            break; /* 切断 or タイムアウト/エラー */
        }

        /* 受信 → ホストバイト序へ */
        uint32_t year   = ntohl(words_net[0]);
        uint32_t month  = ntohl(words_net[1]);
        uint32_t day    = ntohl(words_net[2]);
        uint32_t ndays  = ntohl(words_net[3]);
        uint32_t flags  = ntohl(words_net[4]);
        /* words_net[5] は互換用0 */

        /* 入力→SYSTEMTIME */
        SYSTEMTIME base = {0};
        base.wYear  = (WORD)year;
        base.wMonth = (WORD)month;
        base.wDay   = (WORD)day;

        bool dirAfter = ((flags & FLAG_DIR_MASK) != 0);
        /* flag01（1日目フラグ）は今回は使わない。必要ならここにロジックを追加 */

        /* 計算 */
        SYSTEMTIME target = AddDays(base, (unsigned)(ndays & 0x7fffffff), dirAfter);

        /* 応答の作成：
           - [0..2] を計算結果の年月日
           - [3] はそのまま ndays（クライアント側で short にクリップ）
           - [4] は flags をエコー
           - [5] は 0 */
        uint32_t ans[PACKET_WORDS];
        ans[0] = htonl((uint32_t)target.wYear);
        ans[1] = htonl((uint32_t)target.wMonth);
        ans[2] = htonl((uint32_t)target.wDay);
        ans[3] = htonl(ndays);
        ans[4] = htonl(flags);
        ans[5] = htonl(0);

        if (send_all(cs, (const char*)ans, sizeof(ans)) <= 0){
            break; /* 送信エラー */
        }
    }

    shutdown(cs, SD_BOTH);
    closesocket(cs);
    _endthreadex(0);
    return 0;
}

/* ====== メイン（待ち受け） ====== */
int main(void){
    WSADATA w;
    if (WSAStartup(MAKEWORD(2,2), &w) != 0){
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SOCKET ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == INVALID_SOCKET){
        fprintf(stderr, "socket failed\n");
        WSACleanup();
        return 1;
    }

    /* 127.0.0.1:PORT で待ち受け */
    struct sockaddr_in addr; 
    ZeroMemory(&addr, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(BIND_ADDR);

    int opt = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(ls, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR){
        fprintf(stderr, "bind failed\n");
        closesocket(ls);
        WSACleanup();
        return 1;
    }

    if (listen(ls, BACKLOG) == SOCKET_ERROR){
        fprintf(stderr, "listen failed\n");
        closesocket(ls);
        WSACleanup();
        return 1;
    }

    printf("Coyomi Server listening on %s:%u\n", BIND_ADDR, (unsigned)PORT);

    for(;;){
        struct sockaddr_in cli; int clen = sizeof(cli);
        SOCKET cs = accept(ls, (struct sockaddr*)&cli, &clen);
        if (cs == INVALID_SOCKET){
            fprintf(stderr, "accept failed\n");
            break;
        }

        /* クライアントごとにスレッド */
        uintptr_t th = _beginthreadex(NULL, 0, client_thread, (void*)(uintptr_t)cs, 0, NULL);
        if (!th){
            fprintf(stderr, "thread spawn failed\n");
            closesocket(cs);
            continue;
        }
        CloseHandle((HANDLE)th);
    }

    closesocket(ls);
    WSACleanup();
    return 0;
}
