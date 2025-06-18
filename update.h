#ifndef UPDATE_H
#define UPDATE_H
#define NAME_VALID 0
#define NAME_ERR_EMPTY 1
#define NAME_ERR_INVALID_CHAR 2
#define NAME_ERR_LENGTH 3

#include <sqlite3.h>

// 受験者情報を更新する関数（氏名、試験日、点数など）
void examdata(sqlite3 *db);

int validate_name_update(const char *name);


#endif
