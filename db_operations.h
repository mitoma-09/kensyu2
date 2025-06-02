#ifndef DB_OPERATIONS_H
#define DB_OPERATIONS_H

#include "sqlite3.h" // sqlite3_t 型を使用するため

// データベース接続
sqlite3* connect_to_database(const char *dbname);

// テーブル作成
// ※元のコードにはテーブル作成関数がなかったので仮で追加します。
//   もしcreate_table関数がdb_operations.cに定義されているならこの宣言は必須です。
int create_tables(sqlite3 *db);

// 新規受験者登録
int register_user(sqlite3 *db, const char *name);

// 試験登録
int register_exam(sqlite3 *db, int user_id, const char *exam_date);

// 科目と点数の登録
void register_score(sqlite3 *db, int exam_session_id, const char *subject, int score);

// 参照機能の仮置き (select_dataが元のコードに含まれていないため、必要であれば定義してください)
// int select_data(sqlite3 *db); // 必要であれば追加

#endif // DB_OPERATIONS_H