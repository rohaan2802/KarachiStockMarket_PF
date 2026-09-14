# Karachi Stock Market Trading Simulator (PF)

**Author:** Mohammad Rohaan  
**Roll No.:** 22I-2327  
**Section:** A  
**Course:** Programming Fundamentals — Final Project (BS Cybersecurity)

Console-based **Karachi Stock Exchange–style** trading simulator written in C++ using only Programming Fundamentals constructs: functions, parallel arrays, strings, files, `iomanip`, loops, random numbers, and `conio` / Win32 console APIs.

---

## Hard constraints (ZERO if violated)

| Rule | Status |
|------|--------|
| **No global variables** | Done — all state lives in `main` locals and is passed by parameter |
| **No `goto`** | Done |
| **No OOP / classes** | Done — no classes; function + array design only |
| Top-of-file Name / Roll / Section comments | Done |

---

## Requirements (from `docs/Project.pdf`)

1. **Load / save market data** — read `companies.txt` (CSV: symbol, company name, price) into parallel arrays; on exit write updated prices back.
2. **Live Market screen** — columns Stocks | Company | Prev | Curr | High | Low with ↑ / ↓ arrows.
3. **Enter refreshes prices** — random integer + fractional change; **hard cap ±15% of session-start price**; update high/low; track total shares traded; show top % advancer & decliner.
4. **Portfolio screen** — owner name; holdings table with shares, current, previous, gain/loss, high, low; footer today’s G/L, previous balance, new balance.
5. **Add / remove stock** with symbol and balance / share validations.
6. **Add / withdraw money** with validations.
7. **Save** `companies.txt` and formatted `portfolio.txt` on exit.

### Rubric + bonuses mapped

| Task | Marks | Status |
|------|------:|--------|
| Load stock market data | 10 | **Done** |
| Show Stock Market Screen | 10 | **Done** |
| Random ±, high/low, traded, top A/D | 20 | **Done** |
| Load / store portfolio.txt | 10 | **Done** |
| Show portfolio screen | 10 | **Done** |
| Portfolio G/L, high/low, balances | 20 | **Done** |
| Add stock (+ colors bonus) | 10 (+10) | **Done** |
| Remove stock | 10 | **Done** |
| Add / withdraw money | 10 | **Done** |
| Save market on close (bonus) | 10 | **Done** |
| Dynamic array for holdings (bonus) | 10 | **Done** (`new[]` / `delete[]`) |
| `portfolio.txt` via `iomanip` (bonus) | 10 | **Done** |

Self-evaluation sheet: `docs/Project-self-evaluation-sheet.pdf`  
Illustrated checklist screenshot: `docs/screenshots/29_self_eval_checklist.png`

---

## Keymap

### Live Market

| Key | Action |
|-----|--------|
| **Enter** | Refresh all prices |
| **P** | Open Portfolio |
| **A** | Buy / add stock |
| **R** | Sell / remove stock |
| **M** | Add money |
| **E** | Exit (save files) |

### Portfolio

| Key | Action |
|-----|--------|
| **Enter** | Refresh all prices (same engine) |
| **L** | Live Market |
| **A** | Buy / add stock |
| **R** | Sell / remove stock |
| **M** | Add money |
| **W** | Withdraw money |
| **E** | Exit (save files) |

---

## File formats

### `companies.txt`

```text
SYMBOL,Company Name,price
PSO,Pakistan State Oil,150.70
LUCK,Lucky Cement,488.20
...
```

Twelve KSE-like symbols are included (PSO, LUCK, HBL, OGDC, PPL, POL, MARI, ENGRO, TRG, MEBL, SYS, UBL).

### `portfolio.txt` (written on exit with `iomanip`)

Formatted table with owner name, holdings, today’s G/L, previous balance, and new balance (see sample in `docs/portfolio-sample.txt`).

---

## Algorithms

### Price refresh (±15% hard cap)

1. Store `sessionStart[i]` when the file is loaded (fixed for this run).
2. On **Enter**:
   - `prev = curr`
   - `candidate = curr + intDelta + fracDelta`  
     (`intDelta` ∈ [-3, +3], `fracDelta` ∈ [-0.99, +0.99])
   - Clamp to `[sessionStart * 0.85, sessionStart * 1.15]`
   - Update session high / low
   - `pctChange = ((curr - prev) / prev) * 100`
3. Top advancer / decliner = max / min `pctChange`.
4. On program exit, current prices are written to `companies.txt` and become the next session’s start prices.

### Portfolio gain / loss

