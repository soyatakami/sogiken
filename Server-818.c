// gcc -o Server-818.exe Server-818.c -lws2_32   ./server-818
// サーバ（暦計算側）
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // ← uint8_t, uint16_t, uint32_t

#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 12345
#define PACKET_SIZE 20

// UI側と同じレイアウト（CorrectedData / ResultData 互換）
typedef struct
{
    unsigned int year, month, day, days;
    unsigned short weekday;
    bool startDateSet; // クライアントからの「開始日を含む」
    bool beforeAfter;  // true=加算, false=減算
} RecvDataFromUI;

typedef struct
{
    unsigned int year, month, day, days;
    unsigned short weekday;
    bool startDateSet; // 送り返す「開始日を含む」
    bool beforeAfter;
} SendDataToUI;

// グローバルソケット
static SOCKET g_serverSock = INVALID_SOCKET;
static SOCKET g_clientSock = INVALID_SOCKET;

// 送受信ヘルパ
static int send_all(SOCKET s, const char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = send(s, buf + total, len - total, 0);
        if (n <= 0)
            return n;
        total += n;
    }
    return total;
}
static int recv_all(SOCKET s, char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0)
            return n;
        total += n;
    }
    return total;
}
static void ParsePacketToRecvData(const uint8_t *buf, RecvDataFromUI *recv)
{
    const uint32_t *p32 = (const uint32_t *)buf;
    recv->year = ntohl(p32[0]);
    recv->month = ntohl(p32[1]);
    recv->day = ntohl(p32[2]);
    recv->days = ntohl(p32[3]);
    const uint16_t *p16 = (const uint16_t *)(buf + 16);
    recv->weekday = ntohs(p16[0]);
    recv->startDateSet = buf[18] ? true : false;
    recv->beforeAfter = buf[19] ? true : false;
}

static void BuildPacketFromSendData(const SendDataToUI *send, uint8_t *buf)
{
    uint32_t *p32 = (uint32_t *)buf;
    p32[0] = htonl(send->year);
    p32[1] = htonl(send->month);
    p32[2] = htonl(send->day);
    p32[3] = htonl(send->days);
    uint16_t *p16 = (uint16_t *)(buf + 16);
    p16[0] = htons(send->weekday);
    buf[18] = send->startDateSet ? 1 : 0;
    buf[19] = send->beforeAfter ? 1 : 0;
}

// 日付計算（UI側と同等仕様）
static SYSTEMTIME AddDays(SYSTEMTIME start, unsigned int days, bool addDirection, bool includeStart)
{
    FILETIME ft;
    if (!SystemTimeToFileTime(&start, &ft))
    {
        SYSTEMTIME today;
        GetLocalTime(&today);
        if (!SystemTimeToFileTime(&today, &ft))
            return start;
        start = today;
    }
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    const LONGLONG ONE_DAY_100NS = 24LL * 60LL * 60LL * 10000000LL;
    unsigned int move = days;
    if (includeStart && move > 0)
        move -= 1U; // 開始日を含む→1日少なく動かす
    LONGLONG delta = (LONGLONG)move * ONE_DAY_100NS;
    if (!addDirection)
        delta = -delta;

    uli.QuadPart += delta;
    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;

    SYSTEMTIME out = {0};
    if (!FileTimeToSystemTime(&ft, &out))
        return start;
    return out;
}

// 初期化
static void InitializeServerSocket(void)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        fprintf(stderr, "WSAStartup failed\n");
        exit(1);
    }
    g_serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_serverSock == INVALID_SOCKET)
    {
        fprintf(stderr, "socket failed\n");
        WSACleanup();
        exit(1);
    }
    // 再起動時の bind エラー回避
    BOOL yes = 1;
    setsockopt(g_serverSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_serverSock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        fprintf(stderr, "bind failed\n");
        closesocket(g_serverSock);
        WSACleanup();
        exit(1);
    }
    if (listen(g_serverSock, 8) == SOCKET_ERROR)
    {
        fprintf(stderr, "listen failed\n");
        closesocket(g_serverSock);
        WSACleanup();
        exit(1);
    }
    printf("Server listening on port %d...\n", SERVER_PORT);
}

// 接続待ち（1クライアント受け入れ）
static int ConnectToUI(void)
{
    struct sockaddr_in caddr;
    int clen = sizeof(caddr);
    g_clientSock = accept(g_serverSock, (struct sockaddr *)&caddr, &clen);
    if (g_clientSock == INVALID_SOCKET)
    {
        fprintf(stderr, "accept failed\n");
        return -1;
    }
    printf("Client connected\n");
    return 0;
}

// 受信→計算→送信（1リクエスト）
static void CoyomiReceiveSend(void)
{
    uint8_t pkt[PACKET_SIZE];
    RecvDataFromUI recvData = {0};
    SendDataToUI sendData = {0};

    if (recv_all(g_clientSock, (char *)pkt, PACKET_SIZE) != PACKET_SIZE)
    {
        fprintf(stderr, "receive failed\n");
        return;
    }
    ParsePacketToRecvData(pkt, &recvData);

    SYSTEMTIME start = {0};
    start.wYear = (WORD)recvData.year;
    start.wMonth = (WORD)recvData.month;
    start.wDay = (WORD)recvData.day;

    SYSTEMTIME dest = AddDays(start, recvData.days,
                              recvData.beforeAfter, recvData.startDateSet);

    memset(&sendData, 0, sizeof(sendData));
    sendData.year = dest.wYear;
    sendData.month = dest.wMonth;
    sendData.day = dest.wDay;
    sendData.weekday = dest.wDayOfWeek;
    sendData.days = recvData.days;
    sendData.startDateSet = recvData.startDateSet;
    sendData.beforeAfter = recvData.beforeAfter;

    BuildPacketFromSendData(&sendData, pkt);

    if (send_all(g_clientSock, (const char *)pkt, PACKET_SIZE) != PACKET_SIZE)
    {
        fprintf(stderr, "send failed\n");
        return;
    }

    printf("Result sent: %04u-%02u-%02u (weekday=%u)\n",
           sendData.year, sendData.month, sendData.day, sendData.weekday);
}

// メイン：何度でも計算できるよう accept をループ
int main(void)
{
    InitializeServerSocket();

    for (;;)
    {
        if (ConnectToUI() != 0)
            break;

        RecvDataFromUI recvData = {0};
        SendDataToUI sendData = {0};

        CoyomiReceiveSend();

        closesocket(g_clientSock);
        g_clientSock = INVALID_SOCKET;
        printf("Client disconnected\n");
    }

    if (g_serverSock != INVALID_SOCKET)
        closesocket(g_serverSock);
    WSACleanup();
    return 0;
}