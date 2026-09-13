#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const long PRIZE[N_LANGS][N_LEVELS] = {
  /* EN — dollar ladder */
  {100, 200, 300, 500, 1000,
   2000, 4000, 8000, 16000, 32000,
   64000, 125000, 250000, 500000, 1000000},
  /* PL — złoty ladder */
  {100, 200, 300, 500, 1000,
   2000, 4000, 8000, 16000, 32000,
   64000, 125000, 250000, 500000, 1000000},
  /* DE — euro ladder */
  {50, 100, 200, 300, 500,
   1000, 2000, 4000, 8000, 16000,
   32000, 64000, 125000, 500000, 1000000}
};

static const int SAFE_AFTER[N_SAFE] = {4, 9}; /* 0-based question indices */

void game_seed(Game *g, unsigned seed) {
  g->rng = seed ? seed : 1u;
}

int game_rand(Game *g) {
  g->rng = g->rng * 1103515245u + 12345u;
  return (int)((g->rng >> 16) & 32767);
}

static int rand_n(Game *g, int n) {
  if (n <= 1) return 0;
  return game_rand(g) % n;
}

long game_prize(int lang, int level_index) {
  if (lang < 0 || lang >= N_LANGS) lang = LANG_EN;
  if (level_index < 0 || level_index >= N_LEVELS) return 0;
  return PRIZE[lang][level_index];
}

int game_is_safe_level(int level_index) {
  int i;
  for (i = 0; i < N_SAFE; i++) {
    if (SAFE_AFTER[i] == level_index) return 1;
  }
  return 0;
}

int game_prize_lang(const Game *g) {
  int lang = g->run_lang;
  if (lang < 0 || lang >= N_LANGS) lang = g->lang;
  if (lang < 0 || lang >= N_LANGS) lang = LANG_EN;
  return lang;
}

long game_current_question_prize(const Game *g) {
  if (g->level < 0 || g->level >= N_LEVELS) return 0;
  return game_prize(game_prize_lang(g), g->level);
}

long game_walk_amount(const Game *g) {
  if (g->level <= 0) return 0;
  return game_prize(game_prize_lang(g), g->level - 1);
}

long game_wrong_amount(const Game *g) {
  int i;
  long best = 0;
  for (i = 0; i < N_SAFE; i++) {
    if (g->level > SAFE_AFTER[i]) {
      best = game_prize(game_prize_lang(g), SAFE_AFTER[i]);
    }
  }
  return best;
}

long game_guaranteed(const Game *g) {
  return game_wrong_amount(g);
}

int game_corrects(const Game *g) {
  if (g->status == GAME_WON) return N_LEVELS;
  return g->level;
}

static void group_digits(char *dst, int dstsz, long n, char sep) {
  char tmp[32];
  int i, len, out, pad;
  snprintf(tmp, sizeof(tmp), "%ld", n < 0 ? -n : n);
  len = (int)strlen(tmp);
  out = 0;
  if (n < 0 && out + 1 < dstsz) dst[out++] = '-';
  for (i = 0; i < len && out + 1 < dstsz; i++) {
    pad = len - i;
    if (i > 0 && pad % 3 == 0) dst[out++] = sep;
    dst[out++] = tmp[i];
  }
  dst[out] = '\0';
}

void game_format_date(long when, char *buf, int bufsz) {
  time_t t;
  struct tm *tm;
  if (!buf || bufsz <= 0) return;
  if (when <= 0) {
    snprintf(buf, (size_t)bufsz, "—");
    return;
  }
  t = (time_t)when;
  tm = localtime(&t);
  if (!tm) {
    snprintf(buf, (size_t)bufsz, "—");
    return;
  }
  snprintf(buf, (size_t)bufsz, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1,
           tm->tm_mday);
}

