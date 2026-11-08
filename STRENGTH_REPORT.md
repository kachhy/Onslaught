# Onslaught: Strength Roadmap

*Written 2026-09-23 against commit `8dac1e1` (v2.0, ~3470 CCRL 40/15). Last updated 2026-09-23 at `ac5074d`; all line numbers match that commit.*

This report lists what to work on next, beyond the two items already in progress: **network scaling** and **qsearch delta pruning**. The original findings come from reading the source. Items marked done were verified as described in the progress log.

The expected-gain tiers (**High / Medium / Low**) are rough guesses based on how the same changes went in other engines' SPRT histories. Every item still needs its own verification (see §6 for which kind).

---

## TL;DR

1. ✅ **Time manager fixed** (`ac5074d`). It used to give the whole remaining clock to the last move of every 40-move period, so it lost on time in the exact format CCRL 40/15 uses. See §1.1.
2. **Fix the three cheap search bugs:** history is wiped on every `go`, the singular-extension verification search can be cut off by probcut, NMP or razoring, and the TT ages every iteration instead of every search.
3. **Add a TT to qsearch and pack the TT entries.** That doubles the entries per MB and makes room to cache static eval.
4. **Biggest new features:** correction history, capture history, and continuation history at 2 and 4 plies.
5. **Then:** pruning and LMR refinements, a proper time manager, and Lazy SMP.
6. **SPSA-retune everything after the structural changes.**

---

## Progress log

| Date | Item | Commit | Verification |
|---|---|---|---|
| 2026-09-23 | §1.1 time-manager safety, plus two related loss paths found while fixing it (negative clocks, no move after an aborted first iteration) | `ac5074d` | Bench unchanged at `345282`. Scripted self-play at repeating 40/4 s and 40/10 s: the old build lost on time at move 40 both times, the new build played 100 plies with no flag. No SPRT, because none of the changes can trigger at the SPRT time controls (§6) |

---

## Current state

| Area | Present |
|---|---|
| Search | Iterative deepening, aspiration windows, PVS, check extension, mate-distance pruning, RFP (improving-aware), NMP, ProbCut, razoring, IIR, futility pruning, LMP, SEE pruning of captures, singular extensions (double and negative extensions, multi-cut), LMR (log table, history, cut-node and no-TT terms) |
| Ordering | Staged move picker, TT move, MVV-LVA with a SEE good/bad split, 2 killers, butterfly history, 1-ply continuation history with malus |
| Eval | NNUE `(768×4 mirrored king buckets → 1024)×2 → 8 material output buckets`, SCReLU, lazy accumulator updates, Finny tables, AVX2/AVX-512, 50-move-rule scaling |
| Time | Soft/hard limits from the clock minus `MoveOverhead`, with the hard limit capped at 80% of that (`ac5074d`) |
| Other | Syzygy (WDL in search, DTZ filter at the root), MultiPV, SPSA setup, viriformat datagen, OpenBench |
| **Missing** | Correction history, capture history, multi-ply continuation history, TT in qsearch, multithreading, time-manager scaling, quiet SEE pruning, history pruning |

---

## 1. Bugs and cheap wins

### 1.1 ✅ Time management could lose on time (fixed in `ac5074d`)

**What was wrong:**
- In `movestogo` mode, `hard_cap = time / movestogo + inc`. When `movestogo` was 1 that was the **entire remaining clock**, with nothing held back for GUI or OS latency. CCRL 40/15 repeats its time control, so `movestogo 1` comes up at the end of every period.
- There was no `Move Overhead` UCI option.
- In sudden death, `time/20 + inc/2` could be larger than the clock when the increment was bigger than the remaining time, for example `wtime 200 winc 1000`.

