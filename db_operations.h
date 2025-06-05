#ifndef DB_OPERATIONS_H
#define DB_OPERATIONS_H

#include "sqlite3.h" // sqlite3_t 型を使用するため

// データベース接続
sqlite3* connect_to_database(const char *dbname);

// テーブル作成
// ※元のコードにはテーブル作成関数がなかったので仮で追加します。
//   もしcreate_table関数がdb_operations.cに定義されているならこの宣言は必須です。
int create_tables(sqlite3 *db);

#endif // DB_OPERATIONS_H