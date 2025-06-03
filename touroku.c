#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <sqlite3.h>

#define SUBJECT_COUNT 9

// --- グローバル定義 ---
const char* subjects_ja[] = {"国語", "数学", "英語", "日本史", "世界史", "地理", "物理", "化学", "生物"};
const int liberal_indices[] = {3,4,5};  // 文系科目インデックス
const int science_indices[] = {6,7,8};  // 理系科目インデックス

// --- DB処理 ---
sqlite3* connect_to_database(const char *filename) {
    sqlite3 *db;
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "データベース接続に失敗しました: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS testtable ("
        "name TEXT NOT NULL, "
        "exam_day INTEGER NOT NULL, "
        "nLang INTEGER, "
        "math INTEGER, "
        "Eng INTEGER, "
        "JHist INTEGER, "
        "wHist INTEGER, "
        "geo INTEGER, "
        "phys INTEGER, "
        "chem INTEGER, "
        "bio INTEGER, "
        "ID INTEGER PRIMARY KEY AUTOINCREMENT"
        ");";

    char *errmsg = NULL;
    if (sqlite3_exec(db, create_table_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "テーブル作成に失敗しました: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(1);
    }
    return db;
}

int is_duplicate(sqlite3 *db, const char *name, int exam_day) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM testtable WHERE name = ? AND exam_day = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "重複チェック準備失敗: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, exam_day);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return (count > 0);
}

int register_data(sqlite3 *db, const char *name, int exam_day, int scores[]) {
    sqlite3_stmt *stmt;
    const char *insert_sql =
        "INSERT INTO testtable "
        "(name, exam_day, nLang, math, Eng, JHist, wHist, geo, phys, chem, bio) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "データ登録の準備に失敗しました: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, exam_day);
    for (int i = 0; i < SUBJECT_COUNT; i++) {
        if (scores[i] == -1) {
            sqlite3_bind_null(stmt, 3 + i);
        } else {
            sqlite3_bind_int(stmt, 3 + i, scores[i]);
        }
    }

    int result = sqlite3_step(stmt);
    if (result == SQLITE_DONE) {
        printf("データが正常に登録されました。\n");
    } else {
        fprintf(stderr, "データ登録に失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }
    sqlite3_finalize(stmt);

    // 挿入された行のIDを取得
    int id = sqlite3_last_insert_rowid(db);
    return id;
}

// --- バリデーション関数 ---

int validate_name(const char *name) {
    int len = strlen(name);
    if (len > 60) {
        printf("エラー: 名前は20文字以内で入力してください（全角カタカナ）。\n");
        return 0;
    }
    const unsigned char *p = (const unsigned char *)name;
    while (*p) {
        if (*p < 0x80) {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
            return 0;
        } else if ((*p & 0xF0) == 0xE0) {
            unsigned int codepoint = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            if (!((codepoint >= 0x30A0 && codepoint <= 0x30FF) || codepoint == 0x30FC)) {
                printf("エラー: 名前は全角カタカナで入力してください。\n");
                return 0;
            }
            p += 3;
        } else {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
            return 0;
        }
    }
    return 1;
}

int validate_date(const char *date) {
    if (strlen(date) != 8 || strspn(date, "0123456789") != 8) {
        printf("エラー: 日付は8桁の数字で入力してください（例: 20250513）。\n");
        return 0;
    }
    char year_str[5], month_str[3], day_str[3];
    strncpy(year_str, date, 4); year_str[4] = '\0';
    strncpy(month_str, date + 4, 2); month_str[2] = '\0';
    strncpy(day_str, date + 6, 2); day_str[2] = '\0';

    int year = atoi(year_str);
    int month = atoi(month_str);
    int day = atoi(day_str);

    if (month < 1 || month > 12) {
        printf("エラー: 存在しない月です。\n");
        return 0;
    }
    int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        days_in_month[1] = 29;
    }
    if (day < 1 || day > days_in_month[month - 1]) {
        printf("エラー: 存在しない日です。\n");
        return 0;
    }
    return 1;
}

int validate_score(int score) {
    if (score < 0 || score > 100) {
        printf("エラー: 点数は0〜100の範囲で入力してください。\n");
        return 0;
    }
    return 1;
}

// --- 文系科目登録済み判定 ---
int is_liberal_registered(int reg[]) {
    for (int i = 0; i < 3; i++) {
        if (reg[liberal_indices[i]]) return 1;
    }
    return 0;
}

// --- 理系科目登録済み判定 ---
int is_science_registered(int reg[]) {
    for (int i = 0; i < 3; i++) {
        if (reg[science_indices[i]]) return 1;
    }
    return 0;
}

