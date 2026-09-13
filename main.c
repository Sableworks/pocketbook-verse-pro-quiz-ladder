#include "inkview.h"

#ifndef KEY_HOME
#define KEY_HOME 0x1a
#endif

#include "game.h"
#include "i18n.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define APP_CREDIT "made by Mateusz Blumensztajn (Sableworks)"
#define SAVE_FILE FLASHDIR "/.quizladder.ini"
#define SPLASH_MS 1400
#define DRUM_MS 750
#define SPLASH_TIMER "m_splash"
#define DRUM_TIMER "m_drum"

#define COL_BG WHITE
#define COL_PANEL WHITE
#define COL_INK BLACK
#define COL_MUTED DGRAY
#define COL_LINE BLACK
#define COL_WASH LGRAY
#define COL_INV BLACK
#define COL_ON_INV WHITE
#define COL_GREEN 0x1B7A3A
#define COL_RED 0xB01C1C

#define FONT_TITLE 36
#define FONT_Q 28
#define FONT_ANS 22
#define FONT_UI 20
#define FONT_SMALL 17
#define FONT_LADDER 18

enum {
  UI_SPLASH = 0,
  UI_MENU,
  UI_LANG,
  UI_HELP,
  UI_SCORES,
  UI_STAKE,
  UI_LADDER,
  UI_PLAY,
  UI_CONFIRM,
  UI_DRUM,
  UI_REVEAL,
  UI_SAFE,
  UI_TOP
};

enum {
  CONFIRM_FINAL = 0,
  CONFIRM_WALK = 1,
  CONFIRM_QUIT = 2,
  CONFIRM_NEW = 3
};

enum { OV_NONE = 0, OV_PHONE = 1, OV_AUDIENCE = 2 };

static struct {
  int sw, sh;
  ifont *f_title, *f_q, *f_ans, *f_ui, *f_small, *f_ladder;
  int screen, return_screen;
  int splash;
  int confirm_kind;
  int last_correct;
  int menu_sel;
  int help_scroll;
  int overlay;
  Game game;

  int down, dx, dy;
  irect a_rect[4];
  irect life_rect[3];
  irect walk_rect, lock_rect, prize_rect, ov_ok;
  irect menu_rect[8];
  int menu_n;
  irect yes_rect, no_rect;
  irect ok_rect;
} g;

static const char LETTER[4] = {'A', 'B', 'C', 'D'};

static int Tlang(void) { return g.game.lang; }
static const char *S(int id) { return tr(Tlang(), id); }

static void save_state(void) { game_save(&g.game, SAVE_FILE); }

static int hit(int x, int y, irect r) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static irect mkrect(int x, int y, int w, int h) {
  irect r;
  r.x = x;
  r.y = y;
  r.w = w;
  r.h = h;
  r.flags = 0;
  return r;
}

static void fill_bg(void) { ClearScreen(); }

static void draw_frame(int x, int y, int w, int h, int fill, int border) {
  FillArea(x, y, w, h, fill);
  DrawRect(x, y, w, h, border);
  DrawRect(x + 1, y + 1, w - 2, h - 2, border);
}

static void set_ui_font(ifont *f, int color) {
  if (f) SetFont(f, color);
}

static void draw_wrapped(ifont *f, int color, int x, int y, int w, int h, const char *s,
                         int flags) {
  set_ui_font(f, color);
  DrawTextRect(x, y, w, h, s, flags);
}

static void draw_btn(irect r, const char *label, int fill, int border, int textcol) {
  draw_frame(r.x, r.y, r.w, r.h, fill, border);
  set_ui_font(g.f_ui, textcol);
  DrawTextRect(r.x + 6, r.y, r.w - 12, r.h, label, ALIGN_CENTER | VALIGN_MIDDLE);
}

static void draw_choice(irect r, const char *label, int on) {
  draw_btn(r, label, on ? COL_INV : COL_PANEL, COL_LINE, on ? COL_ON_INV : COL_INK);
}

static void close_fonts(void) {
  if (g.f_title) CloseFont(g.f_title);
  if (g.f_q) CloseFont(g.f_q);
  if (g.f_ans) CloseFont(g.f_ans);
  if (g.f_ui) CloseFont(g.f_ui);
  if (g.f_small) CloseFont(g.f_small);
  if (g.f_ladder) CloseFont(g.f_ladder);
  g.f_title = g.f_q = g.f_ans = g.f_ui = g.f_small = g.f_ladder = NULL;
}

static void draw_credit_footer(void) {
  set_ui_font(g.f_small, COL_MUTED);
  DrawTextRect(16, g.sh - 40, g.sw - 32, 32, APP_CREDIT, ALIGN_CENTER | VALIGN_MIDDLE);
}

static void draw_x(int x, int y, int w, int h) {
  DrawLine(x + 6, y + 6, x + w - 6, y + h - 6, COL_INK);
  DrawLine(x + w - 6, y + 6, x + 6, y + h - 6, COL_INK);
}

static void draw_life_icon(irect r, int kind, int used) {
  int cx = r.x + r.w / 2;
  int cy = r.y + r.h / 2 - 6;
  draw_frame(r.x, r.y, r.w, r.h, used ? COL_WASH : COL_PANEL, used ? COL_MUTED : COL_LINE);
  set_ui_font(g.f_small, used ? COL_MUTED : COL_INK);
  if (kind == 0) {
    DrawTextRect(r.x, cy - 10, r.w, 28, "50:50", ALIGN_CENTER | VALIGN_MIDDLE);
  } else if (kind == 1) {
    DrawRect(cx - 10, cy - 14, 20, 28, COL_INK);
    DrawLine(cx, cy + 14, cx, cy + 20, COL_INK);
    DrawLine(cx - 8, cy + 20, cx + 8, cy + 20, COL_INK);
    DrawTextRect(r.x + 2, r.y + r.h - 20, r.w - 4, 18, S(STR_LIFELINE_PHONE),
                 ALIGN_CENTER | VALIGN_MIDDLE | DOTS);
  } else {
    FillArea(cx - 16, cy - 2, 8, 10, COL_INK);
    FillArea(cx - 4, cy - 6, 8, 14, COL_INK);
    FillArea(cx + 8, cy - 2, 8, 10, COL_INK);
    DrawTextRect(r.x + 2, r.y + r.h - 20, r.w - 4, 18, S(STR_LIFELINE_AUDIENCE),
                 ALIGN_CENTER | VALIGN_MIDDLE | DOTS);
  }
  if (used) draw_x(r.x, r.y, r.w, r.h);
}

