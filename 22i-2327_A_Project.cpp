/*
 * Name       : Mohammad Rohaan
 * Roll No.   : 22I-2327
 * Section    : A
 * Course     : Programming Fundamentals - Final Project
 * Title      : Karachi Stock Market Trading Simulator
 *
 * Restrictions followed:
 *  - No global variables
 *  - No goto
 *  - No OOP / classes (parallel arrays + functions only)
 */

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <cctype>
#include <conio.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif

using namespace std;

#define MAX_COMPANIES 50
#define SYM_LEN 16
#define NAME_LEN 64
#define OWNER_LEN 64

/* Market board — widths fit FULL header names (no clipped letters). */
#define COL_GAP 2
#define MKT_SYM 8
#define MKT_NAME 28
#define MKT_NUM 14
#define MKT_CHG 6
#define MKT_COLS 7
#define MKT_LINE_WIDTH (MKT_SYM + MKT_NAME + MKT_NUM * 4 + MKT_CHG + COL_GAP * (MKT_COLS - 1))
/* Buffer must stay wider than Live Market row or columns wrap to next line. */
#define MIN_CONSOLE_COLS ((MKT_LINE_WIDTH) + 8)

/* Portfolio console table (compact). File uses separate block format. */
#define PF_GAP 1
#define PF_SYM 6
#define PF_NAME 24
#define PF_QTY 6
#define PF_NUM 9
#define PORTFOLIO_COLS 8
#define PORTFOLIO_LINE_WIDTH (PF_SYM + PF_NAME + PF_QTY + PF_NUM * 5 + PF_GAP * (PORTFOLIO_COLS - 1))
#define PF_LABEL_W 20
#define PF_FILE_WIDTH 60

/* ---------- Console helpers (state passed in via HANDLE) ---------- */

void setColor(HANDLE hConsole, WORD attributes);
void resetColor(HANDLE hConsole);
void setAccentCyan(HANDLE hConsole);
void setAccentYellow(HANDLE hConsole);
void setFontBold(HANDLE hConsole, int height = 24);
void clearScreen();
void ensureWideScrollableBuffer(HANDLE hOut);
void snapConsoleToFontGrid(HANDLE hOut);
void fillRestOfWindowBlank(HANDLE hOut);
void configureConsoleIO();
void prepareTextInput();
void printRuleLine(ostream &out, char fill, int width);

