#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <sqlite3.h>
#include <wchar.h>
#include <ctype.h>
#include "database.h" 

#define SUBJECT_COUNT 9
#define LIBERAL_START_INDEX 3
#define LIBERAL_END_INDEX 5
#define SCIENCE_START_INDEX 6
#define SCIENCE_END_INDEX 8

// --- グローバル定義 ---
const char* subjects_ja[] = {"国語", "数学", "英語", "日本史", "世界史", "地理", "物理", "化学", "生物"};
const int liberal_indices[] = {3,4,5};  // 文系科目インデックス
const int science_indices[] = {6,7,8};  // 理系科目インデックス

// --- DB処理 ---

// 受験者が同じ試験日に登録されているかを確認する関数
int is_duplicate(sqlite3 *db, const char *name, int exam_day) {
    sqlite3_stmt *stmt;

    // 名前と試験日が一致するデータの件数を確認するSQL文
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s WHERE name = ? AND exam_day = ?;", DATABASE_TABLENAME);

    // SQL文を準備（プリコンパイル）
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "重複チェック用の準備に失敗: %s\n", sqlite3_errmsg(db));
        return 1; // エラーが発生した場合
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, exam_day);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0); // 件数を取得
    }

    sqlite3_finalize(stmt); // ステートメントを解放
    return (count > 0);     // 件数が0より大きい場合は重複がある
}

// データベースに登録されている受験者の総件数を取得する関数
int get_registered_count(sqlite3 *db) {
    sqlite3_stmt *stmt;
    char sql[256];
    // テーブル内のデータ件数を取得するSQL文
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s;", DATABASE_TABLENAME);

    // SQL文を準備
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "登録件数の取得に失敗: %s\n", sqlite3_errmsg(db));
        return -1; // エラーの場合
    }

    // SQLを実行して件数を取得
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0); // 件数を取得
    }

    sqlite3_finalize(stmt); // ステートメントを解放
    return count;           // 件数を返す
}

// 試験結果をデータベースに登録する関数
int register_data(sqlite3 *db, const char *name, int exam_day, int scores[]) {
    sqlite3_stmt *stmt;

    // 試験結果を挿入するためのSQL文
    char insert_sql[512];
    snprintf(insert_sql, sizeof(insert_sql),
    "INSERT INTO %s (name, exam_day, nLang, math, Eng, JHist, wHist, geo, phys, chem, bio) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);", DATABASE_TABLENAME);


    // SQL文を準備
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "データ登録の準備に失敗: %s\n", sqlite3_errmsg(db));
        return -1; // エラーの場合
    }

    // プレースホルダー（?）に値を設定
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC); // 名前をバインド
    sqlite3_bind_int(stmt, 2, exam_day);                // 試験日をバインド

    // 各科目のスコアをバインド（-1の場合はNULLを設定）
    for (int i = 0; i < SUBJECT_COUNT; i++) {
        if (scores[i] == -1) {
            sqlite3_bind_null(stmt, 3 + i); // NULL値を設定
        } else {
            sqlite3_bind_int(stmt, 3 + i, scores[i]); // スコアを設定
        }
    }

    // SQLを実行してデータを挿入
    int result = sqlite3_step(stmt);
    if (result == SQLITE_DONE) {
        printf("データが正常に登録されました。\n");
        printf("※ 登録内容を修正したい場合は変更機能をご利用ください。\n");
    } else {
        fprintf(stderr, "データ登録に失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1; // エラーの場合
    }

    sqlite3_finalize(stmt); // ステートメントを解放

    // 挿入したデータの行IDを取得して返す
    int id = sqlite3_last_insert_rowid(db);
    return id;
}

// データベース内の最大IDを取得する関数
int get_max_id(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT MAX(id) FROM testtable;";  // 最大IDを取得するSQL
    int max_id = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "最大ID取得失敗: %s\n", sqlite3_errmsg(db));
        return -1;  // エラー時は-1を返す
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        max_id = sqlite3_column_int(stmt, 0);  // 最大IDを取得
    }
    sqlite3_finalize(stmt);

    return max_id;
}

