#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <ctype.h>
#include <time.h>
#include <wchar.h> 
#include <errno.h> 
#include "update.h"

#define MAX_NAME_LEN 20

// 名前の検証関数（UTF-8全角カタカナチェック）
int validate_name_update(const char *name) {
    wchar_t wc;
    const char *ptr = name;
    size_t mblen;
    int char_count = 0;
    mbstate_t state;
    memset(&state, 0, sizeof(state));

    if (*name == '\0') return NAME_ERR_EMPTY;

    while (*ptr) {
        errno = 0; // errnoクリア
        mblen = mbrtowc(&wc, ptr, MB_CUR_MAX, &state);
        if (mblen == (size_t)-1 || mblen == (size_t)-2) return NAME_ERR_INVALID_CHAR;

        if (!((wc >= 0x30A1 && wc <= 0x30FA) || wc == 0x30FC)) return NAME_ERR_INVALID_CHAR;

        char_count++;
        if (char_count > MAX_NAME_LEN) return NAME_ERR_LENGTH;

        ptr += mblen;
    }
    return NAME_VALID;
}

// 日付チェック（YYYYMMDDが正当な日付か）
int is_valid_date(const char *str) {
    if (strlen(str) != 8) return 0;
    for (int i = 0; i < 8; i++) {
        if (!isdigit(str[i])) return 0;
    }
    int y, m, d;
    sscanf(str, "%4d%2d%2d", &y, &m, &d);
    struct tm t = {0};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    time_t epoch = mktime(&t);
    return epoch != -1 && t.tm_mday == d && t.tm_mon == m - 1;
}

// 各科目点数の入力
int input_score(const char *subject) {
    int score;
    while (1) {
        printf("%s の点数（0～100）: ", subject);
        if (scanf("%d", &score) != 1 || score < 0 || score > 100) {
            printf("無効な点数です。再入力してください。\n");
            while (getchar() != '\n');
        } else {
            while (getchar() != '\n'); // バッファクリア
            return score;
        }
    }
}

// メイン更新関数
void examdata(sqlite3 *db) {
    int subchoice;
    printf("【受験者情報変更】\n");
    printf("1. 変更を行う\n");
    printf("2. メインメニューに戻る\n");
    printf("選択してください（1 または 2）: ");
    if (scanf("%d", &subchoice) != 1) {
        printf("無効な入力です。メインメニューに戻ります。\n");
        while(getchar() != '\n');
        return;
    }

    if (subchoice == 2) {
        printf("メインメニューに戻ります。\n");
        while(getchar() != '\n');
        return;
    } else if (subchoice != 1) {
        printf("無効な選択肢です。メインメニューに戻ります。\n");
        while(getchar() != '\n');
        return;
    }

    while(getchar() != '\n'); // 改行除去
    int id;
    char name[100], exam_day[100];
    const char *subjects[] = {
        "nLang", "math", "Eng", "JHist", "wHist", "geo", "phys", "chem", "bio"
    };
    int scores[9];

    printf("既登録受験者情報の変更ができます。\n");
    printf("変更対象の受験者IDを入力してください: ");
    scanf("%d", &id);
    while (getchar() != '\n');

     // --- 1. 既存データ取得 ---
    sqlite3_stmt *stmt_select;
    const char *sql_select =
        "SELECT name, exam_day, nLang, math, Eng, JHist, wHist, geo, phys, chem, bio "
        "FROM testtable WHERE ID = ?";

    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt_select, NULL) != SQLITE_OK) {
        printf("SQLエラー: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt_select, 1, id);

    int rc = sqlite3_step(stmt_select);
    if (rc != SQLITE_ROW) {
        printf("該当するIDの受験者情報が見つかりません。\n");
        sqlite3_finalize(stmt_select);
        return;
    }

    // 既存データ読み込み
    const unsigned char *db_name = sqlite3_column_text(stmt_select, 0);
    int db_exam_day = sqlite3_column_int(stmt_select, 1);

    strncpy(name, (const char *)db_name, sizeof(name));
    name[sizeof(name)-1] = '\0';

    snprintf(exam_day, sizeof(exam_day), "%08d", db_exam_day);

    for (int i = 0; i < 9; i++) {
        scores[i] = sqlite3_column_int(stmt_select, 2 + i);
    }

    sqlite3_finalize(stmt_select);

    // 既存情報表示
    printf("現在の情報:\n");
    printf("氏名: %s\n", name);
    printf("試験日: %s\n", exam_day);
    for (int i = 0; i < 9; i++) {
        printf("%s: %d\n", subjects[i], scores[i]);
    }

    // --- 2. 変更入力 ---

    printf("各項目を変更します。変更しない場合はEnterのみ押してください。\n");
    
    // 氏名入力（最初に）
   while (1) {
    printf("新しい氏名（カタカナ20文字以内）: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    int result = validate_name_update(name);
    if (result == NAME_ERR_EMPTY) {
        printf("エラー: 氏名が空です。1文字以上入力してください。\n");
    } else if (result == NAME_ERR_INVALID_CHAR) {
        printf("エラー: 氏名にカタカナ以外の文字が含まれています。\n");
        printf("※使える文字：全角カタカナ（ァ～ヶ、ー のみ）\n");
    } else if (result == NAME_ERR_LENGTH) {
        printf("エラー: 氏名はカタカナ20文字以内で入力してください。\n");
    } else {
        break; // 正常
    }
}


    // 試験日入力
    while (1) {
        printf("新しい試験日（YYYYMMDD）: ");
        fgets(exam_day, sizeof(exam_day), stdin);
        exam_day[strcspn(exam_day, "\n")] = '\0';
        if (!is_valid_date(exam_day)) {
            printf("無効な日付です。再入力してください。\n");
        } else {
            break;
        }
    }

    // 各科目点数入力
    for (int i = 0; i < 9; i++) {
        scores[i] = input_score(subjects[i]);
    }

    // SQLite更新文の準備
    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE testtable SET name=?, exam_day=?, nLang=?, math=?, Eng=?, "
        "JHist=?, wHist=?, geo=?, phys=?, chem=?, bio=? WHERE ID=?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("SQLエラー: %s\n", sqlite3_errmsg(db));
        return;
    }

    // パラメータのバインド
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, atoi(exam_day));
    for (int i = 0; i < 9; i++) {
        sqlite3_bind_int(stmt, 3 + i, scores[i]);
    }
    sqlite3_bind_int(stmt, 12, id);

    // 実行
    if (sqlite3_step(stmt) == SQLITE_DONE) {
    int changes = sqlite3_changes(db);
    if (changes > 0) {
        printf("受験者情報を正常に更新しました。（%d件変更）\n", changes);
    } else {
        printf("該当するIDが存在しませんでした。更新は行われませんでした。\n");
    }
} else {
    printf("更新に失敗しました: %s\n", sqlite3_errmsg(db));
}

    sqlite3_finalize(stmt);


}


/*main.cと結合したため不要
    int main(void) {
    sqlite3 *db;
    int rc = sqlite3_open("examdata.db", &db);  // 適宜データベース名変更

    if (rc != SQLITE_OK) {
        printf("データベースを開けません: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    examdata(db);

    sqlite3_close(db);
    return 0;
} 

*/

