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

using namespace std;

#define MAX_COMPANIES 50
#define SYM_LEN 16
#define NAME_LEN 64
#define OWNER_LEN 64

/* Fixed column widths — slightly compact so full table fits on screen. */
#define COL_GAP 1
#define COL_SYM 7
#define COL_NAME 22
#define COL_SHARES 10
#define COL_PRICE 14
#define COL_GL 12
#define PORTFOLIO_COLS 8
#define PORTFOLIO_LINE_WIDTH (COL_SYM + COL_NAME + COL_SHARES + COL_PRICE * 4 + COL_GL + COL_GAP * (PORTFOLIO_COLS - 1))

/* Live market board columns */
#define MKT_SYM 7
#define MKT_NAME 24
#define MKT_NUM 14
#define MKT_CHG 5
#define MKT_COLS 7
#define MKT_LINE_WIDTH (MKT_SYM + MKT_NAME + MKT_NUM * 4 + MKT_CHG + COL_GAP * (MKT_COLS - 1))

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
        t = t.substr(0, (size_t)width);
    if ((int)t.size() < width)
        t.insert(0, static_cast<size_t>(width - (int)t.size()), ' ');
    return t;
}

void printCellLeft(ostream &out, const char *s, int width)
{
    out << cellLeft(s, width);
}

void printCellRightText(ostream &out, const string &text, int width)
{
    out << cellRight(text, width);
}

/** Open spacing between columns (not congested). */
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
    ostringstream cell;
    cell << fixed << setprecision(2);
    if (showPlus && value > 0.0001)
        cell << "+";
    cell << value;
    out << cellRight(cell.str(), width);
}

