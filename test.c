#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <string.h> // memsetのため
#include <stdlib.h> // MB_CUR_MAXのため

int main() {
    setlocale(LC_ALL, "ja_JP.UTF-8");

    const char *test_str = "アイウエオカキクケコサシスセソタチツテトナ";
    const char *ptr = test_str;
    wchar_t wc;
    size_t mblen;
    mbstate_t state;
    memset(&state, 0, sizeof(state)); // 初期化

    int char_count = 0;

    printf("テスト文字列: %s\n", test_str);

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