**What changed:**
- **Time limits** (`search.cpp:698-721`): the budget is computed from `clock - MoveOverhead`. The hard limit is capped at 80% of that, and the soft limit is clamped to `[1, hard]`. The formulas are otherwise unchanged.
- **`MoveOverhead` option** (`uci.cpp:120`): 0–5000 ms, default 20. It's one word because `parseSetOption` only reads the first word of an option name, so "Move Overhead" would be silently ignored.
- **Negative clocks** (`uci.cpp:200-205`): `wtime`/`btime` are clamped to 0. Before, `wtime -1` collided with the "no clock" sentinel and the engine searched until it was told to stop. Some GUIs send negative times when a time margin is allowed.
- **No move found** (`search.cpp:891-899`): if the first iteration is aborted, the engine plays the first legal move (the first DTZ-allowed one when the root filter is active) instead of `bestmove 0000`, which is an automatic loss.

**Verification:** a Python driver timed single `go` commands and played self-play games with real clocks for both sides (wall time including pipe latency).

| Test | Before (`8dac1e1`) | After (`ac5074d`) |
|---|---|---|
| Self-play, 40 moves in 4 s, repeating | lost on time at move 40 (ply 79) | 100 plies, no flag; lowest clock 33 ms |
| Self-play, 40 moves in 10 s, repeating | lost on time at move 40 (ply 79) | 100 plies, no flag; lowest clock 65 ms |
| `movestogo 1`, 1000 ms left | 1012 ms (over the clock) | 802 ms |
| 200 ms left + 1 s increment | 382 ms (over the clock) | 160 ms |
| `wtime -1` | never stopped | 12 ms |
| `go nodes 0` | `bestmove 0000` | a legal move |
| `MoveOverhead 500`, `movestogo 1`, 1000 ms left | — | 410 ms (expected ~400) |
| `movestogo 40` / 6 s + 0.1 s | 386 / 272 ms | 384 / 302 ms (normal timing noise) |

The reserve left on the last move of a period scales with the time control. In a real 40/15 game it's about 20% of what's left on move 40, which is several seconds.

**Effect at the SPRT time controls:** the overhead lowers each move's soft limit by about 0.7 ms and its hard limit by about 1 ms. The 80% cap only applies when the clock falls below about two-thirds of the increment, and the clock settles at around 10–20 times the increment. Neither change shows up in an SPRT.

**Still open:** in `movestogo` mode `soft == hard`, so every move runs until the hard stop and wastes its last partial iteration. That costs strength, not games on time, so it belongs with the §3 work.

### 1.2 History is reset on every `go` (small effort, Low–Medium)

**Where:** `search.cpp:696` calls `resetHistory()` at the start of every search.

Everything the butterfly and continuation histories learned on the last move is thrown away.

**Try:** clear history only on `ucinewgame`. As a second test, scale the tables down (for example `h /= 2`) at the start of each `go` instead of zeroing them.

### 1.3 The singular verification search can be cut short (bug, small effort, Low–Medium)

**Where:** NMP `search.cpp:404`, ProbCut `:424`, razoring `:470`

None of the three checks `ss->excluded`. During the verification search at `:549`, any of them can return a cutoff. **ProbCut can even search the excluded move itself**, because the TT move is often a capture. That can inflate `s_score`, which suppresses extensions and triggers false multi-cuts.

**Fix:** add `ss->excluded == NO_MOVE` to all three conditions.

### 1.4 TT entries use half their space (medium effort, Low–Medium, more with larger hash)

**Where:** `src/hash/transposition.h:6-17`

`Move` is a `uint32_t`, so `Entry` pads out to 16 bytes. Each `EntryTriple` is `3 × 16 + 1 = 49` bytes, aligned up to 64. That's 3 entries per cache line, with 15 bytes plus padding wasted.

**Proposed layout:** 10-byte entries in 32-byte buckets, which doubles the entries per MB. It also makes room for the static eval.

```cpp
struct Entry {                 // 10 bytes, 2-byte aligned, no packing pragmas needed
    uint16_t key;
    uint16_t move;             // from | to << 6 | flags << 12; rebuild the piece with board.pieceAt(from)
    int16_t  score;
    int16_t  static_eval;      // lets a TT hit skip the NNUE eval at search.cpp:380
    uint8_t  depth;
    uint8_t  age_pv_bound;     // 5-bit age | 1-bit ttPv | 2-bit bound
};
struct alignas(32) Bucket { Entry entries[3]; uint16_t pad; };  // 32 bytes
```