void printGainLossFixed(HANDLE hConsole, double value, int width)
{
    ostringstream cell;
    cell << fixed << setprecision(2);
    if (value > 0.0001)
        cell << "+" << value;
    else
        cell << value;
    string t = cellRight(cell.str(), width);

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
    printRuleLine(cout, '=', MKT_LINE_WIDTH);
    cout << "                        KARACHI STOCK MARKET (LIVE)\n";
    printRuleLine(cout, '=', MKT_LINE_WIDTH);
    setAccentYellow(hConsole);
    cout << "Show updates: Enter   | Portfolio: P | Add Stock: A | Remove: R | Add Money: M | Exit: E\n";
    cout << "Tip: Press P to open Portfolio (create new or continue existing). Cash starts at Rs. 0.\n";
    cout << "Change: \"\xE2\x86\x91\" = up (rise)   \"\xE2\x86\x93\" = down (fall)   \"=\" = unchanged\n";
    resetColor(hConsole);
    cout << "\n";
    printRuleLine(cout, '-', MKT_LINE_WIDTH);

    setAccentCyan(hConsole);
    printCellLeft(cout, "Symbol", MKT_SYM);
    printColGap(cout);
    printCellLeft(cout, "Company Name", MKT_NAME);
    printColGap(cout);
    printCellRightText(cout, "Previous Price", MKT_NUM);
    printColGap(cout);
    printCellRightText(cout, "Current Price", MKT_NUM);
    printColGap(cout);
    printCellRightText(cout, "Change", MKT_CHG);
    printColGap(cout);
    printCellRightText(cout, "Highest Price", MKT_NUM);
    printColGap(cout);
    printCellRightText(cout, "Lowest Price", MKT_NUM);
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
    printRuleLine(cout, '=', PORTFOLIO_LINE_WIDTH);
    cout << "                     PORTFOLIO OWNER: " << ownerName << " (LIVE)\n";
    printRuleLine(cout, '=', PORTFOLIO_LINE_WIDTH);
    setAccentYellow(hConsole);
    cout << "Owner name on this portfolio: " << ownerName << "\n";
    cout << "Updates: Enter | Live Market: L | Add: A | Remove: R | Money: M | Withdraw: W\n";
    resetColor(hConsole);
    cout << "\n";
    printRuleLine(cout, '-', PORTFOLIO_LINE_WIDTH);

    setAccentCyan(hConsole);
    printCellLeft(cout, "Symbol", COL_SYM);
    printColGap(cout);
    printCellLeft(cout, "Company Name", COL_NAME);
    printColGap(cout);
    printCellRightText(cout, "Share Qty", COL_SHARES);
    printColGap(cout);
    printCellRightText(cout, "Current Price", COL_PRICE);
    printColGap(cout);
    printCellRightText(cout, "Previous Price", COL_PRICE);
    printColGap(cout);
    printCellRightText(cout, "Gain / Loss", COL_GL);
    printColGap(cout);
    printCellRightText(cout, "Highest Price", COL_PRICE);
    printColGap(cout);
    printCellRightText(cout, "Lowest Price", COL_PRICE);
    cout << endl;
    resetColor(hConsole);
    printRuleLine(cout, '-', PORTFOLIO_LINE_WIDTH);
    cout << "\n";

    double todayGL = 0.0;

    if (holdCount == 0)
    {
        cout << "  (No holdings yet — cash may be Rs. 0)\n";
        cout << "  Step 1: press M and enter amount (e.g. 500000)  |  0 = cancel any form\n";
        cout << "  Step 2: press A, type a symbol (e.g. PSO), then share quantity\n\n";
    }

    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;

        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];
        todayGL += gl;

        printCellLeft(cout, symbols[idx], COL_SYM);
        printColGap(cout);
        printCellLeft(cout, names[idx], COL_NAME);
        printColGap(cout);
        {
            ostringstream sh;
            sh << holdShares[h];
            printCellRightText(cout, sh.str(), COL_SHARES);
        }
        printColGap(cout);
        printMoneyCell(cout, currPrice[idx], COL_PRICE, false);
        printColGap(cout);
        printMoneyCell(cout, prevPrice[idx], COL_PRICE, false);
        printColGap(cout);
        printGainLossFixed(hConsole, gl, COL_GL);
        printColGap(cout);
        printMoneyCell(cout, highPrice[idx], COL_PRICE, false);
        printColGap(cout);
        printMoneyCell(cout, lowPrice[idx], COL_PRICE, false);
        cout << endl;
    }

    double previousBalance = balance;
    double newBalance = balance + todayGL;

    cout << "\n";
    printRuleLine(cout, '-', PORTFOLIO_LINE_WIDTH);
    const int labW = 28;
    const int valW = 14;
    {
        ostringstream v;
        cout << cellLeft("Today's Gain or Loss (Rs.)", labW) << " : ";
        printGainLossFixed(hConsole, todayGL, valW);
        cout << endl;
        resetColor(hConsole);
        v.str("");
        v.clear();
        v << fixed << setprecision(2) << previousBalance;
        cout << cellLeft("Previous Balance (Rs.)", labW) << " : " << cellRight(v.str(), valW) << endl;
        v.str("");
        v.clear();
        v << fixed << setprecision(2) << newBalance;
        setAccentYellow(hConsole);
        cout << cellLeft("New Balance (Rs.)", labW) << " : " << cellRight(v.str(), valW) << endl;
        resetColor(hConsole);
        v.str("");
        v.clear();
        v << fixed << setprecision(2) << balance;
        cout << cellLeft("Cash available (Rs.)", labW) << " : " << cellRight(v.str(), valW) << endl;
    }
    setAccentCyan(hConsole);
    printRuleLine(cout, '=', PORTFOLIO_LINE_WIDTH);
    resetColor(hConsole);
    setAccentYellow(hConsole);
    cout << "Enter your choice: ";
    resetColor(hConsole);
    cout.flush();
}

/* ---------- Transactions ---------- */

