#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

// データベース接続・切断
sqlite3* connect_to_database(const char *filename);
void close_database(sqlite3 *db);

#endif