void game_format_money(int lang, long amount, char *buf, int bufsz) {
  char num[40];
  if (!buf || bufsz <= 0) return;
  if (lang == LANG_PL) {
    group_digits(num, sizeof(num), amount, ' ');
    snprintf(buf, (size_t)bufsz, "%s zł", num);
  } else if (lang == LANG_DE) {
    group_digits(num, sizeof(num), amount, '.');
    snprintf(buf, (size_t)bufsz, "%s €", num);
  } else {
    group_digits(num, sizeof(num), amount, ',');
    snprintf(buf, (size_t)bufsz, "$%s", num);
  }
}

void game_set_lang(Game *g, int lang) {
  if (lang < 0 || lang >= N_LANGS) lang = LANG_EN;
  g->lang = lang;
}

static int in_recent(const Game *g, int level, int idx) {
  int i;
  for (i = 0; i < g->recent_n[level]; i++) {
    if (g->recent[level][i] == idx) return 1;
  }
  return 0;
}

static void remember(Game *g, int level, int idx) {
  int i, cap, n;
  n = questions_count(level);
  cap = n - 1;
  if (cap < 1) cap = 1;
  if (cap > N_RECENT) cap = N_RECENT;
  if (g->recent_n[level] < cap) {
    g->recent[level][g->recent_n[level]++] = idx;
    return;
  }
  for (i = 1; i < g->recent_n[level]; i++) g->recent[level][i - 1] = g->recent[level][i];
  g->recent[level][g->recent_n[level] - 1] = idx;
}

static void shuffle_order(Game *g) {
  int i, j, t;
  for (i = 0; i < N_ANSWERS; i++) g->order[i] = i;
  for (i = N_ANSWERS - 1; i > 0; i--) {
    j = rand_n(g, i + 1);
    t = g->order[i];
    g->order[i] = g->order[j];
    g->order[j] = t;
  }
}

static void deal_question(Game *g) {
  int n = questions_count(g->level);
  int i, idx, tries, pick;
  int pool[256];
  int pool_n = 0;

  if (n <= 0) {
    g->status = GAME_LOST;
    return;
  }
  if (n > 256) n = 256;

  for (tries = 0; tries < 2; tries++) {
    pool_n = 0;
    for (i = 0; i < n; i++) {
      if (tries == 0 && in_recent(g, g->level, i)) continue;
      pool[pool_n++] = i;
    }
    if (pool_n > 0) break;
    g->recent_n[g->level] = 0;
  }
  pick = pool[rand_n(g, pool_n)];
  idx = pick;
  g->q_index = idx;
  g->q_id = g->level * 1000 + idx;
  g->used_in_game[g->level] = idx;
  remember(g, g->level, idx);
  shuffle_order(g);
  for (i = 0; i < N_ANSWERS; i++) g->hidden[i] = 0;
  g->selected = -1;
  g->friend_slot = -1;
  for (i = 0; i < N_ANSWERS; i++) g->audience_pct[i] = 0;
}

void game_init(Game *g, int lang, unsigned seed) {
  memset(g, 0, sizeof(*g));
  game_seed(g, seed);
  game_set_lang(g, lang);
  g->run_lang = g->lang;
  g->selected = -1;
  g->friend_slot = -1;
}

void game_new_round(Game *g) {
  int lang = g->lang;
  unsigned rng = g->rng;
  int recent[N_LEVELS][N_RECENT];
  int recent_n[N_LEVELS];
  ScoreEntry scores[N_SCORES];
  int score_n = g->score_n;
  int games = g->games_played;
  int best = g->best_level;
  int perfects = g->perfects;
  memcpy(recent, g->recent, sizeof(recent));
  memcpy(recent_n, g->recent_n, sizeof(recent_n));
  memcpy(scores, g->scores, sizeof(scores));
  memset(g, 0, sizeof(*g));
  g->rng = rng;
  g->lang = lang;
  g->run_lang = lang;
  g->score_n = score_n;
  g->games_played = games;
  g->best_level = best;
  g->perfects = perfects;
  memcpy(g->recent, recent, sizeof(recent));
  memcpy(g->recent_n, recent_n, sizeof(recent_n));
  memcpy(g->scores, scores, sizeof(scores));
  g->selected = -1;
  g->friend_slot = -1;
  g->in_progress = 1;
  g->awaiting_advance = 0;
  deal_question(g);
}

