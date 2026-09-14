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

/* ---------- Console helpers (state passed in via HANDLE) ---------- */

void clearScreen()
{
    system("cls");
}

void setColor(HANDLE hConsole, WORD attributes)
{
    SetConsoleTextAttribute(hConsole, attributes);
}

void resetColor(HANDLE hConsole)
{
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
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

void enableUtf8Console()
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
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

    for (int i = 0; i < companyCount; i++)
    {
        fout << symbols[i] << "," << names[i] << ","
             << fixed << setprecision(2) << currPrice[i] << "\n";
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

/* Random price update: integer + fractional change, hard-capped at +/-15% of session start */
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

        int intDelta = (rand() % 7) - 3;          /* -3 .. +3 */
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
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "========================================================================================\n";
    cout << "                        KARACHI STOCK MARKET (LIVE)\n";
    cout << "========================================================================================\n";
    resetColor(hConsole);

    cout << "Show updates: Enter   | Portfolio: P | Add Stock: A | Remove: R | Add Money: M | Exit: E\n";
    cout << "Tip: Cash starts at Rs. 0 — press M to add money, then A to buy shares.\n";
    cout << "----------------------------------------------------------------------------------------\n";

    cout << left
         << setw(8) << "Stocks"
         << setw(36) << "Company Name"
         << right
         << setw(10) << "Prev"
         << setw(10) << "Curr"
         << setw(4) << " "
         << setw(10) << "High"
         << setw(10) << "Low" << endl;
    cout << "----------------------------------------------------------------------------------------\n";

    for (int i = 0; i < companyCount; i++)
    {
        cout << left << setw(8) << symbols[i]
             << setw(36) << names[i]
             << right << fixed << setprecision(2)
             << setw(10) << prevPrice[i]
             << setw(10) << currPrice[i];

        cout << " ";
        if (currPrice[i] > prevPrice[i] + 0.0001)
        {
            setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            cout << "\xE2\x86\x91"; /* UTF-8 ↑ */
        }
        else if (currPrice[i] < prevPrice[i] - 0.0001)
        {
            setColor(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            cout << "\xE2\x86\x93"; /* UTF-8 ↓ */
        }
        else
        {
            cout << " ";
        }
        resetColor(hConsole);

        cout << right << fixed << setprecision(2)
             << setw(10) << highPrice[i]
             << setw(10) << lowPrice[i] << endl;
    }

    int adv = 0, dec = 0;
    findTopAdvancerDecliner(pctChange, companyCount, adv, dec);

    cout << "----------------------------------------------------------------------------------------\n";
    cout << "Total shares traded today : " << totalSharesTraded << endl;
    cout << "Top % advancer symbol     : " << symbols[adv]
         << "  (" << fixed << setprecision(2) << pctChange[adv] << "%)" << endl;
    cout << "Top % decliner symbol     : " << symbols[dec]
         << "  (" << fixed << setprecision(2) << pctChange[dec] << "%)" << endl;
    cout << "Note: Price moves are hard-capped at +/-15% of session-start price.\n";
    cout << "========================================================================================\n";
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
    setColor(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    cout << "========================================================================================\n";
    cout << "                     PORTFOLIO OWNER: " << ownerName << " (LIVE)\n";
    cout << "========================================================================================\n";
    resetColor(hConsole);

    cout << "Updates: Enter | Live Market: L | Add: A | Remove: R | Money: M | Withdraw: W\n";
    cout << "----------------------------------------------------------------------------------------\n";

    cout << left
         << setw(8) << "Stocks"
         << setw(28) << "Company Name"
         << right
         << setw(8) << "Shares"
         << setw(10) << "Current"
         << setw(10) << "Previous"
         << setw(12) << "Gain/Loss"
         << setw(10) << "High"
         << setw(10) << "Low" << endl;
    cout << "----------------------------------------------------------------------------------------\n";

    double todayGL = 0.0;

    if (holdCount == 0)
    {
        cout << "  (No holdings yet — cash may be Rs. 0)\n";
        cout << "  Step 1: press M and enter amount (e.g. 500000)\n";
        cout << "  Step 2: press A, type a symbol (e.g. PSO), then share quantity\n";
    }

    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;

        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];
        todayGL += gl;

        cout << left << setw(8) << symbols[idx]
             << setw(28) << names[idx]
             << right << setw(8) << holdShares[h]
             << fixed << setprecision(2)
             << setw(10) << currPrice[idx]
             << setw(10) << prevPrice[idx]
             << setw(1) << " ";
        printGainLossColored(hConsole, gl);
        cout << right << fixed << setprecision(2)
             << setw(10) << highPrice[idx]
             << setw(10) << lowPrice[idx] << endl;
    }

    double previousBalance = balance;
    double newBalance = balance + todayGL;

    cout << "----------------------------------------------------------------------------------------\n";
    cout << "Today's Gain or Loss (Rs.) : ";
    printGainLossColored(hConsole, todayGL);
    cout << endl;
    cout << "Previous Balance (Rs.)     : " << fixed << setprecision(2) << previousBalance << endl;
    cout << "New Balance (Rs.)          : " << fixed << setprecision(2) << newBalance << endl;
    cout << "Cash available             : " << fixed << setprecision(2) << balance << endl;
    cout << "========================================================================================\n";
}

/* ---------- Transactions ---------- */

void addMoney(double &balance)
{
    cout << "\nEnter amount to add (Rs.): ";
    double amount;
    cin >> amount;
    if (cin.fail() || amount <= 0)
    {
        cin.clear();
        cin.ignore(10000, '\n');
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
    cout << "\nEnter amount to withdraw (Rs.): ";
    double amount;
    cin >> amount;
    if (cin.fail() || amount <= 0)
    {
        cin.clear();
        cin.ignore(10000, '\n');
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
    cout << "\nEnter stock symbol to BUY: ";
    cin >> symbol;

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
    cout << "Enter number of shares to buy: ";
    cin >> shares;

    if (cin.fail() || shares <= 0)
    {
        cin.clear();
        cin.ignore(10000, '\n');
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
    cout << "\nEnter stock symbol to SELL: ";
    cin >> symbol;

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
    cout << "You own " << holdShares[hIdx] << " shares. Enter shares to sell: ";
    cin >> shares;

    if (cin.fail() || shares <= 0)
    {
        cin.clear();
        cin.ignore(10000, '\n');
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

    fout << setfill('*') << setw(100) << "" << setfill(' ') << "\n";
    fout << "Portfolio owner: " << ownerName << "\n\n";
    fout << left
         << setw(8) << "Stocks"
         << setw(28) << "Company Name"
         << right
         << setw(8) << "shares"
         << setw(12) << "Close"
         << setw(12) << "Previous"
         << setw(12) << "Gain/Loss"
         << setw(12) << "High"
         << setw(12) << "Low" << "\n";
    fout << setfill('*') << setw(100) << "" << setfill(' ') << "\n";

    for (int h = 0; h < holdCount; h++)
    {
        int idx = findSymbolIndex(symbols, companyCount, holdSymbols[h]);
        if (idx < 0)
            continue;
        double gl = (currPrice[idx] - prevPrice[idx]) * holdShares[h];

        fout << left << setw(8) << symbols[idx]
             << setw(28) << names[idx]
             << right << setw(8) << holdShares[h]
             << fixed << setprecision(2)
             << setw(12) << currPrice[idx]
             << setw(12) << prevPrice[idx];

        fout << setw(12);
        if (gl >= 0)
            fout << ("+" + to_string((long long)llround(gl)));
        else
            fout << ((long long)llround(gl));

        fout << fixed << setprecision(2)
             << setw(12) << highPrice[idx]
             << setw(12) << lowPrice[idx] << "\n";
    }

    fout << setfill('*') << setw(100) << "" << setfill(' ') << "\n";
    fout << left << setw(32) << "Today's Gain or Loss (Rs.)"
         << "*\t" << showpos << fixed << setprecision(2) << todayGL
         << noshowpos << "\t*" << "\n";
    fout << left << setw(32) << "Previous Balance (Rs.)"
         << "*\t" << showpos << fixed << setprecision(2) << previousBalance
         << noshowpos << "\t*" << "\n";
    fout << left << setw(32) << "New Balance (Rs.)"
         << "*\t" << showpos << fixed << setprecision(2) << newBalance
         << noshowpos << "\t*" << "\n";
    fout << setfill('*') << setw(50) << "" << setfill(' ') << "\n";

    fout.close();
    return true;
}

void ensureOwnerName(char ownerName[])
{
    if (ownerName[0] == '\0' || strcmp(ownerName, "(empty)") == 0)
    {
        cout << "\nEnter portfolio owner name: ";
        cin.ignore(10000, '\n');
        cin.getline(ownerName, OWNER_LEN);
        if (ownerName[0] == '\0')
            strncpy(ownerName, "Investor", OWNER_LEN - 1);
    }
}

/* ---------- Main ---------- */

int main()
{
    enableUtf8Console();
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
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
        cout << "No companies loaded. Place companies.txt next to the executable.\n";
        cout << "Press any key to exit...";
        _getch();
        return 1;
    }

    cout << "Loaded " << companyCount << " companies from companies.txt\n";
    cout << "Session-start prices locked for +/-15% cap.\n";
    cout << "Press any key to open Live Market...";
    _getch();

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
