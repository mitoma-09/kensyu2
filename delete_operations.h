// delete_operations.h
#ifndef DELETE_OPERATIONS_H
#define DELETE_OPERATIONS_H

#include "sqlite3.h" // sqlite3* 型を使うために必要です

// データベースのテーブル名やSQLクエリの最大長など、
// プロジェクト全体で共通で使う定数があれば、
// db_operations.h（または新しくcommon.hなど）で定義し、
// ここでインクルードするようにします。
// 例: #include "db_operations.h"

// コールバック関数の宣言 (sqlite3_exec で使う可能性があれば)
// この関数は delete_operations.c 内でのみ使われるなら、ヘッダーに宣言する必要はありません。
// ただし、もし他のファイルから sqlite3_exec を呼び出す際にこのコールバックを指定するなら必要です。
// 通常、削除操作ではコールバックは不要なので、ここでは宣言しないのが一般的です。
// int delete_callback(void* data, int argc, char** argv, char** azColName);


// --- 削除機能の関数プロトタイプ宣言 ---

// 全ての試験結果を削除する関数 (開発・デバッグ用。使用には注意が必要です)
// sqlite3_exec を直接使う場合は、delete_callback も必要になることがあります。
int delete_all_test_results(sqlite3 *db);

// 受験者単位で試験結果を削除する関数
// (特定の氏名の受験者に関連する全ての試験結果を削除)
int delete_examinee_data(sqlite3 *db);

// 試験単位で試験結果を削除する関数
// (特定の氏名と特定の日付の試験結果を削除)
int delete_exam_data(sqlite3 *db);


#endif // DELETE_OPERATIONS_H