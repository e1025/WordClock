// Host unit test for src/clock.h (Word Clock Spec v3).
// The expected data below is transcribed independently from clock.md,
// so the test does not share tables with the implementation.
//
// Build and run:
//   g++ -std=c++11 -Wall -Wextra -I src -o /tmp/test_clock test/test_clock.cpp
//   /tmp/test_clock

#include <cstdio>
#include <cstring>
#include <string>
#include <initializer_list>

#include "clock.h"

static int failures = 0;
static int checks = 0;

// --- independent word table from the spec -------------------------------

struct KnownWord {
  const char *name;
  uint8_t row, colStart, colEnd;
  const char *text;  // letters the span must show
};

static const KnownWord KW[] = {
    {"ES", 0, 0, 1, "ES"},
    {"IST", 0, 3, 5, "IST"},
    {"EINS_H", 7, 2, 5, "EINS"},
    {"ZWEI_H", 8, 0, 3, "ZWEI"},
    {"DREI_H", 7, 0, 3, "DREI"},
    {"VIER_H", 8, 4, 7, "VIER"},
    {"FUENF_H", 10, 7, 10, "F\xC3\x9C""NF"},
    {"SECHS_H", 9, 0, 4, "SECHS"},
    {"SIEBEN_H", 9, 4, 9, "SIEBEN"},
    {"ACHT_H", 6, 5, 8, "ACHT"},
    {"NEUN_H", 10, 3, 6, "NEUN"},
    {"ZEHN_H", 10, 0, 3, "ZEHN"},
    {"ELF_H", 8, 8, 10, "ELF"},
    {"ZWOELF_H", 7, 6, 10, "ZW\xC3\x96""LF"},
    {"FUENF5", 2, 7, 10, "F\xC3\x9C""NF"},
    {"ZEHN10", 3, 3, 6, "ZEHN"},
    {"NACH_B", 4, 0, 3, "NACH"},
    {"VOR_B", 3, 8, 10, "VOR"},
    {"HALB", 4, 5, 8, "HALB"},
    {"VIERTEL", 5, 4, 10, "VIERTEL"},
    {"DREIVIERTEL", 5, 0, 10, "DREIVIERTEL"},
    {"UEBER", 6, 0, 3, "\xC3\x9C""BER"},
    {"EINS_C", 1, 2, 5, "EINS"},
    {"ZWEI_C", 1, 0, 3, "ZWEI"},
    {"DREI_C", 0, 7, 10, "DREI"},
    {"VIER_C", 1, 7, 10, "VIER"},
    {"NACH_C", 2, 0, 3, "NACH"},
    {"VOR_C", 2, 4, 6, "VOR"},
};
static const int KW_COUNT = (int)(sizeof(KW) / sizeof(KW[0]));

static const KnownWord *findKnown(const char *name) {
  for (int i = 0; i < KW_COUNT; ++i) {
    if (strcmp(KW[i].name, name) == 0) return &KW[i];
  }
  return 0;
}

// --- helpers -------------------------------------------------------------

static void litFromNames(std::initializer_list<const char *> names, bool lit[11][11]) {
  for (int r = 0; r < 11; ++r)
    for (int c = 0; c < 11; ++c) lit[r][c] = false;
  for (const char *name : names) {
    const KnownWord *w = findKnown(name);
    if (!w) {
      printf("FAIL: unknown word name '%s' in test\n", name);
      ++failures;
      continue;
    }
    for (uint8_t c = w->colStart; c <= w->colEnd; ++c) lit[w->row][c] = true;
  }
}

// Derives the lit set from the frame the implementation built.
static void litFromFrame(int hour, int minute, bool lit[11][11]) {
  char frame[3][11][11];
  WordClock::buildFrame((uint8_t)hour, (uint8_t)minute, frame);
  for (int r = 0; r < 11; ++r) {
    for (int c = 0; c < 11; ++c) {
      lit[r][c] = (unsigned char)frame[0][r][c] == 255 &&
                  (unsigned char)frame[1][r][c] == 255 &&
                  (unsigned char)frame[2][r][c] == 255;
    }
  }
}

