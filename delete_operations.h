// delete_operations.h
#ifndef DELETE_OPERATIONS_H
#define DELETE_OPERATIONS_H

#include <sqlite3.h>

// 削除メニュー表示と操作
void delete_menu(sqlite3 *db);

// 受験者単位削除（名前指定）＋エラー対処付き
int delete_examinee_all_with_validation(sqlite3 *db);

// 試験単位削除（名前＋試験日指定）＋エラー対処付き
int delete_examinee_examday_with_validation(sqlite3 *db);

// 科目単位削除（リセット）
int delete_examinee_subject(sqlite3 *db, const char *name, int exam_day, const char *subject_column);

// 科目単位削除＋エラー対処付き
int delete_examinee_subject_with_validation(sqlite3 *db);


#endif // DELETE_OPERATIONS_H
