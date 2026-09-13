# PocketBook Verse Pro Color — Quiz Ladder

**Quiz Ladder** is an original fifteen-rung quiz for the **PocketBook Verse Pro Color (B300)**. Built with InkView / SDK 6. Other PocketBook models are untested — use at your own risk.

The default language is **English**. Polish and German can be switched in the Language menu.

## Download

**[Download the latest release](https://github.com/Sableworks/pocketbook-verse-pro-quiz-ladder/releases/latest)** — ZIP with `quizladder.app` and `quizladder.png`, ready for your PocketBook. No build tools required.

(GitHub does not allow uploading bare `.app` files, so the binary ships inside a ZIP.)

## Install on device

1. Download the ZIP from the [latest release](https://github.com/Sableworks/pocketbook-verse-pro-quiz-ladder/releases/latest) and unzip it.
2. Connect the PocketBook via USB (PC Link / mass storage).
3. Copy `quizladder.app` **and** `quizladder.png` to `applications/` on device storage (`/mnt/ext1/applications/`).
4. **Disconnect USB** (important — with PC Link active, apps often cannot see files).
5. Launch **Quiz Ladder** from the applications menu.

Home saves an in-progress run. The next launch offers **Resume**. Language, question memory, stats, and high scores (amount + currency + date) live in `/mnt/ext1/.quizladder.ini`.

## How it plays

Fifteen questions, each harder than the last. A **stake** screen names the prize, then you confirm before the answer locks. Safe havens (questions 5 and 10) get their own beat. Home does not throw the run away.

The play screen keeps the question plus A–D and three help icons. The full prize ladder opens when you tap the prize. Phone and audience stay on the same question as an overlay. Changing A–D uses a partial refresh so the panel does not flash the whole page.

| Help | What it does |
| --- | --- |
| **50:50** | Two wrong answers disappear |
| **Phone** | A hint — less reliable on later questions |
| **Audience** | A vote — also weaker near the top |

Walk away to keep the last prize you won. A wrong answer drops you to the last **safe haven** (question 5 or 10). Miss before question 5 and you leave with nothing.

Prize amounts follow familiar cash ladders: US dollars in English, Polish złoty, German euros.

The question bank has **20+ unique items per rung** (hundreds in total), in all three languages. The game avoids recently used questions so replay is not the same fifteen items.

## Controls

| Action | Gesture / key |
| --- | --- |
| Choose A–D | Tap an answer, or **◄ / ►** |
| Confirm | **Confirm** button, or **OK** |
| Helps | Tap 50:50 / Phone / Audience |
| Walk away | **Walk away**, or **Back** |
| Menu | **Menu** (progress is saved) |
| Exit app | **Home** |

## Build from source (optional)

Only needed if you want to modify or rebuild the app. End users should use the [prebuilt release](https://github.com/Sableworks/pocketbook-verse-pro-quiz-ladder/releases/latest).

The Docker image is the PocketBook ARM toolchain (`SDK-B300` 6.8). On Apple Silicon, force `linux/amd64`.

If you already built `pb-rsvp-builder` for another Sableworks PocketBook app, reuse that image:

```bash
python3 scripts/gen_questions.py

docker run --rm --platform linux/amd64 -v "$(pwd):/project" pb-rsvp-builder \
  -c 'mkdir -p build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=/SDK/share/cmake/arm_conf.cmake .. && cmake --build .'
```

First-time image (downloads the SDK):

```bash
docker build --platform linux/amd64 -t pb-rsvp-builder .
```

Output: `build/quizladder.app` and `build/quizladder.png`.

## Host tests (optional)

Game logic (ladder, helps, languages, question bank) without InkView:

```bash
python3 scripts/gen_questions.py
cc -O2 -Wall -o tests/host_smoke tests/host_smoke.c game.c i18n.c questions.c && ./tests/host_smoke
```

## Credits

Made by Mateusz Blumensztajn ([Sableworks](https://github.com/Sableworks)).