void game_clear_progress(Game *g) {
  g->in_progress = 0;
  g->awaiting_advance = 0;
  g->status = GAME_PLAYING;
  g->level = 0;
  g->selected = -1;
}

int game_can_resume(const Game *g) {
  return g->in_progress && g->status == GAME_PLAYING;
}

void game_finish(Game *g, long when) {
  int reached = game_corrects(g);
  long pay = game_payout(g);
  g->games_played++;
  if (reached > g->best_level) g->best_level = reached;
  if (g->status == GAME_WON) g->perfects++;
  if (pay > 0) game_add_score(g, pay, when, game_prize_lang(g), reached);
  g->in_progress = 0;
  g->awaiting_advance = 0;
}

const Question *game_question(const Game *g) {
  return questions_get(g->level, g->q_index);
}

int game_canonical(const Game *g, int slot) {
  if (slot < 0 || slot >= N_ANSWERS) return 0;
  return g->order[slot];
}

int game_correct_slot(const Game *g) {
  const Question *q = game_question(g);
  int i;
  if (!q) return 0;
  for (i = 0; i < N_ANSWERS; i++) {
    if (g->order[i] == q->correct) return i;
  }
  return 0;
}

int game_slot_hidden(const Game *g, int slot) {
  if (slot < 0 || slot >= N_ANSWERS) return 1;
  return g->hidden[slot];
}

void game_select(Game *g, int slot) {
  if (g->status != GAME_PLAYING) return;
  if (slot < 0 || slot >= N_ANSWERS) return;
  if (g->hidden[slot]) return;
  g->selected = slot;
}

int game_lock_answer(Game *g) {
  int ok;
  if (g->status != GAME_PLAYING) return 0;
  if (g->selected < 0 || g->hidden[g->selected]) return 0;
  ok = (g->selected == game_correct_slot(g));
  if (ok) {
    if (g->level >= N_LEVELS - 1) {
      g->status = GAME_WON;
      g->awaiting_advance = 0;
    } else {
      g->awaiting_advance = 1;
    }
    return 1;
  }
  g->status = GAME_LOST;
  g->awaiting_advance = 0;
  return 0;
}

void game_advance(Game *g) {
  if (g->status != GAME_PLAYING) return;
  if (g->level >= N_LEVELS - 1) {
    g->status = GAME_WON;
    g->awaiting_advance = 0;
    return;
  }
  g->level++;
  g->awaiting_advance = 0;
  deal_question(g);
}

long game_payout(const Game *g) {
  if (g->status == GAME_WON) return game_prize(g->lang, N_LEVELS - 1);
  if (g->status == GAME_WALKED) return game_walk_amount(g);
  if (g->status == GAME_LOST) return game_wrong_amount(g);
  return 0;
}

void game_walk_away(Game *g) {
  if (g->status != GAME_PLAYING) return;
  g->status = GAME_WALKED;
}

int game_use_5050(Game *g) {
  int wrong[N_ANSWERS];
  int n = 0, i, a, t;
  if (g->status != GAME_PLAYING || g->used_5050) return 0;
  for (i = 0; i < N_ANSWERS; i++) {
    if (i != game_correct_slot(g)) wrong[n++] = i;
  }
  if (n < 2) return 0;
  /* pick two wrong slots to hide */
  for (i = n - 1; i > 0; i--) {
    a = rand_n(g, i + 1);
    t = wrong[i];
    wrong[i] = wrong[a];
    wrong[a] = t;
  }
  g->hidden[wrong[0]] = 1;
  g->hidden[wrong[1]] = 1;
  if (g->selected >= 0 && g->hidden[g->selected]) g->selected = -1;
  g->used_5050 = 1;
  return 1;
}

