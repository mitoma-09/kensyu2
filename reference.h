#ifndef REFERENCE_H
#define REFERENCE_H

int reference(sqlite3 *database);

int disp_choice1(sqlite3 *db);
int disp_choice2(sqlite3 *db);

int top_sort(int person, char *subject, char *text);
int top_sort_sum(int person, char *text);

int top_sort_day(int day, int person, char *subject, char *text);
int top_sort_day_sum(int day, int person, char *text);

double calc_subject_average(const char *subject, int day, char *text);
double calc_average(int day,char *text);

//double aver_sum(char *text);
//double aver_day_sum(int day, char *text);

int under_average(int day, char *subject, char *text);
int under_average_sum(int day, char *text);

int under_average_all(char *subject, char *text);
int under_average_sum_all(char *text);

int validate_date(const int date);

int execute_sql(const char *sql, int (*callback)(void *, int, char **, char **), void *callback_data);

void display_deviation_scores( const char *subject, int day, char *text);

double calc_subject_std(const char *subject, int day, char *text);

#endif //REFERENCE_H