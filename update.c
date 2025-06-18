#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ctype.h>
#include <time.h>
#include "update.h"

#define MAX_NAME_LEN 20

// カタカナチェック
int is_katakana(const char *str) {
    while (*str) {
        if ((unsigned char)str[0] == 0xE3 &&
            ((unsigned char)str[1] == 0x82 || (unsigned char)str[1] == 0x83)) {
            str += 3; // 3バイト文字なので次へ
        } else {
            return 0;
        }
    }
    return 1;
}

// 日付チェック（YYYYMMDDが正当な日付か）
int is_valid_date(const char *str) {
    if (strlen(str) != 8) return 0;
    for (int i = 0; i < 8; i++) {
        if (!isdigit(str[i])) return 0;
    }
    int y, m, d;
    sscanf(str, "%4d%2d%2d", &y, &m, &d);
    struct tm t = {0};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    time_t epoch = mktime(&t);
    return epoch != -1 && t.tm_mday == d && t.tm_mon == m - 1;
}

// 各科目点数の入力
int input_score(const char *subject) {
    int score;
    while (1) {
        printf("%s の点数（0～100）: ", subject);
        if (scanf("%d", &score) != 1 || score < 0 || score > 100) {
            printf("無効な点数です。再入力してください。\n");
            while (getchar() != '\n');
        } else {
            while (getchar() != '\n'); // バッファクリア
            return score;
        }
    }
}

// メイン更新関数
void examdata(sqlite3 *db) {
    int subchoice;
    printf("【受験者情報変更】\n");
    printf("1. 変更を行う\n");
    printf("2. メインメニューに戻る\n");
    printf("選択してください（1 または 2）: ");
    if (scanf("%d", &subchoice) != 1) {
        printf("無効な入力です。メインメニューに戻ります。\n");
        while(getchar() != '\n');
        return;
    }

    if (subchoice == 2) {
        printf("メインメニューに戻ります。\n");
        while(getchar() != '\n');
        return;
    } else if (subchoice != 1) {
        printf("無効な選択肢です。メインメニューに戻ります。\n");
        while(getchar() != '\n');
        return;
    }

    while(getchar() != '\n'); // 改行除去
    int id;
    char name[100], exam_day[100];
    const char *subjects[] = {
        "nLang", "math", "Eng", "JHist", "wHist", "geo", "phys", "chem", "bio"
    };
    int scores[9];

    printf("既登録受験者情報の変更ができます。\n");
    printf("変更対象の受験者IDを入力してください: ");
    scanf("%d", &id);
    while (getchar() != '\n');

    // 氏名入力（最初に）
    while (1) {
    printf("新しい氏名（カタカナ20文字以内）: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    // 🔽 ここがデバッグ出力です 🔽
    printf("DEBUG: 入力された氏名 = [%s]\n", name);
    printf("DEBUG: is_katakana(name) = %d\n", is_katakana(name));
    printf("DEBUG: strlen(name) = %zu\n", strlen(name));

    if (strlen(name) > MAX_NAME_LEN ) {
        printf("カタカナ20文字以内で再入力してください。\n");
    } else {
        break;
    }
}

    // 試験日入力
    while (1) {
        printf("新しい試験日（YYYYMMDD）: ");
        fgets(exam_day, sizeof(exam_day), stdin);
        exam_day[strcspn(exam_day, "\n")] = '\0';
        if (!is_valid_date(exam_day)) {
            printf("無効な日付です。再入力してください。\n");
        } else {
            break;
        }
    }

    // 各科目点数入力
    for (int i = 0; i < 9; i++) {
        scores[i] = input_score(subjects[i]);
    }

    // SQLite更新文の準備
    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE testtable SET name=?, exam_day=?, nLang=?, math=?, Eng=?, "
        "JHist=?, wHist=?, geo=?, phys=?, chem=?, bio=? WHERE ID=?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("SQLエラー: %s\n", sqlite3_errmsg(db));
        return;
    }

    // パラメータのバインド
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, atoi(exam_day));
    for (int i = 0; i < 9; i++) {
        sqlite3_bind_int(stmt, 3 + i, scores[i]);
    }
    sqlite3_bind_int(stmt, 12, id);

    // 実行
    if (sqlite3_step(stmt) == SQLITE_DONE) {
    int changes = sqlite3_changes(db);
    if (changes > 0) {
        printf("受験者情報を正常に更新しました。（%d件変更）\n", changes);
    } else {
        printf("該当するIDが存在しませんでした。更新は行われませんでした。\n");
    }
} else {
    printf("更新に失敗しました: %s\n", sqlite3_errmsg(db));
}

    sqlite3_finalize(stmt);


}

/*
// ★この main 関数は、メイン機能 (main.c など) と結合したためおそらく不要。
int main(void) {
    sqlite3 *db;
    int rc = sqlite3_open("examdata.db", &db);  // 適宜データベース名変更

    if (rc != SQLITE_OK) {
        printf("データベースを開けません: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    examdata(db);

    sqlite3_close(db);
    return 0;
} 
*/