#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include "db_operations.h" // 自身のヘッダーファイルをインクルード

// データベース接続
sqlite3* connect_to_database(const char *dbname) {
    sqlite3 *db;
    // sqlite3_open_v2 を使用して、より厳密な制御を推奨
    // SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE でファイルが存在しない場合は作成
    if (sqlite3_open_v2(dbname, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK) {
        fprintf(stderr, "データベース接続に失敗しました: %s\n", sqlite3_errmsg(db));
        // 接続失敗時はdbがNULLになる場合があるので、確実にクローズ
        if (db) {
            sqlite3_close(db);
        }
        exit(1);
    }
    printf("データベース接続に成功しました！\n");
    return db;
}

// テーブル作成関数（元のコードには無かったので追加）
// users, exam_sessions, scores テーブルを作成します
int create_tables(sqlite3 *db) {
    char *err_msg = 0;
    const char *sql_users = "CREATE TABLE IF NOT EXISTS users ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "name TEXT NOT NULL UNIQUE);";
    const char *sql_exam_sessions = "CREATE TABLE IF NOT EXISTS exam_sessions ("
                                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                    "exam_date TEXT NOT NULL,"
                                    "year TEXT NOT NULL,"
                                    "user_id INTEGER NOT NULL,"
                                    "FOREIGN KEY (user_id) REFERENCES users(id));";
    const char *sql_scores = "CREATE TABLE IF NOT EXISTS scores ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                             "exam_session_id INTEGER NOT NULL,"
                             "subject TEXT NOT NULL,"
                             "score INTEGER NOT NULL,"
                             "FOREIGN KEY (exam_session_id) REFERENCES exam_sessions(id));";

    // users テーブル作成
    int rc = sqlite3_exec(db, sql_users, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "usersテーブル作成エラー: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }

    // exam_sessions テーブル作成
    rc = sqlite3_exec(db, sql_exam_sessions, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "exam_sessionsテーブル作成エラー: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }

    // scores テーブル作成
    rc = sqlite3_exec(db, sql_scores, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "scoresテーブル作成エラー: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }

    printf("テーブルが正常に作成または存在しています。\n");
    return 0;
}


// 新規受験者登録
int register_user(sqlite3 *db, const char *name) {
    sqlite3_stmt *stmt;

    // 同姓同名チェック
    const char *check_sql = "SELECT id FROM users WHERE name = ?";
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "データベースエラー (register_user - check): %s\n", sqlite3_errmsg(db));
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
        fprintf(stderr, "受験者登録に失敗しました (register_user - insert): %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "受験者登録に失敗しました (register_user - step): %s\n", sqlite3_errmsg(db));
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
        fprintf(stderr, "試験登録に失敗しました (register_exam - prepare): %s\n", sqlite3_errmsg(db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, exam_date, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, year, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, user_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "試験登録に失敗しました (register_exam - step): %s\n", sqlite3_errmsg(db));
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
        fprintf(stderr, "点数登録に失敗しました (register_score - prepare): %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_int(stmt, 1, exam_session_id);
    sqlite3_bind_text(stmt, 2, subject, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, score);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("科目「%s」の点数 %d が登録されました。\n", subject, score);
    } else {
        fprintf(stderr, "点数登録に失敗しました (register_score - step): %s\n", sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
}

// 仮のselect_data関数（参照機能）
// この関数は元のメインコードに定義されていなかったため、仮の定義です。
// 必要に応じて、実際の参照ロジックを実装してください。
int select_data(sqlite3 *db) {
    printf("参照機能が選択されました。\n");
    printf("ここに参照処理のロジックを実装します。\n");
    // 例: 全ユーザーのIDと名前を表示する
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, name FROM users;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL準備エラー (select_data): %s\n", sqlite3_errmsg(db));
        return 1;
    }

    printf("\n--- 登録済み受験者一覧 ---\n");
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        printf("ID: %d, 名前: %s\n", id, name);
    }
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "データ取得エラー (select_data): %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 1;
    }
    printf("--------------------------\n");

    sqlite3_finalize(stmt);
    return 0;
}