Drop the `count` field and treat an all-zero entry as empty. `isLegal` (`movepicker.cpp:7`) already validates TT moves, so rebuilding the move from 16 bits is safe.

Related issues:
- **Age is bumped on every iteration** (`search.cpp:787`) instead of once per `go`. The replacement score `depth - (age - last_seen)` then treats entries from two or three iterations ago as stale. Move `tt.incAge()` to the start of `search()`.
- **Replacement for the same key** (`transposition.cpp:43`) only overwrites when `depth >=` or the bound is exact. The more common rule is `bound == EXACT || depth + 4 > old_depth || age differs`.
- **On fail-low** the stored move is whichever move happened to score highest (`search.cpp:677`). Try keeping the old TT move when the node fails low.

### 1.5 Qsearch doesn't use the TT (small–medium effort, Medium)

**Where:** `search.cpp:130-225`

Qsearch never probes or stores the TT, and it orders moves with its own MVV-LVA selection sort instead of trying the TT move first.

**Fix:**
- Probe at the top and cut off on a matching bound. Qsearch depth is 0, so any entry qualifies.
- Use the TT score to improve stand-pat, the same way as `:376-384`.
- Store at the end with depth 0.
- Build this together with delta pruning, which is already in progress.

### 1.6 Smaller correctness, UCI and build issues

| Where | Issue | Fix |
|---|---|---|
| `search.cpp:300`, `:471` | `quiesce(..., ply + 1, ...)` is called at the **same** node, so mate distances and `seldepth` are off by one | Pass `ply` |
| `search.cpp:156` | In check at `qply >= 2`, qsearch returns the static eval, which is unsound because it could be mate. Qsearch never generates quiet checks, so check sequences are bounded anyway | Remove it and SPRT the change |
| `search.cpp:435-466` | ProbCut searches every noisy move, including losing captures, and doesn't store to the TT on a cutoff | Skip a move if `!SEE(move, prob_beta - static_eval)`; store a `LOWERBOUND` at `depth - 3` on a cutoff |
| `movepicker.cpp:168`, `:211` | Promotions score 40000, which is the "good" threshold, so **under**-promotions are tried before killers. `see.h:11` returns true for every promotion | Score under-promotions as bad noisy moves |
| `uci.cpp:365-378` | `checkStdin` reads one line during search and ignores everything except `stop`, `quit` and `debug`. **An `isready` sent during search is swallowed** with no `readyok` | Reply to `isready`. Better still, move UCI input to its own thread, which Lazy SMP needs anyway (§4) |
| `uci.cpp:229-234` | `go searchmoves` is parsed but never used | Feed it into `rml.allow()` |
| `Makefile` `release` target | Links dynamically. A Windows (MSYS2) build won't start (exit 127) unless the MinGW runtime DLLs are on `PATH` | Add `-static` on non-Darwin, as the `lto` and `native` targets already do |

---

## 2. Missing search features

### 2.1 Correction history (medium effort, **High**)

This is likely the biggest single search gain available. It learns how far static eval is off in similar positions and adjusts for it. `Board` already has `pawnHash()` (`board.h:69`), so a pawn-structure table costs almost nothing to add.

```cpp
constexpr int CORR_ENTRIES = 16384;
constexpr int CORR_GRAIN   = 256;
constexpr int CORR_MAX     = 64 * CORR_GRAIN;          // tune
int pawn_corr[2][CORR_ENTRIES];

int correctEval(const Board& b, int raw) {
    const int c = pawn_corr[b.getSTM()][b.pawnHash() % CORR_ENTRIES];
    return std::clamp(raw + c / CORR_GRAIN, -SCORE_MAX + MAX_GAME_MOVES + 1, SCORE_MAX - MAX_GAME_MOVES - 1);
}

void updateCorr(const Board& b, int depth, int diff /* best_score - raw_eval */) {
    int& e = pawn_corr[b.getSTM()][b.pawnHash() % CORR_ENTRIES];
    const int w = std::min(depth + 1, 16);
    e = std::clamp((e * (256 - w) + diff * CORR_GRAIN * w) / 256, -CORR_MAX, CORR_MAX);
}
```

