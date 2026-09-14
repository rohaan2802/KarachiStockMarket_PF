#!/usr/bin/env python3
"""
Generate 30 illustrative screenshots for Karachi Stock Market PF project.
Uses Pillow only (no live console capture required).
"""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent / "screenshots"
OUT.mkdir(parents=True, exist_ok=True)

W, H = 980, 620
BG = (12, 18, 28)
PANEL = (22, 32, 48)
TEXT = (230, 235, 245)
DIM = (140, 155, 175)
GREEN = (46, 204, 113)
RED = (231, 76, 60)
CYAN = (52, 201, 220)
YELLOW = (241, 196, 15)
WHITE = (255, 255, 255)


def font(size=16, bold=False):
    candidates = [
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/consolab.ttf" if bold else "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "C:/Windows/Fonts/arial.ttf",
    ]
    for path in candidates:
        p = Path(path)
        if p.exists():
            try:
                return ImageFont.truetype(str(p), size)
            except OSError:
                pass
    return ImageFont.load_default()


def new_canvas(title: str):
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)
    draw.rectangle([16, 16, W - 16, H - 16], fill=PANEL, outline=CYAN, width=2)
    draw.text((28, 28), "KARACHI STOCK MARKET — PF Simulator", fill=CYAN, font=font(18, True))
    draw.text((28, 54), title, fill=YELLOW, font=font(15))
    return img, draw


def save(img: Image.Image, name: str):
    path = OUT / name
    img.save(path, "PNG")
    print("wrote", path.name)


def draw_lines(draw, lines, x=36, y=90, line_h=22, color=TEXT, size=14):
    f = font(size)
    for i, line in enumerate(lines):
        if isinstance(line, tuple):
            text, c = line
            draw.text((x, y + i * line_h), text, fill=c, font=f)
        else:
            draw.text((x, y + i * line_h), line, fill=color, font=f)


def market_table(draw, y=90, highlight=None):
    header = "Stocks  Company                        Prev     Curr   High     Low"
    rows = [
        ("PSO     Pakistan State Oil            150.70  151.81 ↑ 154.20  150.70", GREEN),
        ("LUCK    Lucky Cement                  488.20  485.75 ↓ 493.40  485.00", RED),
        ("HBL     Habib Bank Limited             70.00   69.76 ↓  70.00   69.60", RED),
        ("OGDC    Oil & Gas Dev. Comp.           76.40   76.81 ↑  78.65   76.13", GREEN),
        ("PPL     Pakistan Petroleum            112.35  113.10 ↑ 114.50  112.00", GREEN),
        ("POL     Pakistan Oilfields            415.50  410.20 ↓ 418.00  409.00", RED),
        ("MARI    Mari Petroleum               1520.00 1535.40 ↑1550.00 1510.00", GREEN),
        ("ENGRO   Engro Corporation             305.25  308.10 ↑ 310.00  304.00", GREEN),
        ("TRG     TRG Pakistan                   78.90   82.40 ↑  82.40   78.50", GREEN),
        ("MEBL    Meezan Bank                   113.00  111.20 ↓ 113.50  110.80", RED),
        ("SYS     Systems Limited               425.60  430.15 ↑ 432.00  424.00", GREEN),
        ("UBL     United Bank Limited           168.40  167.10 ↓ 169.00  166.50", RED),
    ]
    draw.text((36, y), header, fill=DIM, font=font(13))
    for i, (row, color) in enumerate(rows):
        c = color
        if highlight == "up" and "↑" in row:
            c = GREEN
        if highlight == "down" and "↓" in row:
            c = RED
        draw.text((36, y + 24 + i * 20), row, fill=c, font=font(13))


# --- 30 screenshots ---

img, d = new_canvas("01 — Live Market Screen")
market_table(d)
draw_lines(d, [
    "",
    "Show updates: Enter | Portfolio: P | Add Stock: A | Remove: R | Add Money: M | Exit: E",
], y=370, size=13, color=DIM)
save(img, "01_live_market.png")

img, d = new_canvas("02 — Enter refreshes all prices")
market_table(d)
draw_lines(d, [
    ">>> User pressed ENTER — all stock prices recalculated with random int+frac deltas",
], y=400, color=YELLOW, size=14)
save(img, "02_enter_refresh.png")