int game_use_phone(Game *g) {
  int correct, roll, i, n, pool[N_ANSWERS];
  int accuracy; /* percent */
  if (g->status != GAME_PLAYING || g->used_phone) return 0;
  correct = game_correct_slot(g);
  if (g->level <= 4) accuracy = 92;
  else if (g->level <= 9) accuracy = 72;
  else if (g->level <= 12) accuracy = 48;
  else accuracy = 28;
  roll = rand_n(g, 100);
  if (roll < accuracy) {
    g->friend_slot = correct;
    g->friend_sure = (g->level <= 4) ? 2 : (g->level <= 9 ? 1 : 0);
  } else {
    n = 0;
    for (i = 0; i < N_ANSWERS; i++) {
      if (!g->hidden[i] && i != correct) pool[n++] = i;
    }
    g->friend_slot = n ? pool[rand_n(g, n)] : correct;
    g->friend_sure = 0;
  }
  if (g->friend_slot == correct && g->level <= 9) g->friend_sure = (g->level <= 4) ? 2 : 1;
  g->used_phone = 1;
  return 1;
}

int game_use_audience(Game *g) {
  int correct, i, remain, share, visible, min_wrong;
  int weights[N_ANSWERS];
  int bias;
  if (g->status != GAME_PLAYING || g->used_audience) return 0;
  correct = game_correct_slot(g);
  if (g->level <= 4) bias = 70 + rand_n(g, 21);      /* 70-90 */
  else if (g->level <= 9) bias = 45 + rand_n(g, 21); /* 45-65 */
  else if (g->level <= 12) bias = 28 + rand_n(g, 18);/* 28-45 */
  else bias = 18 + rand_n(g, 18);                    /* 18-35, can lose */

  visible = 0;
  for (i = 0; i < N_ANSWERS; i++) if (!g->hidden[i]) visible++;
  if (visible < 1) visible = 1;

  for (i = 0; i < N_ANSWERS; i++) weights[i] = 0;
  if (g->hidden[correct]) bias = 0;
  weights[correct] = bias;
  remain = 100 - bias;
  min_wrong = (visible > 1) ? 1 : 0;
  for (i = 0; i < N_ANSWERS; i++) {
    if (g->hidden[i] || i == correct) continue;
    visible--;
    if (visible <= 0) {
      weights[i] = remain;
      remain = 0;
    } else {
      share = (remain > min_wrong) ? rand_n(g, remain - min_wrong + 1) : 0;
      if (share < min_wrong) share = min_wrong;
      if (share > remain) share = remain;
      weights[i] = share;
      remain -= share;
    }
  }
  if (remain > 0) {
    if (!g->hidden[correct]) weights[correct] += remain;
    else {
      for (i = 0; i < N_ANSWERS; i++) {
        if (!g->hidden[i]) { weights[i] += remain; break; }
      }
    }
  }
  for (i = 0; i < N_ANSWERS; i++) {
    g->audience_pct[i] = g->hidden[i] ? 0 : weights[i];
  }
  g->used_audience = 1;
  return 1;
}

void game_add_score(Game *g, long amount, long when, int lang, int level) {
  int i, j;
  if (amount <= 0) return;
  if (lang < 0 || lang >= N_LANGS) lang = LANG_EN;
  if (g->score_n < N_SCORES) {
    g->scores[g->score_n].amount = amount;
    g->scores[g->score_n].when = when;
    g->scores[g->score_n].lang = lang;
    g->scores[g->score_n].level = level;
    g->score_n++;
  } else if (amount > g->scores[N_SCORES - 1].amount) {
    g->scores[N_SCORES - 1].amount = amount;
    g->scores[N_SCORES - 1].when = when;
    g->scores[N_SCORES - 1].lang = lang;
    g->scores[N_SCORES - 1].level = level;
  } else {
    return;
  }
  for (i = 1; i < g->score_n; i++) {
    ScoreEntry key = g->scores[i];
    j = i - 1;
    while (j >= 0 && g->scores[j].amount < key.amount) {
      g->scores[j + 1] = g->scores[j];
      j--;
    }
    g->scores[j + 1] = key;
  }
}

