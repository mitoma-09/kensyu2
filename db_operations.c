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

// テーブル作成関数
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