#include <stdio.h>

// �j���̖��O
const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// ���̔N�����邤�N���ǂ�������
int isLeapYear(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

// ���̓�����Ԃ�
int getDaysInMonth(int year, int month) {
    int days_in_month[] = {31, 28, 31, 30, 31, 30,
                           31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year))
        return 29;
    return days_in_month[month - 1];
}

// �c�F���[�̌����ŗj�������߂�i0=�y�j, 1=���j, ..., 6=���j�j
int getWeekday(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int h = (day + 13*(month + 1)/5 + year + year/4 - year/100 + year/400) % 7;
    return (h + 6) % 7; // 0=���j �ɕϊ�
}

// �J�����_�[��\��
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
        printf("    "); // ���j����n�܂�̂ŋ󔒂Ŗ��߂�
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

// ���C���֐�
int main() {
    int year, month;
    printf("�N����͂��Ă�������: ");
    fflush(stdout);
    scanf("%d", &year);
    printf("������͂��Ă������� (1-12): ");
    fflush(stdout);
    scanf("%d", &month);

    if (month < 1 || month > 12) {
        printf("�����Ȍ��ł��B\n");
        return 1;
    }

    printCalendar(year, month);

    return 0;
}
