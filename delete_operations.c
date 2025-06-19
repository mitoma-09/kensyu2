// delete_operations.c
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "delete_operations.h"
#include "touroku.h"

// 以下、名前の妥当性チェック関数や日付チェック関数
int validate_name(const char *name);
void trim_input(char *str);
int touroku_validate_date(const char *date_str);
int is_date_in_valid_range(const char *date_str);

// 以下はDB操作の本体
//指定された受験者のすべてのデータを削除
int delete_examinee_all(sqlite3 *db, const char *name) {
    char *err_msg = NULL;
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM testtable WHERE name = ?;";

    // SQL 文の準備
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 文の準備に失敗しました: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // プレースホルダに値をバインド
    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "SQL 文のバインドに失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    // SQL 文を実行
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "受験者データの削除に失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    printf("受験者「%s」のすべてのデータを削除しました。\n", name);
    sqlite3_finalize(stmt);
    return 0;
}
//指定された受験者の特定の試験日データを削除
int delete_examinee_examday(sqlite3 *db, const char *name, int exam_day) {
    char *err_msg = NULL;
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM testtable WHERE name = ? AND exam_day = ?;";

    // SQL 文の準備
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 文の準備に失敗しました: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // プレースホルダに値をバインド
    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK ||
        sqlite3_bind_int(stmt, 2, exam_day) != SQLITE_OK) {
        fprintf(stderr, "SQL 文のバインドに失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    // SQL 文を実行
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "受験者の試験日データの削除に失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    printf("受験者「%s」の試験日「%d」のデータを削除しました。\n", name, exam_day);
    sqlite3_finalize(stmt);
    return 0;
}
//指定された名前と日付がデータベースに存在するかを確認
int is_name_and_date_exists(sqlite3 *db, const char *name, int exam_day) {
    sqlite3_stmt *stmt;

    // 名前と試験日の存在確認を行うSQL文
    const char *sql = "SELECT COUNT(*) FROM testtable WHERE name = ? AND exam_day = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "ステートメントの準備に失敗: %s\n", sqlite3_errmsg(db));
        return 0; // エラー時は存在しないとみなす
    }

    // 名前と試験日をバインド
    if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK ||
        sqlite3_bind_int(stmt, 2, exam_day) != SQLITE_OK) {
        fprintf(stderr, "バインドに失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }

    // 結果を取得
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0); // 件数を取得
    }
    sqlite3_finalize(stmt);
    return count > 0; // 件数が0より大きければ存在
}


// 受験者単位削除＋入力チェック
int delete_examinee_all_with_validation(sqlite3 *db) {
    char name[61];
    while (1) {
        printf("削除する受験者の名前を全角カタカナで入力してください（20文字以内）: ");
        if (fgets(name, sizeof(name), stdin) == NULL) {
            printf("入力エラーが発生しました。再度入力してください。\n");
            int c; while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == '\n') name[len - 1] = '\0';
        else {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            printf("エラー: 入力が長すぎます。20文字以内で入力してください。\n");
            continue;
        }
        trim_input(name); // 入力文字列のトリム

        int ret = validate_name(name);
        if (ret == 0) { // 名前形式が正しい場合
            if (!is_name_exists(db, name)) { // is_name_exists を利用
                printf("エラー: 指定された名前の受験者データは存在しません。\n");
                continue; // 名前入力を再度促す
            }
            break; // 名前が存在する場合は次の処理へ
        } else if (ret == 1) {
            printf("エラー: 名前を入力してください。\n");
        } else if (ret == 2) {
            printf("エラー: 名前は20文字以内で入力してください（全角カタカナ）。\n");
        } else if (ret == 3) {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
        } else {
            printf("エラー: 名前の形式が正しくありません。\n");
        }
    }

    // 名前が存在する場合に削除を実行
    int rc = delete_examinee_all(db, name);
    if (rc != 0) {
        printf("データベース削除に失敗しました。\n");
    }
    return rc;
}

