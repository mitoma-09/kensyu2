#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

// データベースファイル名の共有定義
#define DATABASE_FILENAME "examdata.db"

// テーブル名の共有定義
#define DATABASE_TABLENAME "testtable"

// テーブル作成SQLの共有定義
#define CREATE_TABLE_SQL \
    "CREATE TABLE IF NOT EXISTS " DATABASE_TABLENAME " (" \
    "name TEXT NOT NULL, " \
    "exam_day INTEGER NOT NULL, " \
    "nLang INTEGER, " \
    "math INTEGER, " \
    "Eng INTEGER, " \
    "JHist INTEGER, " \
    "wHist INTEGER, " \
    "geo INTEGER, " \
    "phys INTEGER, " \
    "chem INTEGER, " \
    "bio INTEGER, " \
    "ID INTEGER PRIMARY KEY AUTOINCREMENT" \
    ");"

// データベース接続・切断
sqlite3* connect_to_database(const char *filename);
void close_database(sqlite3 *db);

#endif
