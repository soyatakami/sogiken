//* Coyomi_Server_Test_Mainのコード
//* コンパイル例（MinGW-w64）:
//* gcc Coyomi_Server_Test_Main_v3.c -o Coyomi_Server_Test_Server_v3.exe -lws2_32 
//*-std=c11 -O2 -Wall -Wextra -Wpedantic

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>   // Sleep
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _MSC_VER
#pragma comment(lib, "Ws2_32.lib") // MSVC向け。MinGWなら -lws2_32 でリンク
#endif

//==============================
// Proto 相当（新フォーマット：6ワード）
//==============================
typedef struct {
    uint32_t year;      // 0
    uint32_t month;     // 1
    uint32_t day;       // 2
    uint32_t ndays;     // 3
    uint32_t flags;     // 4  (dir/flag01/weekday packed)
    uint32_t error;     // 5
} CalendarPacket;

enum { PACKET_WORDS = 6 };

// flagsビット配置（クライアントと一致）
#define FLAG_DIR_MASK      0x00000001u
#define FLAG_FLAG01_MASK   0x00000002u
#define FLAG_WD_MASK       0x00000700u
#define FLAG_WD_SHIFT      8

static inline bool   flags_get_dir(uint32_t f){ return (f & FLAG_DIR_MASK)   != 0; }
static inline bool   flags_get_flag01(uint32_t f){ return (f & FLAG_FLAG01_MASK)!=0; }
static inline short  flags_get_weekday(uint32_t f){ return (short)((f & FLAG_WD_MASK) >> FLAG_WD_SHIFT); }
static inline uint32_t flags_set_weekday(uint32_t f, short wd){
    f &= ~FLAG_WD_MASK;
    f |= ((uint32_t)(wd & 0x7)) << FLAG_WD_SHIFT;
    return f;
}

//==============================
// Wire 相当
//==============================
static int send_all(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, buf + sent, len - sent, 0);
        if (n <= 0) return n;
        sent += n;
    }
    return sent;
}
static int recv_all(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int n = recv(s, buf + got, len - got, 0);
        if (n <= 0) return n;
        got += n;
    }
    return got;
}
static void pack_to_wire(const CalendarPacket* p, uint32_t net[PACKET_WORDS]) {
    net[0]=htonl(p->year);   net[1]=htonl(p->month);
    net[2]=htonl(p->day);    net[3]=htonl(p->ndays);
    net[4]=htonl(p->flags);  net[5]=htonl(p->error);
}
static void unpack_from_wire(CalendarPacket* p, const uint32_t net[PACKET_WORDS]) {
    p->year  = ntohl(net[0]);  p->month = ntohl(net[1]);
    p->day   = ntohl(net[2]);  p->ndays = ntohl(net[3]);
    p->flags = ntohl(net[4]);  p->error = ntohl(net[5]);
}

//==============================
// Network 相当
//==============================
static int   net_startup(void) {
    WSADATA wsa;
    return (WSAStartup(MAKEWORD(2,2), &wsa) == 0) ? 0 : -1;
}
static void  net_cleanup(void) {
    WSACleanup();
}
static SOCKET net_listen_loopback(uint16_t port, int backlog) {
    SOCKET sv = socket(AF_INET, SOCK_STREAM, 0);
    if (sv == INVALID_SOCKET) return INVALID_SOCKET;

    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port   = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1 限定

    if (bind(sv, (struct sockaddr*)&a, sizeof(a)) == SOCKET_ERROR) {
        closesocket(sv);
        return INVALID_SOCKET;
    }
    if (listen(sv, backlog) == SOCKET_ERROR) {
        closesocket(sv);
        return INVALID_SOCKET;
    }
    return sv;
}
static SOCKET net_accept(SOCKET listener) {
    return accept(listener, NULL, NULL);
}
static void   net_close(SOCKET s) {
    if (s != INVALID_SOCKET) closesocket(s);
}
static int    net_recv_packet(SOCKET s, CalendarPacket* pkt) {
    uint32_t net[PACKET_WORDS];
    int r = recv_all(s, (char*)net, (int)sizeof(net));
    if (r <= 0) return r;
    unpack_from_wire(pkt, net);
    return r; // 24 を期待
}
static int    net_send_packet(SOCKET s, const CalendarPacket* pkt) {
    uint32_t net[PACKET_WORDS];
    pack_to_wire(pkt, net);
    return send_all(s, (const char*)net, (int)sizeof(net));
}

//==============================
// Calendar（実装）
//==============================

// 有効範囲（GUIの注記に合わせる）
#define MIN_Y 2010
#define MIN_M 1
#define MIN_D 1
#define MAX_Y 2099
#define MAX_M 12
#define MAX_D 31

enum {
    ERR_OK = 0,
    ERR_INVALID_DATE = 1,
    ERR_RANGE = 2
};

