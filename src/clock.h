#pragma once

// Word Clock Spec v5 — Upper Austrian German, 11x11 matrix.
// Pure logic, no Arduino dependencies, so it can be unit-tested on the host.

#include <stdint.h>

namespace WordClock {

static const uint8_t ROWS = 11;
static const uint8_t COLS = 11;

// LED colors: one RGB triple per cell. Inactive letters are dim gray,
// active letters bright white, cells without a letter never light.
static const char COLOR_OFF[3] = {(char)0, (char)0, (char)0};
static const char COLOR_GRAY[3] = {(char)64, (char)64, (char)64};
static const char COLOR_BRIGHT[3] = {(char)255, (char)255, (char)255};

// Letter grid, UTF-8. nullptr = cell never lights.
const char* const CELL[ROWS][COLS] = {
    {"E", "S", 0, "I", "S", "T", 0, "D", "R", "E", "I"},
    {"Z", "W", "E", "I", "N", "S", 0, "V", "I", "E", "R"},
    {"N", "A", "C", "H", "V", "O", "R", "F", "\xC3\x9C", "N", "F"},
    {0, 0, 0, "Z", "E", "H", "N", 0, "V", "O", "R"},
    {"N", "A", "C", "H", 0, "H", "A", "L", "B", 0, 0},
    {"D", "R", "E", "I", "V", "I", "E", "R", "T", "E", "L"},
    {"\xC3\x9C", "B", "E", "R", 0, "A", "C", "H", "T", 0, 0},
    {"D", "R", "E", "I", "N", "S", "Z", "W", "\xC3\x96", "L", "F"},
    {"Z", "W", "E", "I", "V", "I", "E", "R", "E", "L", "F"},
    {"S", "E", "C", "H", "S", "I", "E", "B", "E", "N", 0},
    {"Z", "E", "H", "N", "E", "U", "N", "F", "\xC3\x9C", "N", "F"},
};

// Word instances: contiguous horizontal spans. Identical strings in
// different roles (hour/counter/base) are distinct instances.
enum WordId : uint8_t {
  W_ES,
  W_IST,
  // hour role
  W_EINS_H,
  W_ZWEI_H,
  W_DREI_H,
  W_VIER_H,
  W_FUENF_H,
  W_SECHS_H,
  W_SIEBEN_H,
  W_ACHT_H,
  W_NEUN_H,
  W_ZEHN_H,
  W_ELF_H,
  W_ZWOELF_H,
  // base connectors
  W_FUENF5,      // "fünf" (5 minutes)
  W_ZEHN10,      // "zehn" (10 minutes)
  W_NACH_B,      // "nach" (base)
  W_VOR_B,       // "vor" (base)
  W_HALB,
  W_VIERTEL,
  W_DREIVIERTEL, // covers VIERTEL's cells as well (full row R5)
  W_UEBER,
  // minute counter
  W_EINS_C,
  W_ZWEI_C,
  W_DREI_C,
  W_VIER_C,
  W_NACH_C,      // "nach" (counter)
  W_VOR_C,       // "vor" (counter)
  WORD_COUNT
};

struct Span {
  uint8_t row;
  uint8_t colStart;
  uint8_t colEnd;
};

const Span SPAN[WORD_COUNT] = {
    {0, 0, 1},    // ES
    {0, 3, 5},    // IST
    {7, 2, 5},    // EINS (hour)
    {8, 0, 3},    // ZWEI (hour)
    {7, 0, 3},    // DREI (hour)
    {8, 4, 7},    // VIER (hour)
    {10, 7, 10},  // FÜNF (hour)
    {9, 0, 4},    // SECHS (hour)
    {9, 4, 9},    // SIEBEN (hour)
    {6, 5, 8},    // ACHT (hour, only instance)
    {10, 3, 6},   // NEUN (hour)
    {10, 0, 3},   // ZEHN (hour)
    {8, 8, 10},   // ELF (hour)
    {7, 6, 10},   // ZWÖLF (hour)
    {2, 7, 10},   // FÜNF (base, 5 min)
    {3, 3, 6},    // ZEHN (base, 10 min)
    {4, 0, 3},    // NACH (base)
    {3, 8, 10},   // VOR (base)
    {4, 5, 8},    // HALB
    {5, 4, 10},   // VIERTEL
    {5, 0, 10},   // DREIVIERTEL
    {6, 0, 3},    // ÜBER
    {1, 2, 5},    // EINS (counter)
    {1, 0, 3},    // ZWEI (counter)
    {0, 7, 10},   // DREI (counter)
    {1, 7, 10},   // VIER (counter)
    {2, 0, 3},    // NACH (counter)
    {2, 4, 6},    // VOR (counter)
};

static const WordId COUNT_WORD[5] = {
    W_ES,  // unused (k = 0 has no counter phrase)
    W_EINS_C,
    W_ZWEI_C,
    W_DREI_C,
    W_VIER_C,
};

inline WordId hourWord(uint8_t spokenHour) {
  return static_cast<WordId>(W_EINS_H + ((spokenHour - 1) % 12));
}

// Base phrase for slot (0-11). Slots 0-3 use the current spoken hour h,
// slots 4-11 the next hour nxt.
inline void appendBase(uint8_t slot, uint8_t h, uint8_t nxt, WordId out[8], uint8_t& n) {
  switch (slot) {
    case 0:
      break;
    case 1:
      out[n++] = W_FUENF5;
      out[n++] = W_NACH_B;
      break;
    case 2:
      out[n++] = W_ZEHN10;
      out[n++] = W_NACH_B;
      break;
    case 3:
      out[n++] = W_VIERTEL;
      out[n++] = W_UEBER;
      break;
    case 4:
    case 10:
      out[n++] = W_ZEHN10;
      out[n++] = W_VOR_B;
      break;
    case 5:
    case 11:
      out[n++] = W_FUENF5;
      out[n++] = W_VOR_B;
      break;
    case 6:
      out[n++] = W_HALB;
      break;
    case 7:
      out[n++] = W_FUENF5;
      out[n++] = W_NACH_B;
      break;
    case 8:
      out[n++] = W_ZEHN10;
      out[n++] = W_NACH_B;
      break;
    case 9:
      out[n++] = W_DREIVIERTEL;
      break;
  }
  switch (slot) {
    case 4:
    case 5:
    case 7:
    case 8:
      out[n++] = W_HALB;
      break;
  }
  out[n++] = hourWord(slot < 4 ? h : nxt);
}

// Computes the active word instances for hour (0-23), minute (0-59).
// Writes at most 8 ids into out and returns the count.
inline uint8_t activeWords(uint8_t hour, uint8_t minute, WordId out[8]) {
  const uint8_t h = ((hour + 11) % 12) + 1;  // spoken hour 1-12
  const uint8_t nxt = (h % 12) + 1;          // next hour (12 wraps to 1)
  const uint8_t b = minute / 5;              // base slot 0-11
  const uint8_t k = minute % 5;              // minute offset 0-4

  uint8_t n = 0;
  out[n++] = W_ES;
  out[n++] = W_IST;

  if (k == 0) {
    appendBase(b, h, nxt, out, n);
  } else if (b == 11) {
    // Wrap special case: the anchor ahead is the next full hour,
    // e.g. 9:58 -> "zwei vor zehn", not "zwei vor neun".
    out[n++] = COUNT_WORD[5 - k];
    out[n++] = W_VOR_C;
    out[n++] = hourWord(nxt);
  } else if (b % 3 == 0) {
    // After an anchor (b in {0, 3, 6, 9})
    out[n++] = COUNT_WORD[k];
    out[n++] = W_NACH_C;
    appendBase(b, h, nxt, out, n);
  } else if ((b + 1) % 3 == 0) {
    // Before an anchor (b in {2, 5, 8})
    out[n++] = COUNT_WORD[5 - k];
    out[n++] = W_VOR_C;
    appendBase(b + 1, h, nxt, out, n);
  } else {
    // Between anchors (b in {1, 4, 7, 10})
    out[n++] = COUNT_WORD[k];
    out[n++] = W_NACH_C;
    appendBase(b, h, nxt, out, n);
  }
  return n;
}

// Renders the word clock into frame: frame[0] = red plane,
// frame[1] = green plane, frame[2] = blue plane, each [row][column].
inline void buildFrame(uint8_t hour, uint8_t minute, char frame[3][ROWS][COLS]) {
  WordId act[8];
  const uint8_t n = activeWords(hour, minute, act);

  bool lit[ROWS][COLS] = {{false}};
  for (uint8_t i = 0; i < n; ++i) {
    const Span& s = SPAN[act[i]];
    for (uint8_t c = s.colStart; c <= s.colEnd; ++c) {
      lit[s.row][c] = true;
    }
  }

  for (uint8_t r = 0; r < ROWS; ++r) {
    for (uint8_t c = 0; c < COLS; ++c) {
      const char* color = COLOR_OFF;
      if (CELL[r][c] != 0) {
        color = lit[r][c] ? COLOR_BRIGHT : COLOR_GRAY;
      }
      frame[0][r][c] = color[0];
      frame[1][r][c] = color[1];
      frame[2][r][c] = color[2];
    }
  }
}

}  // namespace WordClock