```text
gainLoss = (currPrice - prevPrice) * shares
todayGL  = sum(gainLoss over holdings)
newBalance (display) = cashBalance + todayGL
```

Buy deducts `curr * shares` from cash; sell credits the same.  
`totalSharesTraded` increments on every buy and sell.

### Colors (Win32)

- Gain → `FOREGROUND_GREEN | FOREGROUND_INTENSITY`
- Loss → `FOREGROUND_RED | FOREGROUND_INTENSITY`
- Screen clear → `system("cls")`
- Input → `_getch()` from `<conio.h>`

### Dynamic holdings (bonus)

Holdings grow with `expandHoldings()` using `new[]` / `delete[]` (doubling capacity). Freed on exit.

---

## Project layout

```text
KarachiStockMarket_PF/
├── 22i-2327_A_Project.cpp   # main submission source
├── Source.cpp               # identical copy
├── companies.txt
├── portfolio.txt
├── build.bat                # MSVC one-click build
├── README.md
└── docs/
    ├── Project.pdf
    ├── Project-self-evaluation-sheet.pdf
    ├── portfolio-sample.txt
    ├── starter.cpp
    ├── generate_screenshots.py
    └── screenshots/         # 01–30 PNG gallery
```

---

## Build & run (Windows)

### MSVC (Build Tools / Visual Studio)

```bat
build.bat
KSE_Simulator.exe
```

Or manually after `vcvars64.bat`:

```bat
cl /EHsc /W3 /Fe:KSE_Simulator.exe 22i-2327_A_Project.cpp /link user32.lib
```

### MinGW (if installed)

```bat
g++ -std=c++17 -O2 -o KSE_Simulator.exe 22i-2327_A_Project.cpp
```

Run from the project folder so `companies.txt` / `portfolio.txt` resolve correctly.

**Build status (this machine):** MSVC `cl` **succeeded** (`EXITCODE=0`).

---

## Screenshot gallery (30)

Generated with `docs/generate_screenshots.py` (Pillow):

```bat
python docs/generate_screenshots.py
```

| # | File | Covers |
|--:|------|--------|
| 01 | `01_live_market.png` | Live market |
| 02 | `02_enter_refresh.png` | Enter refresh |
| 03 | `03_price_up_down.png` | Up / down arrows |
| 04 | `04_high_low.png` | High / low |
| 05 | `05_top_advancer.png` | Top advancer |
| 06 | `06_top_decliner.png` | Top decliner |
| 07 | `07_total_traded.png` | Total traded |
| 08 | `08_portfolio_empty.png` | Empty portfolio |
| 09 | `09_add_money.png` | Add money |
| 10 | `10_buy_stock.png` | Buy stock |
| 11 | `11_portfolio_holdings.png` | Holdings table |
| 12 | `12_gain_green.png` | Gain green |
| 13 | `13_loss_red.png` | Loss red |
| 14 | `14_sell_stock.png` | Sell stock |
| 15 | `15_withdraw.png` | Withdraw |
| 16 | `16_invalid_symbol.png` | Invalid symbol |
| 17 | `17_insufficient_balance.png` | Insufficient balance |
| 18 | `18_fifteen_percent_cap.png` | ±15% cap note |
| 19 | `19_companies_load.png` | companies load |
| 20 | `20_save_on_exit.png` | Save on exit |
| 21 | `21_key_menu_market.png` | Market keymap |
| 22 | `22_key_menu_portfolio.png` | Portfolio keymap |
| 23 | `23_name_entry.png` | Name entry |
| 24 | `24_formatted_portfolio_file.png` | Formatted portfolio file |
| 25 | `25_multi_stock_table.png` | Multi-stock table |
| 26 | `26_session_start_prices.png` | Session-start prices |
| 27 | `27_footer_stats.png` | Footer stats |
| 28 | `28_color_legend.png` | Color legend |
| 29 | `29_self_eval_checklist.png` | Self-eval checklist |
| 30 | `30_complete_feature_map.png` | Complete feature map |

---

## No-globals design note

There are **zero file-scope variables**. Capacities use `#define`. Market arrays, cash balance, dynamic holdings pointers, and traded-share counters are declared inside `main` and threaded through function parameters. Console color uses a `HANDLE` obtained in `main` and passed down.

---

## References in `docs/`

- Official brief: `Project.pdf`
- Self-evaluation sheet: `Project-self-evaluation-sheet.pdf`
- Starter sketch: `starter.cpp`
- Sample portfolio layout: `portfolio-sample.txt`
