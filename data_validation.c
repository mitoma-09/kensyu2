#include <stdio.h>
#include <string.h>
#include <stdlib.h> // atoiのために必要
#include <ctype.h>  // tolowerのために必要 (今回は不要だが、元のコードにあったので念のため)
#include "data_validation.h" // 自身のヘッダーファイルをインクルード

// 名前入力チェック
// ※元のコードのvalidate_nameは全角カタカナチェックが厳しすぎる可能性があります。
//   UTF-8のマルチバイト文字は1バイト文字とは異なり、単純なASCII範囲外では判断できません。
//   ここでは元のロジックをそのまま使用しますが、日本語の全角文字の判定は複雑です。
//   必要であれば、ライブラリ (例: ICU) を使うか、より堅牢なロジックを検討してください。
int validate_name(const char *name) {
    int len = 0;
    // UTF-8 文字数を正確にカウントするのは困難なため、ここではバイト長を元に概算
    // または、文字数制限であれば、より正確なUTF-8文字数カウント関数が必要
    // ここでは単純にバイト長で判断します
    len = strlen(name);
    if (len > 60) { // UTF-8で20文字だとすると、最大60バイト (1文字3バイトの場合)
        printf("エラー: 名前は20文字以内で入力してください。\n");
        return 0;
    }
    
    // 全角カタカナチェック (元のロジックを流用)
    // 注意: このロジックはUTF-8エンコーディングの全角カタカナのみを想定しており、
    // その他の全角文字（ひらがな、漢字、記号など）や半角文字を許可しません。
    // また、Windows環境では `locale.h` や `setlocale` の設定に依存する場合があります。
    const unsigned char *p = (const unsigned char *)name;
    while (*p) {
        // ASCII文字（半角文字）のチェック
        if (*p < 0x80) {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
            return 0;
        } 
        // UTF-8 3バイト文字のチェック（全角文字の大部分）
        else if ((*p & 0xF0) == 0xE0) { // 3バイト文字の開始バイト
            if (p[1] == '\0' || p[2] == '\0') { // 不完全なUTF-8シーケンス
                printf("エラー: 不正な文字が含まれています。\n");
                return 0;
            }
            unsigned int codepoint = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            // カタカナのUnicode範囲 (U+30A0 ～ U+30FF) と長音記号 (U+30FC)
            if (!((codepoint >= 0x30A0 && codepoint <= 0x30FF) || codepoint == 0x30FC)) {
                printf("エラー: 名前は全角カタカナで入力してください。\n");
                return 0;
            }
            p += 3; // 3バイト進む
        } 
        // その他のマルチバイト文字（4バイト文字など）や不正なシーケンス
        else {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
            return 0;
        }
    }
    return 1;
}

// 日付入力チェック
int validate_date(const char *date) {
    if (strlen(date) != 8 || strspn(date, "0123456789") != 8) {
        printf("エラー: 日付は8桁の数字で入力してください（例: 20250513）。\n");
        return 0;
    }
    char year_str[5], month_str[3], day_str[3];
    strncpy(year_str, date, 4);
    year_str[4] = '\0';
    strncpy(month_str, date + 4, 2);
    month_str[2] = '\0';
    strncpy(day_str, date + 6, 2);
    day_str[2] = '\0';

    int year = atoi(year_str);
    int month = atoi(month_str);
    int day = atoi(day_str);

    if (month < 1 || month > 12) {
        printf("エラー: 存在しない月です。\n");
        return 0;
    }
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    // うるう年判定
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        days_in_month[1] = 29; // 2月を29日に
    }
    if (day < 1 || day > days_in_month[month - 1]) {
        printf("エラー: 存在しない日です。\n");
        return 0;
    }
    return 1;
}

// 点数入力チェック
int validate_score(int score) {
    if (score < 0 || score > 100) {
        printf("エラー: 点数は0〜100の範囲で入力してください。\n");
        return 0;
    }
    return 1;
}