void snapConsoleToFontGrid(HANDLE hOut)
{
    /*
     * Bottom "reflection" = Windows showing a PARTIAL character row
     * when client height (pixels) is not an exact multiple of font height.
     * Trim leftover pixels so only full rows are visible.
     */
    HWND hwnd = GetConsoleWindow();
    if (hwnd == NULL || hOut == NULL || hOut == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_FONT_INFOEX fi;
    ZeroMemory(&fi, sizeof(fi));
    fi.cbSize = sizeof(fi);
    if (!GetCurrentConsoleFontEx(hOut, FALSE, &fi))
        return;

    int fontH = fi.dwFontSize.Y;
    int fontW = fi.dwFontSize.X;
    if (fontH <= 0)
        return;
    if (fontW <= 0)
        fontW = (fontH > 1) ? (fontH / 2) : 8;
    if (fontW <= 0)
        fontW = 8;

    RECT rcClient;
    RECT rcWindow;
    if (!GetClientRect(hwnd, &rcClient) || !GetWindowRect(hwnd, &rcWindow))
        return;

    int clientW = rcClient.right - rcClient.left;
    int clientH = rcClient.bottom - rcClient.top;
    if (clientW <= 0 || clientH <= 0)
        return;

    int trimW = clientW % fontW;
    int trimH = clientH % fontH;
    if (trimW == 0 && trimH == 0)
        return;

    int newOuterW = (rcWindow.right - rcWindow.left) - trimW;
    int newOuterH = (rcWindow.bottom - rcWindow.top) - trimH;
    if (newOuterW < 200)
        newOuterW = rcWindow.right - rcWindow.left;
    if (newOuterH < 200)
        newOuterH = rcWindow.bottom - rcWindow.top;

    SetWindowPos(hwnd, NULL, rcWindow.left, rcWindow.top, newOuterW, newOuterH,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void fillRestOfWindowBlank(HANDLE hOut)
{
    /*
     * Blank every cell below the prompt (and one row past the window).
     * Uses black-on-black so a partial pixel row cannot show yellow glyph tops.
     * Does NOT erase the prompt on the current line.
     */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;

    SHORT left = csbi.srWindow.Left;
    SHORT right = csbi.srWindow.Right;
    SHORT bottom = csbi.srWindow.Bottom;
    SHORT curX = csbi.dwCursorPosition.X;
    SHORT curY = csbi.dwCursorPosition.Y;
    SHORT width = static_cast<SHORT>(right - left + 1);
    if (width < 1)
        return;

    WORD blankAttr = 0x00;
    DWORD written = 0;

    if (curY >= csbi.srWindow.Top && curY <= bottom && curX <= right)
    {
        COORD pos = { curX, curY };
        DWORD n = static_cast<DWORD>(right - curX + 1);
        FillConsoleOutputCharacterA(hOut, ' ', n, pos, &written);
        FillConsoleOutputAttribute(hOut, blankAttr, n, pos, &written);
    }

    for (SHORT y = static_cast<SHORT>(curY + 1); y <= bottom; y++)
    {
        COORD pos = { left, y };
        FillConsoleOutputCharacterA(hOut, ' ', (DWORD)width, pos, &written);
        FillConsoleOutputAttribute(hOut, blankAttr, (DWORD)width, pos, &written);
    }

    if (bottom + 1 < csbi.dwSize.Y)
    {
        COORD pos = { left, static_cast<SHORT>(bottom + 1) };
        FillConsoleOutputCharacterA(hOut, ' ', (DWORD)width, pos, &written);
        FillConsoleOutputAttribute(hOut, blankAttr, (DWORD)width, pos, &written);
    }

    SetConsoleTextAttribute(hOut,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    COORD restore = { curX, curY };
    SetConsoleCursorPosition(hOut, restore);
}

void ensureWideScrollableBuffer(HANDLE hOut)
{
    if (hOut == NULL || hOut == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;

    SHORT winW = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    SHORT winH = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    if (winW < 80) winW = 80;
    if (winH < 25) winH = 25;

    /* Wide enough for Live Market row; tall enough to scroll, not huge. */
    SHORT bufW = winW;
    if (bufW < (SHORT)MIN_CONSOLE_COLS)
        bufW = (SHORT)MIN_CONSOLE_COLS;

    SHORT bufH = static_cast<SHORT>(winH * 8);
    if (bufH < 400)
        bufH = 400;
    if (bufH > 2000)
        bufH = 2000;

    COORD bufferSize = { bufW, bufH };
    SetConsoleScreenBufferSize(hOut, bufferSize);

    SHORT viewW = winW;
    if (viewW > bufW)
        viewW = bufW;
    SHORT viewH = winH;
    if (viewH > bufH)
        viewH = bufH;

    SMALL_RECT view = { 0, 0, static_cast<SHORT>(viewW - 1), static_cast<SHORT>(viewH - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &view);
}

void configureConsoleIO()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    DWORD outMode = 0;
    if (GetConsoleMode(hOut, &outMode))
    {
        outMode |= ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(hOut, outMode);
    }

    DWORD inMode = 0;
    if (GetConsoleMode(hIn, &inMode))
    {
        inMode |= ENABLE_EXTENDED_FLAGS;
        inMode &= ~ENABLE_QUICK_EDIT_MODE; /* selection ghosts while typing */
        inMode |= ENABLE_PROCESSED_INPUT;
        inMode |= ENABLE_LINE_INPUT;
        inMode |= ENABLE_ECHO_INPUT;
        SetConsoleMode(hIn, inMode);
    }

    ios::sync_with_stdio(true);
    cout.flush();
    cin.clear();
}

void clearScreen()
{
    /*
     * Permanent, safe clear (no new problems):
     *  - Does NOT create a new console buffer (that caused blank screen)
     *  - Does NOT shrink buffer to window height (that locked scrollbar)
     *  - Does NOT freopen CONOUT$ (that wrapped Live Market to ~80 cols)
     *  - Scrolls ALL old text (including typed echo) off the buffer, then homes cursor
     */
    cout.flush();
    fflush(stdout);
    cin.clear();

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    FlushConsoleInputBuffer(hIn);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
    {
        system("cls");
        return;
    }

    WORD attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    CHAR_INFO fill;
    fill.Char.AsciiChar = ' ';
    fill.Attributes = attr;

    /* Push every character off the buffer — removes typed-text reflection permanently. */
    SMALL_RECT scrollRect;
    scrollRect.Left = 0;
    scrollRect.Top = 0;
    scrollRect.Right = static_cast<SHORT>(csbi.dwSize.X - 1);
    scrollRect.Bottom = static_cast<SHORT>(csbi.dwSize.Y - 1);
    COORD dest = { 0, static_cast<SHORT>(-csbi.dwSize.Y) };
    ScrollConsoleScreenBufferA(hOut, &scrollRect, NULL, dest, &fill);

    /* Extra fill for anything left at the top. */
    COORD origin = { 0, 0 };
    DWORD cells = static_cast<DWORD>(csbi.dwSize.X) * static_cast<DWORD>(csbi.dwSize.Y);
    DWORD written = 0;
    FillConsoleOutputCharacterA(hOut, ' ', cells, origin, &written);
    FillConsoleOutputAttribute(hOut, attr, cells, origin, &written);

    SetConsoleTextAttribute(hOut, attr);
    SetConsoleCursorPosition(hOut, origin);

    /* Keep viewport at top; buffer height unchanged => scrollbar still works. */
    SHORT winW = static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    SHORT winH = static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    if (winW < 1) winW = 80;
    if (winH < 1) winH = 25;
    if (winW > csbi.dwSize.X) winW = csbi.dwSize.X;
    if (winH > csbi.dwSize.Y) winH = csbi.dwSize.Y;
    SMALL_RECT topView = { 0, 0, static_cast<SHORT>(winW - 1), static_cast<SHORT>(winH - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &topView);
}

void showErrorMessage(HANDLE hConsole, const string &message)
{
    clearScreen();
    setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    cout << "\n\n";
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    cout << "  ERROR\n";
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    resetColor(hConsole);
    cout << "\n" << message << "\n";
    setAccentYellow(hConsole);
    cout << "\nPress any key to continue...";
    resetColor(hConsole);
    cout.flush();
    _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    clearScreen();
}

void showInfoMessage(HANDLE hConsole, const string &message)
{
    clearScreen();
    setAccentCyan(hConsole);
    cout << "\n\n";
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    cout << "  NOTICE\n";
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    resetColor(hConsole);
    cout << "\n" << message << "\n";
    setAccentYellow(hConsole);
    cout << "\nPress any key to continue...";
    resetColor(hConsole);
    cout.flush();
    _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    clearScreen();
}

void setColor(HANDLE hConsole, WORD attributes)
{
    HANDLE h = (hConsole != NULL && hConsole != INVALID_HANDLE_VALUE)
                   ? hConsole
                   : GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, static_cast<WORD>(attributes & 0x0F));
}

void resetColor(HANDLE hConsole)
{
    HANDLE h = (hConsole != NULL && hConsole != INVALID_HANDLE_VALUE)
                   ? hConsole
                   : GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}

void setAccentCyan(HANDLE hConsole)
{
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}

void setAccentYellow(HANDLE hConsole)
{
    setColor(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void printGainLossColored(HANDLE hConsole, double value)
{
    if (value > 0.0001)
    {
        setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        cout << "+" << fixed << setprecision(2) << value;
    }
    else if (value < -0.0001)
    {
        setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        cout << fixed << setprecision(2) << value;
    }
    else
    {
        resetColor(hConsole);
        cout << fixed << setprecision(2) << value;
    }
    resetColor(hConsole);
}

void setFontBold(HANDLE hConsole, int height)
{
    CONSOLE_FONT_INFOEX fontInfo;
    fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    GetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
    fontInfo.dwFontSize.X = 0;
    fontInfo.dwFontSize.Y = static_cast<SHORT>(height);
    fontInfo.FontWeight = FW_BOLD;
    wcscpy_s(fontInfo.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
}

void maximizeConsoleWindow()
{
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL)
    {
        ShowWindow(hwnd, SW_MAXIMIZE);
        Sleep(80); /* let maximize apply before font/buffer layout */
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ensureWideScrollableBuffer(hOut);
    snapConsoleToFontGrid(hOut);
    ensureWideScrollableBuffer(hOut); /* refresh cols/rows after pixel snap */
}

void enableDarkTheme(HANDLE hConsole)
{
    /* Black background + bright white default (cmd color table) */
    system("color 0F");
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    configureConsoleIO();
    setFontBold(hConsole, 22); /* slightly smaller than 24 — cleaner row fit */
    snapConsoleToFontGrid(hConsole);
    ensureWideScrollableBuffer(hConsole);
    resetColor(hConsole);
}

void enableUtf8Console()
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
}

/* ---------- Fixed-width text helpers (exact column alignment) ---------- */

string fitWidth(const char *s, int width)
{
    string t = (s == NULL) ? "" : s;
    if (width < 1)
        return "";
    if ((int)t.size() > width)
        t = t.substr(0, (size_t)width);
    return t;
}

/** Exact left field (pad right with spaces) — avoids iostream sticky-flag misalignment. */
string cellLeft(const char *s, int width)
{
    string t = fitWidth(s, width);
    if ((int)t.size() < width)
        t.append(static_cast<size_t>(width - (int)t.size()), ' ');
    return t;
}

/** Exact right field (pad left with spaces). */
string cellRight(const string &text, int width)
{
    string t = text;
    if ((int)t.size() > width)
        t = t.substr((size_t)((int)t.size() - width)); /* keep rightmost digits */
    if ((int)t.size() < width)
        t.insert(0, static_cast<size_t>(width - (int)t.size()), ' ');
    return t;
}

string formatMoneyText(double value, bool showPlus)
{
    ostringstream cell;
    cell << fixed << setprecision(2);
    if (showPlus && value > 0.0001)
        cell << "+";
    cell << value;
    return cell.str();
}

void printCellLeft(ostream &out, const char *s, int width)
{
    out << cellLeft(s, width);
}

void printCellRightText(ostream &out, const string &text, int width)
{
    out << cellRight(text, width);
}

void printColGap(ostream &out)
{
    out << string(static_cast<size_t>(COL_GAP), ' ');
}

void printRuleLine(ostream &out, char fill, int width)
{
    if (width < 8)
        width = 8;
    out << string(static_cast<size_t>(width), fill) << "\n";
}

void printMoneyCell(ostream &out, double value, int width, bool showPlus)
{
    out << cellRight(formatMoneyText(value, showPlus), width);
}

void printGainLossFixed(HANDLE hConsole, double value, int width)
{
    string t = cellRight(formatMoneyText(value, true), width);

    if (value > 0.0001)
        setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else if (value < -0.0001)
        setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    else
        resetColor(hConsole);
    cout << t;
    resetColor(hConsole);
}

/** label : value for summary lines only. */
void writeLabeled(ostream &out, const char *label, const string &value)
{
    out << left << setw(PF_LABEL_W) << label << " : " << value << "\n";
    out.unsetf(ios_base::adjustfield);
}

void writeLabeledMoney(ostream &out, const char *label, double value, bool showPlus)
{
    writeLabeled(out, label, formatMoneyText(value, showPlus));
}

void writePortfolioFooterLine(ostream &out, const char *label, double value, bool showPlus)
{
    writeLabeledMoney(out, label, value, showPlus);
}

string buildPortfolioHeaderLine()
{
    string gap(static_cast<size_t>(PF_GAP), ' ');
    return cellLeft("Sym", PF_SYM) + gap +
           cellLeft("Company", PF_NAME) + gap +
           cellRight("Qty", PF_QTY) + gap +
           cellRight("Curr", PF_NUM) + gap +
           cellRight("Prev", PF_NUM) + gap +
           cellRight("G/L", PF_NUM) + gap +
           cellRight("High", PF_NUM) + gap +
           cellRight("Low", PF_NUM);
}

string buildPortfolioDataLine(const char *symbol,
                              const char *companyName,
                              int shares,
                              double closePrice,
                              double previousPrice,
                              double gainLoss,
                              double high,
                              double low)
{
    ostringstream sh;
    sh << shares;
    string gap(static_cast<size_t>(PF_GAP), ' ');
    return cellLeft(symbol, PF_SYM) + gap +
           cellLeft(companyName, PF_NAME) + gap +
           cellRight(sh.str(), PF_QTY) + gap +
           cellRight(formatMoneyText(closePrice, false), PF_NUM) + gap +
           cellRight(formatMoneyText(previousPrice, false), PF_NUM) + gap +
           cellRight(formatMoneyText(gainLoss, true), PF_NUM) + gap +
           cellRight(formatMoneyText(high, false), PF_NUM) + gap +
           cellRight(formatMoneyText(low, false), PF_NUM);
}

void printPortfolioDataRowColored(HANDLE hConsole,
                                  const char *symbol,
                                  const char *companyName,
                                  int shares,
                                  double closePrice,
                                  double previousPrice,
                                  double gainLoss,
                                  double high,
                                  double low)
{
    /* unused table path kept for compatibility — console uses blocks */
    (void)hConsole;
    (void)symbol;
    (void)companyName;
    (void)shares;
    (void)closePrice;
    (void)previousPrice;
    (void)gainLoss;
    (void)high;
    (void)low;
}

void printHoldingBlockColored(HANDLE hConsole,
                              int index,
                              const char *symbol,
                              const char *companyName,
                              int shares,
                              double closePrice,
                              double previousPrice,
                              double gainLoss,
                              double high,
                              double low)
{
    cout << "\n";
    printRuleLine(cout, '-', PF_FILE_WIDTH);
    setAccentCyan(hConsole);
    cout << "  HOLDING " << index << "\n";
    resetColor(hConsole);
    printRuleLine(cout, '-', PF_FILE_WIDTH);
    cout << "\n";

    writeLabeled(cout, "Symbol", symbol);
    writeLabeled(cout, "Company Name", companyName);
    {
        ostringstream sh;
        sh << shares;
        writeLabeled(cout, "Shares Owned", sh.str());
    }
    writeLabeledMoney(cout, "Current Price", closePrice, false);
    writeLabeledMoney(cout, "Previous Price", previousPrice, false);

    cout << left << setw(PF_LABEL_W) << "Gain / Loss" << " : ";
    cout.unsetf(ios_base::adjustfield);
    if (gainLoss > 0.0001)
        setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else if (gainLoss < -0.0001)
        setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    else
        resetColor(hConsole);
    cout << formatMoneyText(gainLoss, true) << "\n";
    resetColor(hConsole);

    writeLabeledMoney(cout, "Highest Price", high, false);
    writeLabeledMoney(cout, "Lowest Price", low, false);
    cout << "\n";
}

/* Direction with arrow head + shaft: ↑ up, ↓ down, = unchanged. */
void printChangeArrow(HANDLE hConsole, double prev, double curr)
{
    const char *mark;
    if (curr > prev + 0.0001)
    {
        setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        mark = "\xE2\x86\x91"; /* UTF-8 ↑ */
    }
    else if (curr < prev - 0.0001)
    {
        setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        mark = "\xE2\x86\x93"; /* UTF-8 ↓ */
    }
    else
    {
        setAccentYellow(hConsole);
        mark = "=";
    }
    /* Pad in a fixed field; arrow glyph is one display cell. */
    int pad = MKT_CHG - 1;
    if (pad < 0)
        pad = 0;
    cout << string(static_cast<size_t>(pad), ' ') << mark;
    resetColor(hConsole);
}

bool isCancelToken(const char *s)
{
    if (s == NULL || s[0] == '\0')
        return false;
    if (strcmp(s, "0") == 0)
        return true;
    if (_stricmp(s, "back") == 0 || _stricmp(s, "b") == 0 || _stricmp(s, "cancel") == 0)
        return true;
    return false;
}

/* ---------- File / market data ---------- */

int loadCompanies(const char *filename,
                  char symbols[][SYM_LEN],
                  char names[][NAME_LEN],
                  double sessionStart[],
                  double prevPrice[],
                  double currPrice[],
                  double highPrice[],
                  double lowPrice[],
                  double pctChange[])
{
    ifstream fin(filename);
    if (!fin.is_open())
    {
        cout << "Error: cannot open " << filename << endl;
        return 0;
    }

    int count = 0;
    string line;
    while (getline(fin, line) && count < MAX_COMPANIES)
    {
        if (line.empty())
            continue;

        stringstream ss(line);
        string sym, name, priceStr;
        if (!getline(ss, sym, ','))
            continue;
        if (!getline(ss, name, ','))
            continue;
        if (!getline(ss, priceStr, ','))
            continue;

        double price = atof(priceStr.c_str());
        strncpy(symbols[count], sym.c_str(), SYM_LEN - 1);
        symbols[count][SYM_LEN - 1] = '\0';
        strncpy(names[count], name.c_str(), NAME_LEN - 1);
        names[count][NAME_LEN - 1] = '\0';

        sessionStart[count] = price;
        prevPrice[count] = price;
        currPrice[count] = price;
        highPrice[count] = price;
        lowPrice[count] = price;
        pctChange[count] = 0.0;
        count++;
    }
    fin.close();
    return count;
}

bool saveCompanies(const char *filename,
                   char symbols[][SYM_LEN],
                   char names[][NAME_LEN],
                   double currPrice[],
                   int companyCount)
{
    ofstream fout(filename);
    if (!fout.is_open())
        return false;

    /* CSV required by brief — keep fields tidy: no extra spaces, price always 2 dp */
    fout << fixed << setprecision(2);
    for (int i = 0; i < companyCount; i++)
    {
        fout << symbols[i] << "," << names[i] << "," << currPrice[i] << "\n";
    }
    fout.close();
    return true;
}

int findSymbolIndex(char symbols[][SYM_LEN], int companyCount, const char *query)
{
    for (int i = 0; i < companyCount; i++)
    {
        if (_stricmp(symbols[i], query) == 0)
            return i;
    }
    return -1;
}

/* Random price update: integer + fractional change, hard-capped at +/-15% of session start.
 * Guarantees every stock moves at least 0.01 when the band allows (so arrows always appear). */
void refreshAllPrices(double sessionStart[],
                      double prevPrice[],
                      double currPrice[],
                      double highPrice[],
                      double lowPrice[],
                      double pctChange[],
                      int companyCount)
{
    for (int i = 0; i < companyCount; i++)
    {
        prevPrice[i] = currPrice[i];

        int intDelta = (rand() % 7) - 3;           /* -3 .. +3 */
        double fracDelta = (rand() % 100) / 100.0; /* 0.00 .. 0.99 */
        if (rand() % 2 == 0)
            fracDelta = -fracDelta;

        double candidate = currPrice[i] + intDelta + fracDelta;

        double minAllowed = sessionStart[i] * 0.85;
        double maxAllowed = sessionStart[i] * 1.15;
        if (candidate > maxAllowed)
            candidate = maxAllowed;
        if (candidate < minAllowed)
            candidate = minAllowed;
        if (candidate < 0.01)
            candidate = 0.01;

        /* Force a visible tick when random move collapsed to "unchanged" */
        if (fabs(candidate - prevPrice[i]) < 0.005)
        {
            if (prevPrice[i] + 0.01 <= maxAllowed + 0.0001)
                candidate = prevPrice[i] + 0.01;
            else if (prevPrice[i] - 0.01 >= minAllowed - 0.0001)
                candidate = prevPrice[i] - 0.01;
        }

        currPrice[i] = candidate;

        if (currPrice[i] > highPrice[i])
            highPrice[i] = currPrice[i];
        if (currPrice[i] < lowPrice[i])
            lowPrice[i] = currPrice[i];

        if (prevPrice[i] > 0.0001)
            pctChange[i] = ((currPrice[i] - prevPrice[i]) / prevPrice[i]) * 100.0;
        else
            pctChange[i] = 0.0;
    }
}

void findTopAdvancerDecliner(double pctChange[],
                             int companyCount,
                             int &advIndex,
                             int &decIndex)
{
    advIndex = 0;
    decIndex = 0;
    for (int i = 1; i < companyCount; i++)
    {
        if (pctChange[i] > pctChange[advIndex])
            advIndex = i;
        if (pctChange[i] < pctChange[decIndex])
            decIndex = i;
    }
}

/* ---------- Dynamic holdings list (bonus) ---------- */

void expandHoldings(char **&holdSymbols,
                    int *&holdShares,
                    int &capacity)
{
    int newCap = (capacity == 0) ? 4 : capacity * 2;
    char **newSym = new char *[newCap];
    int *newShares = new int[newCap];

    for (int i = 0; i < newCap; i++)
    {
        newSym[i] = new char[SYM_LEN];
        newSym[i][0] = '\0';
        newShares[i] = 0;
    }

    for (int i = 0; i < capacity; i++)
    {
        strncpy(newSym[i], holdSymbols[i], SYM_LEN - 1);
        newSym[i][SYM_LEN - 1] = '\0';
        newShares[i] = holdShares[i];
        delete[] holdSymbols[i];
    }

    delete[] holdSymbols;
    delete[] holdShares;

    holdSymbols = newSym;
    holdShares = newShares;
    capacity = newCap;
}

void freeHoldings(char **&holdSymbols, int *&holdShares, int capacity)
{
    if (holdSymbols != NULL)
    {
        for (int i = 0; i < capacity; i++)
            delete[] holdSymbols[i];
        delete[] holdSymbols;
        holdSymbols = NULL;
    }
    if (holdShares != NULL)
    {
        delete[] holdShares;
        holdShares = NULL;
    }
}

int findHoldingIndex(char **holdSymbols, int holdCount, const char *symbol)
{
    for (int i = 0; i < holdCount; i++)
    {
        if (_stricmp(holdSymbols[i], symbol) == 0)
            return i;
    }
    return -1;
}

/* ---------- Screens ---------- */

void drawLiveMarket(HANDLE hConsole,
                    char symbols[][SYM_LEN],
                    char names[][NAME_LEN],
                    double prevPrice[],
                    double currPrice[],
                    double highPrice[],
                    double lowPrice[],
                    double pctChange[],
                    int companyCount,
                    long long totalSharesTraded)
{
    clearScreen();
    ensureWideScrollableBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
    setAccentCyan(hConsole);
    printRuleLine(cout, '=', MKT_LINE_WIDTH);
    cout << "                        KARACHI STOCK MARKET (LIVE)\n";
    printRuleLine(cout, '=', MKT_LINE_WIDTH);
    setAccentYellow(hConsole);
    cout << "Show updates: Enter   | Portfolio: P | Add Stock: A | Remove: R | Add Money: M | Exit: E\n";
    cout << "Tip: Press P first to create/open Portfolio. Without portfolio file, A/R/M are blocked.\n";
    cout << "Change: \"\xE2\x86\x91\" = up (rise)   \"\xE2\x86\x93\" = down (fall)   \"=\" = unchanged\n";
    resetColor(hConsole);
    cout << "\n";
    printRuleLine(cout, '-', MKT_LINE_WIDTH);

    setAccentCyan(hConsole);
    /* Headers left-aligned so full names stay visible (not right-truncated). */
    printCellLeft(cout, "Symbol", MKT_SYM);
    printColGap(cout);
    printCellLeft(cout, "Company Name", MKT_NAME);
    printColGap(cout);
    printCellLeft(cout, "Previous Price", MKT_NUM);
    printColGap(cout);
    printCellLeft(cout, "Current Price", MKT_NUM);
    printColGap(cout);
    printCellLeft(cout, "Change", MKT_CHG);
    printColGap(cout);
    printCellLeft(cout, "Highest Price", MKT_NUM);
    printColGap(cout);
    printCellLeft(cout, "Lowest Price", MKT_NUM);
    cout << endl;
    resetColor(hConsole);
    printRuleLine(cout, '-', MKT_LINE_WIDTH);
    cout << "\n";

    for (int i = 0; i < companyCount; i++)
    {
        printCellLeft(cout, symbols[i], MKT_SYM);
        printColGap(cout);
        printCellLeft(cout, names[i], MKT_NAME);
        printColGap(cout);
        printMoneyCell(cout, prevPrice[i], MKT_NUM, false);
        printColGap(cout);
        printMoneyCell(cout, currPrice[i], MKT_NUM, false);
        printColGap(cout);
        printChangeArrow(hConsole, prevPrice[i], currPrice[i]);
        printColGap(cout);
        printMoneyCell(cout, highPrice[i], MKT_NUM, false);
        printColGap(cout);
        printMoneyCell(cout, lowPrice[i], MKT_NUM, false);
        cout << endl;
    }

    int adv = 0, dec = 0;
    findTopAdvancerDecliner(pctChange, companyCount, adv, dec);

    printRuleLine(cout, '-', MKT_LINE_WIDTH);
    setAccentYellow(hConsole);
    cout << "Total shares traded today : " << totalSharesTraded << endl;
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << "Top % advancer symbol     : " << symbols[adv]
         << "  (" << fixed << setprecision(2) << pctChange[adv] << "%)" << endl;
    setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    cout << "Top % decliner symbol     : " << symbols[dec]
         << "  (" << fixed << setprecision(2) << pctChange[dec] << "%)" << endl;
    resetColor(hConsole);
    cout << "Note: Price moves are hard-capped at +/-15% of session-start price.\n";
    setAccentCyan(hConsole);
    printRuleLine(cout, '=', MKT_LINE_WIDTH);
    resetColor(hConsole);
    setAccentYellow(hConsole);
    cout << "Enter your choice: ";
    resetColor(hConsole);
    cout.flush();
    fillRestOfWindowBlank(GetStdHandle(STD_OUTPUT_HANDLE));
    snapConsoleToFontGrid(GetStdHandle(STD_OUTPUT_HANDLE));
}

void computePortfolioTotals(char **holdSymbols,
                            int *holdShares,
                            int holdCount,
                            char symbols[][SYM_LEN],
                            double prevPrice[],
                            double currPrice[],
                            int companyCount,
                            double &todayGL)
{
    todayGL = 0.0;
    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;
        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];
        todayGL += gl;
    }
}

void drawPortfolio(HANDLE hConsole,
                   const char *ownerName,
                   double balance,
                   char **holdSymbols,
                   int *holdShares,
                   int holdCount,
                   char symbols[][SYM_LEN],
                   char names[][NAME_LEN],
                   double prevPrice[],
                   double currPrice[],
                   double highPrice[],
                   double lowPrice[],
                   int companyCount)
{
    clearScreen();
    setAccentCyan(hConsole);
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    cout << "  PORTFOLIO (LIVE)\n";
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    resetColor(hConsole);
    cout << "\n";
    writeLabeled(cout, "Owner Name", ownerName);
    writeLabeledMoney(cout, "Cash Available (Rs.)", balance, false);
    cout << "\n";
    setAccentYellow(hConsole);
    cout << "Enter = refresh prices\n";
    cout << "L = Live Market   A = Buy   R = Sell\n";
    cout << "M = Add money     W = Withdraw money\n";
    resetColor(hConsole);

    double todayGL = 0.0;
    int shown = 0;

    if (holdCount == 0)
    {
        cout << "\n";
        printRuleLine(cout, '-', PF_FILE_WIDTH);
        cout << "\n  No shares owned yet.\n\n";
        cout << "  Step 1: Press M and add money\n";
        cout << "  Step 2: Press A and buy shares\n\n";
        printRuleLine(cout, '-', PF_FILE_WIDTH);
    }

    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;

        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];
        todayGL += gl;
        shown++;
        printHoldingBlockColored(hConsole, shown, symbols[idx], names[idx],
                                 holdShares[h], currPrice[idx], prevPrice[idx],
                                 gl, highPrice[idx], lowPrice[idx]);
    }

    double previousBalance = balance;
    double newBalance = balance + todayGL;

    printRuleLine(cout, '=', PF_FILE_WIDTH);
    setAccentCyan(hConsole);
    cout << "  SUMMARY\n";
    resetColor(hConsole);
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    cout << "\n";

    cout << left << setw(PF_LABEL_W) << "Today's Gain or Loss" << " : ";
    cout.unsetf(ios_base::adjustfield);
    printGainLossFixed(hConsole, todayGL, 14);
    cout << endl;
    resetColor(hConsole);
    writePortfolioFooterLine(cout, "Previous Balance", previousBalance, false);
    setAccentYellow(hConsole);
    writePortfolioFooterLine(cout, "New Balance", newBalance, false);
    resetColor(hConsole);
    writePortfolioFooterLine(cout, "Cash Available", balance, false);
    cout << "\n";
    setAccentCyan(hConsole);
    printRuleLine(cout, '=', PF_FILE_WIDTH);
    resetColor(hConsole);
    setAccentYellow(hConsole);
    cout << "Enter your choice: ";
    resetColor(hConsole);
    cout.flush();
    fillRestOfWindowBlank(GetStdHandle(STD_OUTPUT_HANDLE));
    snapConsoleToFontGrid(GetStdHandle(STD_OUTPUT_HANDLE));
}

/* ---------- Transactions ---------- */

void addMoney(double &balance)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    clearScreen();
    prepareTextInput();
    cout << "----- ADD MONEY -----\n\n";
    cout << "Enter the amount to add in Rs.\n";
    cout << "Enter 0 to cancel and go back.\n\n";
    cout << "Amount: ";
    double amount;
    cin >> amount;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        showErrorMessage(hConsole, "Invalid amount.\nPlease enter a valid number (example: 50000).\nEnter 0 if you want to cancel.");
        return;
    }
    cin.ignore(10000, '\n');
    if (amount == 0)
    {
        showInfoMessage(hConsole, "Add money cancelled.\nNo change was made to your balance.");
        return;
    }
    if (amount < 0)
    {
        showErrorMessage(hConsole, "Invalid amount.\nAmount cannot be negative.\nPlease try again with a positive value.");
        return;
    }
    balance += amount;
    clearScreen();
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << "\n\n[OK] Added Rs. " << fixed << setprecision(2) << amount << "\n";
    resetColor(hConsole);
    cout << "New cash balance: Rs. " << balance << endl;
    setAccentYellow(hConsole);
    cout << "\nPress any key to continue...";
    resetColor(hConsole);
    _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    clearScreen();
}

void withdrawMoney(double &balance)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    clearScreen();
    prepareTextInput();
    cout << "----- WITHDRAW MONEY -----\n\n";
    cout << "Enter the amount to withdraw in Rs.\n";
    cout << "Enter 0 to cancel and go back.\n\n";
    cout << "Amount: ";
    double amount;
    cin >> amount;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        showErrorMessage(hConsole, "Invalid amount.\nPlease enter a valid number (example: 1000).\nEnter 0 if you want to cancel.");
        return;
    }
    cin.ignore(10000, '\n');
    if (amount == 0)
    {
        showInfoMessage(hConsole, "Withdraw cancelled.\nNo change was made to your balance.");
        return;
    }
    if (amount < 0)
    {
        showErrorMessage(hConsole, "Invalid amount.\nAmount cannot be negative.");
        return;
    }
    if (amount > balance)
    {
        ostringstream msg;
        msg << fixed << setprecision(2)
            << "Insufficient balance.\n"
            << "You tried to withdraw: Rs. " << amount << "\n"
            << "Available cash: Rs. " << balance;
        showErrorMessage(hConsole, msg.str());
        return;
    }
    balance -= amount;
    clearScreen();
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << "\n\n[OK] Withdrew Rs. " << fixed << setprecision(2) << amount << "\n";
    resetColor(hConsole);
    cout << "New cash balance: Rs. " << balance << endl;
    setAccentYellow(hConsole);
    cout << "\nPress any key to continue...";
    resetColor(hConsole);
    _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    clearScreen();
}