static void expect(bool ok, const char *what) {
  ++checks;
  if (!ok) {
    ++failures;
    printf("FAIL: %s\n", what);
  }
}

// --- checks --------------------------------------------------------------

static void checkExample(int hour, int minute, const char *label,
                         std::initializer_list<const char *> names) {
  bool expected[11][11], produced[11][11];
  litFromNames(names, expected);
  litFromFrame(hour, minute, produced);

  bool equal = true;
  for (int r = 0; r < 11 && equal; ++r)
    for (int c = 0; c < 11; ++c)
      if (expected[r][c] != produced[r][c]) {
        equal = false;
        break;
      }

  char what[96];
  snprintf(what, sizeof(what), "%d:%02d (%s) lit set mismatch", hour, minute, label);
  expect(equal, what);
}

static void checkSpanTable() {
  // Every span in SPAN[] must match one known word (and spell its text).
  expect(WordClock::WORD_COUNT == KW_COUNT, "word instance counts match");
  bool seen[KW_COUNT] = {false};
  for (int i = 0; i < WordClock::WORD_COUNT; ++i) {
    const WordClock::Span &s = WordClock::SPAN[i];
    bool matched = false;
    for (int j = 0; j < KW_COUNT; ++j) {
      if (seen[j]) continue;
      const KnownWord &w = KW[j];
      if (w.row == s.row && w.colStart == s.colStart && w.colEnd == s.colEnd) {
        seen[j] = true;
        // The span letters must spell the word from the grid.
        char text[16] = "";
        int p = 0;
        for (uint8_t c = s.colStart; c <= s.colEnd && p < 12; ++c) {
          const char *l = WordClock::CELL[s.row][c];
          expect(l != 0, "span covers letter cells only");
          if (l) {
            int len = (int)strlen(l);
            if (p + len < 14) {
              memcpy(text + p, l, len);
              p += len;
            }
          }
        }
        text[p] = 0;
        bool spelled = strcmp(text, w.text) == 0;
        char what[96];
        snprintf(what, sizeof(what), "span (%d,%d-%d) spells '%s'", s.row, s.colStart, s.colEnd, w.text);
        expect(spelled, what);
        matched = true;
        break;
      }
    }
    if (!matched) {
      char what[96];
      snprintf(what, sizeof(what), "SPAN entry %d (%d,%d-%d) matches no spec word", i, s.row, s.colStart, s.colEnd);
      expect(false, what);
    }
  }
}

static void checkFullDay() {
  for (int hour = 0; hour < 24; ++hour) {
    for (int minute = 0; minute < 60; ++minute) {
      char frame[3][11][11];
      WordClock::buildFrame((uint8_t)hour, (uint8_t)minute, frame);
      for (int r = 0; r < 11; ++r) {
        for (int c = 0; c < 11; ++c) {
          const bool hasLetter = WordClock::CELL[r][c] != 0;
          const int red = (unsigned char)frame[0][r][c];
          const int green = (unsigned char)frame[1][r][c];
          const int blue = (unsigned char)frame[2][r][c];
          if (!hasLetter) {
            if (red || green || blue) {
              char what[96];
              snprintf(what, sizeof(what), "%02d:%02d cell (%d,%d) is a dot but lights", hour, minute, r, c);
              expect(false, what);
            }
          } else {
            const bool bright = red == 255 && green == 255 && blue == 255;
            const bool gray = red == 64 && green == 64 && blue == 64;
            if (!bright && !gray) {
              char what[96];
              snprintf(what, sizeof(what), "%02d:%02d cell (%d,%d) has invalid color %d,%d,%d", hour, minute, r, c, red, green, blue);
              expect(false, what);
            }
          }
        }
      }
      // ES and IST are always part of the sentence.
      bool lit[11][11];
      litFromFrame(hour, minute, lit);
      bool ok = lit[0][0] && lit[0][1] && lit[0][3] && lit[0][4] && lit[0][5];
      if (!ok) {
        char what[96];
        snprintf(what, sizeof(what), "%02d:%02d ES IST not lit", hour, minute);
        expect(false, what);
      }
      ++checks;  // count the per-minute pass
    }
  }
}