// --- 名前のバリデーション ---
// 名前が全角カタカナで構成され、20文字以内であるかを検証する
int validate_name(const char *name) {
    size_t len = strlen(name);

    // 名前の長さをチェック（最大60バイト以内）
    if (len > 60) { // UTF-8エンコーディングで全角カタカナは最大3バイトなので、20文字は60バイト以内
        printf("エラー: 名前は20文字以内で入力してください（全角カタカナ）。\n");
        return 0;
    }

    // 名前が全角カタカナで構成されているかを確認
    wchar_t wc;
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    const char *ptr = name;
    size_t mblen;

    while (*ptr) {
        // マルチバイト文字をワイド文字に変換
        mblen = mbrtowc(&wc, ptr, MB_CUR_MAX, &state);
        if (mblen == (size_t)-1 || mblen == (size_t)-2) {
            printf("エラー: 無効な文字が含まれています。\n");
            return 0;
        }

        // 文字が全角カタカナまたは長音符かをチェック
        if (!((wc >= 0x30A0 && wc <= 0x30FF) || wc == 0x30FC)) {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
            return 0;
        }
        ptr += mblen; // 次の文字に移動
    }

    return 1; // 検証成功
}

// --- 名前の存在確認 ---
// データベース内に指定した名前が既に存在するかを確認する
int is_name_exists(sqlite3 *db, const char *name) {
    sqlite3_stmt *stmt;

    // 名前の存在確認を行うSQL文
    const char *sql = "SELECT COUNT(*) FROM testtable WHERE name = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "ステートメントの準備に失敗: %s\n", sqlite3_errmsg(db));
        return 0; // エラー時は存在しないとみなす
    }

    // 名前をバインド
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    // 結果を取得
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0); // 件数を取得
    }
    sqlite3_finalize(stmt);
    return count > 0; // 件数が0より大きければ存在
}

// --- 試験結果の挿入 ---
// 指定した科目に得点を登録する
int insert_examinee(sqlite3 *db, const char *name, int exam_day, int subject_id, int score) {
    // 科目列の対応表
    const char *subject_columns[] = {"nLang", "math", "Eng", "JHist", "wHist", "geo", "phys", "chem", "bio"};

    // 無効な科目IDのチェック
    if (subject_id < 0 || subject_id >= SUBJECT_COUNT) {
        fprintf(stderr, "無効な科目IDです\n");
        return -1;
    }

    // 試験結果を更新するSQL文を作成
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE %s SET %s = ? WHERE name = ? AND exam_day = ?;",
         DATABASE_TABLENAME, subject_columns[subject_id]);


    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "ステートメントの準備に失敗: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // 値をバインド
    sqlite3_bind_int(stmt, 1, score);             // 得点
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC); // 名前
    sqlite3_bind_int(stmt, 3, exam_day);          // 試験日

    // SQL文を実行
    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL実行エラー: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt); // ステートメントを解放
    return 0; // 成功
}

// --- 試験日の存在確認 ---
// 指定した名前と試験日がデータベースに存在するかを確認する
int is_exam_date_exists(sqlite3 *db, const char *name, const char *exam_date_str) {
    sqlite3_stmt *stmt;

    // 名前と試験日の存在確認を行うSQL文
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s WHERE name = ? AND exam_day = ?;", DATABASE_TABLENAME);
    int exists = 0;

    // ステートメントの準備
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0; // エラー時は存在しないとみなす
    }

    // 名前と試験日をバインド
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, exam_date_str, -1, SQLITE_STATIC);

    // SQL文を実行して結果を取得
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        exists = (count > 0); // 件数が0より大きければ存在
    }

    sqlite3_finalize(stmt); // ステートメントを解放
    return exists;
}

// --- 入力のトリム ---
// 文字列の先頭と末尾の空白を削除する
void trim_input(char *str) {
    char *end;

    // 先頭の空白を削除
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;

    // 末尾の空白を削除
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0'; // 終端文字を設定
}

