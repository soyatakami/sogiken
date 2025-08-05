#include <stdio.h>

// 曜日の名前
const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// その年がうるう年かどうか判定
int isLeapYear(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

// 月の日数を返す
int getDaysInMonth(int year, int month) {
    int days_in_month[] = {31, 28, 31, 30, 31, 30,
                           31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year))
        return 29;
    return days_in_month[month - 1];
}

// ツェラーの公式で曜日を求める（0=土曜, 1=日曜, ..., 6=金曜）
int getWeekday(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int h = (day + 13*(month + 1)/5 + year + year/4 - year/100 + year/400) % 7;
    return (h + 6) % 7; // 0=日曜 に変換
}

// カレンダーを表示
void printCalendar(int year, int month) {
    printf("      %d / %d\n", year, month);
    fflush(stdout);
    for (int i = 0; i < 7; i++)
        printf("%s ", weekdays[i]);
    fflush(stdout);
    printf("\n");
    fflush(stdout);

    int start_day = getWeekday(year, month, 1);
    int days = getDaysInMonth(year, month);

    for (int i = 0; i < start_day; i++)
        printf("    "); // 日曜から始まるので空白で埋める
    fflush(stdout);

    for (int date = 1; date <= days; date++) {
        printf("%3d ", date);
        fflush(stdout);
        if ((start_day + date) % 7 == 0)
            printf("\n");
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
}

// メイン関数
int main() {
    int year, month;
    printf("年を入力してください: ");
    fflush(stdout);
    scanf("%d", &year);
    printf("月を入力してください (1-12): ");
    fflush(stdout);
    scanf("%d", &month);

    if (month < 1 || month > 12) {
        printf("無効な月です。\n");
        return 1;
    }

    printCalendar(year, month);

    return 0;
}