**Update it only when all of these hold:**
- the node isn't in check,
- `ss->excluded == NO_MOVE`,
- the best move is quiet or there isn't one,
- the bound agrees with the error: skip `LOWERBOUND && best_score <= raw_eval` and skip `UPPERBOUND && best_score >= raw_eval`.

**Refactors this needs:**
- Keep the **raw corrected eval** in `ss->static_eval`, used for `improving` and the update, separate from the **TT-adjusted eval** used for pruning. Right now `search.cpp:376-384` overwrites `ss->static_eval` with the TT score.
- The beta-cutoff path returns early at `search.cpp:657`. Change it to `break` so the fail-high and fail-low paths share one tail for the TT store and the correction update.

**Follow-ups, each a separate SPRT:**
- non-pawn tables per side, which need two new incremental Zobrist keys,
- a minor-piece table,
- continuation correction indexed by `(ss-1)->move`.

### 2.2 Capture history (medium effort, Medium–High)

Captures are ordered by MVV-LVA plus a SEE sign only (`movepicker.cpp:159-177`).

- Add `capt_hist[stm][piece][to][captured_type]` with the same gravity update as `score_history`.
- On a cutoff, give the best move a bonus and penalize the captures tried before it. When a quiet move causes the cutoff, penalize all captures tried.
- Ordering score: `MVV * k + capt_hist`. Split good and bad captures with `SEE(move, -score / X)` instead of a threshold of 0.
- Use it for LMR of captures (§2.6) and for SEE-pruning margins.

### 2.3 Multi-ply continuation history and the bonus formula (small–medium effort, Medium)

**Where:** `history.cpp:37` uses only `{ 1 }`, and `getContHist` (`:60`) reads only `ss - 1`.

- The search stack is already padded for `ss - 1/2/4` (`search.cpp:690-693`), so this is mostly wiring. Add plies 2 and 4, and sum them for ordering and for LMR history.
- Commit `db58510` ("Switch to 1-ply CH") backed away from this once. It's worth another attempt now that the rest of the search has changed.
- Bonus and malus are both `min(depth², 8192)` (`history.cpp:13`). Try the common linear form with separate bonus and malus terms, and put the constants in SPSA:
  - `bonus = min(A * depth - B, C)`
  - `malus = min(D * depth - E, F)`
- For speed, store a pointer to the continuation-history sub-table in `SearchStack` instead of re-indexing it every time.

### 2.4 Threat-aware quiet history (small effort, Medium)

- Index butterfly history as `[stm][from_attacked][to_attacked][from][to]`. `getThreatenedByXSTM()` already exists, so this is cheap.
- A follow-up: in quiet scoring, reward moving a piece out of a threat and penalize moving one into a threat.

### 2.5 More pruning (small effort, Medium together)

| Technique | Where | Change |
|---|---|---|
| History pruning | move loop | Skip a quiet move at low `lmr_depth` when `hist < -K * depth` |
| History-aware futility | `search.cpp:494`, `:517` | Use `static_eval + A + B * lmr_depth + hist / C <= alpha` per move instead of one node-level flag |
| Quiet SEE pruning | `search.cpp:527` is capture-only | Prune a quiet move if `!SEE(move, -Q * lmr_depth²)` or `-Q * lmr_depth` |
| LMP with improving | `search.cpp:523` | `moves_searched >= (LMP_BASE + depth²) / (2 - improving)` |
| Mate guard | all move-loop pruning | Only prune once `best_score > -SCORE_MAX + MAX_GAME_MOVES` |

### 2.6 LMR refinements (small effort each, Medium together)

**Where:** `search.cpp:583-611`

