#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <wchar.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include "update.h"

#define MAX_NAME_LEN 20

// UTF-8カタカナ検証
int validate_name_update(const char *name) {
    wchar_t wc;
    const char *ptr = name;
    size_t mblen;
    int char_count = 0;
    mbstate_t state;
    memset(&state, 0, sizeof(state));

    if (*name == '\0') return NAME_ERR_EMPTY;

    while (*ptr) {
        errno = 0;
        mblen = mbrtowc(&wc, ptr, MB_CUR_MAX, &state);
        if (mblen == (size_t)-1 || mblen == (size_t)-2) return NAME_ERR_INVALID_CHAR;
        if (!((wc >= 0x30A1 && wc <= 0x30FA) || wc == 0x30FC)) return NAME_ERR_INVALID_CHAR;
        char_count++;
        if (char_count > MAX_NAME_LEN) return NAME_ERR_LENGTH;
        ptr += mblen;
    }
    return NAME_VALID;
}

// 日付検証
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

// 点数入力
int input_score(const char *subject) {
    int score;
    while (1) {
        printf("%s の点数（0～100）: ", subject);
        if (scanf("%d", &score) != 1 || score < 0 || score > 100) {
            printf("無効な点数です。再入力してください。\n");
            while (getchar() != '\n');
        } else {
            while (getchar() != '\n');
            return score;
        }
    }
}

// 更新メイン
void examdata(sqlite3 *db) {
    int id;
    char name[100], exam_day[100];
    int scores[9];
    const char *subjects[] = {"nLang", "math", "Eng", "JHist", "wHist", "geo", "phys", "chem", "bio"};
    const char *subjects_jp[] = {"国語", "数学", "英語", "日本史", "世界史", "地理", "物理", "化学", "生物"};

    printf("【受験者情報変更】\n");
   // 氏名入力とバリデーション
    while(1) {
        printf("氏名を入力してください（カタカナ20文字以内）: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0';
        int res = validate_name_update(name);
        if (res == NAME_VALID) break;
        printf("無効な氏名です。再入力してください。\n");
    }

    // DBから名前で検索（最初の1件を取得）
   sqlite3_stmt *stmt_select;
    const char *sql_select =
        "SELECT ID, exam_day, nLang, math, Eng, JHist, wHist, geo, phys, chem, bio "
        "FROM testtable WHERE name = ? LIMIT 1";
    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt_select, NULL) != SQLITE_OK) {
        printf("SQLエラー: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_text(stmt_select, 1, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt_select);
    if (rc != SQLITE_ROW) {
        printf("該当する受験者が見つかりません。\n");
        sqlite3_finalize(stmt_select);
        return;
    }

    id = sqlite3_column_int(stmt_select, 0);
    int db_exam_day = sqlite3_column_int(stmt_select, 1);
    snprintf(exam_day, sizeof(exam_day), "%08d", db_exam_day);
    for (int i = 0; i < 9; i++) {
        scores[i] = sqlite3_column_int(stmt_select, 2 + i);
    }
    sqlite3_finalize(stmt_select);

     // ここから編集ループ（IDとnameは固定）
    int society_chosen = -1, science_chosen = -1;
    for (int i = 3; i <= 5; i++) if (scores[i] != 0) society_chosen = i;
    for (int i = 6; i <= 8; i++) if (scores[i] != 0) science_chosen = i;

    while (1) {
        printf("【現在の情報】\n");
        printf("1. 氏名: %s\n", name);
        printf("2. 試験日: %s\n", exam_day);
        for (int i = 0; i < 9; i++) {
        printf("%d. %s: ", i + 3, subjects_jp[i]);
        if (scores[i] == 0) {
            printf("入力されていません\n");
        } else {
            printf("%d\n", scores[i]);
        }
}
        int choice;
        printf("変更する項目の番号を入力（0で終了）: ");
        scanf("%d", &choice);
        while(getchar() != '\n');

        if (choice == 0) {
            printf("変更を終了します。\n");
            break;
        }

        if (choice < 1 || choice > 11) {
            printf("無効な番号です。\n");
            continue;
        }

        if (choice >= 6 && choice <= 11) {
            int idx = choice - 3;
            // 選択科目更新制限
            if (idx >= 3 && idx <= 5 && society_chosen != idx && society_chosen != -1) {
                printf("エラー: 他の科目が登録されています。削除機能で登録した科目を削除してください。\n");
                continue;
            }
            if (idx >= 6 && idx <= 8 && science_chosen != idx && science_chosen != -1) {
                printf("エラー: 他の科目が登録されています。削除機能で登録した科目を削除してください。\n");
                continue;
            }
        }

        switch(choice) {
            case 1:
                while(1) {
                    printf("新しい氏名（カタカナ20文字以内）: ");
                    fgets(name, sizeof(name), stdin);
                    name[strcspn(name, "\n")] = '\0';
                    int res = validate_name_update(name);
                    if (res == NAME_VALID) break;
                    printf("無効な氏名です。\n");
                }
                break;
            case 2:
                while(1) {
                    printf("新しい試験日（YYYYMMDD）: ");
                    fgets(exam_day, sizeof(exam_day), stdin);
                    exam_day[strcspn(exam_day, "\n")] = '\0';
                    if (is_valid_date(exam_day)) break;
                    printf("無効な日付です。\n");
                }
                break;
            case 3 ... 11: {
                int idx = choice - 3;
                int score = input_score(subjects_jp[idx]);
                if (idx <= 2 && score == -1) {
                    printf("エラー: 必須科目は選択解除できません。\n");
                    continue;
                }
                if (score == -1) score = 0; // 選択解除処理
                scores[idx] = score;
                if (idx >= 3 && idx <= 5) society_chosen = (score != 0) ? idx : -1;
                if (idx >= 6 && idx <= 8) science_chosen = (score != 0) ? idx : -1;
                break;
            }
        }

        sqlite3_stmt *stmt_update;
        char sql_update[256];
        if (choice == 1)
            strcpy(sql_update, "UPDATE testtable SET name=? WHERE ID=?");
        else if (choice == 2)
            strcpy(sql_update, "UPDATE testtable SET exam_day=? WHERE ID=?");
        else
            snprintf(sql_update, sizeof(sql_update),
                     "UPDATE testtable SET %s=? WHERE ID=?", subjects[choice - 3]);

        if (sqlite3_prepare_v2(db, sql_update, -1, &stmt_update, NULL) != SQLITE_OK) {
            printf("SQLエラー: %s\n", sqlite3_errmsg(db));
            return;
        }

        if (choice == 1)
            sqlite3_bind_text(stmt_update, 1, name, -1, SQLITE_STATIC);
        else if (choice == 2)
            sqlite3_bind_int(stmt_update, 1, atoi(exam_day));
        else
            sqlite3_bind_int(stmt_update, 1, scores[choice - 3]);
        sqlite3_bind_int(stmt_update, 2, id);

        if (sqlite3_step(stmt_update) == SQLITE_DONE)
            printf("更新成功。\n");
        else
            printf("更新失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_update);
    }
}