static int is_leap(int y){
    return ( (y%4==0) && (y%100!=0) ) || (y%400==0);
}
static int days_in_month(int y, int m){
    static const int dm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if(m==2) return dm[1] + (is_leap(y)?1:0);
    if(m>=1 && m<=12) return dm[m-1];
    return 0;
}
static int valid_date_ymd(int y, int m, int d){
    if(y<1 || y>9999) return 0;
    if(m<1 || m>12) return 0;
    int dim = days_in_month(y,m);
    if(d<1 || d>dim) return 0;
    return 1;
}

// Howard Hinnant のアルゴリズムに基づく「日数⇔年月日」変換
static long long days_from_civil(int y, int m, int d){
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2)/5 + d - 1);
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (long long)era * 146097 + (long long)doe - 719468;    // 1970-01-01 -> 0
}
static void civil_from_days(long long z, int* y, int* m, int* d){
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = (unsigned)(z - era * 146097);
    const unsigned yoe = (unsigned)((doe - doe/1460 + doe/36524 - doe/146096) / 365);
    long long yy = (long long)yoe + era * 400;
    const unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);
    const unsigned mp = (5*doy + 2)/153;
    const unsigned dd = doy - (153*mp + 2)/5 + 1;
    const unsigned mm = mp + (mp < 10 ? 3 : -9);
    yy += (mm <= 2);
    *y = (int)yy; *m = (int)mm; *d = (int)dd;
}
// 0=日 .. 6=土 に合わせる（1970/1/1=木）
static int weekday_from_days(long long z){
    int w = (int)((z + 4) % 7);
    if (w < 0) w += 7;
    return w;
}
// 範囲: [2010-01-01, 2099-12-31]
static int in_allowed_range(int y,int m,int d){
    if (!valid_date_ymd(y,m,d)) return 0;
    if (y < MIN_Y) return 0;
    if (y == MIN_Y){
        if (m < MIN_M) return 0;
        if (m == MIN_M && d < MIN_D) return 0;
    }
    if (y > MAX_Y) return 0;
    if (y == MAX_Y){
        if (m > MAX_M) return 0;
        if (m == MAX_M && d > MAX_D) return 0;
    }
    return 1;
}

// 受け取ったパケットを処理：結果年月日を year/month/day に上書きし、weekday を flags に格納
static void calendar_process(CalendarPacket* pkt){
    pkt->error = ERR_OK;

    // 入力日の妥当性
    if (!valid_date_ymd((int)pkt->year, (int)pkt->month, (int)pkt->day)) {
        pkt->error = ERR_INVALID_DATE;
        return;
    }
    // 入力日が有効範囲内か
    if (!in_allowed_range((int)pkt->year, (int)pkt->month, (int)pkt->day)) {
        pkt->error = ERR_RANGE;
        return;
    }

    bool dir_after = flags_get_dir(pkt->flags);
    bool flag01    = flags_get_flag01(pkt->flags);

    // シフト日数（0/1日目を反映）
    uint32_t n = pkt->ndays;
    if (flag01 && n > 0) n -= 1; // 「1日目」なら1引く
    long long shift = dir_after ? (long long)n : -(long long)n;

    // 変換して加算
    long long base = days_from_civil((int)pkt->year, (int)pkt->month, (int)pkt->day);
    long long target = base + shift;

    int y,m,d;
    civil_from_days(target, &y, &m, &d);

    // 結果が範囲外ならエラー
    if (!in_allowed_range(y,m,d)){
        pkt->error = ERR_RANGE;
        return;
    }

    pkt->year  = (uint32_t)y;
    pkt->month = (uint32_t)m;
    pkt->day   = (uint32_t)d;
    pkt->flags = flags_set_weekday(pkt->flags, (short)weekday_from_days(target));
    pkt->error = ERR_OK;
}

//==============================
// 定数
//==============================
#define SERVER_PORT 5000

//==============================
// エントリポイント
//==============================
int main(void){
    if (net_startup() != 0){
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SOCKET listener = net_listen_loopback(SERVER_PORT, 8);
    if (listener == INVALID_SOCKET){
        fprintf(stderr, "listen(127.0.0.1:%d) failed\n", SERVER_PORT);
        net_cleanup();
        return 1;
    }
    printf("Coyomi Server listening on 127.0.0.1:%d ...\n", SERVER_PORT);

    for(;;){
        SOCKET cli = net_accept(listener);
        if (cli == INVALID_SOCKET){
            Sleep(10);
            continue;
        }

        CalendarPacket pkt = {0};
        int r = net_recv_packet(cli, &pkt);
        if (r <= 0){
            net_close(cli);
            continue;
        }

        calendar_process(&pkt);

        (void)net_send_packet(cli, &pkt);
        net_close(cli);
    }

    net_close(listener);
    net_cleanup();
    return 0;
}