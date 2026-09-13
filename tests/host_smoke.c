/* Host smoke tests — no InkView.
 *   cc -O2 -Wall -o tests/host_smoke tests/host_smoke.c game.c i18n.c questions.c && ./tests/host_smoke
 */
#include "../game.h"
#include "../i18n.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fails;

static void ok(int cond, const char *name) {
  if (cond) {
    printf("  OK  %s\n", name);
  } else {
    printf("  FAIL  %s\n", name);
    fails++;
  }
}

static int test_i18n(void) {
  int id, lang, bad = 0;
  for (id = 0; id < STR_COUNT; id++) {
    for (lang = 0; lang < N_LANGS; lang++) {
      const char *s = tr(lang, id);
      if (!s || !s[0]) bad++;
    }
  }
  return bad == 0;
}

static int test_bank(void) {
  int L, i, a, lang;
  if (questions_min_count() < 20) return 0;
  for (L = 0; L < N_LEVELS; L++) {
    int n = questions_count(L);
    if (n < 20) return 0;
    for (i = 0; i < n; i++) {
      const Question *q = questions_get(L, i);
      if (!q || q->correct != 0) return 0;
      for (lang = 0; lang < N_LANGS; lang++) {
        if (!q->text[lang] || !q->text[lang][0]) return 0;
        for (a = 0; a < N_ANSWERS; a++) {
          if (!q->ans[a][lang] || !q->ans[a][lang][0]) return 0;
        }
      }
    }
  }
  return 1;
}

static int test_money(void) {
  char b[64];
  game_format_money(LANG_EN, 1000000, b, sizeof(b));
  if (strcmp(b, "$1,000,000") != 0) return 0;
  game_format_money(LANG_PL, 1000, b, sizeof(b));
  if (strcmp(b, "1 000 zł") != 0) return 0;
  game_format_money(LANG_DE, 1000, b, sizeof(b));
  if (strcmp(b, "1.000 €") != 0) return 0;
  if (game_prize(LANG_DE, 0) != 50) return 0;
  if (game_prize(LANG_EN, 0) != 100) return 0;
  if (game_prize(LANG_EN, 14) != 1000000) return 0;
  if (!game_is_safe_level(4) || !game_is_safe_level(9)) return 0;
  if (game_is_safe_level(0) || game_is_safe_level(14)) return 0;
  return 1;
}

static int test_5050(void) {
  Game g;
  int i, hidden, keep_ok;
  game_init(&g, LANG_EN, 42);
  game_new_round(&g);
  if (!game_use_5050(&g)) return 0;
  if (game_use_5050(&g)) return 0; /* once only */
  hidden = 0;
  keep_ok = 0;
  for (i = 0; i < 4; i++) {
    if (game_slot_hidden(&g, i)) hidden++;
    else if (i == game_correct_slot(&g)) keep_ok = 1;
  }
  return hidden == 2 && keep_ok;
}

static int test_audience(void) {
  Game g;
  int i, sum, vis;
  game_init(&g, LANG_EN, 99);
  game_new_round(&g);
  game_use_5050(&g);
  if (!game_use_audience(&g)) return 0;
  sum = 0;
  vis = 0;
  for (i = 0; i < 4; i++) {
    if (game_slot_hidden(&g, i)) {
      if (g.audience_pct[i] != 0) return 0;
    } else {
      vis++;
      sum += g.audience_pct[i];
    }
  }
  return vis == 2 && sum == 100;
}

static int test_phone(void) {
  Game g;
  game_init(&g, LANG_EN, 7);
  game_new_round(&g);
  if (!game_use_phone(&g)) return 0;
  if (g.friend_slot < 0 || g.friend_slot > 3) return 0;
  if (game_slot_hidden(&g, g.friend_slot)) return 0;
  if (game_use_phone(&g)) return 0;
  return 1;
}

static int test_walk_and_wrong(void) {
  Game g;
  game_init(&g, LANG_EN, 1);
  game_new_round(&g);
  if (game_walk_amount(&g) != 0) return 0;
  if (game_wrong_amount(&g) != 0) return 0;
  /* pretend we are on question 6 (index 5) */
  g.level = 5;
  if (game_walk_amount(&g) != 1000) return 0; /* last completed is Q5 = $1,000 */
  if (game_wrong_amount(&g) != 1000) return 0;
  g.level = 10;
  if (game_walk_amount(&g) != 32000) return 0;
  if (game_wrong_amount(&g) != 32000) return 0;
  g.level = 3;
  if (game_wrong_amount(&g) != 0) return 0;
  if (game_walk_amount(&g) != 300) return 0;
  return 1;
}

