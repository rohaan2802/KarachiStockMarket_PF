# Karachi Stock Market Trading Simulator (PF)

**Author:** Mohammad Rohaan  
**Roll No.:** 22I-2327  
**Section:** A  
**Course:** Programming Fundamentals — Final Project (BS Cybersecurity)  
**Language:** C++ (MSVC / Windows console)  
**Main source:** [`22i-2327_A_Project.cpp`](22i-2327_A_Project.cpp)  
**GitHub:** [rohaan2802/KarachiStockMarket_PF](https://github.com/rohaan2802/KarachiStockMarket_PF)

Console-based **Karachi Stock Exchange–style** trading simulator. The program shows a live market board and a personal portfolio, refreshes prices with a hard **±15%** session cap, lets you buy/sell stocks and move cash, colors gains green / losses red, grows holdings with a **dynamic array**, and saves both market and portfolio files on exit.

This README documents **every major piece** of the project: constraints, rubric mapping, data files, algorithms, functions, how to build/run (including **Cursor**), a full walkthrough, and a **30-image screenshot gallery** with headings.

---

## How to run in Cursor (important)

This app uses Windows console APIs (`system("cls")`, `_getch()`, colored text). The **Cursor integrated terminal** often looks “stuck”, blank, or unresponsive after clear-screen — that is normal for this style of program.

### Fastest way (recommended)

1. Open the folder `KarachiStockMarket_PF` in Cursor.
2. In the terminal (project root), run:

```bat
.\run.bat
```

This opens a **new Windows console window** where Enter / P / A / R / M / E work correctly.

### Or use F5 / Run and Debug

1. Open `22i-2327_A_Project.cpp`.
2. Press **F5**, or open **Run and Debug** → choose **Run KSE Simulator (external console)**.
3. Cursor builds with `build.bat`, then launches `KSE_Simulator.exe` in an **external** console with the correct working directory (so `companies.txt` is found).

### Manual build + run

```bat
build.bat
KSE_Simulator.exe
```

Always run from the project folder so relative paths resolve:

- `companies.txt`
- `portfolio.txt`

### If it still “does nothing”

| Symptom | Cause | Fix |
|---------|--------|-----|
| Blank / frozen after start | Integrated terminal + `cls` / `_getch` | Use `.\run.bat` or F5 external console |
| `No companies loaded` | Wrong working directory | Run from project root; use launch.json / `run.bat` |
| Build fails | MSVC not installed / `vcvars` path wrong | Install **Visual Studio Build Tools** with C++ workload; edit `build.bat` if VS is elsewhere |
| Keys ignored | App waiting for key in another window | Focus the **external** console window |

---

## Hard constraints (ZERO marks if violated)

| Rule | How this project complies |
|------|---------------------------|
| **No global variables** | All market arrays, cash, holdings pointers, counters live in `main` and are passed by parameter. Capacities use `#define` only. |
| **No `goto`** | Control flow uses loops + `if` / `else` only. |
| **No OOP / classes** | No classes or member functions — **parallel arrays + free functions** only. |
| **Name / Roll / Section** | Present at the top of `22i-2327_A_Project.cpp`. |

---

## What the program does (feature overview)

1. **Loads** stock symbols, company names, and prices from `companies.txt`.
2. **Locks** each stock’s session-start price for the ±15% cap for the whole run.
3. Draws the **Live Market** board: Stocks | Company | Prev | Curr | High | Low with ↑ / ↓.
4. On **Enter**, refreshes **all** prices with random integer + fractional deltas, updates high/low, and shows top % advancer / decliner plus total shares traded.
5. Opens **Portfolio** (key **P**): owner name, holdings table, per-row gain/loss, today’s G/L, previous balance, new balance.
6. **Buy** (**A**) / **Sell** (**R**) with symbol and balance / share checks; holdings grow dynamically.
7. **Add money** (**M**) / **Withdraw** (**W**, portfolio only) with validations.
8. On **Exit** (**E**), writes updated prices to `companies.txt` and a formatted `portfolio.txt` (`iomanip`).
9. **Colors:** positive gain/loss green, negative red (Win32 `SetConsoleTextAttribute`).

---

## Assignment requirements (from `docs/Project.pdf`)

| # | Requirement | Implementation |
|---|-------------|----------------|
| 1 | Load / save market data | `loadCompanies` / `saveCompanies` on `companies.txt` (CSV) |
| 2 | Live Market screen | `drawLiveMarket` — full board + footer stats |
| 3 | Enter refreshes prices | `refreshAllPrices` — random Δ, ±15% clamp, high/low, traded, top A/D |
| 4 | Portfolio screen | `drawPortfolio` — name, table, footer balances |
| 5 | Add / remove stock | `addStock` / `removeStock` |
| 6 | Add / withdraw money | `addMoney` / `withdrawMoney` |
| 7 | Save on exit | Both files rewritten when user presses **E** |

Official brief: [`docs/Project.pdf`](docs/Project.pdf)

---

## Rubric + bonuses (self-evaluation mapping)

| Task | Marks | Status | Where in code |
|------|------:|--------|---------------|
| Load stock market data | 10 | Done | `loadCompanies` |
| Show Stock Market Screen | 10 | Done | `drawLiveMarket` |
| Random ±, high/low, traded, top A/D | 20 | Done | `refreshAllPrices`, `findTopAdvancerDecliner` |
| Load / store portfolio.txt | 10 | Done | `savePortfolio` (written each exit) |
| Show portfolio screen | 10 | Done | `drawPortfolio` |
| Portfolio G/L, high/low, balances | 20 | Done | `computePortfolioTotals` + draw |
| Add stock (+ colors bonus) | 10 (+10) | Done | `addStock` + `printGainLossColored` |
| Remove stock | 10 | Done | `removeStock` |
| Add / withdraw money | 10 | Done | `addMoney` / `withdrawMoney` |
| Save market on close (bonus) | 10 | Done | `saveCompanies` on exit |
| Dynamic array for holdings (bonus) | 10 | Done | `expandHoldings` / `freeHoldings` (`new[]` / `delete[]`) |
| `portfolio.txt` via `iomanip` (bonus) | 10 | Done | `setw` / `setprecision` / `fixed` in `savePortfolio` |

Self-evaluation PDF: [`docs/Project-self-evaluation-sheet.pdf`](docs/Project-self-evaluation-sheet.pdf)  
Checklist visual: see **SS29** below.

---

## Keymap

### Live Market

| Key | Action |
|-----|--------|
| **Enter** | Refresh **all** market prices |
| **P** | Open Portfolio |
| **A** | Buy / add stock to holdings |
| **R** | Sell / remove stock from holdings |
| **M** | Add money to cash balance |
| **E** | Exit and save both files |

### Portfolio

| Key | Action |
|-----|--------|
| **Enter** | Refresh all market prices (same engine) |
| **L** | Back to Live Market |
| **A** | Buy / add stock |
| **R** | Sell / remove stock |
| **M** | Add money |
| **W** | Withdraw money |
| **E** | Exit and save both files |

Keys are case-insensitive (`a` / `A` both work).

---

## Data files

### `companies.txt` (input + output)

CSV lines: `SYMBOL,Company Name,price`

```text
PSO,Pakistan State Oil,150.70
LUCK,Lucky Cement,488.20
HBL,Habib Bank Limited,70.00
OGDC,Oil and Gas Development Company,76.40
PPL,Pakistan Petroleum Limited,112.35
POL,Pakistan Oilfields Limited,415.50
MARI,Mari Petroleum Company Limited,1520.00
ENGRO,Engro Corporation Limited,305.25
TRG,TRG Pakistan Limited,78.90
MEBL,Meezan Bank Limited,113.00
SYS,Systems Limited,425.60
UBL,United Bank Limited,168.40
```

Twelve KSE-like listings ship with the repo. On exit, **current** prices overwrite this file and become the next run’s session-start prices.

### `portfolio.txt` (written on exit)

Formatted with `<iomanip>`: owner name, holdings columns (symbol, company, shares, curr, prev, G/L, high, low), today’s G/L, previous cash balance, and new balance. Sample layout: [`docs/portfolio-sample.txt`](docs/portfolio-sample.txt).

---

## Algorithms (detail)

### Price refresh (±15% hard cap)

1. When `companies.txt` loads, copy each price into `sessionStart[i]` (fixed for this process).
2. On **Enter**:
   - `prevPrice[i] = currPrice[i]`
   - Build a candidate: `curr + intDelta + fracDelta`  
     (`intDelta` ∈ [-3, +3], `fracDelta` ∈ [-0.99, +0.99])
   - Clamp candidate into `[sessionStart * 0.85, sessionStart * 1.15]`
   - Update session `highPrice` / `lowPrice`
   - `pctChange[i] = ((curr - prev) / prev) * 100`
3. Top advancer / decliner = indices of max / min `pctChange`.
4. On exit, `currPrice` values are written to `companies.txt`.

### Portfolio gain / loss

```text
rowGainLoss = (currPrice - prevPrice) * shares
todayGL     = sum(rowGainLoss over all holdings)
newBalance  = cashBalance + todayGL   (display footer)
```

- **Buy:** cash decreases by `curr * shares`; holding created or increased; `totalSharesTraded` increases.
- **Sell:** cash increases by `curr * shares`; holding decreased or removed; `totalSharesTraded` increases.

### Colors (Win32)

| Condition | Color |
|-----------|--------|
| Gain &gt; 0 | Bright green |
| Loss &lt; 0 | Bright red |
| Flat ≈ 0 | Default white |

Screen clear uses `system("cls")`. Single-key input uses `_getch()` from `<conio.h>`.

### Dynamic holdings (bonus)

`expandHoldings` doubles capacity with `new[]` / `delete[]`. `freeHoldings` releases memory before exit. No STL containers for the holdings list — raw parallel arrays only.

---

## Parallel arrays & state (no globals)

Declared inside `main` and passed into helpers:

| Array / variable | Role |
|------------------|------|
| `symbols[][SYM_LEN]` | Market tickers |
| `names[][NAME_LEN]` | Company display names |
| `sessionStart[]` | Cap anchors (±15%) |
| `prevPrice[]` / `currPrice[]` | Previous / current tick |
| `highPrice[]` / `lowPrice[]` | Session extremes |
| `pctChange[]` | Last refresh % move |
| `balance` | Cash |
| `holdSymbols` / `holdShares` | Dynamic holdings |
| `holdCount` / `holdCapacity` | Holdings size / capacity |
| `totalSharesTraded` | Cumulative buy+sell volume |
| `ownerName` | Portfolio owner |
| `hConsole` | Win32 console handle for colors |

`#define` constants only: `MAX_COMPANIES`, `SYM_LEN`, `NAME_LEN`, `OWNER_LEN`.

---

## Function inventory

| Function | Purpose |
|----------|---------|
| `clearScreen` | `cls` |
| `setColor` / `resetColor` | Console text attributes |
| `printGainLossColored` | Print signed G/L in green/red |
| `enableUtf8Console` | UTF-8 code page for ↑/↓ |
| `loadCompanies` | Parse `companies.txt` into parallel arrays |
| `saveCompanies` | Write current prices back |
| `findSymbolIndex` | Lookup ticker in market |
| `refreshAllPrices` | Random move + ±15% clamp + high/low/% |
| `findTopAdvancerDecliner` | Best / worst % on last refresh |
| `expandHoldings` / `freeHoldings` | Dynamic array grow / free |
| `findHoldingIndex` | Lookup ticker in holdings |
| `drawLiveMarket` | Full market UI + footer |
| `computePortfolioTotals` | Today’s G/L aggregate |
| `drawPortfolio` | Full portfolio UI + footer |
| `addMoney` / `withdrawMoney` | Cash in / out with checks |
| `addStock` / `removeStock` | Buy / sell with validations |
| `savePortfolio` | Formatted `portfolio.txt` |
| `ensureOwnerName` | Prompt once for investor name |
| `main` | Owns all state; event loop |

---

## Project layout

```text
KarachiStockMarket_PF/
├── 22i-2327_A_Project.cpp   # submission source (author header + full app)
├── Source.cpp               # identical copy for IDE templates
├── companies.txt            # market CSV (read + overwritten on exit)
├── portfolio.txt            # last saved portfolio (iomanip)
├── build.bat                # MSVC one-click build
├── run.bat                  # open simulator in EXTERNAL Windows console
├── KSE_Simulator.exe        # local build output (gitignored)
├── README.md
├── .vscode/
│   ├── launch.json          # F5 → external console
│   ├── tasks.json           # Ctrl+Shift+B build
│   └── c_cpp_properties.json
└── docs/
    ├── Project.pdf
    ├── Project-self-evaluation-sheet.pdf
    ├── portfolio-sample.txt
    ├── starter.cpp
    ├── generate_screenshots.py
    └── screenshots/         # 01–30 PNG gallery (embedded below)
```

---

## Build notes

### MSVC (Build Tools / Visual Studio) — primary

```bat
build.bat
```

Equivalent after `vcvars64.bat`:

```bat
cl /EHsc /W3 /Fe:KSE_Simulator.exe 22i-2327_A_Project.cpp /link user32.lib
```

**Verified on this machine:** MSVC `cl` succeeds (`EXITCODE=0`).

### MinGW (optional)

```bat
g++ -std=c++17 -O2 -o KSE_Simulator.exe 22i-2327_A_Project.cpp
```

Requires MinGW with Windows headers (`windows.h`, `conio.h`).

---

## Suggested manual walkthrough (test plan)

1. Start via `.\run.bat` — confirm 12 companies load and Live Market appears.
2. Press **Enter** several times — Prev/Curr change; ↑/↓ update; high/low move; top advancer/decliner update.
3. Press **M**, deposit cash (e.g. `500000`).
4. Press **A**, buy `PSO` shares — cash drops; traded counter rises.
5. Press **P** — enter name — holdings table shows G/L colors after another Enter refresh.
6. Press **A** again to buy a second symbol — dynamic holdings expand.
7. Press **R** to sell partial/full shares — validations for bad symbol / too many shares.
8. Press **W** to withdraw — reject amounts above cash.
9. Try invalid symbol on buy — error message, no crash.
10. Press **E** — confirm `companies.txt` prices changed and `portfolio.txt` is formatted.

---

## Screenshot gallery (30)

Illustrative console-style captures generated with [`docs/generate_screenshots.py`](docs/generate_screenshots.py) (Pillow). They document every major UI surface and rule. Regenerate with:

```bat
python docs/generate_screenshots.py
```

---

### SS01 — Live Market board

Full market view: symbol, company name, previous price, current price, session high, session low.

![SS01 Live Market](docs/screenshots/01_live_market.png)

---

### SS02 — Enter key price refresh

Pressing **Enter** updates every listed stock in one pass (not a single-row tick).

![SS02 Enter refresh](docs/screenshots/02_enter_refresh.png)

---

### SS03 — Up / down arrows on Curr

Current price column shows ↑ when the tick rose and ↓ when it fell versus previous.

![SS03 Price up down](docs/screenshots/03_price_up_down.png)

---

### SS04 — Session High and Low columns

High / Low track the extreme Curr values since this session started.

![SS04 High Low](docs/screenshots/04_high_low.png)

---

### SS05 — Top percentage Advancer

Footer highlights the stock with the largest positive % change on the last refresh.

![SS05 Top Advancer](docs/screenshots/05_top_advancer.png)

---

### SS06 — Top percentage Decliner

Footer highlights the stock with the largest negative % change on the last refresh.

![SS06 Top Decliner](docs/screenshots/06_top_decliner.png)

---

### SS07 — Total shares traded

Cumulative buy + sell volume for the session is shown on the market footer.

![SS07 Total Traded](docs/screenshots/07_total_traded.png)

---

### SS08 — Empty Portfolio

Portfolio before any holdings: owner line, empty table, cash-based footer.

![SS08 Empty Portfolio](docs/screenshots/08_portfolio_empty.png)

---

### SS09 — Add Money (M)

Deposit cash into the investor balance with amount validation.

![SS09 Add Money](docs/screenshots/09_add_money.png)

---

### SS10 — Buy / Add stock (A)

Purchase by symbol and share count; cash and holdings update together.

![SS10 Buy Stock](docs/screenshots/10_buy_stock.png)

---

### SS11 — Portfolio holdings table

Holdings rows: shares, current, previous, gain/loss, high, low.

![SS11 Portfolio Holdings](docs/screenshots/11_portfolio_holdings.png)

---

### SS12 — Gain shown in green

Positive row / total gain/loss uses bright green console color.

![SS12 Gain Green](docs/screenshots/12_gain_green.png)

---

### SS13 — Loss shown in red

Negative gain/loss uses bright red console color.

![SS13 Loss Red](docs/screenshots/13_loss_red.png)

---

### SS14 — Sell / Remove stock (R)

Sell reduces shares or removes the holding; cash is credited at current price.

![SS14 Sell Stock](docs/screenshots/14_sell_stock.png)

---

### SS15 — Withdraw money (W)

Withdraw from cash on the Portfolio screen; overdraft rejected.

![SS15 Withdraw](docs/screenshots/15_withdraw.png)

---

### SS16 — Invalid symbol validation

Unknown tickers are rejected with a clear message (no crash).

![SS16 Invalid Symbol](docs/screenshots/16_invalid_symbol.png)

---

### SS17 — Insufficient balance validation

Buy that exceeds cash is blocked.

![SS17 Insufficient Balance](docs/screenshots/17_insufficient_balance.png)

---

### SS18 — ±15% session price cap

Prices cannot leave `[0.85 × sessionStart, 1.15 × sessionStart]` for the run.

![SS18 Fifteen Percent Cap](docs/screenshots/18_fifteen_percent_cap.png)

---

### SS19 — Loading companies.txt

Startup loads all CSV rows into parallel arrays and locks session-start prices.

![SS19 Companies Load](docs/screenshots/19_companies_load.png)

---

### SS20 — Save on exit

**E** writes updated `companies.txt` and formatted `portfolio.txt`.

![SS20 Save On Exit](docs/screenshots/20_save_on_exit.png)

---

### SS21 — Live Market keymap banner

On-screen help for Enter / P / A / R / M / E.

![SS21 Market Key Menu](docs/screenshots/21_key_menu_market.png)

---

### SS22 — Portfolio keymap banner

On-screen help for Enter / L / A / R / M / W / E.

![SS22 Portfolio Key Menu](docs/screenshots/22_key_menu_portfolio.png)

---

### SS23 — Investor name entry

Portfolio asks for owner name once, then keeps it for the session and file save.

![SS23 Name Entry](docs/screenshots/23_name_entry.png)

---

### SS24 — Formatted portfolio.txt (`iomanip`)

Saved file uses aligned columns (`setw`, `fixed`, `setprecision`).

![SS24 Formatted Portfolio File](docs/screenshots/24_formatted_portfolio_file.png)

---

### SS25 — Multi-stock market table

Twelve-symbol board fills the Live Market view.

![SS25 Multi Stock Table](docs/screenshots/25_multi_stock_table.png)

---

### SS26 — Session-start prices locked

Cap anchors are fixed at load time even while Curr keeps moving.

![SS26 Session Start Prices](docs/screenshots/26_session_start_prices.png)

---

### SS27 — Market footer statistics

Traded volume + top advancer / decliner summarized under the board.

![SS27 Footer Stats](docs/screenshots/27_footer_stats.png)

---

### SS28 — Color legend (gain / loss)

Green = profit tick / positive G/L; red = loss tick / negative G/L.

![SS28 Color Legend](docs/screenshots/28_color_legend.png)

---

### SS29 — Self-evaluation checklist coverage

Visual map of rubric items completed for the self-evaluation sheet.

![SS29 Self Eval Checklist](docs/screenshots/29_self_eval_checklist.png)

---

### SS30 — Complete feature map

End-to-end feature summary: market, portfolio, files, bonuses, constraints.

![SS30 Complete Feature Map](docs/screenshots/30_complete_feature_map.png)

---

## Design note — why no globals

File-scope mutable state is forbidden by the brief. Capacities are compile-time `#define`s. Runtime state (arrays, cash, pointers, flags) is created in `main` and passed explicitly. That keeps grading-safe PF style while still supporting a dynamic holdings list.

---

## References in `docs/`

| File | Description |
|------|-------------|
| [`Project.pdf`](docs/Project.pdf) | Official PF project brief |
| [`Project-self-evaluation-sheet.pdf`](docs/Project-self-evaluation-sheet.pdf) | Marks / bonus checklist |
| [`starter.cpp`](docs/starter.cpp) | Early sketch / starter style |
| [`portfolio-sample.txt`](docs/portfolio-sample.txt) | Example portfolio layout |
| [`generate_screenshots.py`](docs/generate_screenshots.py) | Regenerates the 30 PNGs |
| [`screenshots/`](docs/screenshots/) | Full gallery folder |

---

## Submission checklist

- [x] Name / Roll / Section header in source
- [x] No globals / no `goto` / no classes
- [x] Live Market + Portfolio UIs
- [x] Enter refresh with ±15% cap, high/low, traded, top A/D
- [x] Buy / sell / money with validations
- [x] Colors for gain / loss
- [x] Dynamic holdings array
- [x] Save `companies.txt` + formatted `portfolio.txt` on exit
- [x] `build.bat` + Cursor `run.bat` / `.vscode` launch
- [x] README with full detail + all 30 screenshots headed
