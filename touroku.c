#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <sqlite3.h>
#include <wchar.h>
#include <ctype.h>

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

int get_registered_count(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM testtable;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "登録数取得エラー: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
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
        printf("※ 登録内容の修正が必要な場合は、変更機能をご利用ください。\n");
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

int validate_name(const char *name) {
    size_t len = strlen(name);

    // 文字数チェック
    if (len > 60) { // UTF-8エンコーディングで全角カタカナは最大3バイトなので、20文字は60バイト以内
        printf("エラー: 名前は20文字以内で入力してください（全角カタカナ）。\n");
        return 0;
    }

    // 全角カタカナのみを許可
    wchar_t wc;
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    const char *ptr = name;
    size_t mblen;

    while (*ptr) {
        mblen = mbrtowc(&wc, ptr, MB_CUR_MAX, &state);
        if (mblen == (size_t)-1 || mblen == (size_t)-2) {
            printf("エラー: 無効な文字が含まれています。\n");
            return 0;
        }

        // 全角カタカナまたは長音符かどうか
        if (!((wc >= 0x30A0 && wc <= 0x30FF) || wc == 0x30FC)) {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
            return 0;
        }
        ptr += mblen;
    }

    return 1;
}

int is_name_exists(sqlite3 *db, const char *name) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM testtable WHERE name = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count > 0;
}

int insert_examinee(sqlite3 *db, const char *name, int exam_day, int subject_id, int score) {

    const char *subject_columns[] = {"nLang", "math", "Eng", "JHist", "wHist", "geo", "phys", "chem", "bio"};

    if (subject_id < 0 || subject_id >= SUBJECT_COUNT) {
        fprintf(stderr, "Invalid subject_id\n");
        return -1;
    }

    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE testtable SET %s = ? WHERE name = ? AND exam_day = ?;",
        subject_columns[subject_id]);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparing statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, exam_day);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error executing statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

void trim_input(char *str) {
    char *end;

    // 前後のスペース削除
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';
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

// 新規登録用関数
int register_new_examinee(sqlite3 *db) {
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
        if (scanf("%8s", exam_date_str) != 1) {
            printf("試験日入力エラー\n");
            while (getchar() != '\n');
            return 1;
        }
        getchar();
    } while (!validate_date(exam_date_str));

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

// 既登録受験者の試験結果追加登録用関数
int register_existing_examinee(sqlite3 *db) {
    char name[61];
    char exam_date_str[9];
    int exam_day;
    int scores[SUBJECT_COUNT] = {0};
    int registered[SUBJECT_COUNT] = {0};
    int registered_subject_count = 0;

    // 名前入力＆バリデーション
    while (1) {
        printf("名前を全角カタカナで入力してください（20文字以内）: ");
        if (fgets(name, sizeof(name), stdin) == NULL) {
            printf("入力エラー\n");
            return 1;
        }
        name[strcspn(name, "\n")] = '\0';  // 改行除去
        trim_input(name);
        if (validate_name(name)) break;
        printf("名前の形式が正しくありません。\n");
    }

    // 名前のみ存在チェック
    if (!is_name_exists(db, name)) {
        printf("エラー: 該当する受験者が登録されていません。\n");
        return 1;
    }

    printf("既登録の受験者です。試験結果を追加登録してください。\n");

    // 試験日入力＆バリデーション
    do {
        printf("試験日を8桁で入力してください（例: 20250513）: ");
        if (scanf("%8s", exam_date_str) != 1) {
            printf("試験日入力エラー\n");
            while (getchar() != '\n');
            return 1;
        }
        getchar();
    } while (!validate_date(exam_date_str));
    exam_day = atoi(exam_date_str);

    // 科目・点数入力ループ
    while (registered_subject_count < SUBJECT_COUNT) {
        int subject_choice;
        printf("\n科目を選択してください:\n");
        for (int i = 0; i < SUBJECT_COUNT; i++) {
        printf("%d. %s\n", i + 1, subjects_ja[i]);
        }
        printf("0. 登録終了\n");

        if (scanf("%d", &subject_choice) != 1) {
            printf("入力エラー\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        if (subject_choice == 0) {
            break;
        }
        if (subject_choice < 1 || subject_choice > SUBJECT_COUNT) {
            printf("無効な選択です。\n");
            continue;
        }
        if (registered[subject_choice - 1]) {
            printf("その科目は既に登録済みです。\n");
            continue;
        }

        int score;
        printf("点数を0〜100で入力してください: ");
        if (scanf("%d", &score) != 1 || score < 0 || score > 100) {
            printf("無効な点数です。\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        scores[subject_choice - 1] = score;
        registered[subject_choice - 1] = 1;
        registered_subject_count++;
    }

    if (registered_subject_count == 0) {
        printf("登録する科目がありません。\n");
        return 1;
    }

    // 登録処理：科目ごとにDB登録
    for (int i = 0; i < SUBJECT_COUNT; i++) {
        if (registered[i]) {
            char exam_day_str[20];
            sprintf(exam_day_str, "%d", exam_day);
            if (insert_examinee(db, name, exam_day_str, i, scores[i]) != 0) {
                printf("登録に失敗しました。\n");
                return 1;
            }
        }
    }

    printf("試験結果を追加登録しました。\n");
    return 0;
}

// main関数
int main() {
    setlocale(LC_ALL, "ja_JP.UTF-8");

    sqlite3 *db;
    db = connect_to_database("examdata.db"); 

    while (1) {
        printf("\n受験者情報管理\n");
        printf("1: 新規登録\n");
        printf("2: 既登録者試験結果追加登録\n");
        printf("0: 終了\n");
        printf("選択してください: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("入力エラー\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        if (choice == 0) break;
        else if (choice == 1) register_new_examinee(db);
        else if (choice == 2) register_existing_examinee(db);
        else printf("無効な選択です。\n");
    }

    sqlite3_close(db);
    return 0;
}