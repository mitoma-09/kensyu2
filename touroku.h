#ifndef TOUROKU_H
#define TOUROKU_H

#include <sqlite3.h> // sqlite3* 型を使用するため

int touroku_main(sqlite3 *db);
void reset_db_connection(sqlite3 **db);
int validate_name(const char *name);
void trim_input(char *str);
int touroku_validate_date(const char *date_str);
int is_date_in_valid_range(const char *date_str);
int is_name_exists(sqlite3 *db, const char *name);

#endif // TOUROKU_H