# Word Clock Spec v5 — Upper Austrian German, 11×11 matrix

## 1. Grid

11 rows × 11 columns, addressed `R0–R10`, `C0–C10`. Each cell = one LED showing its letter; `.` = cell never lights. A letter may belong to several words (shared letters).

```text
R0  E S . I S T . D R E I
R1  Z W E I N S . V I E R
R2  N A C H V O R F Ü N F
R3  . . . Z E H N . V O R
R4  N A C H . H A L B . .
R5  D R E I V I E R T E L
R6  Ü B E R . A C H T . .
R7  D R E I N S Z W Ö L F
R8  Z W E I V I E R E L F
R9  S E C H S I E B E N .
R10 Z E H N E U N F Ü N F
```

Compact form, one string per row:

```text
ES.IST.DREI
ZWEINS.VIER
NACHVORFÜNF
...ZEHN.VOR
NACH.HALB..
DREIVIERTEL
ÜBER.ACHT..
DREINSZWÖLF
ZWEIVIERELF
SECHSIEBEN.
ZEHNEUNFÜNF
```

## 2. Word instances (fixed cells, assigned per role)

Each word is a contiguous horizontal span `Rr:Ca–Cb`. Identical strings in different roles use **different instances** so they can light simultaneously.


| Role            | Word → cells                                                                                                                                                                                                 |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Static          | ES R0:0–1 · IST R0:3–5                                                                                                                                                                                       |
| Hour `H(x)`     | EINS R7:2–5 · ZWEI R8:0–3 · DREI R7:0–3 · VIER R8:4–7 · FÜNF R10:7–10 · SECHS R9:0–4 · SIEBEN R9:4–9 · ACHT R6:5–8 *(only instance — exception)* · NEUN R10:3–6 · ZEHN R10:0–3 · ELF R8:8–10 · ZWÖLF R7:6–10 |
| Base connectors | NACHᵇ R4:0–3 · FÜNF⁵ R2:7–10 · ZEHN¹⁰ R3:3–6 · VORᵇ R3:8–10 · HALB R4:5–8 · VIERTEL R5:4–10 · DREIVIERTEL R5:0–10 · ÜBER R6:0–3                                                                              |
| Minute counter  | EINSᶜ R1:2–5 · ZWEIᶜ R1:0–3 · DREIᶜ R0:7–10 · VIERᶜ R1:7–10 · NACHᶜ R2:0–3 · VORᶜ R2:4–6                                                                                                                     |
| Spare           | none — all instances are used                                                                                                                                                                                |


## 3. Time computation

```text
input: H (0–23), M (0–59)
h   = ((H + 11) mod 12) + 1     // spoken hour 1–12
nxt = (h mod 12) + 1           // next hour (12 wraps to 1)
b   = floor(M / 5)             // base slot 0–11
k   = M mod 5                  // minute offset 0–4
```

Anchor slots: **A = {0, 3, 6, 9}** (full hour, viertel über, halb, dreiviertel).

Sentence order: **ES IST** · *(counter phrase)* · *base phrase*.

## 4. Base phrase table (slot `b` → instances)


| b   | Instances                | Reads as                                              |
| --- | ------------------------ | ----------------------------------------------------- |
| 0   | H(h)                     | „«h»"                                                 |
| 1   | FÜNF⁵ NACHᵇ H(h)         | „fünf nach «h»"                                       |
| 2   | ZEHN¹⁰ NACHᵇ H(h)        | „zehn nach «h»"                                       |
| 3   | VIERTEL ÜBER H(h)        | „viertel über «h»" *(dialect: 15 past, current hour)* |
| 4   | ZEHN¹⁰ VORᵇ HALB H(nxt)  | „zehn vor halb «nxt»"                                 |
| 5   | FÜNF⁵ VORᵇ HALB H(nxt)   | „fünf vor halb «nxt»"                                 |
| 6   | HALB H(nxt)              | „halb «nxt»"                                          |
| 7   | FÜNF⁵ NACHᵇ HALB H(nxt)  | „fünf nach halb «nxt»"                                |
| 8   | ZEHN¹⁰ NACHᵇ HALB H(nxt) | „zehn nach halb «nxt»"                                |
| 9   | DREIVIERTEL H(nxt)       | „dreiviertel «nxt»" *(dialect: 15 to)*                |
| 10  | ZEHN¹⁰ VORᵇ H(nxt)       | „zehn vor «nxt»"                                      |
| 11  | FÜNF⁵ VORᵇ H(nxt)        | „fünf vor «nxt»"                                      |