static void layout_menu_buttons(int n, int top) {
  int i, bh = (n >= 6) ? 68 : 76;
  int gap = 10;
  int x = 56;
  int w = g.sw - 112;
  g.menu_n = n;
  for (i = 0; i < n; i++) g.menu_rect[i] = mkrect(x, top + i * (bh + gap), w, bh);
}

static void draw_splash(void) {
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(24, g.sh / 2 - 140, g.sw - 48, 90, S(STR_APP), ALIGN_CENTER | VALIGN_MIDDLE);
  set_ui_font(g.f_ui, COL_INK);
  DrawTextRect(32, g.sh / 2 - 30, g.sw - 64, 70, S(STR_TAGLINE), ALIGN_CENTER | VALIGN_TOP);
  draw_credit_footer();
  FullUpdate();
}

static void draw_menu(void) {
  const char *labels[8];
  int n, i, resume = game_can_resume(&g.game);
  if (resume) {
    labels[0] = S(STR_RESUME);
    labels[1] = S(STR_NEW_GAME);
    labels[2] = S(STR_LANGUAGE);
    labels[3] = S(STR_HOW_TO);
    labels[4] = S(STR_SCORES);
    labels[5] = S(STR_EXIT);
    n = 6;
  } else {
    labels[0] = S(STR_PLAY);
    labels[1] = S(STR_LANGUAGE);
    labels[2] = S(STR_HOW_TO);
    labels[3] = S(STR_SCORES);
    labels[4] = S(STR_EXIT);
    n = 5;
  }
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(20, 28, g.sw - 40, 70, S(STR_APP), ALIGN_CENTER | VALIGN_MIDDLE);
  set_ui_font(g.f_small, COL_MUTED);
  DrawTextRect(24, 100, g.sw - 48, 40, S(STR_TAGLINE), ALIGN_CENTER | VALIGN_TOP);
  layout_menu_buttons(n, 160);
  if (g.menu_sel >= n) g.menu_sel = 0;
  for (i = 0; i < n; i++) draw_choice(g.menu_rect[i], labels[i], i == g.menu_sel);
  draw_credit_footer();
  FullUpdate();
}

static void draw_lang(void) {
  char lab[64];
  const char *base[4];
  int i;
  base[0] = S(STR_LANG_EN);
  base[1] = S(STR_LANG_PL);
  base[2] = S(STR_LANG_DE);
  base[3] = S(STR_BACK);
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(20, 40, g.sw - 40, 70, S(STR_LANGUAGE), ALIGN_CENTER | VALIGN_MIDDLE);
  layout_menu_buttons(4, 160);
  g.menu_n = 4;
  for (i = 0; i < 4; i++) {
    if (i < 3 && i == Tlang()) snprintf(lab, sizeof(lab), "%s   ✓", base[i]);
    else snprintf(lab, sizeof(lab), "%s", base[i]);
    draw_choice(g.menu_rect[i], lab, i == g.menu_sel);
  }
  FullUpdate();
}

static void draw_help(void) {
  int box_y = 80, box_h = g.sh - 180;
  int text_h, max_scroll;
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(16, 16, g.sw - 32, 56, S(STR_HOW_TO), ALIGN_CENTER | VALIGN_MIDDLE);
  draw_frame(20, box_y, g.sw - 40, box_h, COL_PANEL, COL_LINE);
  set_ui_font(g.f_ui, COL_INK);
  text_h = TextRectHeight(g.sw - 72, S(STR_HELP_BODY), ALIGN_LEFT | HYPHENS);
  max_scroll = text_h - (box_h - 24);
  if (max_scroll < 0) max_scroll = 0;
  if (g.help_scroll > max_scroll) g.help_scroll = max_scroll;
  if (g.help_scroll < 0) g.help_scroll = 0;
  SetClip(24, box_y + 8, g.sw - 48, box_h - 16);
  DrawTextRect(36, box_y + 12 - g.help_scroll, g.sw - 72, text_h + 8, S(STR_HELP_BODY),
               ALIGN_LEFT | VALIGN_TOP | HYPHENS);
  SetClip(0, 0, g.sw, g.sh);
  g.ok_rect = mkrect(80, g.sh - 84, g.sw - 160, 64);
  draw_choice(g.ok_rect, S(STR_BACK), 1);
  FullUpdate();
}

static void draw_scores(void) {
  char line[160], money[48], date[24], st[80];
  int i, y;
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(16, 12, g.sw - 32, 48, S(STR_SCORES), ALIGN_CENTER | VALIGN_MIDDLE);
  snprintf(st, sizeof(st), S(STR_STATS_GAMES), g.game.games_played);
  set_ui_font(g.f_small, COL_INK);
  DrawTextRect(24, 64, g.sw - 48, 24, st, ALIGN_CENTER | VALIGN_MIDDLE);
  snprintf(st, sizeof(st), S(STR_STATS_BEST), g.game.best_level);
  DrawTextRect(24, 88, g.sw / 2 - 16, 24, st, ALIGN_CENTER | VALIGN_MIDDLE);
  snprintf(st, sizeof(st), S(STR_STATS_PERFECT), g.game.perfects);
  DrawTextRect(g.sw / 2, 88, g.sw / 2 - 16, 24, st, ALIGN_CENTER | VALIGN_MIDDLE);
  if (g.game.score_n <= 0) {
    set_ui_font(g.f_ui, COL_INK);
    DrawTextRect(32, 200, g.sw - 64, 120, S(STR_EMPTY_SCORES), ALIGN_CENTER | VALIGN_MIDDLE);
  } else {
    y = 124;
    for (i = 0; i < g.game.score_n; i++) {
      game_format_money(g.game.scores[i].lang, g.game.scores[i].amount, money, sizeof(money));
      game_format_date(g.game.scores[i].when, date, sizeof(date));
      snprintf(line, sizeof(line), "%d.  %s   ·   %s", i + 1, money, date);
      draw_frame(32, y, g.sw - 64, 52, COL_PANEL, COL_LINE);
      set_ui_font(g.f_ans, COL_INK);
      DrawTextRect(44, y, g.sw - 88, 52, line, ALIGN_LEFT | VALIGN_MIDDLE | DOTS);
      y += 58;
    }
  }
  g.ok_rect = mkrect(80, g.sh - 84, g.sw - 160, 64);
  draw_choice(g.ok_rect, S(STR_BACK), 1);
  FullUpdate();
}