void addStock(char symbols[][SYM_LEN],
              char names[][NAME_LEN],
              double currPrice[],
              int companyCount,
              char **&holdSymbols,
              int *&holdShares,
              int &holdCount,
              int &holdCapacity,
              double &balance,
              long long &totalSharesTraded)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    char symbol[SYM_LEN];
    int shares;

    clearScreen();
    prepareTextInput();
    cout << "----- BUY STOCK -----\n\n";
    cout << "Available symbols: ";
    for (int i = 0; i < companyCount; i++)
    {
        cout << symbols[i];
        if (i + 1 < companyCount)
            cout << ", ";
    }
    cout << "\n\nEnter the stock symbol you want to BUY.\n";
    cout << "Enter 0 to cancel and go back.\n\n";
    cout << "Symbol: ";
    cin >> symbol;
    cin.ignore(10000, '\n');

    if (isCancelToken(symbol))
    {
        showInfoMessage(hConsole, "Buy cancelled.\nNo shares were purchased.");
        return;
    }

    int mIdx = findSymbolIndex(symbols, companyCount, symbol);
    if (mIdx < 0)
    {
        ostringstream msg;
        msg << "Invalid stock symbol: \"" << symbol << "\"\n"
            << "Please enter a symbol from the available list.\n"
            << "Example: PSO";
        showErrorMessage(hConsole, msg.str());
        return;
    }

    cout << "Current price of " << symbols[mIdx] << " (" << names[mIdx]
         << "): Rs. " << fixed << setprecision(2) << currPrice[mIdx] << endl;
    cout << "Enter how many shares to buy.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Shares: ";
    cin >> shares;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        showErrorMessage(hConsole, "Invalid share quantity.\nPlease enter a whole number (example: 10).\nEnter 0 to cancel.");
        return;
    }
    cin.ignore(10000, '\n');
    if (shares == 0)
    {
        showInfoMessage(hConsole, "Buy cancelled.\nNo shares were purchased.");
        return;
    }
    if (shares < 0)
    {
        showErrorMessage(hConsole, "Invalid share quantity.\nShares cannot be negative.");
        return;
    }

    double cost = currPrice[mIdx] * shares;
    if (cost > balance)
    {
        ostringstream msg;
        msg << fixed << setprecision(2)
            << "Insufficient balance to buy.\n"
            << "Required : Rs. " << cost << "\n"
            << "Available: Rs. " << balance << "\n";
        if (balance <= 0.0001)
            msg << "Tip: Press M to add money first, then press A to buy.";
        else
            msg << "Tip: Add more money (M) or buy fewer shares.";
        showErrorMessage(hConsole, msg.str());
        return;
    }

    int hIdx = findHoldingIndex(holdSymbols, holdCount, symbols[mIdx]);
    if (hIdx >= 0)
    {
        holdShares[hIdx] += shares;
    }
    else
    {
        if (holdCount >= holdCapacity)
            expandHoldings(holdSymbols, holdShares, holdCapacity);
        strncpy(holdSymbols[holdCount], symbols[mIdx], SYM_LEN - 1);
        holdSymbols[holdCount][SYM_LEN - 1] = '\0';
        holdShares[holdCount] = shares;
        holdCount++;
    }

    balance -= cost;
    totalSharesTraded += shares;

    clearScreen();
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << "\n\n[OK] Bought " << shares << " shares of " << symbols[mIdx] << "\n";
    resetColor(hConsole);
    cout << "Cost: Rs. " << fixed << setprecision(2) << cost << endl;
    cout << "Remaining cash: Rs. " << balance << endl;
    setAccentYellow(hConsole);
    cout << "\nPress any key to continue...";
    resetColor(hConsole);
    _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    clearScreen();
}

