#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include "sqlite3.h"

// データベース接続
sqlite3* connect_to_database(const char *dbname) {
    sqlite3 *db;
    if (sqlite3_open(dbname, &db)) {
        fprintf(stderr, "データベース接続に失敗しました: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    printf("データベース接続に成功しました！\n");
    return db;
}

// 名前入力チェック
int validate_name(const char *name) {
    int len = strlen(name);
    if (len > 60) {
        printf("エラー: 名前は20文字以内で入力してください。\n");
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
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
        days_in_month[1] = 29;
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

// 科目名取得
const char *get_subject_name(int choice) {
    const char *subjects[] = {"国語", "数学", "英語", "日本史", "世界史", "地理", "物理", "化学", "生物"};
    if (choice >= 1 && choice <= 9) {
        return subjects[choice - 1];
    }
    return NULL;
}

    int social_selected = 0; // 社会科目が選択済みか
    int science_selected = 0; // 理科科目が選択済みか

// 科目一覧表示
void display_subjects() {
    const char *subjects[] = {"国語", "数学", "英語", "日本史", "世界史", "地理", "物理", "化学", "生物"};
    printf("科目一覧:\n");
    for (int i = 0; i < 9; i++) {
        if ((i >= 3 && i <= 5 && social_selected) || // 社会科目が選択済み
            (i >= 6 && i <= 8 && science_selected)) { // 理科科目が選択済み
            printf("  %d: %s（選択不可）\n", i + 1, subjects[i]);
        } else { 
                printf("  %d: %s\n", i + 1, subjects[i]);
        }
    }
}
// 新規受験者登録
int register_user(sqlite3 *db, const char *name) {
    sqlite3_stmt *stmt;

    // 同姓同名チェック
    const char *check_sql = "SELECT id FROM users WHERE name = ?";
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "データベースエラー: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int existing_id = sqlite3_column_int(stmt, 0);
        printf("既に登録されています。受験者ID: %d\n", existing_id);
        sqlite3_finalize(stmt);
        return existing_id; // 既存のIDを返す
    }
    sqlite3_finalize(stmt);

    // 新規登録処理
    const char *insert_sql = "INSERT INTO users (name) VALUES (?)";
    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "受験者登録に失敗しました: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "受験者登録に失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    int user_id = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return user_id;
}


// 試験登録
int register_exam(sqlite3 *db, int user_id, const char *exam_date) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO exam_sessions (exam_date, year, user_id) VALUES (?, ?, ?)";
    char year[5];
    strncpy(year, exam_date, 4);
    year[4] = '\0';

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "試験登録に失敗しました: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, exam_date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, year, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "試験登録に失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    int exam_session_id = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return exam_session_id;
}

// 科目と点数の登録
void register_score(sqlite3 *db, int exam_session_id, const char *subject, int score) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO scores (exam_session_id, subject, score) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "点数登録に失敗しました: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_int(stmt, 1, exam_session_id);
    sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, score);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("科目「%s」の点数 %d が登録されました。\n", subject, score);
    } else {
        fprintf(stderr, "点数登録に失敗しました: %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

// 入力確認
void confirm_registration(const char *name, const char *exam_date, char subjects[][20], int scores[], int subject_count) {
    printf("\n--- 登録内容確認 ---\n");
    printf("名前: %s\n", name);
    printf("試験日: %s\n", exam_date);
    printf("科目と点数:\n");
    for (int i = 0; i < subject_count; i++) {
        printf("  %s: %d点\n", subjects[i], scores[i]);
    }
    printf("--------------------\n");
}

int main() {
    char subjects[10][20];
    int scores[10];
    int subject_count = 0;

    setlocale(LC_ALL, "ja_JP.UTF-8");

    printf("プログラム開始\n");

    sqlite3 *db = connect_to_database("testmanager.db");

    char name[21];
    char exam_date[9];
    int user_id = 0, exam_session_id = 0, score, subject_choice;
    char continue_choice;

    // ユーザー選択：新規登録 or 既登録受験者
    char user_option;
    printf("新規登録(n) もしくは 既登録受験者(e) を選択してください: ");
    scanf(" %c", &user_option);
    getchar(); // 改行を消費

    if (tolower(user_option) == 'n') {
        // 名前入力（新規登録）
        do {
            printf("名前を入力してください（20文字以内）: ");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\n")] = '\0';
        } while (!validate_name(name));

        user_id = register_user(db, name);
        if (user_id == 0) {
            fprintf(stderr, "受験者登録に失敗しました。\n");
            sqlite3_close(db);
            return 1;
        }
        printf("受験者ID: %d が登録されました。\n", user_id); //IDを表示する
    } else if (tolower(user_option) == 'e') {
        // 既登録受験者ID入力
        printf("既登録受験者のIDを入力してください: ");
        scanf("%d", &user_id);

        // 入力されたIDが存在するかチェック
        sqlite3_stmt *stmt;
        const char *check_sql = "SELECT name FROM users WHERE id = ?";
        if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "データベースエラー: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }
        sqlite3_bind_int(stmt, 1, user_id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *registered_name = (const char *)sqlite3_column_text(stmt, 0);
            printf("登録済み受験者: %s\n", registered_name);
        } else {
            printf("エラー: 受験者ID %d は存在しません。\n", user_id);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }
        sqlite3_finalize(stmt);
    } else {
        printf("無効な選択です。プログラムを終了します。\n");
        sqlite3_close(db);
        return 1;
    }

    // 試験日入力
    do {
        printf("試験日を入力してください（例: 20250513）: ");
        scanf("%8s", exam_date);
    } while (!validate_date(exam_date));

    // 科目と点数登録
do {
    display_subjects();
    do {
        printf("科目を選択してください（1〜9）: ");
        scanf("%d", &subject_choice);
    } while (subject_choice < 1 || subject_choice > 9);

    const char *subject = get_subject_name(subject_choice);
    if (subject == NULL) {
        printf("無効な科目が選択されました。\n");
        continue;
    }
    strcpy(subjects[subject_count], subject);

    do {
        printf("点数を入力してください: ");
        scanf("%d", &score);
    } while (!validate_score(score));

    scores[subject_count] = score;
    subject_count++;

    printf("科目「%s」に点数 %d を登録しました。\n", subject, score);

    // ここで5科目に達したらループを抜ける
    if (subject_count == 5) {
        printf("5科目が登録されました。自動的に登録を終了します。\n");
        break;
    }

    printf("他の科目を登録しますか？（y/n）: ");
    scanf(" %c", &continue_choice);
} while (tolower(continue_choice) == 'y');

    // 試験セッション登録
    exam_session_id = register_exam(db, user_id, exam_date);
    if (exam_session_id == 0) {
        fprintf(stderr, "試験セッション登録に失敗しました。\n");
        sqlite3_close(db);
        return 1;
    }

    // 点数の登録
    for (int i = 0; i < subject_count; i++) {
        register_score(db, exam_session_id, subjects[i], scores[i]);
    }

    printf("登録が完了しました。\n");

    sqlite3_close(db);
    printf("プログラム終了\n");
    return 0;
}