static void draw_ladder_body(void) {
  int top = 80, h = g.sh - 180, row = h / N_LEVELS, i;
  char buf[48];
  draw_frame(36, top, g.sw - 72, h, COL_PANEL, COL_LINE);
  for (i = N_LEVELS - 1; i >= 0; i--) {
    int yi = top + 6 + (N_LEVELS - 1 - i) * row;
    int cur = (g.game.level == i && g.game.status == GAME_PLAYING);
    int passed = (g.game.level > i) || (g.game.status == GAME_WON);
    int fill = COL_PANEL, tc = COL_INK;
    if (cur) {
      fill = COL_INV;
      tc = COL_ON_INV;
    } else if (passed) {
      fill = COL_WASH;
    }
    FillArea(40, yi, g.sw - 80, row - 4, fill);
    if (game_is_safe_level(i) && !cur) DrawRect(40, yi, g.sw - 80, row - 4, COL_LINE);
    game_format_money(game_prize_lang(&g.game), game_prize(game_prize_lang(&g.game), i), buf,
                      sizeof(buf));
    set_ui_font(g.f_ladder, tc);
    DrawTextRect(56, yi, g.sw - 112, row - 4, buf, ALIGN_CENTER | VALIGN_MIDDLE);
  }
}

static void draw_ladder(void) {
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(16, 16, g.sw - 32, 52, S(STR_LADDER), ALIGN_CENTER | VALIGN_MIDDLE);
  draw_ladder_body();
  g.ok_rect = mkrect(80, g.sh - 84, g.sw - 160, 64);
  draw_choice(g.ok_rect, S(STR_BACK), 1);
  FullUpdate();
}

static void draw_stake(void) {
  char qn[80], money[48], gbuf[48], line[128];
  long guar;
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(24, 80, g.sw - 48, 60, S(STR_LETS_PLAY), ALIGN_CENTER | VALIGN_MIDDLE);
  snprintf(qn, sizeof(qn), S(STR_Q_OF), g.game.level + 1);
  set_ui_font(g.f_q, COL_INK);
  DrawTextRect(24, 160, g.sw - 48, 50, qn, ALIGN_CENTER | VALIGN_MIDDLE);
  set_ui_font(g.f_ui, COL_INK);
  DrawTextRect(24, 230, g.sw - 48, 40, S(STR_PLAYING_FOR), ALIGN_CENTER | VALIGN_MIDDLE);
  game_format_money(game_prize_lang(&g.game), game_current_question_prize(&g.game), money,
                    sizeof(money));
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(24, 280, g.sw - 48, 70, money, ALIGN_CENTER | VALIGN_MIDDLE);
  guar = game_guaranteed(&g.game);
  game_format_money(game_prize_lang(&g.game), guar, gbuf, sizeof(gbuf));
  snprintf(line, sizeof(line), "%s: %s", S(STR_GUARANTEED_NOW), gbuf);
  set_ui_font(g.f_small, COL_MUTED);
  DrawTextRect(24, 360, g.sw - 48, 40, line, ALIGN_CENTER | VALIGN_MIDDLE);
  g.prize_rect = mkrect(80, 450, g.sw - 160, 70);
  draw_choice(g.prize_rect, S(STR_LADDER), 0);
  g.ok_rect = mkrect(80, g.sh - 120, g.sw - 160, 80);
  draw_choice(g.ok_rect, S(STR_CONTINUE), 1);
  FullUpdate();
}

static void layout_play(void) {
  int m = 16, y, i, ah = 86, gap = 10, bot = 76, life_h = 72;
  int play_w = g.sw - 2 * m;
  y = 12;
  for (i = 0; i < 3; i++) {
    int bw = (play_w - 16) / 3;
    g.life_rect[i] = mkrect(m + i * (bw + 8), y, bw, life_h);
  }
  g.walk_rect = mkrect(m, g.sh - bot - 10, (play_w - m) / 2, bot);
  g.lock_rect = mkrect(m + (play_w - m) / 2 + m, g.sh - bot - 10, (play_w - m) / 2, bot);
  {
    int ans_top = g.sh - bot - 18 - 4 * (ah + gap);
    if (ans_top < 220) ans_top = 220;
    for (i = 0; i < 4; i++)
      g.a_rect[i] = mkrect(m, ans_top + i * (ah + gap), play_w, ah);
  }
}

static void draw_answer_slot(int slot, int reveal) {
  const Question *q = game_question(&g.game);
  irect r = g.a_rect[slot];
  char line[400];
  int can = game_canonical(&g.game, slot);
  int hidden = game_slot_hidden(&g.game, slot);
  int fill = COL_PANEL, tc = COL_INK;
  const char *ans;
  const char *mark = "";

  if (!q) return;
  if (hidden) {
    FillArea(r.x, r.y, r.w, r.h, COL_WASH);
    DrawRect(r.x, r.y, r.w, r.h, COL_MUTED);
    return;
  }
  ans = q->ans[can][Tlang()];
  if (reveal) {
    if (slot == game_correct_slot(&g.game)) {
      fill = COL_INV;
      tc = COL_ON_INV;
      mark = "✓  ";
    } else if (slot == g.game.selected) {
      fill = COL_WASH;
      mark = "✗  ";
    }
  } else if (slot == g.game.selected) {
    fill = COL_INV;
    tc = COL_ON_INV;
  }
  snprintf(line, sizeof(line), "%s%c:  %s", mark, LETTER[slot], ans ? ans : "");
  draw_frame(r.x, r.y, r.w, r.h, fill, COL_LINE);
  set_ui_font(g.f_ans, tc);
  DrawTextRect(r.x + 12, r.y, r.w - 24, r.h, line, ALIGN_LEFT | VALIGN_MIDDLE | DOTS);
}

