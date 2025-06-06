#ifndef UPDATE_H
#define UPDATE_H

#include <sqlite3.h>

// 受験者情報を更新する関数（氏名、試験日、点数など）
void examdata(sqlite3 *db);

#endif
