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
        unsigned char c = (unsigned char)*str;
        if (c < 0xA4 || c > 0xDF) return 0;  // カタカナの範囲
        str++;
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
void update_candidate_info(sqlite3 *db) {
    int id;
    char name[100], exam_day[100];
    const char *subjects[] = {
        "nLang", "math", "Eng", "JHist", "wHist", "geo", "phys", "chem", "bio"
    };
    int scores[9];

    printf("変更対象の受験者IDを入力してください: ");
    scanf("%d", &id);
    while (getchar() != '\n');

    // 氏名入力（最初に）
    while (1) {
        printf("新しい氏名（カタカナ20文字以内）: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0';
        if (strlen(name) > MAX_NAME_LEN || !is_katakana(name)) {
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
        printf("受験者情報を正常に更新しました。\n");
    } else {
        printf("更新に失敗しました: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);


}

int main(void) {
    sqlite3 *db;
    int rc = sqlite3_open("examdata.db", &db);  // 適宜データベース名変更

    if (rc != SQLITE_OK) {
        printf("データベースを開けません: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    update_candidate_info(db);

    sqlite3_close(db);
    return 0;
}