static void draw_play_chrome(int reveal) {
  const Question *q;
  char head[96], money[48], qn[48];
  int m = 16, y, qh, i;

  layout_play();
  for (i = 0; i < 3; i++) {
    int used = (i == 0) ? g.game.used_5050 : (i == 1) ? g.game.used_phone : g.game.used_audience;
    draw_life_icon(g.life_rect[i], i, used);
  }
  game_format_money(game_prize_lang(&g.game), game_current_question_prize(&g.game), money,
                    sizeof(money));
  snprintf(qn, sizeof(qn), S(STR_Q_OF), g.game.level + 1);
  snprintf(head, sizeof(head), "%s   ·   %s", qn, money);
  y = g.life_rect[0].y + g.life_rect[0].h + 10;
  g.prize_rect = mkrect(m, y, g.sw - 2 * m, 36);
  set_ui_font(g.f_small, COL_INK);
  DrawTextRect(g.prize_rect.x, g.prize_rect.y, g.prize_rect.w, g.prize_rect.h, head,
               ALIGN_CENTER | VALIGN_MIDDLE);
  y += 40;
  qh = g.a_rect[0].y - y - 10;
  draw_frame(m, y, g.sw - 2 * m, qh, COL_PANEL, COL_LINE);
  q = game_question(&g.game);
  draw_wrapped(g.f_q, COL_INK, m + 12, y + 8, g.sw - 2 * m - 24, qh - 16,
               (q && q->text[Tlang()]) ? q->text[Tlang()] : "",
               ALIGN_LEFT | VALIGN_TOP | HYPHENS);
  for (i = 0; i < 4; i++) draw_answer_slot(i, reveal);
  draw_choice(g.walk_rect, S(STR_WALK), 0);
  draw_choice(g.lock_rect, S(STR_LOCK), g.game.selected >= 0 && !reveal);
}

static void draw_overlay_card(void) {
  const Question *q = game_question(&g.game);
  int y0 = g.sh * 55 / 100;
  char line[360];
  int i;
  FillArea(0, y0, g.sw, g.sh - y0, COL_PANEL);
  DrawLine(0, y0, g.sw, y0, COL_LINE);
  DrawLine(0, y0 + 1, g.sw, y0 + 1, COL_LINE);
  if (g.overlay == OV_PHONE) {
    int slot = g.game.friend_slot;
    int sid = STR_PHONE_GUESS;
    const char *ans = "";
    if (g.game.friend_sure >= 2) sid = STR_PHONE_SURE;
    else if (g.game.friend_sure == 1) sid = STR_PHONE_THINK;
    if (q && slot >= 0) ans = q->ans[game_canonical(&g.game, slot)][Tlang()];
    snprintf(line, sizeof(line), "%c:  %s", LETTER[slot < 0 ? 0 : slot], ans);
    set_ui_font(g.f_ui, COL_INK);
    DrawTextRect(24, y0 + 16, g.sw - 48, 40, S(sid), ALIGN_CENTER | VALIGN_MIDDLE);
    set_ui_font(g.f_q, COL_INK);
    DrawTextRect(32, y0 + 60, g.sw - 64, 80, line, ALIGN_CENTER | VALIGN_MIDDLE | HYPHENS);
  } else if (g.overlay == OV_AUDIENCE) {
    int left = 40, bw = g.sw - 80, bh = 48;
    set_ui_font(g.f_ui, COL_INK);
    DrawTextRect(24, y0 + 8, g.sw - 48, 32, S(STR_AUDIENCE_TITLE), ALIGN_CENTER | VALIGN_MIDDLE);
    for (i = 0; i < 4; i++) {
      int yy = y0 + 48 + i * (bh + 6);
      int w = (bw * g.game.audience_pct[i]) / 100;
      char lab[32];
      if (game_slot_hidden(&g.game, i)) continue;
      if (w < 6 && g.game.audience_pct[i] > 0) w = 6;
      DrawRect(left, yy, bw, bh, COL_LINE);
      if (w > 2) FillArea(left + 1, yy + 1, w - 2, bh - 2, COL_INV);
      snprintf(lab, sizeof(lab), "%c   %d%%", LETTER[i], g.game.audience_pct[i]);
      set_ui_font(g.f_ans, (w > bw / 3) ? COL_ON_INV : COL_INK);
      DrawTextRect(left + 10, yy, bw - 20, bh, lab, ALIGN_LEFT | VALIGN_MIDDLE);
    }
  }
  g.ov_ok = mkrect(80, g.sh - 74, g.sw - 160, 58);
  draw_choice(g.ov_ok, S(STR_OK), 1);
}

static void draw_play(int reveal) {
  fill_bg();
  draw_play_chrome(reveal);
  if (g.overlay != OV_NONE) draw_overlay_card();
  FullUpdate();
}

static void redraw_answers_partial(void) {
  int i, y0, y1;
  layout_play();
  y0 = g.a_rect[0].y;
  y1 = g.lock_rect.y + g.lock_rect.h;
  FillArea(0, y0, g.sw, y1 - y0, COL_BG);
  for (i = 0; i < 4; i++) draw_answer_slot(i, 0);
  draw_choice(g.walk_rect, S(STR_WALK), 0);
  draw_choice(g.lock_rect, S(STR_LOCK), g.game.selected >= 0);
  PartialUpdate(0, y0, g.sw, y1 - y0);
}