## 5. Minute counting (k = M mod 5)


| Case                      | Condition                   | Counter phrase    | Base             |
| ------------------------- | --------------------------- | ----------------- | ---------------- |
| Exact                     | k = 0                       | —                 | base(b)          |
| After an anchor           | k &gt; 0, b ∈ A             | `COUNT[k]` NACHᶜ  | base(b)          |
| Before an anchor          | k &gt; 0, b+1 ∈ A           | `COUNT[5−k]` VORᶜ | base(b+1 mod 12) |
| Between (no anchor ahead) | k &gt; 0, b ∈ {1, 4, 7, 10} | `COUNT[k]` NACHᶜ  | base(b)          |


`COUNT[1–4]` = EINSᶜ, ZWEIᶜ, DREIᶜ, VIERᶜ.

**Wrap special case (b = 11):** the anchor ahead is the next full hour, so the base is `H(nxt)` alone — e.g. 9:58 → „zwei vor **zehn**", not „zwei vor neun".

## 6. Rendering

Lit set = union of all cells of all active instances (ES, IST, counter words, base words). Every cell lights if *any* active instance covers it; `.` cells never light. Because hour and counter roles use disjoint instances, collisions are impossible by construction. Row 2 packs NACHᶜ·VORᶜ·FÜNF⁵ with no gaps between them; this is safe because at most two of the three are ever lit at once and the unlit one provides the separating spare letters. **Reading property:** scanning the lit cells row by row, top to bottom, left to right, yields the words exactly in sentence order — this is why the counter NACHᶜ sits in row 2 (right after the counter words in rows 0–1) and the base NACHᵇ in row 4, e.g. 14:53 → „es ist drei nach zehn vor drei".

## 7. Verification examples


| Time  | Display                                                          |
| ----- | ---------------------------------------------------------------- |
| 9:26  | „es ist vier vor halb zehn"                                      |
| 9:29  | „eins vor halb zehn"                                             |
| 9:31  | „eins nach halb zehn"                                            |
| 9:23  | „drei nach zehn vor halb zehn"                                   |
| 14:53 | „es ist drei nach zehn vor drei" (row-major reading)             |
| 9:36  | „eins nach fünf nach halb zehn"                                  |
| 9:58  | „zwei vor zehn"                                                  |
| 8:14  | „eins vor viertel über acht"                                     |
| 9:44  | „eins vor dreiviertel zehn"                                      |
| 9:16  | „eins nach viertel über neun"                                    |
| 15:45 | „dreiviertel vier"                                               |
| 11:22 | „es ist zwei nach zehn vor halb zwölf"                           |
| 12:31 | „eins nach halb eins" (12→1 wrap)                                |
| 1:58  | „zwei vor zwei" (hour &amp; counter use distinct ZWEI instances) |


## 8. How to operate

1. **Tick** — once per minute (at second 0), read the current time `H:M` (24-hour clock). On power-up, render immediately.
2. **Compute** — derive `h`, `nxt`, `b`, `k` as defined in section 3.
3. **Counter phrase** — apply section 5: none if `k = 0`; otherwise `COUNT[k]` NACHᶜ (after an anchor or between slots), or `COUNT[5−k]` VORᶜ (before an anchor).
4. **Base phrase** — apply the slot table in section 4; in the wrap case (`b = 11`, `k > 0`) the base is `H(nxt)` alone.
5. **Collect instances** — ES, IST, counter phrase, base phrase.
6. **Render** — light the union of all their cells (section 6); every other cell goes dark; `.` cells are permanently dark.
7. **Hold** — keep the pattern static until the next minute tick; the lit set only changes at minute boundaries.