#ifndef TOUROKU_H
#define TOUROKU_H

#include <sqlite3.h> // sqlite3* 型を使用するため

int touroku_main(sqlite3 *db);
void reset_db_connection(sqlite3 **db);

#endif // TOUROKU_H