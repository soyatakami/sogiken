//* InvalidValueHandling.c

// InvalidValueHandling.c
// - InputData の年月日＋日数を検証
// - 年月日が範囲外 or 日数入力が「半角数字・正の数」違反なら、開始日を「今日」に差し替え
// - 日数は 小数→切り上げ、上限 10000 まで補正
// - 補正結果で SentData を構成（flags は dir_after/flag01 から）
// - エラーIDの 0→1 更新は on_error(ErrorID) コールバックで通知（例: record_error）

// InvalidValueHandling.c
// 入力(InputData)の妥当性チェックと補正を行い、最終送信用 SentData を確定する。
// - カレンダー日付が範囲外なら今日に差し替え（ERR_ID2）
// - 日数入力の検証：空/不正/<=0 → 1日に補正（ERR_ID6）＋「正の数のみ」（ERR_ID3）
// - 小数は切り上げ（ERR_ID4）
// - 上限 10000 に丸め（ERR_ID5）
// - 必要なエラーは on_error_raised コールバックで 0→1 へ更新を依頼
// - DTP を今日に合わせる必要がある場合は out_set_dtp_today=true を返す
// - 週番号はサーバ側で付与する想定なので flags の weekday は 0 固定

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <wchar.h>
#include <stdlib.h>   // wcstod
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "coyomi_all.h"  // ここに ErrorID / 構造体 / 定数 / プロトタイプがまとまっている

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif

// ---- ユーティリティ ----
static inline bool in_allowed_range_yMd(int y,int m,int d){
    if (y < MIN_Y || y > MAX_Y) return false;
    if (y == MIN_Y){
        if (m < MIN_M) return false;
        if (m == MIN_M && d < MIN_D) return false;
    }
    if (y == MAX_Y){
        if (m > MAX_M) return false;
        if (m == MAX_M && d > MAX_D) return false;
    }
    return true;
}

static inline uint32_t make_flags(bool dir_after, bool flag01){
    uint32_t f = 0u;
    if (dir_after) f |= FLAG_DIR_MASK;
    if (flag01)    f |= FLAG_FLAG01_MASK;
    // weekday はサーバ側設定の想定なので 0 固定
    return f;
}

// ndays の UTF-8 テキストを検証・補正して返す
// - invalid や <=0 → 1（日）に補正し ERR_ID6, さらに ERR_ID3（正の数のみ）を発火
// - 少数は切り上げ → ERR_ID4
// - 上限 10000 → ERR_ID5
// 返り値: 補正後 ndays
static uint32_t parse_and_fix_ndays_utf8(const char* utf8,
                                         void (*on_error_raised)(int),
                                         bool* out_need_today)
{
    if (out_need_today) *out_need_today = false;

    // 空/NULL
    if (!utf8 || !*utf8){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    // UTF-8 → wchar_t
    wchar_t wbuf[64];
    int wn = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, _countof(wbuf));
    if (wn <= 0){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    // 先頭の空白をスキップ
    const wchar_t* s = wbuf;
    while(*s==L' '||*s==L'\t'||*s==L'\r'||*s==L'\n') ++s;
    if (!*s){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    bool has_dot = (wcschr(s, L'.') != NULL);
    wchar_t* endp = NULL;
    double val = wcstod(s, &endp);

    // 余剰文字の有無を確認（空白以外が残っていたら NG）
    bool extra = false;
    if (endp){
        while(*endp){
            if (!iswspace(*endp)) { extra = true; break; }
            ++endp;
        }
    }else{
        extra = true;
    }

    if (!isfinite(val) || extra){
        if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
        if (out_need_today) *out_need_today = true;
        return 1u;
    }

    uint32_t n;
    if (has_dot){
        // 少数 → 切り上げ
        double c = ceil(val);
        if (c < 1.0){
            if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
            if (out_need_today) *out_need_today = true;
            return 1u;
        }
        if (c > 1e9) c = 1e9; // 過剰防衛
        n = (uint32_t)c;
        if (on_error_raised) on_error_raised(ERR_ID4_ROUNDUP);
    }else{
        // 整数期待
        if (val <= 0.0){
            if (on_error_raised){ on_error_raised(ERR_ID6_INVALID_INT); on_error_raised(ERR_ID3_POSITIVE_ONLY); }
            if (out_need_today) *out_need_today = true;
            return 1u;
        }
        if (val > 1e9) val = 1e9;
        n = (uint32_t)val;
    }

    if (n > 10000u){
        n = 10000u;
        if (on_error_raised) on_error_raised(ERR_ID5_CAP_10000);
    }
    return n;
}

// ---- 公開 API 実装（coyomi_all.h に宣言済み） ----

void InvalidValueHandling_GetTodayTime(SYSTEMTIME* out_now){
    if (!out_now) return;
    GetLocalTime(out_now);
}

void InvalidValueHandling_InvalidValueHandling(
    const InputData* in,
    SentData* out_sd,
    bool* out_set_dtp_today,
    wchar_t* opt_errbuf, size_t opt_errcap, // 使わない場合は NULL,0 で OK
    void (*on_error_raised)(int error_id)
){
    if (out_set_dtp_today) *out_set_dtp_today = false;
    if (out_sd) memset(out_sd, 0, sizeof(*out_sd));
    (void)opt_errbuf; (void)opt_errcap; // 今回は文字列構築なし

    if (!in || !out_sd){
        // 入力または出力先が無い場合は、今日/ndays=1/フラグ0 で返す
        SYSTEMTIME now; GetLocalTime(&now);
        out_sd->year  = (uint32_t)now.wYear;
        out_sd->month = (uint32_t)now.wMonth;
        out_sd->day   = (uint32_t)now.wDay;
        out_sd->ndays = 1u;
        out_sd->flags = 0u;
        if (out_set_dtp_today) *out_set_dtp_today = true;
        return;
    }

    // 1) 開始日の検証（範囲外なら今日に差し替え）
    bool need_today_by_date = false;
    uint32_t y = in->year, m = in->month, d = in->day;
    if (!in_allowed_range_yMd((int)y,(int)m,(int)d)){
        need_today_by_date = true;
        if (on_error_raised) on_error_raised(ERR_ID2_START_OUT_OF_RANGE);
    }

    SYSTEMTIME today;
    if (need_today_by_date){
        GetLocalTime(&today);
        y = (uint32_t)today.wYear;
        m = (uint32_t)today.wMonth;
        d = (uint32_t)today.wDay;
    }

    // 2) 日数の検証・補正
    bool need_today_by_ndays = false;
    uint32_t nd = parse_and_fix_ndays_utf8(in->ndays_text, on_error_raised, &need_today_by_ndays);

    // ndays 側が NG のときも開始日は今日に差し替える
    if (need_today_by_ndays && !need_today_by_date){
        GetLocalTime(&today);
        y = (uint32_t)today.wYear;
        m = (uint32_t)today.wMonth;
        d = (uint32_t)today.wDay;
    }

    // 3) flags（weekday は 0 固定）
    uint32_t flags = make_flags(in->dir_after, in->flag01);

    // 4) 送信用構造体に確定値を格納
    out_sd->year  = y;
    out_sd->month = m;
    out_sd->day   = d;
    out_sd->ndays = nd;
    out_sd->flags = flags;

    // 5) DTP を今日に合わせるべきかの通知
    if (out_set_dtp_today) *out_set_dtp_today = (need_today_by_date || need_today_by_ndays);
}
