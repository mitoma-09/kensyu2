#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"

sqlite3* connect_to_database(const char *filename) {
    sqlite3 *db;

    // データベースファイルを開く（存在しない場合は作成される）
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "データベースに接続できません: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // 試験結果を保存するテーブルを作成
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

    // テーブル作成用のSQL文を実行
    char *errmsg = NULL;
    if (sqlite3_exec(db, create_table_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "テーブルを作成できません: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(1);
    }

    return db; // データベース接続のポインタを返す
}