// 試験単位削除＋入力チェック
int delete_examinee_examday_with_validation(sqlite3 *db) {
    char name[61];
    char exam_date_str[16];
    int exam_day;

    // 名前入力チェック
    while (1) {
        printf("削除する受験者の名前を全角カタカナで入力してください（20文字以内）: ");
        if (fgets(name, sizeof(name), stdin) == NULL) {
            printf("入力エラーが発生しました。再度入力してください。\n");
            int c; while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        size_t len = strlen(name);
        if (len > 0 && name[len - 1] == '\n') name[len - 1] = '\0';
        else {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            printf("エラー: 入力が長すぎます。20文字以内で入力してください。\n");
            continue;
        }
        trim_input(name);

        int ret = validate_name(name);
        if (ret == 0) {
            if (!is_name_exists(db, name)) { // is_name_exists を利用
                printf("エラー: 指定された名前の受験者データは存在しません。\n");
                continue; // 名前入力を再度促す
            }
            break;
        } else if (ret == 1) {
            printf("エラー: 名前を入力してください。\n");
        } else if (ret == 2) {
            printf("エラー: 名前は20文字以内で入力してください（全角カタカナ）。\n");
        } else if (ret == 3) {
            printf("エラー: 名前は全角カタカナで入力してください。\n");
        } else {
            printf("エラー: 名前の形式が正しくありません。\n");
        }
    }

    // 試験日入力チェック
    while (1) {
        printf("削除する試験日を8桁で入力してください（例: 20250513）: ");
        if (fgets(exam_date_str, sizeof(exam_date_str), stdin) == NULL) {
            printf("入力エラーが発生しました。\n");
            continue;
        }
        exam_date_str[strcspn(exam_date_str, "\n")] = '\0';
        trim_input(exam_date_str);

        if (strlen(exam_date_str) != 8 || strspn(exam_date_str, "0123456789") != 8) {
            printf("エラー: 試験日は8桁の数字で入力してください（例: 20250513）。\n");
            continue;
        }

        if (!touroku_validate_date(exam_date_str)) continue;
        if (!is_date_in_valid_range(exam_date_str)) continue;

        exam_day = atoi(exam_date_str);

        if (!is_name_and_date_exists(db, name, exam_day)) {
            printf("エラー: 指定された名前と試験日のデータは存在しません。\n");
            continue; // 再入力を促す
        }

        
        break;
    }

    int rc = delete_examinee_examday(db, name, exam_day);
    if (rc != 0) {
        printf("データベース削除に失敗しました。\n");
    }
    return rc;
}

// 全てのデータを削除
int delete_all_data(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM testtable;";

    // SQL 文の準備
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 文の準備に失敗しました: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // SQL 文を実行
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "全てのデータ削除に失敗しました: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    printf("全てのデータを削除しました。\n");
    sqlite3_finalize(stmt);
    return 0;
}

// 削除メニュー表示と選択処理
void delete_menu(sqlite3 *db) {
    while (1) {
        printf("\n=== 削除メニュー ===\n");
        printf("1. 受験者単位の削除\n");
        printf("2. 試験単位の削除\n");
        printf("3. 全てのデータを削除\n");
        printf("4. キャンセル\n");
        printf("番号を入力してください > ");

        char input[10];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("入力エラーです。\n");
            continue;
        }

        // 改行削除
        input[strcspn(input, "\n")] = '\0';

        // 数字のみチェック
        int valid = 1;
        for (size_t i = 0; i < strlen(input); i++) {
            if (!isdigit((unsigned char)input[i])) {
                valid = 0;
                break;
            }
        }

        if (!valid || strlen(input) == 0) {
            printf("数字のみを入力してください。\n");
            continue;
        }

        int choice = atoi(input);

        if (choice == 1) {
            if (delete_examinee_all_with_validation(db) != 0) {
                printf("受験者単位の削除に失敗しました。\n");
            }
            break;
        } else if (choice == 2) {
            if (delete_examinee_examday_with_validation(db) != 0) {
                printf("試験単位の削除に失敗しました。\n");
            }
            break;
        } else if (choice == 3) {
            char confirm1[8];
            char confirm2[8];
            printf("本当に全てのデータを削除しますか？ [y/N]: ");
            if (fgets(confirm1, sizeof(confirm1), stdin) == NULL) {
                printf("入力エラーが発生しました。\n");
                continue;
            }
            if (tolower(confirm1[0]) != 'y') {
                printf("全てのデータ削除をキャンセルしました。\n");
                break;
            }

            printf("確認: 本当に削除します。よろしいですか？ [y/N]: ");
            if (fgets(confirm2, sizeof(confirm2), stdin) == NULL) {
                printf("入力エラーが発生しました。\n");
                continue;
            }
            if (tolower(confirm2[0]) == 'y') {
                if (delete_all_data(db) != 0) {
                    printf("全てのデータ削除に失敗しました。\n");
                }
            } else {
                printf("全てのデータ削除をキャンセルしました。\n");
            }
            break;
        } else if (choice == 4) {
            printf("削除操作をキャンセルしました。\n");
            break;
        } else {
            printf("1～4の数字を入力してください。\n");
        }
    }
}