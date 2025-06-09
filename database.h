#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

// データベースに接続し、テーブルを初期化する関数
sqlite3* connect_to_database(const char *filename);

#endif // DATABASE_H