// --- 日付のバリデーション ---
// 日付が8桁の数字であり、存在する日付かを確認する
int touroku_validate_date(const char *date) {
    // 入力が8桁の数字で構成されているかをチェック
    if (strlen(date) != 8 || strspn(date, "0123456789") != 8) {
        printf("エラー: 試験日は8桁の数字で入力してください（例: 20250513）。\n");
        return 0;
    }

    // 年月日を抽出
    char year_str[5], month_str[3], day_str[3];
    strncpy(year_str, date, 4); year_str[4] = '\0';
    strncpy(month_str, date + 4, 2); month_str[2] = '\0';
    strncpy(day_str, date + 6, 2); day_str[2] = '\0';

    int year = atoi(year_str);
    int month = atoi(month_str);
    int day = atoi(day_str);

    // 月が1～12であるかをチェック
    if (month < 1 || month > 12) {
        printf("エラー: 月は1から12の範囲で指定してください。\n");
        return 0;
    }

    // 各月の日数を設定（うるう年対応）
    int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        days_in_month[1] = 29; // 2月が29日
    }

    // 日がその月の日数内に収まっているかをチェック
    if (day < 1 || day > days_in_month[month - 1]) {
        printf("エラー: 該当する月にはその日付が存在しません。\n");
        return 0;
    }

    return 1;
}
//点数が範囲に収まっているかチェック
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

// 新規登録用関数
int register_new_examinee(sqlite3 *db) {
    
    // 最大IDを取得して登録上限をチェックする
    int max_id = get_max_id(db);
    if (max_id < 0) {
        // データベースエラーがあった場合の処理
        printf("データベースエラーにより登録できません。\n");
        return 1;
    }
    if (max_id >= 1000) {
        // 最大IDが1000以上の場合は登録不可とする
        printf("登録可能なIDの上限(1000件)に達しました。これ以上登録できません。\n");
        return 1;
    }

    char name[61];
    char exam_date_str[9];
    int exam_day;
    int scores[SUBJECT_COUNT];
    int registered[SUBJECT_COUNT] = {0};
    int registered_subject_count = 0;

    while (1) {
        printf("名前を全角カタカナで入力してください（20文字以内）: ");
        if (fgets(name, sizeof(name), stdin) == NULL) {
            printf("入力エラー\n");
            return 1;
        }
        name[strcspn(name, "\n")] = '\0';
        trim_input(name);
        if (validate_name(name)) break;
        printf("名前の形式が正しくありません。\n");
    }

    do {
    printf("試験日を8桁で入力してください（例: 20250513）: ");
        if (fgets(exam_date_str, sizeof(exam_date_str), stdin) == NULL) {
            printf("入力エラーが発生しました。\n");
            return 1;
        }
        exam_date_str[strcspn(exam_date_str, "\n")] = '\0';
    } while (!touroku_validate_date(exam_date_str));

    exam_day = atoi(exam_date_str);

    if (is_duplicate(db, name, exam_day)) {
        printf("エラー: 同じ名前と試験日のデータが既に存在します。\n");
        return 1;
    }

    int score;
    while (registered_subject_count < 5) {
        printf("\n--- 科目一覧 ---\n");
        for (int i = 0; i < SUBJECT_COUNT; i++) {
            printf(" %d: %s", i + 1, subjects_ja[i]);
            if (registered[i]) printf(" [登録済み]");
            printf("\n");
        }
        printf("------------------\n");
        printf("科目を選択してください（1〜9、終了は0）: ");

        char input[10];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("入力エラー\n");
            continue;
        }
        input[strcspn(input, "\n")] = '\0';

        char *endptr;
        int sel = strtol(input, &endptr, 10);
        if (*endptr != '\0' || sel < 0 || sel > 9) {
            printf("0〜9の数字を入力してください。\n");
            continue;
        }
        if (sel == 0) break;

        int idx = sel - 1;

        if (registered[idx]) {
            printf("科目「%s」は既に登録済みです。\n", subjects_ja[idx]);
            continue;
        }

        int is_liberal = 0;
        for (int i = 0; i < 3; i++) {
            if (idx == liberal_indices[i]) is_liberal = 1;
        }
        if (is_liberal && is_liberal_registered(registered)) {
            printf("文系科目は一つだけ選択可能です。\n");
            continue;
        }

        int is_science = 0;
        for (int i = 0; i < 3; i++) {
            if (idx == science_indices[i]) is_science = 1;
        }
        if (is_science && is_science_registered(registered)) {
            printf("理系科目は一つだけ選択可能です。\n");
            continue;
        }

        do {
            printf("点数を入力してください（0〜100）: ");
            if (scanf("%d", &score) != 1) {
                printf("整数を入力してください。\n");
                while (getchar() != '\n');
                continue;
            }
            while (getchar() != '\n');
        } while (!validate_score(score));

        scores[idx] = score;
        registered[idx] = 1;
        registered_subject_count++;

        printf("科目「%s」に点数 %d を登録しました。\n", subjects_ja[idx], score);

        if (registered_subject_count >= 5) {
            printf("最大登録科目数に達しました。\n");
            break;
        }

        while (1) {
            printf("他の科目を登録しますか？（y/n）: ");
            char yn[10];
            if (fgets(yn, sizeof(yn), stdin) == NULL) {
                printf("入力エラー\n");
                continue;
            }
            if (yn[0] == 'y' || yn[0] == 'Y') break;
            else if (yn[0] == 'n' || yn[0] == 'N') goto registration_done;
            else printf("yかnで答えてください。\n");
        }
    }