static void draw_confirm(void) {
  char money[48], body[192];
  long amt;
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  if (g.confirm_kind == CONFIRM_WALK) {
    amt = game_walk_amount(&g.game);
    game_format_money(game_prize_lang(&g.game), amt, money, sizeof(money));
    DrawTextRect(24, 160, g.sw - 48, 90,
                 (amt <= 0) ? S(STR_WALK_ZERO) : S(STR_WALK_Q), ALIGN_CENTER | VALIGN_MIDDLE);
    if (amt > 0) {
      set_ui_font(g.f_title, COL_INK);
      DrawTextRect(24, 260, g.sw - 48, 70, money, ALIGN_CENTER | VALIGN_MIDDLE);
    }
  } else if (g.confirm_kind == CONFIRM_QUIT) {
    DrawTextRect(24, 180, g.sw - 48, 80, S(STR_MENU), ALIGN_CENTER | VALIGN_MIDDLE);
    set_ui_font(g.f_ui, COL_INK);
    DrawTextRect(32, 270, g.sw - 64, 80, S(STR_QUIT_Q), ALIGN_CENTER | VALIGN_MIDDLE);
  } else if (g.confirm_kind == CONFIRM_NEW) {
    DrawTextRect(24, 180, g.sw - 48, 80, S(STR_NEW_GAME), ALIGN_CENTER | VALIGN_MIDDLE);
    set_ui_font(g.f_ui, COL_INK);
    DrawTextRect(32, 270, g.sw - 64, 80, S(STR_NEW_GAME_Q), ALIGN_CENTER | VALIGN_MIDDLE);
  } else {
    const Question *q = game_question(&g.game);
    int slot = g.game.selected;
    char line[320];
    const char *ans = "";
    DrawTextRect(24, 100, g.sw - 48, 90, S(STR_FINAL_Q), ALIGN_CENTER | VALIGN_MIDDLE);
    if (q && slot >= 0) ans = q->ans[game_canonical(&g.game, slot)][Tlang()];
    snprintf(line, sizeof(line), "%c:  %s", LETTER[slot < 0 ? 0 : slot], ans);
    set_ui_font(g.f_q, COL_INK);
    DrawTextRect(32, 210, g.sw - 64, 140, line, ALIGN_CENTER | VALIGN_TOP | HYPHENS);
    (void)body;
  }
  g.yes_rect = mkrect(50, g.sh - 220, g.sw / 2 - 70, 80);
  g.no_rect = mkrect(g.sw / 2 + 20, g.sh - 220, g.sw / 2 - 70, 80);
  draw_choice(g.yes_rect, S(STR_YES), 1);
  draw_choice(g.no_rect, S(STR_NO), 0);
  FullUpdate();
}

static void draw_drum(void) {
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(24, g.sh / 2 - 50, g.sw - 48, 100, S(STR_DRUM), ALIGN_CENTER | VALIGN_MIDDLE);
  FullUpdate();
}

static void draw_safe(void) {
  char money[48];
  game_format_money(game_prize_lang(&g.game), game_current_question_prize(&g.game), money,
                    sizeof(money));
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(24, 140, g.sw - 48, 70, S(STR_SAFE_TITLE), ALIGN_CENTER | VALIGN_MIDDLE);
  DrawTextRect(24, 230, g.sw - 48, 70, money, ALIGN_CENTER | VALIGN_MIDDLE);
  set_ui_font(g.f_ui, COL_INK);
  DrawTextRect(32, 320, g.sw - 64, 120, S(STR_SAFE_BODY), ALIGN_CENTER | VALIGN_TOP | HYPHENS);
  g.ok_rect = mkrect(80, g.sh - 120, g.sw - 160, 80);
  draw_choice(g.ok_rect, S(STR_CONTINUE), 1);
  FullUpdate();
}

static void draw_top(void) {
  char money[48];
  game_format_money(game_prize_lang(&g.game), game_payout(&g.game), money, sizeof(money));
  fill_bg();
  set_ui_font(g.f_title, COL_INK);
  DrawTextRect(20, 120, g.sw - 40, 80, S(STR_TOP), ALIGN_CENTER | VALIGN_MIDDLE);
  DrawTextRect(20, 220, g.sw - 40, 70, S(STR_WON), ALIGN_CENTER | VALIGN_MIDDLE);
  set_ui_font(g.f_q, COL_INK);
  DrawTextRect(20, 320, g.sw - 40, 70, money, ALIGN_CENTER | VALIGN_MIDDLE);
  draw_credit_footer();
  g.ok_rect = mkrect(80, g.sh - 120, g.sw - 160, 80);
  draw_choice(g.ok_rect, S(STR_MENU), 1);
  FullUpdate();
}