void removeStock(char symbols[][SYM_LEN],
                 char names[][NAME_LEN],
                 double currPrice[],
                 int companyCount,
                 char **&holdSymbols,
                 int *&holdShares,
                 int &holdCount,
                 double &balance,
                 long long &totalSharesTraded)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (holdCount == 0)
    {
        showErrorMessage(hConsole, "Portfolio is empty.\nThere are no shares to sell.\nBuy shares first with A, then try again.");
        return;
    }

    char symbol[SYM_LEN];
    int shares;

    clearScreen();
    prepareTextInput();
    cout << "----- SELL STOCK -----\n\n";
    cout << "Your holdings: ";
    for (int i = 0; i < holdCount; i++)
    {
        cout << holdSymbols[i] << "(" << holdShares[i] << ")";
        if (i + 1 < holdCount)
            cout << ", ";
    }
    cout << "\n\nEnter the stock symbol you want to SELL.\n";
    cout << "Enter 0 to cancel and go back.\n\n";
    cout << "Symbol: ";
    cin >> symbol;
    cin.ignore(10000, '\n');

    if (isCancelToken(symbol))
    {
        showInfoMessage(hConsole, "Sell cancelled.\nNo shares were sold.");
        return;
    }

    int hIdx = findHoldingIndex(holdSymbols, holdCount, symbol);
    if (hIdx < 0)
    {
        ostringstream msg;
        msg << "You do not hold symbol \"" << symbol << "\".\n"
            << "Please choose a symbol from your holdings list.";
        showErrorMessage(hConsole, msg.str());
        return;
    }

    int mIdx = findSymbolIndex(symbols, companyCount, holdSymbols[hIdx]);
    cout << "Current price: Rs. " << fixed << setprecision(2) << currPrice[mIdx] << endl;
    cout << "You own " << holdShares[hIdx] << " shares.\n";
    cout << "Enter how many shares to sell.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Shares: ";
    cin >> shares;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        showErrorMessage(hConsole, "Invalid share quantity.\nPlease enter a whole number (example: 5).\nEnter 0 to cancel.");
        return;
    }
    cin.ignore(10000, '\n');
    if (shares == 0)
    {
        showInfoMessage(hConsole, "Sell cancelled.\nNo shares were sold.");
        return;
    }
    if (shares < 0)
    {
        showErrorMessage(hConsole, "Invalid share quantity.\nShares cannot be negative.");
        return;
    }

    if (shares > holdShares[hIdx])
    {
        ostringstream msg;
        msg << "Not enough shares to sell.\n"
            << "You only have " << holdShares[hIdx] << " share(s) of " << holdSymbols[hIdx] << ".";
        showErrorMessage(hConsole, msg.str());
        return;
    }

    double proceeds = currPrice[mIdx] * shares;
    balance += proceeds;
    holdShares[hIdx] -= shares;
    totalSharesTraded += shares;

    if (holdShares[hIdx] == 0)
    {
        for (int i = hIdx; i < holdCount - 1; i++)
        {
            strncpy(holdSymbols[i], holdSymbols[i + 1], SYM_LEN - 1);
            holdSymbols[i][SYM_LEN - 1] = '\0';
            holdShares[i] = holdShares[i + 1];
        }
        holdCount--;
    }

    clearScreen();
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << "\n\n[OK] Sold " << shares << " shares of " << symbols[mIdx] << "\n";
    resetColor(hConsole);
    cout << "Received: Rs. " << fixed << setprecision(2) << proceeds << endl;
    cout << "New cash balance: Rs. " << balance << endl;
    setAccentYellow(hConsole);
    cout << "\nPress any key to continue...";
    resetColor(hConsole);
    _getch();
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    clearScreen();
}

