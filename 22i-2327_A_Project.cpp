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
#include <conio.h>
#include <windows.h>

using namespace std;

#define MAX_COMPANIES 50
#define SYM_LEN 16
#define NAME_LEN 64
#define OWNER_LEN 64

/* Fixed column widths for portfolio.txt AND console portfolio (must match) */
#define COL_SYM 8
#define COL_NAME 28
#define COL_SHARES 12
#define COL_PRICE 14
#define COL_GL 14
#define PORTFOLIO_LINE_WIDTH (COL_SYM + COL_NAME + COL_SHARES + COL_PRICE * 4 + COL_GL)

/* Live market board columns — widths fit full header labels */
#define MKT_SYM 8
#define MKT_NAME 34
#define MKT_NUM 14
#define MKT_CHG 8
#define MKT_LINE_WIDTH (MKT_SYM + MKT_NAME + MKT_NUM * 4 + MKT_CHG)

/* ---------- Console helpers (state passed in via HANDLE) ---------- */

void clearScreen()
{
    system("cls");
}

void setColor(HANDLE hConsole, WORD attributes)
{
    /* Keep black background (low nibble only = FG); dark theme. */
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(attributes & 0x0F));
}

void resetColor(HANDLE hConsole)
{
    /* Bright white on black — high contrast default */
    SetConsoleTextAttribute(hConsole,
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

void setFontBold(HANDLE hConsole, int height = 24)
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
        Sleep(50); /* let maximize apply before font/buffer layout */
    }
}

void enableDarkTheme(HANDLE hConsole)
{
    /* Black background + bright white default (cmd color table) */
    system("color 0F");
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    setFontBold(hConsole, 24);
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

void printCellLeft(ostream &out, const char *s, int width)
{
    out << left << setfill(' ') << setw(width) << fitWidth(s, width);
}

void printCellRightText(ostream &out, const string &text, int width)
{
    string t = text;
    if ((int)t.size() > width)
        t = t.substr(0, (size_t)width);
    out << right << setfill(' ') << setw(width) << t;
}

void printMoneyCell(ostream &out, double value, int width, bool showPlus)
{
    ostringstream cell;
    cell << fixed << setprecision(2);
    if (showPlus && value > 0.0001)
        cell << "+";
    cell << value;
    printCellRightText(out, cell.str(), width);
}

void printGainLossFixed(HANDLE hConsole, double value, int width)
{
    ostringstream cell;
    cell << fixed << setprecision(2);
    if (value > 0.0001)
        cell << "+" << value;
    else
        cell << value;
    string t = cell.str();
    if ((int)t.size() > width)
        t = t.substr(0, (size_t)width);
    while ((int)t.size() < width)
        t = " " + t;

    if (value > 0.0001)
        setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    else if (value < -0.0001)
        setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
    else
        resetColor(hConsole);
    cout << t;
    resetColor(hConsole);
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
    setAccentCyan(hConsole);
    cout << "========================================================================================\n";
    cout << "                        KARACHI STOCK MARKET (LIVE)\n";
    cout << "========================================================================================\n";
    setAccentYellow(hConsole);
    cout << "Show updates: Enter   | Portfolio: P | Add Stock: A | Remove: R | Add Money: M | Exit: E\n";
    cout << "Tip: Cash starts at Rs. 0 — press M to add money, then A to buy shares.\n";
    cout << "Change: \xE2\x86\x91 = up (rise)   \xE2\x86\x93 = down (fall)   = = unchanged\n";
    resetColor(hConsole);
    cout << "----------------------------------------------------------------------------------------\n";

    setAccentCyan(hConsole);
    printCellLeft(cout, "Symbol", MKT_SYM);
    printCellLeft(cout, "Company Name", MKT_NAME);
    printCellRightText(cout, "Previous Price", MKT_NUM);
    printCellRightText(cout, "Current Price", MKT_NUM);
    printCellRightText(cout, "Change", MKT_CHG);
    printCellRightText(cout, "Highest Price", MKT_NUM);
    printCellRightText(cout, "Lowest Price", MKT_NUM);
    cout << endl;
    resetColor(hConsole);
    cout << "----------------------------------------------------------------------------------------\n";

    for (int i = 0; i < companyCount; i++)
    {
        printCellLeft(cout, symbols[i], MKT_SYM);
        printCellLeft(cout, names[i], MKT_NAME);
        printMoneyCell(cout, prevPrice[i], MKT_NUM, false);
        printMoneyCell(cout, currPrice[i], MKT_NUM, false);
        printChangeArrow(hConsole, prevPrice[i], currPrice[i]);
        printMoneyCell(cout, highPrice[i], MKT_NUM, false);
        printMoneyCell(cout, lowPrice[i], MKT_NUM, false);
        cout << endl;
    }

    int adv = 0, dec = 0;
    findTopAdvancerDecliner(pctChange, companyCount, adv, dec);

    cout << "----------------------------------------------------------------------------------------\n";
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
    cout << "========================================================================================\n";
    resetColor(hConsole);
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
    cout << "========================================================================================\n";
    cout << "                     PORTFOLIO OWNER: " << ownerName << " (LIVE)\n";
    cout << "========================================================================================\n";
    setAccentYellow(hConsole);
    cout << "Updates: Enter | Live Market: L | Add: A | Remove: R | Money: M | Withdraw: W\n";
    resetColor(hConsole);
    cout << "----------------------------------------------------------------------------------------\n";

    setAccentCyan(hConsole);
    printCellLeft(cout, "Symbol", COL_SYM);
    printCellLeft(cout, "Company Name", COL_NAME);
    printCellRightText(cout, "Share Qty", COL_SHARES);
    printCellRightText(cout, "Current Price", COL_PRICE);
    printCellRightText(cout, "Previous Price", COL_PRICE);
    printCellRightText(cout, "Gain / Loss", COL_GL);
    printCellRightText(cout, "Highest Price", COL_PRICE);
    printCellRightText(cout, "Lowest Price", COL_PRICE);
    cout << endl;
    resetColor(hConsole);
    cout << "----------------------------------------------------------------------------------------\n";

    double todayGL = 0.0;

    if (holdCount == 0)
    {
        cout << "  (No holdings yet — cash may be Rs. 0)\n";
        cout << "  Step 1: press M and enter amount (e.g. 500000)  |  0 = cancel any form\n";
        cout << "  Step 2: press A, type a symbol (e.g. PSO), then share quantity\n";
    }

    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;

        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];
        todayGL += gl;

        printCellLeft(cout, symbols[idx], COL_SYM);
        printCellLeft(cout, names[idx], COL_NAME);
        {
            ostringstream sh;
            sh << holdShares[h];
            printCellRightText(cout, sh.str(), COL_SHARES);
        }
        printMoneyCell(cout, currPrice[idx], COL_PRICE, false);
        printMoneyCell(cout, prevPrice[idx], COL_PRICE, false);
        printGainLossFixed(hConsole, gl, COL_GL);
        printMoneyCell(cout, highPrice[idx], COL_PRICE, false);
        printMoneyCell(cout, lowPrice[idx], COL_PRICE, false);
        cout << endl;
    }

    double previousBalance = balance;
    double newBalance = balance + todayGL;

    cout << "----------------------------------------------------------------------------------------\n";
    cout << "Today's Gain or Loss (Rs.) : ";
    printGainLossFixed(hConsole, todayGL, COL_GL);
    cout << endl;
    resetColor(hConsole);
    cout << "Previous Balance (Rs.)     : " << fixed << setprecision(2) << previousBalance << endl;
    setAccentYellow(hConsole);
    cout << "New Balance (Rs.)          : " << fixed << setprecision(2) << newBalance << endl;
    resetColor(hConsole);
    cout << "Cash available             : " << fixed << setprecision(2) << balance << endl;
    setAccentCyan(hConsole);
    cout << "========================================================================================\n";
    resetColor(hConsole);
}