static void draw_reveal(void) {
  char money[48], line[200];
  long pay;
  int i;
  fill_bg();
  if (g.game.status == GAME_WALKED) {
    pay = game_payout(&g.game);
    game_format_money(game_prize_lang(&g.game), pay, money, sizeof(money));
    set_ui_font(g.f_title, COL_INK);
    DrawTextRect(20, 140, g.sw - 40, 70, S(STR_YOU_LEAVE), ALIGN_CENTER | VALIGN_MIDDLE);
    set_ui_font(g.f_q, COL_INK);
    DrawTextRect(20, 230, g.sw - 40, 70, money, ALIGN_CENTER | VALIGN_MIDDLE);
    g.ok_rect = mkrect(80, g.sh - 120, g.sw - 160, 80);
    draw_choice(g.ok_rect, S(STR_MENU), 1);
    FullUpdate();
    return;
  }
  if (g.game.status == GAME_LOST) {
    const Question *q = game_question(&g.game);
    int cs = game_correct_slot(&g.game);
    const char *ans = "";
    pay = game_payout(&g.game);
    game_format_money(game_prize_lang(&g.game), pay, money, sizeof(money));
    set_ui_font(g.f_title, COL_RED);
    DrawTextRect(20, 24, g.sw - 40, 50, S(STR_WRONG), ALIGN_CENTER | VALIGN_MIDDLE);
    if (q) ans = q->ans[game_canonical(&g.game, cs)][Tlang()];
    snprintf(line, sizeof(line), "%s %c: %s", S(STR_CORRECT_WAS), LETTER[cs], ans);
    set_ui_font(g.f_ui, COL_INK);
    DrawTextRect(24, 80, g.sw - 48, 90, line, ALIGN_CENTER | VALIGN_TOP | HYPHENS);
    snprintf(line, sizeof(line), "%s: %s", S(STR_YOU_WIN), money);
    set_ui_font(g.f_q, COL_INK);
    DrawTextRect(20, 170, g.sw - 40, 44, line, ALIGN_CENTER | VALIGN_MIDDLE);
    layout_play();
    for (i = 0; i < 4; i++) {
      g.a_rect[i].x = 24;
      g.a_rect[i].w = g.sw - 48;
      g.a_rect[i].h = 78;
      g.a_rect[i].y = 230 + i * 88;
      draw_answer_slot(i, 1);
    }
    g.ok_rect = mkrect(80, g.sh - 90, g.sw - 160, 70);
    draw_choice(g.ok_rect, S(STR_MENU), 1);
    FullUpdate();
    return;
  }
  set_ui_font(g.f_title, COL_GREEN);
  DrawTextRect(20, 40, g.sw - 40, 60, S(STR_CORRECT), ALIGN_CENTER | VALIGN_MIDDLE);
  game_format_money(game_prize_lang(&g.game), game_current_question_prize(&g.game), money,
                    sizeof(money));
  snprintf(line, sizeof(line), "%s: %s", S(STR_YOU_WIN), money);
  set_ui_font(g.f_q, COL_INK);
  DrawTextRect(20, 110, g.sw - 40, 50, line, ALIGN_CENTER | VALIGN_MIDDLE);
  layout_play();
  for (i = 0; i < 4; i++) {
    g.a_rect[i].x = 24;
    g.a_rect[i].w = g.sw - 48;
    g.a_rect[i].h = 80;
    g.a_rect[i].y = 180 + i * 90;
    draw_answer_slot(i, 1);
  }
  g.ok_rect = mkrect(80, g.sh - 100, g.sw - 160, 74);
  draw_choice(g.ok_rect, S(STR_CONTINUE), 1);
  FullUpdate();
}

static void go_menu(void) {
  g.screen = UI_MENU;
  g.overlay = OV_NONE;
  g.menu_sel = 0;
  SetPanelType(1);
  draw_menu();
}

static void go_play(void) {
  g.overlay = OV_NONE;
  g.screen = UI_PLAY;
  SetPanelType(0);
  draw_play(0);
}

static void go_stake(void) {
  g.overlay = OV_NONE;
  g.screen = UI_STAKE;
  SetPanelType(0);
  draw_stake();
}

static void start_game(void) {
  game_new_round(&g.game);
  g.last_correct = 0;
  save_state();
  go_stake();
}

static void resume_game(void) {
  SetPanelType(0);
  if (g.game.awaiting_advance) {
    g.last_correct = 1;
    g.screen = UI_REVEAL;
    draw_reveal();
  } else {
    go_play();
  }
}

static void finish_run(void) {
  game_finish(&g.game, (long)time(NULL));
  save_state();
}

static void splash_timer(void);

static void splash_finish(void) {
  if (!g.splash) return;
  g.splash = 0;
  ClearTimer(splash_timer);
  go_menu();
}

static void splash_timer(void) { splash_finish(); }

static void drum_timer(void);

static void open_reveal(void) {
  ClearTimer(drum_timer);
  if (g.game.status == GAME_WON) {
    finish_run();
    g.screen = UI_TOP;
    draw_top();
    return;
  }
  if (g.game.status == GAME_LOST || g.game.status == GAME_WALKED) finish_run();
  else save_state();
  g.screen = UI_REVEAL;
  draw_reveal();
}

static void drum_timer(void) { open_reveal(); }

static void do_lock(void) {
  if (g.game.selected < 0) return;
  g.last_correct = game_lock_answer(&g.game);
  save_state();
  g.screen = UI_DRUM;
  draw_drum();
  SetWeakTimer(DRUM_TIMER, drum_timer, DRUM_MS);
  SetHardTimer(DRUM_TIMER "_h", drum_timer, DRUM_MS);
}

static void after_correct_continue(void) {
  if (g.last_correct && game_is_safe_level(g.game.level)) {
    g.screen = UI_SAFE;
    draw_safe();
    return;
  }
  game_advance(&g.game);
  save_state();
  go_stake();
}

static void select_next(int dir) {
  int i, slot;
  if (g.screen != UI_PLAY || g.overlay != OV_NONE) return;
  slot = g.game.selected;
  for (i = 0; i < 4; i++) {
    slot += dir;
    if (slot < 0) slot = 3;
    if (slot > 3) slot = 0;
    if (!game_slot_hidden(&g.game, slot)) {
      game_select(&g.game, slot);
      redraw_answers_partial();
      return;
    }
  }
}

static void activate_menu_item(int i) {
  int resume = game_can_resume(&g.game);
  if (g.screen == UI_MENU) {
    if (resume) {
      if (i == 0) resume_game();
      else if (i == 1) {
        g.confirm_kind = CONFIRM_NEW;
        g.screen = UI_CONFIRM;
        SetPanelType(0);
        draw_confirm();
      } else if (i == 2) {
        g.screen = UI_LANG;
        g.menu_sel = Tlang();
        draw_lang();
      } else if (i == 3) {
        g.help_scroll = 0;
        g.screen = UI_HELP;
        draw_help();
      } else if (i == 4) {
        g.screen = UI_SCORES;
        draw_scores();
      } else if (i == 5) {
        save_state();
        CloseApp();
      }
    } else {
      if (i == 0) start_game();
      else if (i == 1) {
        g.screen = UI_LANG;
        g.menu_sel = Tlang();
        draw_lang();
      } else if (i == 2) {
        g.help_scroll = 0;
        g.screen = UI_HELP;
        draw_help();
      } else if (i == 3) {
        g.screen = UI_SCORES;
        draw_scores();
      } else if (i == 4) {
        save_state();
        CloseApp();
      }
    }
    return;
  }
  if (g.screen == UI_LANG) {
    if (i >= 0 && i <= 2) {
      game_set_lang(&g.game, i);
      save_state();
      g.menu_sel = i;
      draw_lang();
    } else {
      go_menu();
    }
  }
}