void writePortfolioSeparator(ostream &out)
{
    printRuleLine(out, '=', PF_FILE_WIDTH);
}

void writePortfolioSoftLine(ostream &out)
{
    printRuleLine(out, '-', PF_FILE_WIDTH);
}

void clearPortfolioMemory(char ownerName[],
                          double &balance,
                          int &holdCount,
                          bool &portfolioReady,
                          bool &onPortfolio)
{
    ownerName[0] = '\0';
    balance = 0.0;
    holdCount = 0;
    portfolioReady = false;
    onPortfolio = false;
}

bool savePortfolio(const char *filename,
                   const char *ownerName,
                   double balance,
                   char **holdSymbols,
                   int *holdShares,
                   int holdCount,
                   char symbols[][SYM_LEN],
                   char names[][NAME_LEN],
                   double prevPrice[],
                   double currPrice[],
                   double highPrice[],
                   double lowPrice[],
                   int companyCount)
{
    ofstream fout(filename);
    if (!fout.is_open())
        return false;

    double todayGL = 0.0;
    computePortfolioTotals(holdSymbols, holdShares, holdCount,
                           symbols, prevPrice, currPrice, companyCount, todayGL);
    double previousBalance = balance;
    double newBalance = balance + todayGL;

    writePortfolioSeparator(fout);
    fout << "  KARACHI STOCK MARKET - YOUR PORTFOLIO\n";
    fout << "  (Easy to read block format)\n";
    writePortfolioSeparator(fout);
    fout << "\n";
    writeLabeled(fout, "Owner Name", ownerName);
    writeLabeledMoney(fout, "Cash (Rs.)", balance, false);
    fout << "\n";

    if (holdCount == 0)
    {
        writePortfolioSoftLine(fout);
        fout << "No shares bought yet.\n";
        fout << "Open the app, press M to add money, then A to buy.\n";
        writePortfolioSoftLine(fout);
    }

    int shown = 0;
    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;
        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];
        shown++;

        writePortfolioSoftLine(fout);
        fout << "[HOLDING " << shown << "]\n";
        writeLabeled(fout, "Symbol", symbols[idx]);
        writeLabeled(fout, "Company Name", names[idx]);
        {
            ostringstream sh;
            sh << holdShares[h];
            writeLabeled(fout, "Shares Owned", sh.str());
        }
        writeLabeledMoney(fout, "Current Price", currPrice[idx], false);
        writeLabeledMoney(fout, "Previous Price", prevPrice[idx], false);
        writeLabeledMoney(fout, "Gain / Loss", gl, true);
        writeLabeledMoney(fout, "Highest Price", highPrice[idx], false);
        writeLabeledMoney(fout, "Lowest Price", lowPrice[idx], false);
    }

    fout << "\n";
    writePortfolioSeparator(fout);
    fout << "  SUMMARY\n";
    writePortfolioSeparator(fout);
    writePortfolioFooterLine(fout, "Today's Gain or Loss", todayGL, true);
    writePortfolioFooterLine(fout, "Previous Balance", previousBalance, false);
    writePortfolioFooterLine(fout, "New Balance", newBalance, false);
    writePortfolioFooterLine(fout, "Cash Available", balance, false);
    writePortfolioSeparator(fout);
    fout << "Tip: Do not edit this file while the program is running.\n";

    fout.close();
    return true;
}