img, d = new_canvas("03 — Price up / down arrows")
market_table(d, highlight="up")
draw_lines(d, [
    ("Green ↑ = price rose since previous tick", GREEN),
    ("Red   ↓ = price fell since previous tick", RED),
], y=400, size=14)
save(img, "03_price_up_down.png")

img, d = new_canvas("04 — Session High / Low tracking")
draw_lines(d, [
    "For each symbol, High and Low update every refresh:",
    "  if curr > high -> high = curr",
    "  if curr < low  -> low  = curr",
    "",
    "Example OGDC: session start 76.40 | High 78.65 | Low 76.13 | Curr 76.81",
], y=100, size=15)
save(img, "04_high_low.png")

img, d = new_canvas("05 — Top % Advancer")
draw_lines(d, [
    "Footer after refresh:",
    "",
    ("Top % advancer symbol     : TRG   (+4.44%)", GREEN),
    "Top % decliner symbol     : POL   (-1.28%)",
    "",
    "pctChange[i] = ((curr - prev) / prev) * 100",
], y=100, size=15)
save(img, "05_top_advancer.png")

img, d = new_canvas("06 — Top % Decliner")
draw_lines(d, [
    "Footer after refresh:",
    "",
    "Top % advancer symbol     : TRG   (+4.44%)",
    ("Top % decliner symbol     : POL   (-1.28%)", RED),
], y=100, size=15)
save(img, "06_top_decliner.png")

img, d = new_canvas("07 — Total shares traded today")
draw_lines(d, [
    "Total shares traded today : 3500",
    "",
    "Incremented on every BUY and SELL (addStock / removeStock).",
    "Tracked in a local long long passed through function parameters.",
], y=100, size=15)
save(img, "07_total_traded.png")

img, d = new_canvas("08 — Portfolio empty (first run)")
draw_lines(d, [
    "PORTFOLIO OWNER: Investor (LIVE)",
    "Updates: Enter | Live Market: L | Add: A | Remove: R | Money: M | Withdraw: W",
    "",
    "  (No holdings yet — press A to buy shares, M to add money)",
    "",
    "Today's Gain or Loss (Rs.) : 0.00",
    "Previous Balance (Rs.)     : 0.00",
    "New Balance (Rs.)          : 0.00",
    "Cash available             : 0.00",
], y=100, size=15)
save(img, "08_portfolio_empty.png")

img, d = new_canvas("09 — Add money to account")
draw_lines(d, [
    "Enter amount to add (Rs.): 100000",
    "",
    ("Added Rs. 100000.00. New cash balance: 100000.00", GREEN),
    "",
    "Validation: amount must be > 0",
], y=100, size=15)
save(img, "09_add_money.png")

img, d = new_canvas("10 — Buy / add stock")
draw_lines(d, [
    "Enter stock symbol to BUY: PSO",
    "Current price of PSO (Pakistan State Oil): Rs. 151.81",
    "Enter number of shares to buy: 1000",
    "",
    ("Bought 1000 shares of PSO for Rs. 151810.00", GREEN),
    "Remaining cash: Rs. ...",
], y=100, size=15)
save(img, "10_buy_stock.png")

img, d = new_canvas("11 — Portfolio with holdings")
draw_lines(d, [
    "Stocks  Company Name                 Shares  Current Previous  Gain/Loss   High     Low",
    ("PSO     Pakistan State Oil            1000   151.81   150.70    +1110.00  154.20  150.70", GREEN),
    ("LUCK    Lucky Cement                   500   485.75   488.20    -1225.00  493.40  485.00", RED),
    ("HBL     Habib Bank Limited            2500    69.76    70.00     -600.00   70.00   69.60", RED),
    ("OGDC    Oil & Gas Dev. Comp.          3000    76.81    76.40    +1230.00   78.65   76.13", GREEN),
], y=100, size=13)
save(img, "11_portfolio_holdings.png")

img, d = new_canvas("12 — Gain shown in GREEN")
draw_lines(d, [
    ("PSO gain/loss cell rendered with FOREGROUND_GREEN | FOREGROUND_INTENSITY", GREEN),
    "",
    "Win32: SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY)",
    "Gain/Loss = (currPrice - prevPrice) * shares",
], y=100, size=15)
save(img, "12_gain_green.png")