static void handle_play_tap(int x, int y) {
  int i;
  if (g.overlay != OV_NONE) {
    if (hit(x, y, g.ov_ok)) {
      g.overlay = OV_NONE;
      draw_play(0);
    }
    return;
  }
  for (i = 0; i < 4; i++) {
    if (hit(x, y, g.a_rect[i])) {
      game_select(&g.game, i);
      redraw_answers_partial();
      return;
    }
  }
  if (hit(x, y, g.life_rect[0])) {
    if (game_use_5050(&g.game)) {
      save_state();
      draw_play(0);
    }
    return;
  }
  if (hit(x, y, g.life_rect[1])) {
    if (game_use_phone(&g.game)) {
      save_state();
      g.overlay = OV_PHONE;
      draw_play(0);
    }
    return;
  }
  if (hit(x, y, g.life_rect[2])) {
    if (game_use_audience(&g.game)) {
      save_state();
      g.overlay = OV_AUDIENCE;
      draw_play(0);
    }
    return;
  }
  if (hit(x, y, g.prize_rect)) {
    g.return_screen = UI_PLAY;
    g.screen = UI_LADDER;
    draw_ladder();
    return;
  }
  if (hit(x, y, g.walk_rect)) {
    g.confirm_kind = CONFIRM_WALK;
    g.screen = UI_CONFIRM;
    draw_confirm();
    return;
  }
  if (hit(x, y, g.lock_rect) && g.game.selected >= 0) {
    g.confirm_kind = CONFIRM_FINAL;
    g.screen = UI_CONFIRM;
    draw_confirm();
  }
}

static void handle_tap(int x, int y) {
  int i;
  if (g.splash) {
    splash_finish();
    return;
  }
  if (g.screen == UI_MENU || g.screen == UI_LANG) {
    for (i = 0; i < g.menu_n; i++) {
      if (hit(x, y, g.menu_rect[i])) {
        g.menu_sel = i;
        activate_menu_item(i);
        return;
      }
    }
    return;
  }
  if (g.screen == UI_HELP) {
    if (hit(x, y, g.ok_rect)) go_menu();
    return;
  }
  if (g.screen == UI_SCORES) {
    if (hit(x, y, g.ok_rect)) go_menu();
    return;
  }
  if (g.screen == UI_STAKE) {
    if (hit(x, y, g.prize_rect)) {
      g.return_screen = UI_STAKE;
      g.screen = UI_LADDER;
      draw_ladder();
    } else if (hit(x, y, g.ok_rect)) {
      go_play();
    }
    return;
  }
  if (g.screen == UI_LADDER) {
    if (hit(x, y, g.ok_rect)) {
      g.screen = g.return_screen;
      if (g.screen == UI_STAKE) draw_stake();
      else go_play();
    }
    return;
  }
  if (g.screen == UI_PLAY) {
    handle_play_tap(x, y);
    return;
  }
  if (g.screen == UI_CONFIRM) {
    if (hit(x, y, g.yes_rect)) {
      if (g.confirm_kind == CONFIRM_FINAL) do_lock();
      else if (g.confirm_kind == CONFIRM_WALK) {
        game_walk_away(&g.game);
        finish_run();
        g.screen = UI_REVEAL;
        draw_reveal();
      } else if (g.confirm_kind == CONFIRM_NEW) {
        game_clear_progress(&g.game);
        start_game();
      } else {
        save_state();
        go_menu();
      }
    } else if (hit(x, y, g.no_rect)) {
      if (g.confirm_kind == CONFIRM_NEW) go_menu();
      else go_play();
    }
    return;
  }
  if (g.screen == UI_REVEAL) {
    if (hit(x, y, g.ok_rect)) {
      if (g.game.status == GAME_PLAYING && g.last_correct) after_correct_continue();
      else go_menu();
    }
    return;
  }
  if (g.screen == UI_SAFE) {
    if (hit(x, y, g.ok_rect)) {
      game_advance(&g.game);
      save_state();
      go_stake();
    }
    return;
  }
  if (g.screen == UI_TOP) {
    if (hit(x, y, g.ok_rect)) go_menu();
    return;
  }
  if (g.screen == UI_DRUM) return;
}

static void redraw_current(void) {
  if (g.splash) draw_splash();
  else if (g.screen == UI_MENU) draw_menu();
  else if (g.screen == UI_LANG) draw_lang();
  else if (g.screen == UI_HELP) draw_help();
  else if (g.screen == UI_SCORES) draw_scores();
  else if (g.screen == UI_STAKE) draw_stake();
  else if (g.screen == UI_LADDER) draw_ladder();
  else if (g.screen == UI_PLAY) draw_play(0);
  else if (g.screen == UI_CONFIRM) draw_confirm();
  else if (g.screen == UI_DRUM) draw_drum();
  else if (g.screen == UI_REVEAL) draw_reveal();
  else if (g.screen == UI_SAFE) draw_safe();
  else if (g.screen == UI_TOP) draw_top();
}