/* ---------- Transactions ---------- */

void addMoney(double &balance)
{
    cout << "\nEnter amount to add (Rs.)  [0 = Cancel]: ";
    double amount;
    cin >> amount;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid amount. Press any key...";
        _getch();
        return;
    }
    cin.ignore(10000, '\n');
    if (amount == 0)
    {
        cout << "Cancelled. Press any key...";
        _getch();
        return;
    }
    if (amount < 0)
    {
        cout << "Invalid amount. Press any key...";
        _getch();
        return;
    }
    balance += amount;
    cout << "Added Rs. " << fixed << setprecision(2) << amount
         << ". New cash balance: " << balance << endl;
    cout << "Press any key to continue...";
    _getch();
}

void withdrawMoney(double &balance)
{
    cout << "\nEnter amount to withdraw (Rs.)  [0 = Cancel]: ";
    double amount;
    cin >> amount;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid amount. Press any key...";
        _getch();
        return;
    }
    cin.ignore(10000, '\n');
    if (amount == 0)
    {
        cout << "Cancelled. Press any key...";
        _getch();
        return;
    }
    if (amount < 0)
    {
        cout << "Invalid amount. Press any key...";
        _getch();
        return;
    }
    if (amount > balance)
    {
        cout << "Insufficient balance. Available: Rs. "
             << fixed << setprecision(2) << balance << endl;
        cout << "Press any key to continue...";
        _getch();
        return;
    }
    balance -= amount;
    cout << "Withdrew Rs. " << fixed << setprecision(2) << amount
         << ". New cash balance: " << balance << endl;
    cout << "Press any key to continue...";
    _getch();
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
    char symbol[SYM_LEN];
    int shares;

    cout << "\nAvailable symbols: ";
    for (int i = 0; i < companyCount; i++)
    {
        cout << symbols[i];
        if (i + 1 < companyCount)
            cout << ", ";
    }
    cout << "\nEnter stock symbol to BUY  [0 = Cancel]: ";
    cin >> symbol;
    cin.ignore(10000, '\n');

    if (isCancelToken(symbol))
    {
        cout << "Cancelled. Press any key...";
        _getch();
        return;
    }

    int mIdx = findSymbolIndex(symbols, companyCount, symbol);
    if (mIdx < 0)
    {
        cout << "Invalid symbol \"" << symbol
             << "\". Please choose a symbol from the list above.\n";
        cout << "Press any key to continue...";
        _getch();
        return;
    }

    cout << "Current price of " << symbols[mIdx] << " (" << names[mIdx]
         << "): Rs. " << fixed << setprecision(2) << currPrice[mIdx] << endl;
    cout << "Enter number of shares to buy  [0 = Cancel]: ";
    cin >> shares;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid share quantity.\n";
        cout << "Press any key to continue...";
        _getch();
        return;
    }
    cin.ignore(10000, '\n');
    if (shares == 0)
    {
        cout << "Cancelled. Press any key...";
        _getch();
        return;
    }
    if (shares < 0)
    {
        cout << "Invalid share quantity.\n";
        cout << "Press any key to continue...";
        _getch();
        return;
    }

    double cost = currPrice[mIdx] * shares;
    if (cost > balance)
    {
        cout << "Insufficient balance.\n";
        cout << "  Required : Rs. " << fixed << setprecision(2) << cost << endl;
        cout << "  Available: Rs. " << balance << endl;
        if (balance <= 0.0001)
            cout << "  Tip: Press M first to ADD MONEY, then press A again to buy.\n";
        else
            cout << "  Tip: Press M to add more cash, or buy fewer shares.\n";
        cout << "Press any key to continue...";
        _getch();
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

    cout << "Bought " << shares << " shares of " << symbols[mIdx]
         << " for Rs. " << fixed << setprecision(2) << cost << endl;
    cout << "Remaining cash: Rs. " << balance << endl;
    cout << "Press any key to continue...";
    _getch();
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
    if (holdCount == 0)
    {
        cout << "\nPortfolio is empty. Nothing to sell.\n";
        cout << "Press any key to continue...";
        _getch();
        return;
    }

    char symbol[SYM_LEN];
    int shares;

    cout << "\nYour holdings: ";
    for (int i = 0; i < holdCount; i++)
    {
        cout << holdSymbols[i] << "(" << holdShares[i] << ")";
        if (i + 1 < holdCount)
            cout << ", ";
    }
    cout << "\nEnter stock symbol to SELL  [0 = Cancel]: ";
    cin >> symbol;
    cin.ignore(10000, '\n');

    if (isCancelToken(symbol))
    {
        cout << "Cancelled. Press any key...";
        _getch();
        return;
    }

    int hIdx = findHoldingIndex(holdSymbols, holdCount, symbol);
    if (hIdx < 0)
    {
        cout << "You do not hold symbol \"" << symbol << "\".\n";
        cout << "Press any key to continue...";
        _getch();
        return;
    }

    int mIdx = findSymbolIndex(symbols, companyCount, holdSymbols[hIdx]);
    cout << "Current price: Rs. " << fixed << setprecision(2) << currPrice[mIdx] << endl;
    cout << "You own " << holdShares[hIdx] << " shares. Enter shares to sell  [0 = Cancel]: ";
    cin >> shares;
    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Invalid share quantity.\n";
        cout << "Press any key to continue...";
        _getch();
        return;
    }
    cin.ignore(10000, '\n');
    if (shares == 0)
    {
        cout << "Cancelled. Press any key...";
        _getch();
        return;
    }
    if (shares < 0)
    {
        cout << "Invalid share quantity.\n";
        cout << "Press any key to continue...";
        _getch();
        return;
    }

    if (shares > holdShares[hIdx])
    {
        cout << "Not enough shares. You only have " << holdShares[hIdx] << ".\n";
        cout << "Press any key to continue...";
        _getch();
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

    cout << "Sold " << shares << " shares of " << symbols[mIdx]
         << " for Rs. " << fixed << setprecision(2) << proceeds << endl;
    cout << "New cash balance: Rs. " << balance << endl;
    cout << "Press any key to continue...";
    _getch();
}