int game_save(const Game *g, const char *path) {
  FILE *f;
  int L, i;
  if (!path) return 0;
  f = fopen(path, "w");
  if (!f) return 0;
  fprintf(f, "lang=%d\n", g->lang);
  fprintf(f, "run_lang=%d\n", g->run_lang);
  fprintf(f, "rng=%u\n", g->rng);
  fprintf(f, "games=%d\n", g->games_played);
  fprintf(f, "best=%d\n", g->best_level);
  fprintf(f, "perfects=%d\n", g->perfects);
  fprintf(f, "in_progress=%d\n", g->in_progress);
  fprintf(f, "awaiting=%d\n", g->awaiting_advance);
  fprintf(f, "status=%d\n", g->status);
  fprintf(f, "level=%d\n", g->level);
  fprintf(f, "q_index=%d\n", g->q_index);
  fprintf(f, "selected=%d\n", g->selected);
  fprintf(f, "used_5050=%d\n", g->used_5050);
  fprintf(f, "used_phone=%d\n", g->used_phone);
  fprintf(f, "used_audience=%d\n", g->used_audience);
  fprintf(f, "friend_slot=%d\n", g->friend_slot);
  fprintf(f, "friend_sure=%d\n", g->friend_sure);
  fprintf(f, "order=%d,%d,%d,%d\n", g->order[0], g->order[1], g->order[2], g->order[3]);
  fprintf(f, "hidden=%d,%d,%d,%d\n", g->hidden[0], g->hidden[1], g->hidden[2], g->hidden[3]);
  fprintf(f, "audience=%d,%d,%d,%d\n", g->audience_pct[0], g->audience_pct[1],
          g->audience_pct[2], g->audience_pct[3]);
  fprintf(f, "scores=%d\n", g->score_n);
  for (i = 0; i < g->score_n; i++) {
    fprintf(f, "score%d=%ld,%ld,%d,%d\n", i, g->scores[i].amount, g->scores[i].when,
            g->scores[i].lang, g->scores[i].level);
  }
  for (L = 0; L < N_LEVELS; L++) {
    fprintf(f, "used%d=", L);
    for (i = 0; i < g->recent_n[L]; i++) {
      if (i) fputc(',', f);
      fprintf(f, "%d", g->recent[L][i]);
    }
    fputc('\n', f);
  }
  fclose(f);
  return 1;
}

static void parse_used_line(Game *g, int level, const char *val) {
  const char *p = val;
  g->recent_n[level] = 0;
  while (*p && g->recent_n[level] < N_RECENT) {
    char *end = NULL;
    long v = strtol(p, &end, 10);
    if (end == p) break;
    g->recent[level][g->recent_n[level]++] = (int)v;
    p = end;
    if (*p == ',') p++;
  }
}

