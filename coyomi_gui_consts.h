//*coyomi_gui_consts.h

#ifndef COYOMI_GUI_CONSTS_H
#define COYOMI_GUI_CONSTS_H

// IDC_DTP … DateTimePicker の子コントロールID（DTM_GETSYSTEMTIME 等に使う）
// IDC_FLAG0 / IDC_FLAG1 … 「0日目/1日目」のラジオボタン
// IDC_DIR_BACK / IDC_DIR_FWD … 「前/後」のラジオボタン
// IDC_NDAYS … 日数入力 EDIT のID
// IDC_SEND … 「実行」ボタンのID（WM_COMMANDで押下判定）
// IDC_EXIT … 「終了」ボタンのID
// IDC_STATUS … メッセージ表示用 EDIT のID
// IDC_OUTALL … 結果表示用 EDIT のID

enum {
    IDC_NDAYS = 1100,
    IDC_DIR_BACK,
    IDC_DIR_FWD,
    IDC_FLAG0,
    IDC_FLAG1,
    IDC_SEND,
    IDC_OUTALL,
    IDC_STATUS,
    IDC_EXIT,
    IDC_DTP,
    // 必要なら他のIDもここに追加可能
};

// ── ウィンドウ（クライアント基準）寸法計算用 ──
#define GUI_SIZE     300
#define GUI_RATIO_X  10
#define GUI_RATIO_Y  18

// ── 入力＆エラー制御（許容範囲） ──
#define MIN_Y 2010
#define MIN_M 1
#define MIN_D 1
#define MAX_Y 2099
#define MAX_M 12
#define MAX_D 31

// ── flags 割り当て ──
#define FLAG_DIR_MASK     0x00000001u
#define FLAG_FLAG01_MASK  0x00000002u
#define FLAG_WD_MASK      0x00000700u
#define FLAG_WD_SHIFT     8

// ── Wire/Packet ──
#define PACKET_WORDS 6 // [year,month,day,ndays,flags,error]

// ── 位置・サイズ・フォントpt（元の変数名を #define 化） ──
#define GUI_Comment_1_Xp 20
#define GUI_Comment_1_Yp 25
#define GUI_Comment_1_Xs 60
#define GUI_Comment_1_Ys 20
#define GUI_Comment_1_pt 12

#define GUI_Calendar_Xp 100
#define GUI_Calendar_Yp 20
#define GUI_Calendar_Xs 200
#define GUI_Calendar_Ys 30
#define GUI_Calendar_pt 12

#define GUI_Comment_2_Xp 320
#define GUI_Comment_2_Yp 20
#define GUI_Comment_2_Xs 200
#define GUI_Comment_2_Ys 35
#define GUI_Comment_2_pt 6

#define GUI_Comment_3_Xp 20
#define GUI_Comment_3_Yp 75
#define GUI_Comment_3_Xs 75
#define GUI_Comment_3_Ys 20
#define GUI_Comment_3_pt 12

#define GUI_RadioBox_Zero_Xp 100
#define GUI_RadioBox_Zero_Yp 75
#define GUI_RadioBox_Zero_Xs 75
#define GUI_RadioBox_Zero_Ys 20
#define GUI_RadioBox_Zero_pt 12

#define GUI_RadioBox_One_Xp 180
#define GUI_RadioBox_One_Yp 75
#define GUI_RadioBox_One_Xs 75
#define GUI_RadioBox_One_Ys 20
#define GUI_RadioBox_One_pt 12

#define GUI_Comment_4_Xp 260
#define GUI_Comment_4_Yp 75
#define GUI_Comment_4_Xs 75
#define GUI_Comment_4_Ys 20
#define GUI_Comment_4_pt 12

#define GUI_Comment_5_Xp 20
#define GUI_Comment_5_Yp 125
#define GUI_Comment_5_Xs 75
#define GUI_Comment_5_Ys 20
#define GUI_Comment_5_pt 12

#define GUI_ShiftDays_Xp 100
#define GUI_ShiftDays_Yp 125
#define GUI_ShiftDays_Xs 75
#define GUI_ShiftDays_Ys 25
#define GUI_ShiftDays_pt 12

#define GUI_Comment_6_Xp 180
#define GUI_Comment_6_Yp 125
#define GUI_Comment_6_Xs 25
#define GUI_Comment_6_Ys 20
#define GUI_Comment_6_pt 12

#define GUI_RadioBox_Before_Xp 210
#define GUI_RadioBox_Before_Yp 110
#define GUI_RadioBox_Before_Xs 50
#define GUI_RadioBox_Before_Ys 20
#define GUI_RadioBox_Before_pt 12

#define GUI_RadioBox_After_Xp 210
#define GUI_RadioBox_After_Yp 140
#define GUI_RadioBox_After_Xs 50
#define GUI_RadioBox_After_Ys 20
#define GUI_RadioBox_After_pt 12

#define GUI_Comment_7_Xp 20
#define GUI_Comment_7_Yp 155
#define GUI_Comment_7_Xs 200
#define GUI_Comment_7_Ys 12
#define GUI_Comment_7_pt 6

#define GUI_Execution_Xp 270
#define GUI_Execution_Yp 125
#define GUI_Execution_Xs 100
#define GUI_Execution_Ys 25
#define GUI_Execution_pt 12

#define GUI_End_Xp 400
#define GUI_End_Yp 125
#define GUI_End_Xs 100
#define GUI_End_Ys 25
#define GUI_End_pt 12

#define GUI_Comment_8_Xp 20
#define GUI_Comment_8_Yp 170
#define GUI_Comment_8_Xs 150
#define GUI_Comment_8_Ys 20
#define GUI_Comment_8_pt 12

#define GUI_Comment_9_Xp 270
#define GUI_Comment_9_Yp 170
#define GUI_Comment_9_Xs 150
#define GUI_Comment_9_Ys 20
#define GUI_Comment_9_pt 12

#define GUI_Message_Xp 20
#define GUI_Message_Yp 200
#define GUI_Message_Xs 225
#define GUI_Message_Ys 75
#define GUI_Message_pt 6

#define GUI_OutAll_Xp 270
#define GUI_OutAll_Yp 200
#define GUI_OutAll_Xs 225
#define GUI_OutAll_Ys 75
#define GUI_OutAll_pt 10

#endif // COYOMI_GUI_CONSTS_H