void addMoney(double &balance)
{
    cout << "\n----- ADD MONEY -----\n";
    cout << "Enter the amount to add in Rs.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Amount: ";
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
    cout << "\n----- WITHDRAW MONEY -----\n";
    cout << "Enter the amount to withdraw in Rs.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Amount: ";
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

    cout << "\n----- BUY STOCK -----\n";
    cout << "Available symbols: ";
    for (int i = 0; i < companyCount; i++)
    {
        cout << symbols[i];
        if (i + 1 < companyCount)
            cout << ", ";
    }
    cout << "\nEnter the stock symbol you want to BUY.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Symbol: ";
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
    cout << "Enter how many shares to buy.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Shares: ";
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

    cout << "\n----- SELL STOCK -----\n";
    cout << "Your holdings: ";
    for (int i = 0; i < holdCount; i++)
    {
        cout << holdSymbols[i] << "(" << holdShares[i] << ")";
        if (i + 1 < holdCount)
            cout << ", ";
    }
    cout << "\nEnter the stock symbol you want to SELL.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Symbol: ";
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
    cout << "You own " << holdShares[hIdx] << " shares.\n";
    cout << "Enter how many shares to sell.\n";
    cout << "Enter 0 to cancel and go back.\n";
    cout << "Shares: ";
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
    out << cellRight(cell.str(), width);
}

void writePortfolioSeparator(ostream &out)
{
    printRuleLine(out, '*', PORTFOLIO_LINE_WIDTH);
}

void writePortfolioHeaderRow(ostream &out)
{
    printCellLeft(out, "Symbol", COL_SYM);
    printColGap(out);
    printCellLeft(out, "Company Name", COL_NAME);
    printColGap(out);
    printCellRightText(out, "Share Qty", COL_SHARES);
    printColGap(out);
    printCellRightText(out, "Current Price", COL_PRICE);
    printColGap(out);
    printCellRightText(out, "Previous Price", COL_PRICE);
    printColGap(out);
    printCellRightText(out, "Gain / Loss", COL_GL);
    printColGap(out);
    printCellRightText(out, "Highest Price", COL_PRICE);
    printColGap(out);
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
    printColGap(out);
    printCellLeft(out, companyName, COL_NAME);
    printColGap(out);
    {
        ostringstream sh;
        sh << shares;
        printCellRightText(out, sh.str(), COL_SHARES);
    }
    printColGap(out);
    printMoneyCell(out, closePrice, COL_PRICE, false);
    printColGap(out);
    printMoneyCell(out, previousPrice, COL_PRICE, false);
    printColGap(out);
    writeAlignedMoney(out, gainLoss, COL_GL, true);
    printColGap(out);
    printMoneyCell(out, high, COL_PRICE, false);
    printColGap(out);
    printMoneyCell(out, low, COL_PRICE, false);
    out << "\n";
}