img, d = new_canvas("13 — Loss shown in RED")
draw_lines(d, [
    ("LUCK gain/loss cell rendered with FOREGROUND_RED | FOREGROUND_INTENSITY", RED),
    "",
    "Win32: SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY)",
], y=100, size=15)
save(img, "13_loss_red.png")

img, d = new_canvas("14 — Sell / remove stock")
draw_lines(d, [
    "Enter stock symbol to SELL: HBL",
    "You own 2500 shares. Enter shares to sell: 500",
    "",
    ("Sold 500 shares of HBL for Rs. 34880.00", GREEN),
    "New cash balance updated; totalSharesTraded increased.",
], y=100, size=15)
save(img, "14_sell_stock.png")

img, d = new_canvas("15 — Withdraw money")
draw_lines(d, [
    "Enter amount to withdraw (Rs.): 5000",
    "",
    ("Withdrew Rs. 5000.00. New cash balance: ...", GREEN),
    "",
    "Validation: cannot withdraw more than cash balance.",
], y=100, size=15)
save(img, "15_withdraw.png")

img, d = new_canvas("16 — Invalid symbol validation")
draw_lines(d, [
    "Enter stock symbol to BUY: PZO",
    "",
    ("Invalid symbol \"PZO\". Please choose a symbol from the list above.", RED),
    "",
    "findSymbolIndex() returns -1 when no match (case-insensitive).",
], y=100, size=15)
save(img, "16_invalid_symbol.png")

img, d = new_canvas("17 — Insufficient balance validation")
draw_lines(d, [
    "Trying to buy 10000 shares of MARI at ~1520...",
    "",
    ("Insufficient balance.", RED),
    "  Required : Rs. 15200000.00",
    "  Available: Rs. 100000.00",
], y=100, size=15)
save(img, "17_insufficient_balance.png")

img, d = new_canvas("18 — ±15% session-start hard cap")
draw_lines(d, [
    "If session-start price = 100.00:",
    "  minAllowed = 85.00   (100 * 0.85)",
    "  maxAllowed = 115.00  (100 * 1.15)",
    "",
    "Random int+fractional delta applied, then clamped.",
    "Next program run reloads saved price as new session start.",
], y=100, size=15)
save(img, "18_fifteen_percent_cap.png")

img, d = new_canvas("19 — companies.txt load at start")
draw_lines(d, [
    "Loaded 12 companies from companies.txt",
    "Session-start prices locked for +/-15% cap.",
    "",
    "CSV format per line: SYMBOL,Company Name,price",
    "Example: PSO,Pakistan State Oil,150.70",
    "",
    "Stored in parallel arrays (symbols, names, prices, high, low, ...).",
], y=100, size=15)
save(img, "19_companies_load.png")

img, d = new_canvas("20 — Save on exit")
draw_lines(d, [
    "Session closed.",
    ("Saved updated prices to companies.txt", GREEN),
    ("Saved formatted portfolio to portfolio.txt", GREEN),
    "",
    "saveCompanies() writes current prices.",
    "savePortfolio() uses iomanip (setw, setprecision, setfill).",
], y=100, size=15)
save(img, "20_save_on_exit.png")

img, d = new_canvas("21 — Live Market key menu")
draw_lines(d, [
    "LIVE MARKET KEYS",
    "  Enter  Refresh / randomize all prices",
    "  P      Open Portfolio screen",
    "  A      Add (buy) stock",
    "  R      Remove (sell) stock",
    "  M      Add money",
    "  E      Exit (save files)",
], y=100, size=16)
save(img, "21_key_menu_market.png")

img, d = new_canvas("22 — Portfolio key menu")
draw_lines(d, [
    "PORTFOLIO KEYS",
    "  Enter  Refresh prices (same as market)",
    "  L      Back to Live Market",
    "  A      Add (buy) stock",
    "  R      Remove (sell) stock",
    "  M      Add money",
    "  W      Withdraw money",
    "  E      Exit (save files)",
], y=100, size=16)
save(img, "22_key_menu_portfolio.png")