int game_load(Game *g, const char *path) {
  FILE *f;
  char line[1024];
  if (!path) return 0;
  f = fopen(path, "r");
  if (!f) return 0;
  while (fgets(line, sizeof(line), f)) {
    char *eq = strchr(line, '=');
    char *nl;
    if (!eq) continue;
    *eq = '\0';
    eq++;
    nl = strchr(eq, '\n');
    if (nl) *nl = '\0';
    nl = strchr(eq, '\r');
    if (nl) *nl = '\0';
    if (strcmp(line, "lang") == 0) {
      g->lang = atoi(eq);
      if (g->lang < 0 || g->lang >= N_LANGS) g->lang = LANG_EN;
    } else if (strcmp(line, "run_lang") == 0) {
      g->run_lang = atoi(eq);
      if (g->run_lang < 0 || g->run_lang >= N_LANGS) g->run_lang = g->lang;
    } else if (strcmp(line, "rng") == 0) {
      g->rng = (unsigned)strtoul(eq, NULL, 10);
      if (g->rng == 0) g->rng = 1;
    } else if (strcmp(line, "games") == 0) {
      g->games_played = atoi(eq);
    } else if (strcmp(line, "best") == 0) {
      g->best_level = atoi(eq);
    } else if (strcmp(line, "perfects") == 0) {
      g->perfects = atoi(eq);
    } else if (strcmp(line, "in_progress") == 0) {
      g->in_progress = atoi(eq);
    } else if (strcmp(line, "awaiting") == 0) {
      g->awaiting_advance = atoi(eq);
    } else if (strcmp(line, "status") == 0) {
      g->status = atoi(eq);
    } else if (strcmp(line, "level") == 0) {
      g->level = atoi(eq);
    } else if (strcmp(line, "q_index") == 0) {
      g->q_index = atoi(eq);
    } else if (strcmp(line, "selected") == 0) {
      g->selected = atoi(eq);
    } else if (strcmp(line, "used_5050") == 0) {
      g->used_5050 = atoi(eq);
    } else if (strcmp(line, "used_phone") == 0) {
      g->used_phone = atoi(eq);
    } else if (strcmp(line, "used_audience") == 0) {
      g->used_audience = atoi(eq);
    } else if (strcmp(line, "friend_slot") == 0) {
      g->friend_slot = atoi(eq);
    } else if (strcmp(line, "friend_sure") == 0) {
      g->friend_sure = atoi(eq);
    } else if (strcmp(line, "order") == 0) {
      sscanf(eq, "%d,%d,%d,%d", &g->order[0], &g->order[1], &g->order[2], &g->order[3]);
    } else if (strcmp(line, "hidden") == 0) {
      sscanf(eq, "%d,%d,%d,%d", &g->hidden[0], &g->hidden[1], &g->hidden[2], &g->hidden[3]);
    } else if (strcmp(line, "audience") == 0) {
      sscanf(eq, "%d,%d,%d,%d", &g->audience_pct[0], &g->audience_pct[1],
             &g->audience_pct[2], &g->audience_pct[3]);
    } else if (strcmp(line, "scores") == 0) {
      g->score_n = atoi(eq);
      if (g->score_n < 0) g->score_n = 0;
      if (g->score_n > N_SCORES) g->score_n = N_SCORES;
    } else if (strncmp(line, "score", 5) == 0) {
      int idx = atoi(line + 5);
      long amount = 0, when = 0;
      int slang = 0, slev = 0;
      if (idx >= 0 && idx < N_SCORES) {
        if (sscanf(eq, "%ld,%ld,%d,%d", &amount, &when, &slang, &slev) < 2)
          sscanf(eq, "%ld,%ld", &amount, &when);
        g->scores[idx].amount = amount;
        g->scores[idx].when = when;
        g->scores[idx].lang = (slang >= 0 && slang < N_LANGS) ? slang : LANG_EN;
        g->scores[idx].level = slev;
      }
    } else if (strncmp(line, "used", 4) == 0) {
      int L = atoi(line + 4);
      if (L >= 0 && L < N_LEVELS) parse_used_line(g, L, eq);
    }
  }
  fclose(f);
  if (g->level < 0 || g->level >= N_LEVELS) g->in_progress = 0;
  else {
    int n = questions_count(g->level);
    if (g->q_index < 0 || g->q_index >= n) g->in_progress = 0;
  }
  if (g->status != GAME_PLAYING) g->in_progress = 0;
  g->q_id = g->level * 1000 + g->q_index;
  return 1;
}
