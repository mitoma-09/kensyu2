#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

void trim_input(char *str) {
    char *end;
    end = str + strlen(str) - 1;
    while (end >= str && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t')) {
        *end = '\0';
        end--;
    }
}

int main() {
    setlocale(LC_ALL, "ja_JP.UTF-8");

    char test_str[] = "アイウエオカキクケコサシスセソタチツテトナ\n";
    trim_input(test_str);

    const char *ptr = test_str;
    wchar_t wc;
    size_t mblen;
    mbstate_t state;
    memset(&state, 0, sizeof(state));

    int char_count = 0;

    printf("テスト文字列: %s\n", test_str);
    printf("入力文字列のバイト長: %zu\n", strlen(test_str));

    for (size_t i = 0; i < strlen(test_str); i++) {
        printf("Byte[%zu]: %02X\n", i, (unsigned char)test_str[i]);
        if (test_str[i] < 0x20 || test_str[i] == 0x7F) {
            printf("不可視文字検出: Byte[%zu]: %02X\n", i, (unsigned char)test_str[i]);
        }
    }

    while (*ptr) {
        mblen = mbrtowc(&wc, ptr, MB_CUR_MAX, &state);
        if (mblen == (size_t)-1 || mblen == (size_t)-2) {
            printf("[DEBUG] mbrtowc エラー\n");
            break;
        }
        printf("[DEBUG] 読み取った文字: '%lc', バイト長: %zu\n", wc, mblen);

        char_count++;
        ptr += mblen;
    }

    printf("総文字数: %d\n", char_count);
    return 0;
}