void prepareTextInput()
{
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    cin.clear();
    /* Discard any partial line still sitting in the iostream buffer. */
    if (cin.rdbuf()->in_avail() > 0)
        cin.ignore(cin.rdbuf()->in_avail());
}

bool portfolioFileExists(const char *filename)
{
    ifstream f(filename);
    return f.good();
}

string trimString(const string &s)
{
    size_t a = 0;
    while (a < s.size() && isspace(static_cast<unsigned char>(s[a])))
        a++;
    size_t b = s.size();
    while (b > a && isspace(static_cast<unsigned char>(s[b - 1])))
        b--;
    return s.substr(a, b - a);
}

bool peekPortfolioOwner(const char *filename, char ownerOut[])
{
    ownerOut[0] = '\0';
    ifstream in(filename);
    if (!in.is_open())
        return false;
    string line;
    while (getline(in, line))
    {
        string t = trimString(line);
        size_t colon = t.find(':');
        if (colon == string::npos)
            continue;
        string key = trimString(t.substr(0, colon));
        if (_stricmp(key.c_str(), "Portfolio owner") != 0)
            continue;
        string name = trimString(t.substr(colon + 1));
        strncpy(ownerOut, name.c_str(), OWNER_LEN - 1);
        ownerOut[OWNER_LEN - 1] = '\0';
        return ownerOut[0] != '\0';
    }
    return false;
}

bool loadPortfolio(const char *filename,
                   char ownerName[],
                   double &balance,
                   char **&holdSymbols,
                   int *&holdShares,
                   int &holdCount,
                   int &holdCapacity,
                   char symbols[][SYM_LEN],
                   int companyCount)
{
    ifstream in(filename);
    if (!in.is_open())
        return false;

    ownerName[0] = '\0';
    balance = 0.0;
    holdCount = 0;

    string pendingSym;
    int pendingShares = -1;
    bool inSummary = false;

    string line;
    while (getline(in, line))
    {
        string t = trimString(line);
        if (t.empty())
            continue;
        if (t[0] == '=' || t[0] == '-' || t[0] == '*')
            continue;
        if (t.find("PORTFOLIO") != string::npos || t.find("KARACHI STOCK") != string::npos)
            continue;
        if (t.find("[HOLDING") != string::npos || t.find("HOLDING") == 0)
            continue;
        if (t.find("Easy to read") != string::npos || t.find("Tip:") == 0)
            continue;
        if (t.find("Sym") == 0 && t.find("Company") != string::npos)
            continue;
        if (t.find("SUMMARY") != string::npos || t.find("SESSION SUMMARY") != string::npos)
        {
            inSummary = true;
            /* commit any pending holding before summary */
            if (!pendingSym.empty() && pendingShares > 0)
            {
                int mIdx = findSymbolIndex(symbols, companyCount, pendingSym.c_str());
                if (mIdx >= 0)
                {
                    if (holdCount >= holdCapacity)
                        expandHoldings(holdSymbols, holdShares, holdCapacity);
                    strncpy(holdSymbols[holdCount], symbols[mIdx], SYM_LEN - 1);
                    holdSymbols[holdCount][SYM_LEN - 1] = '\0';
                    holdShares[holdCount] = pendingShares;
                    holdCount++;
                }
                pendingSym.clear();
                pendingShares = -1;
            }
            continue;
        }
        if (t.find("No holdings") != string::npos || t.find("No shares") != string::npos)
            continue;

        size_t colon = t.find(':');
        if (colon == string::npos)
        {
            /* Legacy single-line row: SYMBOL ... shares ... */
            string sym;
            {
                stringstream ss(t);
                ss >> sym;
            }
            if (sym.empty())
                continue;
            int mIdx = findSymbolIndex(symbols, companyCount, sym.c_str());
            if (mIdx < 0)
                continue;
            int shares = 0;
            for (size_t i = 0; i < t.size(); i++)
            {
                if (isdigit(static_cast<unsigned char>(t[i])))
                {
                    /* skip price-like decimals by taking first integer token after company */
                    shares = atoi(t.c_str() + i);
                    break;
                }
            }
            /* Better legacy parse: find share qty as standalone int before a price with dot */
            {
                stringstream ss(t);
                string tok;
                ss >> tok; /* symbol */
                int foundShares = 0;
                while (ss >> tok)
                {
                    bool allDigit = !tok.empty();
                    for (size_t k = 0; k < tok.size(); k++)
                    {
                        if (!isdigit(static_cast<unsigned char>(tok[k])))
                        {
                            allDigit = false;
                            break;
                        }
                    }
                    if (allDigit)
                    {
                        foundShares = atoi(tok.c_str());
                        break;
                    }
                }
                if (foundShares > 0)
                    shares = foundShares;
            }
            if (shares <= 0)
                continue;
            if (holdCount >= holdCapacity)
                expandHoldings(holdSymbols, holdShares, holdCapacity);
            strncpy(holdSymbols[holdCount], symbols[mIdx], SYM_LEN - 1);
            holdSymbols[holdCount][SYM_LEN - 1] = '\0';
            holdShares[holdCount] = shares;
            holdCount++;
            continue;
        }

        string key = trimString(t.substr(0, colon));
        string val = trimString(t.substr(colon + 1));

        if (_stricmp(key.c_str(), "Portfolio owner") == 0 ||
            _stricmp(key.c_str(), "Owner Name") == 0)
        {
            strncpy(ownerName, val.c_str(), OWNER_LEN - 1);
            ownerName[OWNER_LEN - 1] = '\0';
            continue;
        }
        if (_stricmp(key.c_str(), "Cash available") == 0 ||
            _stricmp(key.c_str(), "Cash available (Rs.)") == 0 ||
            _stricmp(key.c_str(), "Cash (Rs.)") == 0 ||
            _stricmp(key.c_str(), "Cash Available") == 0)
        {
            if (!inSummary)
                balance = atof(val.c_str());
            continue;
        }
        if (inSummary)
            continue;

        if (_stricmp(key.c_str(), "Symbol") == 0)
        {
            if (!pendingSym.empty() && pendingShares > 0)
            {
                int mIdx = findSymbolIndex(symbols, companyCount, pendingSym.c_str());
                if (mIdx >= 0)
                {
                    if (holdCount >= holdCapacity)
                        expandHoldings(holdSymbols, holdShares, holdCapacity);
                    strncpy(holdSymbols[holdCount], symbols[mIdx], SYM_LEN - 1);
                    holdSymbols[holdCount][SYM_LEN - 1] = '\0';
                    holdShares[holdCount] = pendingShares;
                    holdCount++;
                }
            }
            pendingSym = val;
            pendingShares = -1;
            continue;
        }
        if (_stricmp(key.c_str(), "Share Qty") == 0 ||
            _stricmp(key.c_str(), "Shares Owned") == 0)
        {
            pendingShares = atoi(val.c_str());
            continue;
        }
    }

    if (!pendingSym.empty() && pendingShares > 0)
    {
        int mIdx = findSymbolIndex(symbols, companyCount, pendingSym.c_str());
        if (mIdx >= 0)
        {
            if (holdCount >= holdCapacity)
                expandHoldings(holdSymbols, holdShares, holdCapacity);
            strncpy(holdSymbols[holdCount], symbols[mIdx], SYM_LEN - 1);
            holdSymbols[holdCount][SYM_LEN - 1] = '\0';
            holdShares[holdCount] = pendingShares;
            holdCount++;
        }
    }

    if (ownerName[0] == '\0')
        strncpy(ownerName, "Investor", OWNER_LEN - 1);
    return true;
}