registration_done:

    for (int i = 0; i < SUBJECT_COUNT; i++) {
        if (!registered[i]) scores[i] = -1;
    }

    int id = register_data(db, name, exam_day, scores);
    if (id < 0) {
        printf("登録に失敗しました。\n");
        return 1;
    }
    printf("登録ID: %d\n", id);
    return 0;
}

// 名前＋試験日の重複チェック関数
int is_duplicate_examinee(sqlite3 *db, const char *name, int exam_day) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM testtable WHERE name = ? AND exam_day = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "重複チェック準備失敗: %s\n", sqlite3_errmsg(db));
        return 1;  // エラー時は重複あり扱いにするのが安全
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

// 既登録受験者の違う試験日で新規試験結果を登録する関数
int register_existing_examinee(sqlite3 *db) {
    char name[61];
    char exam_date_str[9];
    int exam_day;
    int scores[SUBJECT_COUNT] = {0};
    int registered[SUBJECT_COUNT] = {0};
    int registered_subject_count = 0;

    int liberal_selected = 0; // 文系科目が選択済みか
    int science_selected = 0; // 理系科目が選択済みか

    char input[32];
    char *endptr;

    // 名前入力＆バリデーション
    while (1) {
        printf("名前を全角カタカナで入力してください（20文字以内）: ");
        if (fgets(name, sizeof(name), stdin) == NULL) {
            printf("入力エラー\n");
            return 1;
        }
        name[strcspn(name, "\n")] = '\0';
        trim_input(name);
        if (validate_name(name)) break;
        printf("名前の形式が正しくありません。\n");
    }

    // 名前存在チェック
    if (!is_name_exists(db, name)) {
        printf("エラー: 該当する受験者が登録されていません。\n");
        return 1;
    }

    printf("違う試験日に新規で試験結果を登録してください。\n");

    // 新しい試験日入力＆バリデーション
    while (1) {
        printf("新しい試験日を8桁で入力してください（例: 20250513）: ");
        if (fgets(exam_date_str, sizeof(exam_date_str), stdin) == NULL) {
            printf("入力エラー\n");
            return 1;
        }
        exam_date_str[strcspn(exam_date_str, "\n")] = '\0';

        if (!touroku_validate_date(exam_date_str)) {
            printf("日付の形式が正しくありません。\n");
            continue;
        }
        exam_day = atoi(exam_date_str);

        // 既に同じ試験日が登録されていないかチェック
        if (is_exam_date_exists(db, name, exam_date_str)) {
            printf("その試験日は既に登録されています。別の日付を入力してください。\n");
            continue;
        }
        break;
    }

    // 科目入力前に1回だけバッファクリア
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // 科目・点数入力ループ
    while (registered_subject_count < 5) {
        printf("--- 科目一覧 ---\n");
        for (int i = 0; i < SUBJECT_COUNT; i++) {
            printf("%d. %s", i + 1, subjects_ja[i]);
            if (registered[i]) printf(" [登録済み]");
            printf("\n");
        }
        printf("0. 登録終了\n");

        int subject_choice;

        while (1) {
            printf("科目を選択してください（1〜%d、終了は0）: ", SUBJECT_COUNT);
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("入力エラー\n");
                return 1;
            }
            input[strcspn(input, "\n")] = '\0';  // fgetsの改行を消す

            subject_choice = (int)strtol(input, &endptr, 10);
            if (*endptr != '\0' || subject_choice < 0 || subject_choice > SUBJECT_COUNT) {
                printf("0〜%dの数字を入力してください。\n", SUBJECT_COUNT);
                continue;
            }
        break;
    }

        if (subject_choice == 0) break;

        int idx = subject_choice - 1;
        if (registered[idx]) {
            printf("科目「%s」は既に登録済みです。\n", subjects_ja[idx]);
            continue;
        }

        // 文系科目チェック
        int is_liberal = (idx >= LIBERAL_START_INDEX && idx <= LIBERAL_END_INDEX);
        if (is_liberal && liberal_selected) {
            printf("文系科目は既に選択済みです。\n");
            continue;
        }

        // 理系科目チェック
        int is_science = (idx >= SCIENCE_START_INDEX && idx <= SCIENCE_END_INDEX);
        if (is_science && science_selected) {
            printf("理系科目は既に選択済みです。\n");
            continue;
        }

        int score;
        while (1) {
            printf("点数を入力してください（0〜100）: ");
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("入力エラー\n");
                return 1;
            }
            input[strcspn(input, "\n")] = '\0';

            score = (int)strtol(input, &endptr, 10);
            if (*endptr != '\0' || score < 0 || score > 100) {
                printf("0〜100の整数を入力してください。\n");
                continue;
            }
            break;
        }

        scores[idx] = score;
        registered[idx] = 1;
        registered_subject_count++;
        if (is_liberal) liberal_selected = 1;
        if (is_science) science_selected = 1;

        printf("科目「%s」に点数 %d を登録しました。\n", subjects_ja[idx], score);

        if (registered_subject_count >= 5) {
            printf("最大登録科目数に達しました。\n");
            break;
        }

        // 続けて登録するか確認
        while (1) {
            printf("他の科目を登録しますか？（y/n）: ");
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("入力エラー\n");
                return 1;
            }
            if (input[0] == 'y' || input[0] == 'Y') break;
            else if (input[0] == 'n' || input[0] == 'N') goto registration_done;
            else printf("yかnで答えてください。\n");
        }
    }

    registration_done:

    // 登録処理
    for (int i = 0; i < SUBJECT_COUNT; i++) {
        if (registered[i]) {
            if (insert_examinee(db, name, exam_day, i, scores[i]) != 0) {
                fprintf(stderr, "Error inserting examinee data.\n");
                return 1;
            }
        }
    }

    printf("新しい試験日の試験結果を登録しました。\n");
    return 0;
}

