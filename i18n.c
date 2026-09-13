#include "i18n.h"

#include "questions.h"

static const char *T[STR_COUNT][N_LANGS] = {
  {"Quiz Ladder", "Quiz Ladder", "Quiz Ladder"},
  {"15 questions. 3 helps. Climb the ladder.",
   "15 pytań. 3 podpowiedzi. Wejdź na szczyt.",
   "15 Fragen. 3 Hilfen. Die Leiter hinauf."},
  {"made by Mateusz Blumensztajn (Sableworks)",
   "made by Mateusz Blumensztajn (Sableworks)",
   "made by Mateusz Blumensztajn (Sableworks)"},
  {"Play", "Graj", "Spielen"},
  {"Resume", "Wznów", "Fortsetzen"},
  {"Language", "Język", "Sprache"},
  {"How to play", "Jak grać", "So wird gespielt"},
  {"High scores", "Rekordy", "Bestenliste"},
  {"Exit", "Wyjście", "Beenden"},
  {"Back", "Wstecz", "Zurück"},
  {"Continue", "Dalej", "Weiter"},
  {"Main menu", "Menu główne", "Hauptmenü"},
  {"OK", "OK", "OK"},
  {"Yes", "Tak", "Ja"},
  {"No", "Nie", "Nein"},
  {"Walk away", "Rezygnuję", "Aussteigen"},
  {"Walk away with this prize?",
   "Zrezygnować z tej wygranej?",
   "Mit diesem Gewinn aussteigen?"},
  {"Walk away with nothing?",
   "Zrezygnować bez wygranej?",
   "Ohne Gewinn aussteigen?"},
  {"Lock this answer?",
   "Zatwierdzić tę odpowiedź?",
   "Diese Antwort festlegen?"},
  {"Correct!", "Dobrze!", "Richtig!"},
  {"Wrong answer", "Błędna odpowiedź", "Falsche Antwort"},
  {"You climbed the ladder!",
   "Wszedłeś na szczyt drabinki!",
   "Sie haben die Leiter erklommen!"},
  {"TOP RUNG", "SZCZYT", "SPITZE"},
  {"You leave with", "Odchodzisz z", "Sie gehen mit"},
  {"You have won", "Wygrywasz", "Sie haben gewonnen"},
  {"The correct answer was",
   "Prawidłowa odpowiedź to",
   "Die richtige Antwort war"},
  {"No scores yet. Play a game!",
   "Brak wyników. Zagraj partię!",
   "Noch keine Ergebnisse. Spielen Sie!"},
  {"50:50", "50:50", "50:50"},
  {"Phone", "Telefon", "Telefon"},
  {"Audience", "Publiczność", "Publikum"},
  {"Your friend is sure:",
   "Przyjaciel jest pewien:",
   "Ihr Freund ist sicher:"},
  {"Your friend thinks:",
   "Przyjaciel uważa:",
   "Ihr Freund denkt:"},
  {"Your friend is guessing:",
   "Przyjaciel zgaduje:",
   "Ihr Freund tippt:"},
  {"The audience voted",
   "Publiczność zagłosowała",
   "Das Publikum hat abgestimmt"},
  {"Question %d of 15", "Pytanie %d z 15", "Frage %d von 15"},
  {"Playing for", "Grasz o", "Sie spielen um"},
  {"Guaranteed so far", "Gwarantowane dotąd", "Bisher garantiert"},
  {"Safe haven!", "Próg gwarantowany!", "Sicherheitsstufe!"},
  {"That prize is now yours even if you miss later.",
   "Ta kwota zostaje, nawet jeśli później się pomylisz.",
   "Dieser Betrag bleibt Ihnen, auch wenn Sie später danebenliegen."},
  {"Let's see…", "Sprawdźmy…", "Mal sehen…"},
  {"Ladder", "Drabinka", "Leiter"},
  {"Games: %d", "Partie: %d", "Spiele: %d"},
  {"Best: question %d", "Najlepiej: pytanie %d", "Beste: Frage %d"},
  {"Perfect runs: %d", "Pełne partie: %d", "Volle Läufe: %d"},
  {"Answer 15 questions, each harder than the last.\n\n"
   "A, B, C, D — pick one, then confirm.\n\n"
   "Helps (once each):\n"
   "• 50:50 — two wrong answers go\n"
   "• Phone — a hint, not a guarantee\n"
   "• Audience — a vote; weaker near the top\n\n"
   "Walk away to keep the last prize you won.\n"
   "A wrong answer drops you to the last safe haven "
   "(question 5 or 10). Miss before question 5 and you leave with nothing.\n\n"
   "Tap the prize to open the money ladder.\n"
   "Home saves the run — you can resume later.\n\n"
   "Hardware: ◄ ► choose  ·  OK confirm  ·  Back walk away  ·  Menu leave  ·  Home exit",
   "Odpowiedz na 15 coraz trudniejszych pytań.\n\n"
   "A, B, C, D — wybierz, potem potwierdź.\n\n"
   "Podpowiedzi (każda raz):\n"
   "• 50:50 — odpadają dwie błędne\n"
   "• Telefon — wskazówka, nie pewnik\n"
   "• Publiczność — głos; później mniej pewny\n\n"
   "Rezygnacja zostawia ostatnią wygraną.\n"
   "Błąd zrzuca do ostatniego progu (pytanie 5. lub 10.). "
   "Pomyłka przed 5. = nic.\n\n"
   "Stuknij kwotę, żeby otworzyć drabinkę.\n"
   "Home zapisuje partię — możesz wrócić.\n\n"
   "Przyciski: ◄ ► wybór  ·  OK zatwierdź  ·  Back rezygnacja  ·  Menu wyjście  ·  Home koniec",
   "Beantworten Sie 15 immer schwerere Fragen.\n\n"
   "A, B, C, D — wählen, dann bestätigen.\n\n"
   "Hilfen (jeweils einmal):\n"
   "• 50:50 — zwei falsche fallen weg\n"
   "• Telefon — ein Tipp, keine Garantie\n"
   "• Publikum — eine Abstimmung; oben unsicherer\n\n"
   "Aussteigen sichert den letzten Gewinn.\n"
   "Falsch fällt auf die letzte Sicherheitsstufe (Frage 5 oder 10). "
   "Daneben vor Frage 5 = nichts.\n\n"
   "Tippen Sie auf den Betrag für die Leiter.\n"
   "Home speichert den Lauf — Sie können fortsetzen.\n\n"
   "Tasten: ◄ ► wählen  ·  OK bestätigen  ·  Zurück aussteigen  ·  Menü verlassen  ·  Home beenden"},
  {"English", "English", "English"},
  {"Polski", "Polski", "Polski"},
  {"Deutsch", "Deutsch", "Deutsch"},
  {"New game", "Nowa gra", "Neues Spiel"},
  {"Abandon the current game?",
   "Porzucić bieżącą partię?",
   "Aktuelles Spiel aufgeben?"},
  {"Confirm", "Potwierdź", "Bestätigen"},
  {"Leave this game? Progress is saved.",
   "Wyjść z gry? Postęp zostanie zapisany.",
   "Spiel verlassen? Der Stand wird gespeichert."},
  {"Here we go", "Zaczynamy", "Los geht's"}
};

const char *tr(int lang, int id) {
  if (lang < 0 || lang >= N_LANGS) lang = LANG_EN;
  if (id < 0 || id >= STR_COUNT) return "";
  return T[id][lang];
}

const char *lang_name(int lang) {
  if (lang == LANG_PL) return "Polski";
  if (lang == LANG_DE) return "Deutsch";
  return "English";
}

const char *currency_suffix(int lang) {
  if (lang == LANG_PL) return "zł";
  if (lang == LANG_DE) return "€";
  return "$";
}