void askOwnerNameInteractive(HANDLE hConsole, char ownerName[])
{
    prepareTextInput();
    setAccentCyan(hConsole);
    cout << "\n----- PORTFOLIO OWNER -----\n";
    resetColor(hConsole);
    cout << "Please type the owner name for this portfolio.\n";
    cout << "Then press Enter to confirm.\n";
    cout << "Enter 0 to use the default name \"Investor\".\n\n";
    cout << "Enter owner name: ";
    cin.getline(ownerName, OWNER_LEN);
    string n = trimString(ownerName);
    if (n.empty() || isCancelToken(n.c_str()))
        strncpy(ownerName, "Investor", OWNER_LEN - 1);
    else
    {
        strncpy(ownerName, n.c_str(), OWNER_LEN - 1);
        ownerName[OWNER_LEN - 1] = '\0';
    }
}

/**
 * First P in session: load existing file quietly, or ask name and create.
 * Later P: open current session portfolio only (no continue/new menu).
 * If file disappears mid-run: keep session data and rewrite the file.
 */
bool setupPortfolioSession(HANDLE hConsole,
                           char ownerName[],
                           double &balance,
                           char **&holdSymbols,
                           int *&holdShares,
                           int &holdCount,
                           int &holdCapacity,
                           char symbols[][SYM_LEN],
                           char names[][NAME_LEN],
                           double prevPrice[],
                           double currPrice[],
                           double highPrice[],
                           double lowPrice[],
                           int companyCount,
                           bool portfolioReady)
{
    const char *pfFile = "portfolio.txt";

    /* File deleted / missing: never offer "existing", never restore old session quietly. */
    if (!portfolioFileExists(pfFile))
    {
        if (portfolioReady)
        {
            showInfoMessage(hConsole,
                            "Portfolio file was deleted.\n"
                            "Old session data will not be restored.\n"
                            "Please create a NEW portfolio.");
        }

        balance = 0.0;
        holdCount = 0;
        ownerName[0] = '\0';

        clearScreen();
        setAccentCyan(hConsole);
        printRuleLine(cout, '=', PORTFOLIO_LINE_WIDTH);
        cout << " PORTFOLIO SETUP\n";
        printRuleLine(cout, '=', PORTFOLIO_LINE_WIDTH);
        resetColor(hConsole);
        cout << "\nNo portfolio file found.\n";
        cout << "Please set up a new portfolio to continue.\n";

        askOwnerNameInteractive(hConsole, ownerName);
        balance = 0.0;
        holdCount = 0;

        if (!savePortfolio(pfFile, ownerName, balance,
                           holdSymbols, holdShares, holdCount,
                           symbols, names, prevPrice, currPrice,
                           highPrice, lowPrice, companyCount))
        {
            showErrorMessage(hConsole, "Could not save the portfolio.\nPlease try again.");
            return false;
        }

        clearScreen();
        setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        cout << "\n\n[OK] New portfolio ready for owner \"" << ownerName << "\".\n";
        resetColor(hConsole);
        setAccentYellow(hConsole);
        cout << "\nPress any key to open your portfolio...";
        resetColor(hConsole);
        _getch();
        clearScreen();
        return true;
    }

    /* File exists — continue / new menu */
    clearScreen();
    setAccentCyan(hConsole);
    printRuleLine(cout, '=', PORTFOLIO_LINE_WIDTH);
    cout << " PORTFOLIO\n";
    printRuleLine(cout, '=', PORTFOLIO_LINE_WIDTH);
    resetColor(hConsole);

    char existingOwner[OWNER_LEN] = "";
    peekPortfolioOwner(pfFile, existingOwner);

    if (portfolioReady)
        cout << "\nCurrent session owner: " << ownerName << "\n";
    else
    {
        cout << "\nA portfolio is already available.\n";
        if (existingOwner[0] != '\0')
            cout << "Current owner on file: " << existingOwner << "\n";
    }

    cout << "\nPlease choose an option:\n";
    if (portfolioReady)
        cout << "  1) Continue with the current portfolio\n";
    else
        cout << "  1) Continue with the existing portfolio\n";
    cout << "  2) Create a NEW portfolio\n";
    cout << "  0) Cancel and return to Live Market\n\n";
    cout << "Enter your choice (0, 1, or 2): ";

    prepareTextInput();
    int choice = -1;
    cin >> choice;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        showErrorMessage(hConsole, "Invalid choice.\nPlease enter only 0, 1, or 2.");
        return false;
    }
    cin.ignore(10000, '\n');

    if (choice == 0)
    {
        showInfoMessage(hConsole, "Cancelled.\nReturning to Live Market.");
        return false;
    }

    if (choice == 1)
    {
        if (portfolioReady)
        {
            if (holdCount == 0)
            {
                loadPortfolio(pfFile, ownerName, balance,
                              holdSymbols, holdShares, holdCount, holdCapacity,
                              symbols, companyCount);
            }
            savePortfolio(pfFile, ownerName, balance,
                          holdSymbols, holdShares, holdCount,
                          symbols, names, prevPrice, currPrice,
                          highPrice, lowPrice, companyCount);
            return true;
        }

        if (!loadPortfolio(pfFile, ownerName, balance,
                           holdSymbols, holdShares, holdCount, holdCapacity,
                           symbols, companyCount))
        {
            showErrorMessage(hConsole, "Could not read portfolio data.\nA new portfolio will be created.");
            askOwnerNameInteractive(hConsole, ownerName);
            balance = 0.0;
            holdCount = 0;
            savePortfolio(pfFile, ownerName, balance,
                          holdSymbols, holdShares, holdCount,
                          symbols, names, prevPrice, currPrice,
                          highPrice, lowPrice, companyCount);
        }
        return true;
    }

    if (choice == 2)
    {
        askOwnerNameInteractive(hConsole, ownerName);
        balance = 0.0;
        holdCount = 0;
        if (!savePortfolio(pfFile, ownerName, balance,
                           holdSymbols, holdShares, holdCount,
                           symbols, names, prevPrice, currPrice,
                           highPrice, lowPrice, companyCount))
        {
            showErrorMessage(hConsole, "Could not save the new portfolio.\nPlease try again.");
            return false;
        }
        clearScreen();
        setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        cout << "\n\n[OK] New portfolio ready for owner \"" << ownerName << "\".\n";
        resetColor(hConsole);
        setAccentYellow(hConsole);
        cout << "\nPress any key to open your portfolio...";
        resetColor(hConsole);
        _getch();
        clearScreen();
        return true;
    }

    showErrorMessage(hConsole, "Wrong choice.\nPlease enter only 0, 1, or 2.");
    return false;
}

/* ---------- Main ---------- */

