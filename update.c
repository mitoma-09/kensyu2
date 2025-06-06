#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ctype.h>
#include <time.h>

#define MAX_NAME_LEN 20

// UTF-8カタカナ判定関数
int is_katakana_utf8(const char *str) {
    while (*str) {
        if ((unsigned char)*str == 0xE3 && ((unsigned char)str[1] == 0x82 || (unsigned char)str[1] == 0x83)) {
            unsigned char c = (unsigned char)str[2];
            // UTF-8カタカナの範囲：0xA1〜0xBF
            if (c >= 0xA1 && c <= 0xBF) {
                str += 3;
                continue;
            }
            return 0;
        } else {
            return 0;  // UTF-8のカタカナ以外は不正
        }
    }
    return 1;
}

void update_name_only(sqlite3 *db) {
    int id;
    char name[100];

    printf("変更対象の受験者IDを入力してください: ");
    scanf("%d", &id);
    while (getchar() != '\n');  // 改行除去

    while (1) {
        printf("新しい氏名（カタカナ20文字以内）: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0';  // 改行除去

        if (strlen(name) == 0) {
            printf("氏名は空白にできません。再入力してください。\n");
            continue;
        }

        if (strlen(name) > MAX_NAME_LEN) {
            printf("氏名が長すぎます。20文字以内で再入力してください。\n");
            continue;
        }

        if (!is_katakana_utf8(name)) {
            printf("カタカナで入力してください。再入力してください。\n");
            continue;
        }

        break;
    }

    const char *sql = "UPDATE testtable SET name=? WHERE ID=?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("SQLエラー: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        printf("氏名を正常に更新しました。\n");
    } else {
        printf("更新に失敗しました: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}

int main(void) {
    sqlite3 *db;
    int rc = sqlite3_open("examdata.db", &db);

    if (rc != SQLITE_OK) {
        printf("データベースを開けません: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    update_name_only(db);

    sqlite3_close(db);
    return 0;
}