- **Start earlier.** `LMR_MOVES_CUTOFF = 5`. Try `moves_searched >= 1 + is_pv` (or `2 + is_pv`).
- **Use `improving`.** The comment at `:584` mentions it, but the code never uses it.
- **Missing adjustments:** `-1` for PV nodes or ttPv, `+1` when the TT move is a capture, `-1` for moves that give check, and `+1` when the node's static eval is far below alpha.
- **Reduce captures and promotions too**, with their own base table and capture history (§2.2).
- **Do-deeper/do-shallower:** after the reduced search fails high, set `new_depth += (score > best_score + M)` and `new_depth -= (score < best_score + new_depth)`.
- **Post-LMR continuation-history update:** after the full-depth re-search, reward the move if it beat alpha and penalize it otherwise.

### 2.7 Other search tweaks

| Item | Where | Change |
|---|---|---|
| NMP | `search.cpp:405` | Add `min((static_eval - beta) / NMP_EVAL_DIV, 3)` to the reduction. Try `static_eval >= beta - a * depth + b` as the entry condition |
| RFP | `search.cpp:398` | Return `(static_eval + beta) / 2` instead of `static_eval` |
| Singular extensions | `search.cpp:540-564` | Lower the depth threshold from 8 to about 6. Tune the `s_beta` margin (`2 * depth` now). **Cap double extensions**: add a `double_exts` counter to `SearchStack`, because nothing limits them now and the search can blow up. Add a negative extension for cut nodes (`-2`). Consider a triple extension later |
| ttPv flag | TT, search | Store whether the node was ever on the PV. Use it to reduce less and prune less |
| Aspiration windows | `search.cpp:823-836` | On a fail-low, set `beta = (alpha + beta) / 2`. On a fail-high, re-search at `depth - 1` (reset after an exact score) |
| Killers | `movepicker.cpp:232-241` | With better history, 1 killer (or none) often tests better than 2 |
| Qsearch move picker | `search.cpp:179-200` | Use `MovePicker(QSEARCH)` so qsearch gets the TT move, capture history and staged generation |

---

## 3. Time management upgrade (medium effort, Medium)

This builds on the §1.1 fix. The soft limit should scale with:

- **The share of nodes spent on the best move.** Count nodes per root move. If most of the effort went to the best move, stop early; if not, keep going.
- **Best-move stability.** Keep a count of how many iterations in a row had the same best move and look up a scale factor from it.
- **Score changes.** Spend more time when the score drops between iterations and less when it's steady.
- **A real soft limit in `movestogo` mode.** Right now `soft == hard` there (§1.1, still open). Make the hard limit a multiple of the soft one, still under the 80% cap.

This gains at every time control and doesn't depend on anything else in this report.

---

## 4. Lazy SMP (large effort, High on multi-CPU lists, none on 1-CPU)

**Where:** `uci.cpp:118` fixes `Threads` at `1..1`, so the 4-CPU lists and multi-core events currently get no benefit from extra cores.

**State that has to change:**

| State | Where | Change |
|---|---|---|
| `searching` flag | `uci.cpp:17` (**thread_local**) | Must become a shared `std::atomic<bool>`. As a thread-local it would start out false in helper threads, so they'd exit immediately |
| `score_history`, `cont_hist` | `history.cpp:4-5` | Per thread (put them in a `ThreadData` struct) |
| `accumulator_cache` (Finny tables) | `nnue.cpp:37` | Per thread |
| Global `Board` | `uci.cpp:16` | Each thread copies it, including its accumulator stack |
| `nodes`, `seldepth`, `tb_hits` | `search.cpp:23-25` | Already thread-local. Sum them across threads for `info` output |
| PV table, `SearchStack` | `search.cpp:685`, `:692` | Already per call |
| TT | `transposition.cpp` | Races are tolerable because TT moves go through `isLegal`. Remove or relax the `table_size++` and `last_seen` writes on reads. A torn 10-byte read can mix fields; accept it or add the XOR-key trick |
| Stdin polling | `uci.cpp:345` | Only the main thread should poll, or move input to its own thread (see §1.6) |

Only the main thread prints `info` and chooses `bestmove`. A later improvement is to pick `bestmove` by votes across threads.

---

## 5. Network work after the current scaling

- **Architecture:**
  - The next steps after a wider L1 are more king buckets (the current 4 are rank-only), then either **threat inputs** or a small hidden stack (`L1 → 16 → 32 → 1`, pairwise multiplication). Threat inputs have been one of the bigger recent eval gains in other engines.
  - Use a factorizer during training when adding buckets.
