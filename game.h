#ifndef QUIZ_LADDER_GAME_H
#define QUIZ_LADDER_GAME_H

#include "questions.h"

#define N_SAFE 2
#define N_SCORES 10
#define N_RECENT 48

enum {
  GAME_PLAYING = 0,
  GAME_WON = 1,
  GAME_LOST = 2,
  GAME_WALKED = 3
};

typedef struct {
  long amount;
  long when;
  int lang;
  int level; /* questions answered correctly, 0–15 */
} ScoreEntry;

typedef struct {
  int lang;     /* UI language */
  int run_lang; /* prize ladder locked for the current run */
  int level;
  int q_index;
  int q_id;
  int order[N_ANSWERS];
  int hidden[N_ANSWERS];
  int selected;
  int used_5050;
  int used_phone;
  int used_audience;
  int audience_pct[N_ANSWERS];
  int friend_slot;
  int friend_sure;
  int status;
  int used_in_game[N_LEVELS];
  int recent[N_LEVELS][N_RECENT];
  int recent_n[N_LEVELS];
  ScoreEntry scores[N_SCORES];
  int score_n;
  unsigned rng;
  int in_progress;
  int awaiting_advance;
  int games_played;
  int best_level;
  int perfects;
} Game;

void game_seed(Game *g, unsigned seed);
int game_rand(Game *g);

void game_init(Game *g, int lang, unsigned seed);
void game_set_lang(Game *g, int lang);
void game_new_round(Game *g);
void game_clear_progress(Game *g);
int game_can_resume(const Game *g);
void game_finish(Game *g, long when);

const Question *game_question(const Game *g);
int game_canonical(const Game *g, int slot);
int game_correct_slot(const Game *g);
int game_slot_hidden(const Game *g, int slot);

void game_select(Game *g, int slot);
int game_lock_answer(Game *g);
void game_advance(Game *g);
void game_walk_away(Game *g);
long game_payout(const Game *g);
int game_corrects(const Game *g);

int game_use_5050(Game *g);
int game_use_phone(Game *g);
int game_use_audience(Game *g);

int game_prize_lang(const Game *g);
long game_prize(int lang, int level_index);
long game_current_question_prize(const Game *g);
long game_walk_amount(const Game *g);
long game_wrong_amount(const Game *g);
long game_guaranteed(const Game *g);
int game_is_safe_level(int level_index);

void game_format_money(int lang, long amount, char *buf, int bufsz);
void game_format_date(long when, char *buf, int bufsz);
void game_add_score(Game *g, long amount, long when, int lang, int level);

int game_save(const Game *g, const char *path);
int game_load(Game *g, const char *path);

#endif
