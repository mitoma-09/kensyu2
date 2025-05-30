#include <stdio.h>
#include <sqlite3.h>
#include "db_operations.h" // db_operations.h をインクルード

int main(int argc, char **argv) {
    sqlite3 *db;
    int rc;

    // DB接続
    rc = sqlite3_open("mydatabase.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(1);
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    // テーブル作成（初回起動時のみ実行されると仮定）
    // 通常はアプリケーション起動時にテーブルが存在するか確認し、なければ作成する
    if (create_table(db) != 0) {
        sqlite3_close(db);
        return(1);
    }

    // ★ ここからユーザー操作のメインループを開始します ★
    int firstNum; // ユーザーの選択を保持する変数

    do {
        printf("\n---テスト結果管理ツールへようこそ---\n");
        printf("---利用したい機能を選択してください---\n");
        printf("1.【登録】\n");
        printf("2.【変更】\n");
        printf("3.【削除】\n");
        printf("4.【参照】\n");
        printf("\n");
        printf("終了したい場合は 5 を入力してください\n");
        printf(">"); // ユーザー入力のプロンプト

        // 機能選択の入力処理
        if (scanf("%d", &firstNum) != 1) {
            // scanf が整数以外の入力を検出した場合
            printf("無効な入力です。数字を入力してください。\n");
            // 不正な入力をクリアする処理 (重要!)
            while (getchar() != '\n');
            firstNum = -1; // ループを継続させるための範囲外の値
            continue; // ループの先頭に戻って再入力促す
        }

        // 入力値の範囲チェック
        if (firstNum < 1 || firstNum > 5) {
            printf("入力された数字が範囲外です (1～5)。再度入力してください。\n");
            // この場合も continue でループの先頭に戻ります
            continue;
        }

        // ユーザーが5を選択した場合はループを抜ける（終了）
        if (firstNum == 5) {
            break; // do-while ループを抜ける
        }

        // 選択された操作に応じた処理
        switch (firstNum) {
            case 1: // 1.登録
                printf("受験者情報の登録、もしくは、既登録者のテスト結果の登録ができます。\n");
                // TODO: 登録処理を呼び出す関数を追加
                break;
            case 2: // 2.変更
                printf("受験者情報の変更ができます。\n");
                // TODO: 変更処理を呼び出す関数を追加
                break;
            case 3: // 3.削除
                printf("受験者情報の削除ができます。\n");
                // TODO: 削除処理を呼び出す関数を追加
                break;
            case 4: // 4.参照
                printf("結果による様々な情報を参照できます。\n");
                // TODO: 参照処理を呼び出す関数を追加
                select_data(db); // 仮に既存のselect_dataをここに移動
                break;
            default:
                // 範囲チェックで弾かれるため、通常ここには到達しない
                break;
        }

        // 各ケースの処理が終わった後、再度メニューを表示するためにループの先頭に戻る

    } while (firstNum != 5); // firstNum が 5 でない限りループを続ける

    // ユーザーが 5 を選択し、ループを抜けた後の処理
    printf("テスト結果管理ツールを終了します。\n");

    // DBクローズ
    sqlite3_close(db);

    return 0; // プログラムを正常終了
}