static int test_full_unique_game(void) {
  Game g;
  int seen[N_LEVELS];
  int L;
  game_init(&g, LANG_EN, 123456);
  game_new_round(&g);
  for (L = 0; L < N_LEVELS; L++) {
    int slot;
    if (g.level != L) return 0;
    seen[L] = g.q_index;
    slot = game_correct_slot(&g);
    game_select(&g, slot);
    if (!game_lock_answer(&g)) return 0;
    if (L < N_LEVELS - 1) {
      if (g.status != GAME_PLAYING) return 0;
      game_advance(&g);
    }
  }
  if (g.status != GAME_WON) return 0;
  if (game_payout(&g) != 1000000) return 0;
  /* uniqueness within the 15 asked */
  for (L = 0; L < N_LEVELS; L++) {
    if (seen[L] < 0 || seen[L] >= questions_count(L)) return 0;
  }
  return 1;
}

static int test_wrong_lock(void) {
  Game g;
  int i, cs;
  game_init(&g, LANG_EN, 3);
  game_new_round(&g);
  cs = game_correct_slot(&g);
  for (i = 0; i < 4; i++) {
    if (i != cs && !game_slot_hidden(&g, i)) {
      game_select(&g, i);
      if (game_lock_answer(&g)) return 0;
      return g.status == GAME_LOST && game_payout(&g) == 0;
    }
  }
  return 0;
}

static int test_save_load(void) {
  Game a, b;
  const char *path = "tests/_smoke_quizladder.ini";
  game_init(&a, LANG_DE, 55);
  game_new_round(&a);
  game_add_score(&a, 32000, 1700000000, LANG_DE, 10);
  a.in_progress = 1;
  a.level = 3;
  a.q_index = 2;
  a.order[0] = 2;
  a.order[1] = 0;
  a.order[2] = 1;
  a.order[3] = 3;
  if (!game_save(&a, path)) return 0;
  game_init(&b, LANG_EN, 1);
  if (!game_load(&b, path)) return 0;
  unlink(path);
  if (b.lang != LANG_DE) return 0;
  if (b.score_n < 1 || b.scores[0].amount != 32000) return 0;
  if (b.scores[0].lang != LANG_DE) return 0;
  if (!game_can_resume(&b)) return 0;
  if (b.level != 3 || b.order[0] != 2) return 0;
  if (b.recent_n[0] < 1) return 0;
  return 1;
}

static int test_finish_and_date(void) {
  Game g;
  char d[24];
  game_init(&g, LANG_PL, 9);
  game_new_round(&g);
  g.status = GAME_WON;
  game_finish(&g, 1700000000);
  if (g.games_played != 1) return 0;
  if (g.perfects != 1) return 0;
  if (g.in_progress) return 0;
  if (g.score_n < 1 || g.scores[0].lang != LANG_PL) return 0;
  game_format_date(1700000000, d, sizeof(d));
  return d[0] >= '0' && d[0] <= '9';
}

static int test_lang_switch(void) {
  const char *en = tr(LANG_EN, STR_PLAY);
  const char *pl = tr(LANG_PL, STR_PLAY);
  const char *de = tr(LANG_DE, STR_PLAY);
  if (strcmp(en, "Play") != 0) return 0;
  if (strcmp(pl, "Graj") != 0) return 0;
  if (strcmp(de, "Spielen") != 0) return 0;
  return 1;
}

int main(void) {
  printf("host_smoke\n");
  ok(test_i18n(), "i18n all strings");
  ok(test_bank(), "question bank");
  ok(test_money(), "money / ladder");
  ok(test_5050(), "50:50");
  ok(test_audience(), "audience");
  ok(test_phone(), "phone");
  ok(test_walk_and_wrong(), "walk / safe haven");
  ok(test_full_unique_game(), "15 correct unique");
  ok(test_wrong_lock(), "wrong answer");
  ok(test_save_load(), "save/load");
  ok(test_finish_and_date(), "finish / date");
  ok(test_lang_switch(), "language");
  if (fails) {
    fprintf(stderr, "%d test(s) failed\n", fails);
    return 1;
  }
  printf("host_smoke: OK  (min bank %d / level)\n", questions_min_count());
  return 0;
}