//DB接続をリセットする関数
void reset_db_connection(sqlite3 **db) {
    // 既存の接続を閉じる
    if (*db != NULL) {
        sqlite3_close(*db);
        *db = NULL;
    }

    // 新しい接続を確立
    if (sqlite3_open("examdata.db", db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(*db));
    }
}


// --- main関数 ---

int touroku_main(sqlite3 *db) {
    // ロケールを日本語に設定（全角カタカナなどの文字処理のため）
    //setlocale(LC_ALL, "ja_JP.UTF-8");

    // データベースに接続
    //db = connect_to_database(DATABASE_FILENAME);

    // メインループ
    while (1) {
        // メニューの表示
        printf("\n受験者情報管理\n");
        printf("1: 新規登録\n"); // 新しい受験者の登録
        printf("2: 既登録者試験結果追加登録\n"); // 既存受験者への試験結果の追加
        printf("0: 終了\n"); // プログラムを終了
        printf("選択してください: ");

        int choice;
        // ユーザーの入力を取得
        if (scanf("%d", &choice) != 1) {
            // 入力エラー時の処理
            printf("入力エラー\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF); // 入力バッファをクリア
            continue;
        }
        int c;
        while ((c = getchar()) != '\n' && c != EOF); // 入力バッファをクリア

        // ユーザーの選択に基づく処理
        if (choice == 0) {
            reset_db_connection(&db);
            break; // 0を選択した場合、ループを終了（プログラム終了）
        }else if (choice == 1){ 
            reset_db_connection(&db);
            register_new_examinee(db); // 新規受験者の登録処理
        }else if (choice == 2) {
            reset_db_connection(&db);
            register_existing_examinee(db); // 既存受験者の試験結果登録処理
        }else 
            printf("無効な選択です。\n"); // 不正な選択時のエラーメッセージ
    }

    // データベース接続を閉じる
    close_database(db);

    // 終了メッセージ
    printf("プログラムを終了します。\n");

    // プログラムの終了
    return 0;
}