int main() {
    setlocale(LC_ALL, "ja_JP.UTF-8");

    sqlite3 *db = connect_to_database("testmanager.db");

    char name[100];
    char exam_date_str[9];
    int exam_day;
    int scores[SUBJECT_COUNT];
    int registered[SUBJECT_COUNT] = {0};

    // 点数初期化：-1は未入力
    for (int i = 0; i < SUBJECT_COUNT; i++) {
        scores[i] = -1;
    }

    // 名前入力（全角カタカナ）
    do {
        printf("名前を全角カタカナで入力してください（20文字以内）: ");
        if (fgets(name, sizeof(name), stdin) == NULL) {
            fprintf(stderr, "名前入力エラー\n");
            sqlite3_close(db);
            return 1;
        }
        name[strcspn(name, "\n")] = '\0';
    } while (!validate_name(name));

    // 試験日入力
    do {
        printf("試験日を8桁で入力してください（例: 20250513）: ");
        if (scanf("%8s", exam_date_str) != 1) {
            fprintf(stderr, "試験日入力エラー\n");
            sqlite3_close(db);
            return 1;
        }
        getchar(); // 改行消費
    } while (!validate_date(exam_date_str));

    exam_day = atoi(exam_date_str);

    // 重複チェック
    if (is_duplicate(db, name, exam_day)) {
        printf("エラー: 同じ名前と試験日のデータが既に存在します。\n");
        sqlite3_close(db);
        return 1;
    }

    int registered_count = 0;

    while (1) {
        printf("\n--- 科目一覧 ---\n");
        for (int i = 0; i < SUBJECT_COUNT; i++) {
            printf(" %d: %s", i + 1, subjects_ja[i]);
            if (registered[i]) {
                printf(" [登録済み]");
            }
            printf("\n");
        }
        printf("------------------\n");

        if (registered_count >= 5) {
            printf("最大5科目の登録に達しました。これ以上登録できません。\n");
            break;
        }
    }

       while (1) {
    printf("\n--- 科目一覧 ---\n");
    for (int i = 0; i < SUBJECT_COUNT; i++) {
        printf(" %d: %s", i + 1, subjects_ja[i]);
        if (registered[i]) {
            printf(" [登録済み]");
        }
        printf("\n");
    }
    printf("------------------\n");

    if (registered_count >= 5) {
        printf("最大5科目の登録に達しました。これ以上登録できません。\n");
        break;
    }

    while (1) {
        printf("科目を選択してください（1〜9、終了は0）: ");
        char input[10]; // 入力バッファ
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("入力エラー：もう一度入力してください。\n");
            continue; // 再入力を促す
        }

        // 改行を削除
        input[strcspn(input, "\n")] = '\0';

        // 数値チェック
        char *endptr;
        int sel = strtol(input, &endptr, 10);
        if (*endptr != '\0' || sel < 0 || sel > 9) {
            printf("エラー: 0〜9の数字を入力してください。\n");
            continue; // 再入力を促す
        }

        if (sel == 0) {
            goto end_program; // プログラム終了
        }

        int idx = sel - 1;

        if (registered[idx]) {
            printf("科目「%s」は既に登録済みです。\n", subjects_ja[idx]);
            continue;
        }

        // 文系科目は一つだけ
        int is_liberal = 0;
        for (int i = 0; i < 3; i++) {
            if (idx == liberal_indices[i]) {
                is_liberal = 1;
                break;
            }
        }
        if (is_liberal && is_liberal_registered(registered)) {
            printf("文系科目（日本史、世界史、地理）は一つだけ選択可能です。\n");
            continue;
        }

        // 理系科目は一つだけ
        int is_science = 0;
        for (int i = 0; i < 3; i++) {
            if (idx == science_indices[i]) {
                is_science = 1;
                break;
            }
        }
        if (is_science && is_science_registered(registered)) {
            printf("理系科目（物理、化学、生物）は一つだけ選択可能です。\n");
            continue;
        }

        int score;
        do {
            printf("点数を入力してください（0〜100）: ");
            if (scanf("%d", &score) != 1) {
                printf("整数を入力してください。\n");
                while (getchar() != '\n'); // 入力バッファをクリア
                continue;
            }
            while (getchar() != '\n'); // 改行を消費
        } while (!validate_score(score));

        scores[idx] = score;
        registered[idx] = 1;
        registered_count++;

        printf("科目「%s」に点数 %d を登録しました。\n", subjects_ja[idx], score);

        if (registered_count >= 5) {
            printf("最大登録科目数に達しました。\n");
            break;
        }
    }

    while (1) {
        printf("他の科目を登録しますか？（y/n）: ");
        char yn[10]; // 入力バッファ
        if (fgets(yn, sizeof(yn), stdin) == NULL) {
            printf("入力エラー\n");
            continue; // 再入力を促す
        }

        // 最初の文字で判定
        if (yn[0] == 'y' || yn[0] == 'Y') {
            break; // 次の登録に進む
        } else if (yn[0] == 'n' || yn[0] == 'N') {
            goto end_program; // プログラム終了
        } else {
            printf("エラー: 'y' または 'n' を入力してください。\n");
        }
    }
}

// end_program ラベルでプログラムの終了部分にジャンプ
end_program:
sqlite3_close(db);
printf("プログラム終了\n");
return 0;
}