// Spec v5 reading property: scanning the lit cells row by row (top to
// bottom, left to right) yields the words exactly in sentence order.
static void checkReadingProperty() {
  for (int hour = 0; hour < 24; ++hour) {
    for (int minute = 0; minute < 60; ++minute) {
      WordClock::WordId act[8];
      const uint8_t n = WordClock::activeWords((uint8_t)hour, (uint8_t)minute, act);

      // Expected: the active words' letters in sentence order.
      std::string expected;
      for (uint8_t i = 0; i < n; ++i) {
        const WordClock::Span &s = WordClock::SPAN[act[i]];
        for (uint8_t c = s.colStart; c <= s.colEnd; ++c) {
          expected += WordClock::CELL[s.row][c];
        }
      }

      // Actual: bright cells read row-major.
      bool lit[11][11];
      litFromFrame(hour, minute, lit);
      std::string actual;
      for (int r = 0; r < 11; ++r) {
        for (int c = 0; c < 11; ++c) {
          if (lit[r][c]) actual += WordClock::CELL[r][c];
        }
      }

      if (expected != actual) {
        char what[96];
        snprintf(what, sizeof(what), "%02d:%02d row-major reading != sentence order", hour, minute);
        expect(false, what);
      }
      ++checks;
    }
  }
}

int main() {
  checkSpanTable();

  // Section 7 verification examples: expected instance lists from the spec.
  checkExample(9, 26, "es ist vier vor halb zehn", {"ES", "IST", "VIER_C", "VOR_C", "HALB", "ZEHN_H"});
  checkExample(9, 29, "eins vor halb zehn", {"ES", "IST", "EINS_C", "VOR_C", "HALB", "ZEHN_H"});
  checkExample(9, 31, "eins nach halb zehn", {"ES", "IST", "EINS_C", "NACH_C", "HALB", "ZEHN_H"});
  checkExample(9, 23, "drei nach zehn vor halb zehn", {"ES", "IST", "DREI_C", "NACH_C", "ZEHN10", "VOR_B", "HALB", "ZEHN_H"});
  checkExample(14, 53, "drei nach zehn vor drei (row-major reading)", {"ES", "IST", "DREI_C", "NACH_C", "ZEHN10", "VOR_B", "DREI_H"});
  checkExample(9, 36, "eins nach fuenf nach halb zehn", {"ES", "IST", "EINS_C", "NACH_C", "FUENF5", "NACH_B", "HALB", "ZEHN_H"});
  checkExample(9, 58, "zwei vor zehn (wrap)", {"ES", "IST", "ZWEI_C", "VOR_C", "ZEHN_H"});
  checkExample(8, 14, "eins vor viertel ueber acht", {"ES", "IST", "EINS_C", "VOR_C", "VIERTEL", "UEBER", "ACHT_H"});
  checkExample(9, 44, "eins vor dreiviertel zehn", {"ES", "IST", "EINS_C", "VOR_C", "DREIVIERTEL", "ZEHN_H"});
  checkExample(9, 16, "eins nach viertel ueber neun", {"ES", "IST", "EINS_C", "NACH_C", "VIERTEL", "UEBER", "NEUN_H"});
  checkExample(15, 45, "dreiviertel vier", {"ES", "IST", "DREIVIERTEL", "VIER_H"});
  checkExample(11, 22, "zwei nach zehn vor halb zwoelf", {"ES", "IST", "ZWEI_C", "NACH_C", "ZEHN10", "VOR_B", "HALB", "ZWOELF_H"});
  checkExample(12, 31, "eins nach halb eins (12->1 wrap)", {"ES", "IST", "EINS_C", "NACH_C", "HALB", "EINS_H"});
  checkExample(1, 58, "zwei vor zwei (distinct ZWEI instances)", {"ES", "IST", "ZWEI_C", "VOR_C", "ZWEI_H"});

  checkFullDay();
  checkReadingProperty();

  printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
