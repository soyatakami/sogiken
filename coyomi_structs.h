//*coyomi_structs.h
//構造体だけにする，変なプロトコル宣言に使用しない
//モジュール毎に切り分けているため，二重で定義してしまうものが出てくる

#ifndef COYOMI_STRUCTS_H
#define COYOMI_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

// 入力値格納用（GUIから取得した“そのまま”）
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    char     ndays_text[32]; // 日数（UTF-8文字列）
    bool     flag01;         // 0/1日目（true=1日目）
    bool     dir_after;      // 前後（true=後, false=前）
} InputData;

// 送信用（ワイヤへ載せるデータ）
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t ndays;
    uint32_t flags;          // [weekday:bit8..10] [flag01:bit1] [dir:bit0]
} SentData;

// 受信用（サーバから返るデータ）
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t ndays;
    uint32_t flags;          // 同上
} ReceiveData;

#endif // COYOMI_STRUCTS_H