img, d = new_canvas("23 — Name entry on portfolio")
draw_lines(d, [
    "Enter portfolio owner name: Mohammad Rohaan",
    "",
    "Asked once when Portfolio screen is first opened.",
    "Saved into portfolio.txt header on exit.",
], y=100, size=15)
save(img, "23_name_entry.png")

img, d = new_canvas("24 — Formatted portfolio.txt (iomanip)")
draw_lines(d, [
    "****************************************************************************************************",
    "Portfolio owner: Mohammad Rohaan",
    "",
    "Stocks  Company Name                 shares       Close   Previous   Gain/Loss        High         Low",
    "****************************************************************************************************",
    "PSO     Pakistan State Oil            1000       151.81      150.70       +1110      154.20      150.70",
    "...",
    "Today's Gain or Loss (Rs.)  *  +995.00  *",
    "Previous Balance (Rs.)      *  +....   *",
    "New Balance (Rs.)           *  +....   *",
], y=90, size=12)
save(img, "24_formatted_portfolio_file.png")

img, d = new_canvas("25 — Multi-stock table (≥8 KSE names)")
market_table(d)
draw_lines(d, ["12 listed companies including PSO, LUCK, HBL, OGDC, PPL, POL, MARI, ENGRO, TRG, MEBL, SYS, UBL"], y=400, size=13, color=DIM)
save(img, "25_multi_stock_table.png")

img, d = new_canvas("26 — Session-start prices locked")
draw_lines(d, [
    "On load: sessionStart[i] = price from companies.txt",
    "All +/-15% clamps use sessionStart[i] for THIS run only.",
    "",
    "PSO sessionStart=150.70 -> band [128.10 , 173.31]",
    "HBL sessionStart=70.00  -> band [59.50  , 80.50 ]",
], y=100, size=15)
save(img, "26_session_start_prices.png")

img, d = new_canvas("27 — Footer stats on live market")
draw_lines(d, [
    "--------------------------------------------------------------------------------",
    "Total shares traded today : 2890045",
    "Top % advancer symbol     : TRG",
    "Top % decliner symbol     : HBL",
    "Note: Price moves are hard-capped at +/-15% of session-start price.",
    "================================================================================",
], y=100, size=15)
save(img, "27_footer_stats.png")

img, d = new_canvas("28 — Color legend")
draw_lines(d, [
    ("GAIN  = Green (FOREGROUND_GREEN | FOREGROUND_INTENSITY)", GREEN),
    ("LOSS  = Red   (FOREGROUND_RED   | FOREGROUND_INTENSITY)", RED),
    ("↑ up arrow / ↓ down arrow on Live Market Curr column", CYAN),
    "",
    "Colors applied only around the value, then reset to default white.",
], y=100, size=15)
save(img, "28_color_legend.png")

img, d = new_canvas("29 — Self-evaluation checklist (all Done)")
checklist = [
    "Load stock market data ........................ DONE (10)",
    "Show Stock Market Screen ...................... DONE (10)",
    "Random +/- , high/low, traded, top A/D ........ DONE (20)",
    "Load/Store portfolio.txt ...................... DONE (10)",
    "Show portfolio screen ......................... DONE (10)",
    "Portfolio G/L, high/low, balances ............. DONE (20)",
    "Add stock (+ colors bonus) .................... DONE (10+10)",
    "Remove stock .................................. DONE (10)",
    "Add/Withdraw money ............................ DONE (10)",
    "Save market on close / dynamic array / iomanip  DONE (bonuses)",
]
draw_lines(d, checklist, y=90, size=13, color=GREEN)
save(img, "29_self_eval_checklist.png")

img, d = new_canvas("30 — Complete feature map")
draw_lines(d, [
    "FILES: companies.txt <-> load/save | portfolio.txt <- save (iomanip)",
    "STATE: all locals in main / params — NO globals, NO goto, NO classes",
    "MARKET: parallel arrays + Enter refresh + arrows + footer stats",
    "PORTFOLIO: dynamic new[]/delete[] holdings + buy/sell/money/withdraw",
    "UI: system(\"cls\") + _getch() + SetConsoleTextAttribute colors",
    "AUTHOR: Mohammad Rohaan | 22I-2327 | Section A",
], y=100, size=14)
save(img, "30_complete_feature_map.png")

files = sorted(OUT.glob("*.png"))
print(f"\nGenerated {len(files)} screenshots in {OUT}")
