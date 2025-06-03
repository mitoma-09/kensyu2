#ifndef SUBJECT_UTILS_H
#define SUBJECT_UTILS_H

// 科目名取得
const char *get_subject_name(int choice);

// 科目一覧表示
// display_subjectsはグローバル変数 social_selected, science_selected に依存しているので、
// これらも関数として引数で渡すか、グローバル変数としてここに宣言する必要があります。
// 今回は一時的にグローバル変数をそのまま残しますが、本来は引数で渡すのが望ましいです。
extern int social_selected; // グローバル変数の宣言
extern int science_selected; // グローバル変数の宣言
void display_subjects();

#endif // SUBJECT_UTILS_H