static int main_handler(int type, int par1, int par2) {
  if (type == EVT_INIT) {
    memset(&g, 0, sizeof(g));
    g.sw = ScreenWidth();
    g.sh = ScreenHeight();
    SetOrientation(0);
    game_init(&g.game, LANG_EN, (unsigned)time(NULL) ^ (unsigned)(g.sw * 17 + g.sh));
    game_load(&g.game, SAVE_FILE);
    if (g.game.rng == 0) game_seed(&g.game, (unsigned)time(NULL) | 1u);
    if (g.game.run_lang < 0 || g.game.run_lang >= N_LANGS) g.game.run_lang = g.game.lang;
    g.game.q_id = g.game.level * 1000 + g.game.q_index;

    g.f_title = OpenFont(DEFAULTFONTB, FONT_TITLE, 1);
    if (!g.f_title) g.f_title = OpenFont(DEFAULTFONT, FONT_TITLE, 1);
    g.f_q = OpenFont(DEFAULTFONT, FONT_Q, 1);
    g.f_ans = OpenFont(DEFAULTFONT, FONT_ANS, 1);
    g.f_ui = OpenFont(DEFAULTFONT, FONT_UI, 1);
    g.f_small = OpenFont(DEFAULTFONT, FONT_SMALL, 1);
    g.f_ladder = OpenFont(DEFAULTFONT, FONT_LADDER, 1);

    SetPanelType(0);
    g.splash = 1;
    g.screen = UI_SPLASH;
    draw_splash();
    SetWeakTimer(SPLASH_TIMER, splash_timer, SPLASH_MS);
    SetHardTimer(SPLASH_TIMER "_h", splash_timer, SPLASH_MS);
    return 0;
  }

  if (type == EVT_SHOW || type == EVT_PREVPAGE || type == EVT_NEXTPAGE) {
    redraw_current();
    return 0;
  }

  if (type == EVT_EXIT) {
    save_state();
    close_fonts();
    return 0;
  }

  if (type == EVT_POINTERDOWN) {
    g.down = 1;
    g.dx = par1;
    g.dy = par2;
    return 0;
  }

  if (type == EVT_POINTERUP) {
    int x = par1, y = par2;
    int dy;
    if (!g.down) return 0;
    g.down = 0;
    dy = y - g.dy;
    if (g.screen == UI_HELP && abs(dy) >= 40) {
      g.help_scroll += (dy > 0) ? -80 : 80;
      draw_help();
      return 0;
    }
    if (abs(x - g.dx) < 40 && abs(y - g.dy) < 40) handle_tap(x, y);
    return 0;
  }

  if (type == EVT_KEYPRESS) {
    int key = par1;
    if (g.splash) {
      splash_finish();
      return 0;
    }
    if (key == KEY_HOME) {
      save_state();
      CloseApp();
      return 0;
    }
    if (g.screen == UI_MENU) {
      if (key == KEY_PREV || key == KEY_UP || key == KEY_PREV2 || key == KEY_LEFT) {
        g.menu_sel--;
        if (g.menu_sel < 0) g.menu_sel = g.menu_n - 1;
        draw_menu();
      } else if (key == KEY_NEXT || key == KEY_DOWN || key == KEY_NEXT2 || key == KEY_RIGHT) {
        g.menu_sel++;
        if (g.menu_sel >= g.menu_n) g.menu_sel = 0;
        draw_menu();
      } else if (key == KEY_OK) {
        activate_menu_item(g.menu_sel);
      } else if (key == KEY_BACK) {
        save_state();
        CloseApp();
      }
      return 0;
    }
    if (g.screen == UI_LANG) {
      if (key == KEY_PREV || key == KEY_UP || key == KEY_PREV2) {
        g.menu_sel--;
        if (g.menu_sel < 0) g.menu_sel = 3;
        draw_lang();
      } else if (key == KEY_NEXT || key == KEY_DOWN || key == KEY_NEXT2) {
        g.menu_sel++;
        if (g.menu_sel > 3) g.menu_sel = 0;
        draw_lang();
      } else if (key == KEY_OK) {
        activate_menu_item(g.menu_sel);
      } else if (key == KEY_BACK) {
        go_menu();
      }
      return 0;
    }
    if (g.screen == UI_HELP) {
      if (key == KEY_PREV || key == KEY_UP || key == KEY_PREV2) {
        g.help_scroll -= 80;
        draw_help();
      } else if (key == KEY_NEXT || key == KEY_DOWN || key == KEY_NEXT2) {
        g.help_scroll += 80;
        draw_help();
      } else if (key == KEY_BACK || key == KEY_OK || key == KEY_MENU) {
        go_menu();
      }
      return 0;
    }
    if (g.screen == UI_SCORES || g.screen == UI_LADDER || g.screen == UI_STAKE ||
        g.screen == UI_SAFE || g.screen == UI_TOP) {
      if (key == KEY_BACK || key == KEY_OK || key == KEY_MENU)
        handle_tap(g.ok_rect.x + 2, g.ok_rect.y + 2);
      return 0;
    }
    if (g.screen == UI_CONFIRM) {
      if (key == KEY_OK || key == KEY_LEFT) handle_tap(g.yes_rect.x + 2, g.yes_rect.y + 2);
      else if (key == KEY_BACK || key == KEY_RIGHT)
        handle_tap(g.no_rect.x + 2, g.no_rect.y + 2);
      return 0;
    }
    if (g.screen == UI_REVEAL) {
      if (key == KEY_OK || key == KEY_BACK) handle_tap(g.ok_rect.x + 2, g.ok_rect.y + 2);
      return 0;
    }
    if (g.screen == UI_DRUM) return 0;
    if (g.screen == UI_PLAY) {
      if (g.overlay != OV_NONE) {
        if (key == KEY_OK || key == KEY_BACK) {
          g.overlay = OV_NONE;
          draw_play(0);
        }
        return 0;
      }
      if (key == KEY_PREV || key == KEY_UP || key == KEY_PREV2 || key == KEY_LEFT)
        select_next(-1);
      else if (key == KEY_NEXT || key == KEY_DOWN || key == KEY_NEXT2 || key == KEY_RIGHT)
        select_next(+1);
      else if (key == KEY_OK) {
        if (g.game.selected < 0) select_next(+1);
        else {
          g.confirm_kind = CONFIRM_FINAL;
          g.screen = UI_CONFIRM;
          draw_confirm();
        }
      } else if (key == KEY_BACK) {
        g.confirm_kind = CONFIRM_WALK;
        g.screen = UI_CONFIRM;
        draw_confirm();
      } else if (key == KEY_MENU) {
        g.confirm_kind = CONFIRM_QUIT;
        g.screen = UI_CONFIRM;
        draw_confirm();
      }
      return 0;
    }
    return 0;
  }

  return 0;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  InkViewMain(main_handler);
  return 0;
}