/* Write one money cell into a fixed-width field (optional leading +) */
void writeAlignedMoney(ostream &out, double value, int width, bool showPlus)
{
    ostringstream cell;
    cell << fixed << setprecision(2);
    if (showPlus && value > 0.0001)
        cell << "+";
    cell << value;
    printCellRightText(out, cell.str(), width);
}

void writePortfolioSeparator(ostream &out)
{
    out << setfill('*') << setw(PORTFOLIO_LINE_WIDTH) << "" << setfill(' ') << "\n";
}

void writePortfolioHeaderRow(ostream &out)
{
    printCellLeft(out, "Symbol", COL_SYM);
    printCellLeft(out, "Company Name", COL_NAME);
    printCellRightText(out, "Share Qty", COL_SHARES);
    printCellRightText(out, "Current Price", COL_PRICE);
    printCellRightText(out, "Previous Price", COL_PRICE);
    printCellRightText(out, "Gain / Loss", COL_GL);
    printCellRightText(out, "Highest Price", COL_PRICE);
    printCellRightText(out, "Lowest Price", COL_PRICE);
    out << "\n";
}

void writePortfolioDataRow(ostream &out,
                           const char *symbol,
                           const char *companyName,
                           int shares,
                           double closePrice,
                           double previousPrice,
                           double gainLoss,
                           double high,
                           double low)
{
    printCellLeft(out, symbol, COL_SYM);
    printCellLeft(out, companyName, COL_NAME);
    {
        ostringstream sh;
        sh << shares;
        printCellRightText(out, sh.str(), COL_SHARES);
    }
    printMoneyCell(out, closePrice, COL_PRICE, false);
    printMoneyCell(out, previousPrice, COL_PRICE, false);
    writeAlignedMoney(out, gainLoss, COL_GL, true);
    printMoneyCell(out, high, COL_PRICE, false);
    printMoneyCell(out, low, COL_PRICE, false);
    out << "\n";
}

