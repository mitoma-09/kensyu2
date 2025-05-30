// db_operations.c
#include "db_operations.h"
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

// テーブル作成関数
int create_table(sqlite3 *db) {
    char *zErrMsg = 0;
    int rc;
    char *sql;

    sql = "CREATE TABLE IF NOT EXISTS results (" \
          "name TEXT NOT NULL," \
          "exam_date TEXT NOT NULL," \
          "kokugo INTEGER);";

    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 1; // 失敗
    } else {
        fprintf(stdout, "Table created or already exists successfully\n");
        return 0; // 成功
    }
}

// データ登録関数
int insert_data(sqlite3 *db) {
    char *zErrMsg = 0;
    int rc;
    char *sql;

    sql = "INSERT INTO results (name, exam_date, kokugo) VALUES ('山田太郎', '2025-05-20', 80);" \
          "INSERT INTO results (name, exam_date, kokugo) VALUES ('佐藤花子', '2025-05-20', 90);" \
          "INSERT INTO results (name, exam_date, kokugo) VALUES ('鈴木一郎', '2025-05-25', 75);";

    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 1; // 失敗
    } else {
        fprintf(stdout, "Data inserted successfully\n");
        return 0; // 成功
    }
}

// データ参照関数
void select_data(sqlite3 *db) {
    char *sql = "SELECT name, exam_date, kokugo FROM results;";
    sqlite3_stmt *stmt;
    int rc;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    fprintf(stdout, "\n--- Registered Data ---\n");
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        printf("Name: %s, Exam Date: %s, Kokugo: %d\n",
               sqlite3_column_text(stmt, 0),
               sqlite3_column_text(stmt, 1),
               sqlite3_column_int(stmt, 2));
    }

    sqlite3_finalize(stmt);
}