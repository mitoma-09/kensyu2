#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"

// データベースに接続し、テーブルを作成する
sqlite3* connect_to_database(const char *filename) {
    sqlite3 *db;

    // データベースファイルを開く
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "データベースに接続できません: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // テーブル作成SQL（マクロから取得）
    const char *create_table_sql = CREATE_TABLE_SQL;

    // SQL実行
    char *errmsg = NULL;
    if (sqlite3_exec(db, create_table_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "テーブルを作成できません: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(1);
    }

    return db;
}

// データベースを閉じる
void close_database(sqlite3 *db) {
    if (db) {
        sqlite3_close(db);
    }
}
