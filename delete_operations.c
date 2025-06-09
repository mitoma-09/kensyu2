// delete_operations.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"
#include "database.h" // create_tables などで TABLE_NAME が定義されている可能性あり
#include "delete_operations.h"

// MAX_SQL_SIZE の定義は db_operations.h または共通ヘッダーに移動するのが理想的です
// ここで定義する場合は他の場所と重複しないように注意
#ifndef MAX_SQL_SIZE
#define MAX_SQL_SIZE 2000
#endif

// コールバック関数（削除操作では通常NULLでOKだが、必要なら）
int delete_callback(void* data, int argc, char** argv, char** azColName) {
    return 0;
}

// 受験者単位で試験結果を削除する
int delete_examinee_data(sqlite3 *db) {
    char name[100]; // 氏名入力用バッファ
    char sql[MAX_SQL_SIZE];
    sqlite3_stmt *stmt_select_id; // ユーザーID取得用
    sqlite3_stmt *stmt_delete_user; // ユーザー削除用
    int rc;
    int user_id = -1; // 取得したユーザーIDを格納

    printf("\n--- 受験者単位削除 ---\n");
    printf("削除したい受験者の氏名を入力してください: ");
    if (scanf("%99s", name) != 1) {
        printf("無効な入力です。\n");
        while (getchar() != '\n'); // 入力バッファをクリア
        return 1;
    }
    while (getchar() != '\n'); // scanfの後の改行を消費

    // 1. まず氏名から users.id を取得する
    const char *select_user_id_sql = "SELECT id FROM users WHERE name = ?;";
    rc = sqlite3_prepare_v2(db, select_user_id_sql, -1, &stmt_select_id, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL準備エラー (ユーザーID取得): %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_bind_text(stmt_select_id, 1, name, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt_select_id);
    if (rc == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt_select_id, 0); // ユーザーIDが見つかった
    } else if (rc == SQLITE_DONE) {
        printf("氏名 '%s' の受験者は見つかりませんでした。\n", name);
        sqlite3_finalize(stmt_select_id);
        return 0; // 見つからなかった場合は削除せず成功終了
    } else {
        fprintf(stderr, "ユーザーID取得実行エラー: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_select_id);
        return 1;
    }
    sqlite3_finalize(stmt_select_id); // ステートメントを解放

    printf("氏名 '%s' (ID: %d) の全ての試験結果を削除します。よろしいですか？ (y/n): ", name, user_id);
    char confirm;
    scanf(" %c", &confirm);
    while (getchar() != '\n');

    if (confirm != 'y' && confirm != 'Y') {
        printf("削除をキャンセルしました。\n");
        return 0;
    }

    // 2. ユーザーを users テーブルから削除
    // ON DELETE CASCADE が設定されている場合、関連するexam_sessionsとscoresも自動的に削除される
    snprintf(sql, MAX_SQL_SIZE, "DELETE FROM users WHERE id = ?;");

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt_delete_user, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL準備エラー (ユーザー削除): %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_bind_int(stmt_delete_user, 1, user_id); // 取得したユーザーIDをバインド

    rc = sqlite3_step(stmt_delete_user);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL実行エラー (ユーザー削除): %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_delete_user);
        return 1;
    }

    printf("氏名 '%s' (ID: %d) の試験結果が削除されました。\n", name, user_id);

    sqlite3_finalize(stmt_delete_user); // ステートメントを解放
    return 0;
}

int delete_exam_data(sqlite3 *db) {
    char name[100];
    char exam_date_str[9]; // YYYYMMDD形式の文字列として扱う
    char sql[MAX_SQL_SIZE];
    sqlite3_stmt *stmt_select_id; // ユーザーIDとセッションID取得用
    sqlite3_stmt *stmt_delete_session; // セッション削除用
    int rc;
    int user_id = -1;
    int exam_session_id = -1;

    printf("\n--- 試験単位削除 ---\n");
    printf("削除したい受験者の氏名を入力してください: ");
    if (scanf("%99s", name) != 1) {
        printf("無効な入力です。\n");
        while (getchar() != '\n');
        return 1;
    }
    while (getchar() != '\n');

    printf("削除したい試験の実施日を入力してください (YYYYMMDD形式): ");
    // exam_date は TEXT 型なので文字列として読み込むのが安全です
    if (scanf("%8s", exam_date_str) != 1 || strlen(exam_date_str) != 8) {
        printf("無効な入力です。YYYYMMDD形式で8桁の数字を入力してください。\n");
        while (getchar() != '\n');
        return 1;
    }
    while (getchar() != '\n');

    // 1. 氏名から users.id を取得
    const char *select_user_id_sql = "SELECT id FROM users WHERE name = ?;";
    rc = sqlite3_prepare_v2(db, select_user_id_sql, -1, &stmt_select_id, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL準備エラー (ユーザーID取得): %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_bind_text(stmt_select_id, 1, name, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt_select_id);
    if (rc == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt_select_id, 0);
    } else if (rc == SQLITE_DONE) {
        printf("氏名 '%s' の受験者は見つかりませんでした。\n", name);
        sqlite3_finalize(stmt_select_id);
        return 0;
    } else {
        fprintf(stderr, "ユーザーID取得実行エラー: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_select_id);
        return 1;
    }
    sqlite3_finalize(stmt_select_id); // ステートメントを解放

    // 2. user_id と exam_date_str から exam_sessions.id を取得
    const char *select_session_id_sql = "SELECT id FROM exam_sessions WHERE user_id = ? AND exam_date = ?;";
    rc = sqlite3_prepare_v2(db, select_session_id_sql, -1, &stmt_select_id, NULL); // stmt_select_id を再利用
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL準備エラー (セッションID取得): %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_bind_int(stmt_select_id, 1, user_id);
    sqlite3_bind_text(stmt_select_id, 2, exam_date_str, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt_select_id);
    if (rc == SQLITE_ROW) {
        exam_session_id = sqlite3_column_int(stmt_select_id, 0); // セッションIDが見つかった
    } else if (rc == SQLITE_DONE) {
        printf("氏名 '%s' の %s の試験結果は見つかりませんでした。\n", name, exam_date_str);
        sqlite3_finalize(stmt_select_id);
        return 0;
    } else {
        fprintf(stderr, "セッションID取得実行エラー: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_select_id);
        return 1;
    }
    sqlite3_finalize(stmt_select_id); // ステートメントを解放

    printf("氏名 '%s' の %s の試験結果を削除します。よろしいですか？ (y/n): ", name, exam_date_str);
    char confirm;
    scanf(" %c", &confirm);
    while (getchar() != '\n');

    if (confirm != 'y' && confirm != 'Y') {
        printf("削除をキャンセルしました。\n");
        return 0;
    }

    // 3. 該当する exam_sessions レコードを削除
    // ON DELETE CASCADE が設定されている場合、関連する scores も自動的に削除される
    snprintf(sql, MAX_SQL_SIZE, "DELETE FROM exam_sessions WHERE id = ?;");

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt_delete_session, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL準備エラー (セッション削除): %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_bind_int(stmt_delete_session, 1, exam_session_id);

    rc = sqlite3_step(stmt_delete_session);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL実行エラー (セッション削除): %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt_delete_session);
        return 1;
    }

    printf("氏名 '%s' の %s の試験結果が削除されました。\n", name, exam_date_str);

    sqlite3_finalize(stmt_delete_session); // ステートメントを解放
    return 0;
}