void writePortfolioFooterLine(ostream &out, const char *label, double value)
{
    const int labW = 28;
    const int valW = 14;
    ostringstream v;
    v << fixed << setprecision(2);
    if (value > 0.0001)
        v << "+";
    v << value;
    out << cellLeft(label, labW) << " : " << cellRight(v.str(), valW) << "\n";
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
    fout << "Portfolio owner: " << ownerName << "\n";
    fout << "Cash available (Rs.): " << fixed << setprecision(2) << balance << "\n\n";
    writePortfolioHeaderRow(fout);
    writePortfolioSeparator(fout);
    fout << "\n";

    if (holdCount == 0)
    {
        fout << "(No holdings in this session)\n\n";
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

    fout << "\n";
    writePortfolioSeparator(fout);
    writePortfolioFooterLine(fout, "Today's Gain or Loss (Rs.)", todayGL);
    writePortfolioFooterLine(fout, "Previous Balance (Rs.)", previousBalance);
    writePortfolioFooterLine(fout, "New Balance (Rs.)", newBalance);
    writePortfolioFooterLine(fout, "Cash available (Rs.)", balance);
    writePortfolioSeparator(fout);

    fout.close();
    return true;
}

void prepareTextInput()
{
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    cin.clear();
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
        const string key = "Portfolio owner:";
        size_t pos = line.find(key);
        if (pos != string::npos)
        {
            string name = trimString(line.substr(pos + key.size()));
            strncpy(ownerOut, name.c_str(), OWNER_LEN - 1);
            ownerOut[OWNER_LEN - 1] = '\0';
            return ownerOut[0] != '\0';
        }
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

    string line;
    while (getline(in, line))
    {
        string t = trimString(line);
        if (t.empty())
            continue;

        if (t.find("Portfolio owner:") == 0)
        {
            string name = trimString(t.substr(string("Portfolio owner:").size()));
            strncpy(ownerName, name.c_str(), OWNER_LEN - 1);
            ownerName[OWNER_LEN - 1] = '\0';
            continue;
        }

        if (t.find("Cash available (Rs.):") == 0)
        {
            string num = trimString(t.substr(string("Cash available (Rs.):").size()));
            balance = atof(num.c_str());
            continue;
        }

        /* Skip separators / headers / footers */
        if (t[0] == '*' || t.find("Symbol") == 0 || t.find("Company Name") != string::npos)
            continue;
        if (t.find("Today's Gain") != string::npos || t.find("Previous Balance") != string::npos ||
            t.find("New Balance") != string::npos || t.find("No holdings") != string::npos)
            continue;

        /* Holding row: starts with a known market symbol */
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
        int sharePos = COL_SYM + COL_GAP + COL_NAME + COL_GAP;
        if ((int)line.size() >= sharePos + COL_SHARES)
        {
            string shareCell = trimString(line.substr((size_t)sharePos, (size_t)COL_SHARES));
            shares = atoi(shareCell.c_str());
        }
        else
        {
            /* Fallback: 2nd token as shares */
            stringstream ss(t);
            string tmp;
            ss >> tmp >> tmp; /* skip symbol; next may be company words — weaker */
            /* try scan for first integer after symbol */
            for (size_t i = sym.size(); i < t.size(); i++)
            {
                if (isdigit(static_cast<unsigned char>(t[i])))
                {
                    shares = atoi(t.c_str() + i);
                    break;
                }
            }
        }
        if (shares <= 0)
            continue;

        if (holdCount >= holdCapacity)
            expandHoldings(holdSymbols, holdShares, holdCapacity);
        strncpy(holdSymbols[holdCount], symbols[mIdx], SYM_LEN - 1);
        holdSymbols[holdCount][SYM_LEN - 1] = '\0';
        holdShares[holdCount] = shares;
        holdCount++;
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

    /* Session already has a portfolio — still offer continue / new when opening from Market. */
    if (portfolioReady)
    {
        if (!portfolioFileExists(pfFile))
        {
            clearScreen();
            setAccentYellow(hConsole);
            cout << "\nYour portfolio file is not available anymore.\n";
            resetColor(hConsole);
            cout << "Keeping your current session data and saving it again.\n";
            cout << "Owner: " << ownerName << "\n";
            if (!savePortfolio(pfFile, ownerName, balance,
                               holdSymbols, holdShares, holdCount,
                               symbols, names, prevPrice, currPrice,
                               highPrice, lowPrice, companyCount))
            {
                setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                cout << "Could not save portfolio right now. Session data is still in memory.\n";
                resetColor(hConsole);
            }
            else
            {
                setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
                cout << "Portfolio saved again successfully.\n";
                resetColor(hConsole);
            }
            cout << "\nPress any key to continue...";
            _getch();
            return true;
        }

        clearScreen();
        setAccentCyan(hConsole);
        printRuleLine(cout, '=', 72);
        cout << "                         PORTFOLIO\n";
        printRuleLine(cout, '=', 72);
        resetColor(hConsole);
        cout << "\nCurrent session owner: " << ownerName << "\n";
        cout << "\nPlease choose an option:\n";
        cout << "  1) Continue with the current portfolio\n";
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
            return false;
        }
        cin.ignore(10000, '\n');
        if (choice == 0)
            return false;
        if (choice == 1)
        {
            /* Keep memory; refresh file from current session */
            savePortfolio(pfFile, ownerName, balance,
                          holdSymbols, holdShares, holdCount,
                          symbols, names, prevPrice, currPrice,
                          highPrice, lowPrice, companyCount);
            return true;
        }
        if (choice == 2)
        {
            askOwnerNameInteractive(hConsole, ownerName);
            balance = 0.0;
            holdCount = 0;
            savePortfolio(pfFile, ownerName, balance,
                          holdSymbols, holdShares, holdCount,
                          symbols, names, prevPrice, currPrice,
                          highPrice, lowPrice, companyCount);
            return true;
        }
        return false;
    }

    /* First open this session */
    clearScreen();
    setAccentCyan(hConsole);
    printRuleLine(cout, '=', 72);
    cout << "                         PORTFOLIO\n";
    printRuleLine(cout, '=', 72);
    resetColor(hConsole);

    if (portfolioFileExists(pfFile))
    {
        char existingOwner[OWNER_LEN] = "";
        peekPortfolioOwner(pfFile, existingOwner);

        cout << "\nA portfolio is already available.\n";
        if (existingOwner[0] != '\0')
            cout << "Current owner on file: " << existingOwner << "\n";

        cout << "\nPlease choose an option:\n";
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
            cout << "Invalid input. Returning to Live Market.\n";
            cout << "Press any key...";
            _getch();
            return false;
        }
        cin.ignore(10000, '\n');

        if (choice == 0)
        {
            cout << "Cancelled. Returning to Live Market.\n";
            cout << "Press any key...";
            _getch();
            return false;
        }

        if (choice == 1)
        {
            if (!loadPortfolio(pfFile, ownerName, balance,
                               holdSymbols, holdShares, holdCount, holdCapacity,
                               symbols, companyCount))
            {
                setAccentYellow(hConsole);
                cout << "\nCould not read portfolio data. Please create a new one.\n";
                resetColor(hConsole);
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
                setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                cout << "\nCould not save the new portfolio.\n";
                resetColor(hConsole);
                cout << "Press any key...";
                _getch();
                return false;
            }
            setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            cout << "\nNew portfolio ready for owner \"" << ownerName << "\".\n";
            resetColor(hConsole);
            cout << "Press any key to open your portfolio...";
            _getch();
            return true;
        }

        cout << "Invalid choice. Returning to Live Market.\n";
        cout << "Press any key...";
        _getch();
        return false;
    }

    /* No file yet — create with owner name only */
    cout << "\nWelcome! Please set up your portfolio to continue.\n";
    askOwnerNameInteractive(hConsole, ownerName);
    balance = 0.0;
    holdCount = 0;

    if (!savePortfolio(pfFile, ownerName, balance,
                       holdSymbols, holdShares, holdCount,
                       symbols, names, prevPrice, currPrice,
                       highPrice, lowPrice, companyCount))
    {
        setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        cout << "\nCould not save the portfolio. Please try again.\n";
        resetColor(hConsole);
        cout << "Press any key to return to Live Market...";
        _getch();
        return false;
    }

    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    cout << "\nPortfolio ready for owner \"" << ownerName << "\".\n";
    resetColor(hConsole);
    cout << "Press any key to open your portfolio...";
    _getch();
    return true;
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
            else if (ch == 'A')
            {
                addStock(symbols, names, currPrice, companyCount,
                         holdSymbols, holdShares, holdCount, holdCapacity,
                         balance, totalSharesTraded);
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
            }
            else if (ch == 'R')
            {
                removeStock(symbols, names, currPrice, companyCount,
                            holdSymbols, holdShares, holdCount,
                            balance, totalSharesTraded);
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
            }
            else if (ch == 'M')
            {
                addMoney(balance);
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
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
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
                onPortfolio = false;
            }
            else if (ch == 'A')
            {
                addStock(symbols, names, currPrice, companyCount,
                         holdSymbols, holdShares, holdCount, holdCapacity,
                         balance, totalSharesTraded);
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
            }
            else if (ch == 'R')
            {
                removeStock(symbols, names, currPrice, companyCount,
                            holdSymbols, holdShares, holdCount,
                            balance, totalSharesTraded);
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
            }
            else if (ch == 'M')
            {
                addMoney(balance);
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
            }
            else if (ch == 'W')
            {
                withdrawMoney(balance);
                if (portfolioReady)
                    savePortfolio("portfolio.txt", ownerName, balance,
                                  holdSymbols, holdShares, holdCount,
                                  symbols, names, prevPrice, currPrice,
                                  highPrice, lowPrice, companyCount);
            }
            else if (ch == 'E')
            {
                running = false;
            }
        }
    }

    /* Always persist portfolio on exit (creates file if it was never opened). */
    if (strcmp(ownerName, "(empty)") == 0 || ownerName[0] == '\0')
        strncpy(ownerName, "Investor", OWNER_LEN - 1);

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