int main()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    maximizeConsoleWindow();
    enableDarkTheme(hConsole);
    srand((unsigned int)time(0));

    char symbols[MAX_COMPANIES][SYM_LEN];
    char names[MAX_COMPANIES][NAME_LEN];
    double sessionStart[MAX_COMPANIES];
    double prevPrice[MAX_COMPANIES];
    double currPrice[MAX_COMPANIES];
    double highPrice[MAX_COMPANIES];
    double lowPrice[MAX_COMPANIES];
    double pctChange[MAX_COMPANIES];

    int companyCount = loadCompanies("companies.txt",
                                     symbols, names,
                                     sessionStart, prevPrice, currPrice,
                                     highPrice, lowPrice, pctChange);

    if (companyCount <= 0)
    {
        setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        cout << "No companies loaded. Place companies.txt next to the executable.\n";
        resetColor(hConsole);
        cout << "Press any key to exit...";
        _getch();
        return 1;
    }

    clearScreen();
    setAccentCyan(hConsole);
    cout << "========================================================================================\n";
    cout << "                   KARACHI STOCK MARKET TRADING SIMULATOR\n";
    cout << "========================================================================================\n";
    resetColor(hConsole);
    cout << "Loaded " << companyCount << " companies from companies.txt\n";
    cout << "Session-start prices locked for +/-15% cap.\n";
    setAccentYellow(hConsole);
    cout << "\nPress any key to open Live Market...";
    resetColor(hConsole);
    _getch();

    /* First tick so Previous/Current differ and every stock shows ↑ / ↓ on open. */
    refreshAllPrices(sessionStart, prevPrice, currPrice,
                     highPrice, lowPrice, pctChange, companyCount);

    char ownerName[OWNER_LEN] = "(empty)";
    double balance = 0.0;
    long long totalSharesTraded = 0;

    char **holdSymbols = NULL;
    int *holdShares = NULL;
    int holdCount = 0;
    int holdCapacity = 0;
    expandHoldings(holdSymbols, holdShares, holdCapacity);

    bool running = true;
    bool onPortfolio = false;
    bool portfolioReady = false;

    while (running)
    {
        /* If portfolio.txt vanished mid-run, wipe ALL related session data. */
        if (!portfolioFileExists("portfolio.txt") &&
            (portfolioReady || onPortfolio || holdCount > 0 || balance > 0.0001 ||
             (ownerName[0] != '\0' && strcmp(ownerName, "(empty)") != 0)))
        {
            clearPortfolioMemory(ownerName, balance, holdCount, portfolioReady, onPortfolio);
            showErrorMessage(hConsole,
                             "Portfolio file was deleted.\n"
                             "All portfolio data in memory has been cleared.\n"
                             "Buy / Sell / Money are blocked.\n"
                             "Press P to create a NEW portfolio.");
            continue;
        }

        if (onPortfolio)
        {
            drawPortfolio(hConsole, ownerName, balance,
                          holdSymbols, holdShares, holdCount,
                          symbols, names, prevPrice, currPrice,
                          highPrice, lowPrice, companyCount);
        }
        else
        {
            drawLiveMarket(hConsole, symbols, names,
                           prevPrice, currPrice, highPrice, lowPrice,
                           pctChange, companyCount, totalSharesTraded);
        }

        int key = _getch();

        /* Handle Enter (CR or LF) and extended keys */
        if (key == 0 || key == 224)
        {
            _getch(); /* discard extended scan code */
            continue;
        }

        if (key == 13) /* Enter -> refresh ALL market prices */
        {
            refreshAllPrices(sessionStart, prevPrice, currPrice,
                             highPrice, lowPrice, pctChange, companyCount);
            continue;
        }

        char ch = (char)key;
        if (ch >= 'a' && ch <= 'z')
            ch = (char)(ch - 'a' + 'A');

        if (!onPortfolio)
        {
            if (ch == 'P')
            {
                /* Only flush if file still exists — never recreate a deleted file here. */
                if (portfolioFileExists("portfolio.txt") &&
                    (holdCount > 0 || balance > 0.0001 || portfolioReady))
                {
                    if (ownerName[0] == '\0' || strcmp(ownerName, "(empty)") == 0)
                        strncpy(ownerName, "Investor", OWNER_LEN - 1);
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
                    portfolioReady = true;
                }

                bool ok = setupPortfolioSession(hConsole, ownerName, balance,
                                                holdSymbols, holdShares, holdCount, holdCapacity,
                                                symbols, names, prevPrice, currPrice,
                                                highPrice, lowPrice, companyCount,
                                                portfolioReady);
                if (!ok)
                {
                    onPortfolio = false;
                    continue;
                }
                portfolioReady = true;
                onPortfolio = true;
            }
            else if (ch == 'A' || ch == 'R' || ch == 'M')
            {
                if (!portfolioFileExists("portfolio.txt"))
                {
                    clearPortfolioMemory(ownerName, balance, holdCount, portfolioReady, onPortfolio);
                    showErrorMessage(hConsole,
                                     "No portfolio file found.\n"
                                     "All portfolio data cleared.\n"
                                     "Press P to create a NEW portfolio first.\n"
                                     "Buy / Sell / Money are not allowed without a portfolio.");
                    continue;
                }
                if (ch == 'A')
                {
                    addStock(symbols, names, currPrice, companyCount,
                             holdSymbols, holdShares, holdCount, holdCapacity,
                             balance, totalSharesTraded);
                }
                else if (ch == 'R')
                {
                    removeStock(symbols, names, currPrice, companyCount,
                                holdSymbols, holdShares, holdCount,
                                balance, totalSharesTraded);
                }
                else
                {
                    addMoney(balance);
                }
                if (ownerName[0] == '\0' || strcmp(ownerName, "(empty)") == 0)
                    strncpy(ownerName, "Investor", OWNER_LEN - 1);
                portfolioReady = true;
                savePortfolio("portfolio.txt", ownerName, balance,
                              holdSymbols, holdShares, holdCount,
                              symbols, names, prevPrice, currPrice,
                              highPrice, lowPrice, companyCount);
            }
            else if (ch == 'E')
            {
                running = false;
            }
            else
            {
                ostringstream msg;
                msg << "Wrong choice: \"" << ch << "\"\n\n"
                    << "Valid keys on Live Market:\n"
                    << "  Enter = refresh prices\n"
                    << "  P     = Portfolio\n"
                    << "  A     = Buy stock\n"
                    << "  R     = Sell stock\n"
                    << "  M     = Add money\n"
                    << "  E     = Exit";
                showErrorMessage(hConsole, msg.str());
            }
        }
        else
        {
            if (ch == 'L')
            {
                if (portfolioFileExists("portfolio.txt"))
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
                onPortfolio = false;
            }
            else if (ch == 'A' || ch == 'R' || ch == 'M' || ch == 'W')
            {
                if (!portfolioFileExists("portfolio.txt"))
                {
                    clearPortfolioMemory(ownerName, balance, holdCount, portfolioReady, onPortfolio);
                    showErrorMessage(hConsole,
                                     "Portfolio file was deleted.\n"
                                     "All portfolio data cleared.\n"
                                     "Press P to create a NEW portfolio.\n"
                                     "No buy / sell / money until then.");
                    continue;
                }
                if (ch == 'A')
                {
                    addStock(symbols, names, currPrice, companyCount,
                             holdSymbols, holdShares, holdCount, holdCapacity,
                             balance, totalSharesTraded);
                }
                else if (ch == 'R')
                {
                    removeStock(symbols, names, currPrice, companyCount,
                                holdSymbols, holdShares, holdCount,
                                balance, totalSharesTraded);
                }
                else if (ch == 'M')
                {
                    addMoney(balance);
                }
                else
                {
                    withdrawMoney(balance);
                }
                savePortfolio("portfolio.txt", ownerName, balance,
                              holdSymbols, holdShares, holdCount,
                              symbols, names, prevPrice, currPrice,
                              highPrice, lowPrice, companyCount);
            }
            else if (ch == 'E')
            {
                running = false;
            }
            else
            {
                ostringstream msg;
                msg << "Wrong choice: \"" << ch << "\"\n\n"
                    << "Valid keys on Portfolio:\n"
                    << "  Enter = refresh prices\n"
                    << "  L     = Live Market\n"
                    << "  A     = Buy stock\n"
                    << "  R     = Sell stock\n"
                    << "  M     = Add money\n"
                    << "  W     = Withdraw money\n"
                    << "  E     = Exit";
                showErrorMessage(hConsole, msg.str());
            }
        }
    }

    /* Always persist portfolio on exit only if file/session exists. */
    if (strcmp(ownerName, "(empty)") == 0 || ownerName[0] == '\0')
        strncpy(ownerName, "Investor", OWNER_LEN - 1);

    bool okCompanies = saveCompanies("companies.txt", symbols, names, currPrice, companyCount);
    bool okPortfolio = false;
    if (portfolioReady || portfolioFileExists("portfolio.txt") || holdCount > 0 || balance > 0.0001)
    {
        okPortfolio = savePortfolio("portfolio.txt", ownerName, balance,
                                    holdSymbols, holdShares, holdCount,
                                    symbols, names, prevPrice, currPrice,
                                    highPrice, lowPrice, companyCount);
    }

    clearScreen();
    cout << "Session closed.\n";
    if (okCompanies)
        cout << "Saved updated prices to companies.txt\n";
    else
        cout << "WARNING: failed to save companies.txt\n";
    if (okPortfolio)
        cout << "Saved portfolio to portfolio.txt (block format)\n";
    else if (!portfolioFileExists("portfolio.txt") && !portfolioReady)
        cout << "No portfolio to save.\n";
    else
        cout << "WARNING: failed to save portfolio.txt\n";

    freeHoldings(holdSymbols, holdShares, holdCapacity);

    cout << "Press any key to exit...";
    _getch();
    return 0;
}
