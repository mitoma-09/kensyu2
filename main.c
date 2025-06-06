#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h> // setlocale のため
#include <ctype.h>  // tolower のため

#include "sqlite3.h"
#include "db_operations.h"     // DB操作関連の関数

#include "reference.h"   //参照機能

#define MAX_SQL_SIZE 2000

int main(int argc, char **argv) {
    // 1. ロケール設定
    setlocale(LC_ALL, "ja_JP.UTF-8");

    sqlite3 *db; // DB接続ポインタ

    // 2. データベース接続
    // connect_to_database関数内で "testmanager.db" を使用していることを確認してください
    db = connect_to_database("testmanager.db");
    if (db == NULL) {
        // connect_to_databaseが失敗した場合のエラー処理
        fprintf(stderr, "データベース接続に失敗しました。\n");
        return 1; // プログラム終了
    }

    // 3. テーブル作成（初回起動時のみ実行されることを想定）
    // create_tables関数内で必要なテーブル定義と名前が使われていることを確認してください
    if (create_tables(db) != 0) {
        fprintf(stderr, "テーブル作成に失敗しました。\n");
        sqlite3_close(db);
        return 1; // プログラム終了
    }

    int firstNum; // ユーザーの選択を保持する変数

    // 4. メインメニューとユーザーインタラクションループ
    do {
        printf("\n---テスト結果管理ツールへようこそ---\n");
        printf("---利用したい機能を選択してください---\n");
        printf("1.【登録】\n");
        printf("2.【変更】\n");
        printf("3.【削除】\n");
        printf("4.【参照】\n");
        printf("\n");
        printf("終了したい場合は 5 を入力してください\n");
        printf(">");

        if (scanf("%d", &firstNum) != 1) {
            printf("無効な入力です。数字を入力してください。\n");
            while (getchar() != '\n'); // 不正な入力をクリア
            firstNum = -1; // ループを継続させるための範囲外の値
            continue;
        }
        while (getchar() != '\n'); // scanfの後の改行を消費

        if (firstNum < 1 || firstNum > 5) {
            printf("入力された数字が範囲外です (1～5)。再度入力してください。\n");
            continue;
        }

        if (firstNum == 5) {
            break; // ユーザーが5を選択した場合はループを抜ける（終了）
        }

        // 5. 選択された操作に応じた処理
        switch (firstNum) {
            case 1: // 1.登録
                printf("登録処理を実行します。（未実装）\n");
                // TODO: 登録処理を呼び出す関数を追加 (例: register_data(db); )
                break;
            case 2: // 2.変更
                printf("受験者情報の変更ができます。（未実装）\n");
                // TODO: 変更処理を呼び出す関数を追加 (例: modify_data(db); )
                break;
            case 3: // 3.削除
                printf("受験者情報の削除ができます。（未実装）\n");
                // TODO: 削除処理を呼び出す関数を追加 (例: delete_data(db); )
                break;
            case 4: // 4.参照
                printf("結果による様々な情報を参照できます。\n");
                //disp_choice1(db);
                //disp_choice2(db); 
                break;
            default:
                break;
        }

    } while (firstNum != 5);

    printf("テスト結果管理ツールを終了します。\n");

    // 6. データベースを閉じる
    sqlite3_close(db);

    return 0; // プログラム正常終了
}