void writePortfolioFooterLine(ostream &out, const char *label, double value)
{
    const int labelWidth = 30;
    const int valueWidth = 16;
    out << left << setw(labelWidth) << label
        << " * "
        << right;
    writeAlignedMoney(out, value, valueWidth, true);
    out << " *" << "\n";
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
    fout << "Portfolio owner: " << ownerName << "\n\n";
    writePortfolioHeaderRow(fout);
    writePortfolioSeparator(fout);

    if (holdCount == 0)
    {
        fout << left << setw(PORTFOLIO_LINE_WIDTH)
             << "(No holdings in this session)" << "\n";
    }

    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;
        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];
        writePortfolioDataRow(fout,
                              symbols[idx],
                              names[idx],
                              holdShares[h],
                              currPrice[idx],
                              prevPrice[idx],
                              gl,
                              highPrice[idx],
                              lowPrice[idx]);
    }

    writePortfolioSeparator(fout);
    writePortfolioFooterLine(fout, "Today's Gain or Loss (Rs.)", todayGL);
    writePortfolioFooterLine(fout, "Previous Balance (Rs.)", previousBalance);
    writePortfolioFooterLine(fout, "New Balance (Rs.)", newBalance);
    writePortfolioSeparator(fout);

    fout.close();
    return true;
}

void ensureOwnerName(char ownerName[])
{
    if (ownerName[0] == '\0' || strcmp(ownerName, "(empty)") == 0)
    {
        cout << "\nEnter portfolio owner name  [0 = use Investor / Cancel default]: ";
        cin.getline(ownerName, OWNER_LEN);
        if (ownerName[0] == '\0' || isCancelToken(ownerName))
            strncpy(ownerName, "Investor", OWNER_LEN - 1);
        ownerName[OWNER_LEN - 1] = '\0';
    }
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
    bool nameAsked = false;

    while (running)
    {
        if (onPortfolio)
        {
            if (!nameAsked)
            {
                ensureOwnerName(ownerName);
                nameAsked = true;
            }
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
                onPortfolio = true;
            }
            else if (ch == 'A')
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
            else if (ch == 'E')
            {
                running = false;
            }
        }
        else
        {
            if (ch == 'L')
            {
                onPortfolio = false;
            }
            else if (ch == 'A')
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
            else if (ch == 'W')
            {
                withdrawMoney(balance);
            }
            else if (ch == 'E')
            {
                running = false;
            }
        }
    }

    bool okCompanies = saveCompanies("companies.txt", symbols, names, currPrice, companyCount);
    bool okPortfolio = savePortfolio("portfolio.txt", ownerName, balance,
                                     holdSymbols, holdShares, holdCount,
                                     symbols, names, prevPrice, currPrice,
                                     highPrice, lowPrice, companyCount);

    clearScreen();
    cout << "Session closed.\n";
    if (okCompanies)
        cout << "Saved updated prices to companies.txt\n";
    else
        cout << "WARNING: failed to save companies.txt\n";
    if (okPortfolio)
        cout << "Saved formatted portfolio to portfolio.txt\n";
    else
        cout << "WARNING: failed to save portfolio.txt\n";

    freeHoldings(holdSymbols, holdShares, holdCapacity);

    cout << "Press any key to exit...";
    _getch();
    return 0;
}