- **Data volume:**
  - The 12 binpacks in the working tree total about 58 MB, very roughly low tens of millions of positions. If that's most of the corpus, bigger nets will run short of data. Plan for billions of positions.
  - `DG_THREADS` (`datagen.cpp:35`) is a compile-time `1`, although worker threads are already implemented. Make it a CLI argument.
- **Relabelling:** once a stronger net exists, re-scoring older data with it is cheap Elo.
- **Retune after swapping nets:** `NNUE_WEIGHT_SCALAR` (`features.h:54`) and the 50-move-rule scaling curve.

---

## 6. Tuning and testing process

- **Retune after structural changes.** Almost every change in §1–§2 shifts the best values for the existing margins, so run an SPSA session after each batch.
- **Hard-coded constants to add to `SEARCH_TUNABLES`:**

  | Constant | Where |
  |---|---|
  | SE depth `8`, TT-depth margin `3`, `s_beta = tt_score - 2*depth`, double-extension margin `20` | `search.cpp:540-557` |
  | `MAX_HISTORY` and the `depth²` bonus | `history.h:8`, `history.cpp:13` |
  | Qsearch in-check `qply` cap | `search.cpp:156` |
  | `SEE_VALUES` | `see.h:7` |
  | `NNUE_WEIGHT_SCALAR` | `features.h:54` |
  | All new constants from §2 | — |

- **Match the test to the change.** Only SPRT a change that can trigger at the SPRT time controls (sudden death plus increment, no `movestogo`). Time-manager safety fixes that only act at `movestogo`, on a nearly empty clock or on unusual GUI input can't be measured there. Check those with a scripted self-play game at the affected time control plus an unchanged bench, as in §1.1.
- **Keep allocation changes separate.** Time-manager changes that do alter allocation at the SPRT time controls (all of §3) still get their own STC and LTC SPRTs, never bundled with search changes.

---

## 7. Suggested order

| # | Item | Section | Type | Effort | Expected |
|---|---|---|---|---|---|
| 1 | ✅ Time-manager safety (overhead, clamps) | §1.1 | Bug | S | **Done** in `ac5074d` |
| 2 | Guard NMP, ProbCut and razoring in the singular verification search | §1.3 | Bug | S | Low–Med |
| 3 | Keep history between moves | §1.2 | Cheap win | S | Low–Med |
| 4 | Smaller correctness fixes (`ply + 1`, `isready`, ProbCut SEE, static release build) | §1.6 | Bug | S | Low |
| 5 | TT in qsearch | §1.5 | Feature | S–M | Medium |
| 6 | Pack the TT, cache static eval, age per `go` | §1.4 | Performance | M | Low–Med |
| 7 | Correction history (pawn first) | §2.1 | Feature | M | **High** |
| 8 | Capture history | §2.2 | Feature | M | Med–High |
| 9 | Continuation history at plies 2 and 4, new bonus formula | §2.3 | Feature | S–M | Medium |
| 10 | History pruning, quiet SEE pruning, LMP with improving | §2.5 | Feature | S | Medium |
| 11 | LMR refinements | §2.6 | Tuning | S each | Medium together |
| 12 | NMP, RFP, SE, aspiration and ttPv tweaks | §2.7 | Tuning | S each | Low–Med each |
| 13 | SPSA retune | §6 | Process | Compute | Medium |
| 14 | Time-manager scaling (nodes, stability, score, `movestogo` soft limit) | §3 | Feature | M | Medium |
| 15 | Lazy SMP | §4 | Feature | L | High (multi-CPU) |
| 16 | Next network architecture and more data | §5 | Eval | L | High |

**Phases:**
- **Phase 1** (items 2–6): short, independent diffs, each its own branch. Item 1 is done.
- **Phase 2** (items 7–12): the core search work. Rebase each item on the last one that passed.
- **Phase 3** (items 13–14): retune, then the time manager.
- **Phase 4** (items 15–16): SMP and network work, in parallel with each other.
