#ifndef QUIZ_LADDER_QUESTIONS_H
#define QUIZ_LADDER_QUESTIONS_H

#define N_LANGS 3
#define LANG_EN 0
#define LANG_PL 1
#define LANG_DE 2

#define N_LEVELS 15
#define N_ANSWERS 4

typedef struct {
  const char *text[N_LANGS];
  const char *ans[N_ANSWERS][N_LANGS];
  int correct;
} Question;

int questions_count(int level);
const Question *questions_get(int level, int index);
int questions_min_count(void);

#endif
