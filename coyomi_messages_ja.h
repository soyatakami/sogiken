//*coyomi_messages_ja.h

#ifndef COYOMI_MESSAGES_JA_H
#define COYOMI_MESSAGES_JA_H

// ── エラーメッセージ（ワイド文字列） ──
#define MSG_ERR_ID1 L"Connectが出来ない場合 \r\n\tCoyomiSeverアプリが起動しているか確認してください．"
#define MSG_ERR_ID2 L"開始日が有効範囲外です． \r\n\t2010/01/01～2099/12/31以外の範囲なので\r\n\t今日の日付を開始日に指定します．"
#define MSG_ERR_ID3 L"半角数字・正の数のみ入力可能です．\r\n\t無効地が入力された為，今日の日付を開始日に指定します．"
#define MSG_ERR_ID4 L"少数での日数の入力が有ったので．整数一桁目で切り上げました．"
#define MSG_ERR_ID5 L"日数の上限（10000日）を超えているので，10000日に変更しました．"
#define MSG_ERR_ID6 L"半角数字・正の整数以外の入力があった為，1日に変更しました．"
#define MSG_ERR_ID7 L"Sent．Receiveが連続で5回通信ができない場合 \r\n\t通信が確立できません．"

// ── GUI 表示テキスト（ワイド文字列） ──
#define GUI_Comment_1    L"開始日"
#define GUI_Comment_2    L"有効範囲\r\n2010/01/01～2099/12/31\r\n*半角数字・正の整数のみ入力可"
#define GUI_Comment_3    L"開始日を"
#define GUI_Comment_4    L"とする．"
#define GUI_Comment_5    L"開始日の"
#define GUI_Comment_6    L"日"
#define GUI_Comment_7    L"*半角数字・正の整数のみ入力可"
#define GUI_Comment_8    L"[メッセージ表示欄]"
#define GUI_Comment_9    L"[計算結果表示欄]"

#define GUI_Execution    L"実行"
#define GUI_End          L"終了"

#define GUI_RadioBox_Before L"前"
#define GUI_RadioBox_After  L"後"
#define GUI_RadioBox_Zero   L"0日目"
#define GUI_RadioBox_One    L"1日目"

#endif // COYOMI_MESSAGES_JA_H
