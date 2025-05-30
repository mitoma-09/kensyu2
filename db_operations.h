// db_operations.h
#ifndef DB_OPERATIONS_H
#define DB_OPERATIONS_H

#include <sqlite3.h>

// テーブル作成関数
int create_table(sqlite3 *db);

// データ登録関数
int insert_data(sqlite3 *db);

// データ参照関数
void select_data(sqlite3 *db);

#endif