#include <iostream>
#include <string.h> 
#include <chrono>

using namespace std;

const int BOARD_SIZE = 8;
int MAX_DEPTH;
int TIME_LIMIT_MS;

char board[BOARD_SIZE][BOARD_SIZE];
int slotsLeft = BOARD_SIZE * BOARD_SIZE;
bool isHumanMove;

struct StateAttributes {
    int computerStreaks[4] = { 0,0,0,0 }; // # of possible solutions that are len 0, 1, 2, 3  *away* from happening
    int humanStreaks[4] = { 0,0,0,0 };
    int computerCriticalPairs = 0;
    int humanCriticalPairs = 0;
};

int minValue(int depth, int alpha, int beta, StateAttributes& oldAttributes);
int maxValue(int depth, int alpha, int beta, StateAttributes& oldAttributes);
void minimax(int depth, int& resultRow, int& resultCol);

void printBoard() {
    cout << "\n  1 2 3 4 5 6 7 8\n";
    for (int i = 0; i < BOARD_SIZE; i++) {
        cout << (char)('A' + i) << " ";
        for (int j = 0; j < BOARD_SIZE; j++) {
            cout << board[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void humanMove()
{
    int r, c;
    // loop until good input is found. loop is broken when valid input is recieved.
    while (true) {
        cout << "Choose your next move: ";
        char row;
        char col;

        cin >> row;
        cin >> col;

        row = toupper(row);

        // turn chars into integer row/col
        r = (row - 'A');
        c = (col - '1');

        if (r < 0 || r > 7 || c < 0 || c > 7)
            cout << "\nInvalid input" << endl;
        else if (board[r][c] != '-')
            cout << "\nMove already taken!" << endl;
        else
            break;

        cin.ignore(256, '\n'); // remove (up to 256) extra, unread characters, until the newline
    }

    board[r][c] = 'O';
    slotsLeft -= 1;
}


void saveHorizontalStreak(char symbol, int r, int startC, int len, StateAttributes& currAttributes) {
    int* arr;
    int* criticalPairs;
    if (symbol == 'X')
    {
        arr = currAttributes.computerStreaks;
        criticalPairs = &currAttributes.computerCriticalPairs;
    }
    else if (symbol == 'O')
    {
        arr = currAttributes.humanStreaks;
        criticalPairs = &currAttributes.humanCriticalPairs;
    }
    else
        return;

    int distFromSolution = 4 - len;

    if (distFromSolution <= 0)
        arr[0]++;
    else {
        int old2 = arr[2];
        int expandC;
        // explore left to find more opportunity.  EMPTY LOOP BODY. 
        // backwards can expand only into empty space, and not itself, in the situation - X - - X, to avoid double counting.
        int movesToWin = distFromSolution;
        for (expandC = startC - 1; (startC - expandC) <= distFromSolution && expandC >= 0 && (board[r][expandC] == '-' || board[r][expandC] == symbol); expandC--)
            if (board[r][expandC] == symbol)
                movesToWin -= 1;
        if ((startC - expandC) > distFromSolution)
            arr[movesToWin]++;

        movesToWin = distFromSolution;
        // explore right to find more opportunity.  EMPTY LOOP BODY. forward can expand into empty space OR into itself in the situation - X - - X
        for (expandC = startC + len; (expandC - startC) <= distFromSolution && expandC < BOARD_SIZE && (board[r][expandC] == '-' || board[r][expandC] == symbol); expandC++)
            if (board[r][expandC] == symbol)
                movesToWin -= 1;
        if ((expandC - startC) > distFromSolution && board[r][startC + 3] != symbol)  // exclude the case where the last character is already set, to avoid double counting
            arr[movesToWin]++;

        if (arr[2] - old2 == 2)
            *criticalPairs += 1;
    }
}

void saveVerticalStreak(char symbol, int startR, int c, int len, StateAttributes& currAttributes) {
    int* arr;
    int* criticalPairs;
    if (symbol == 'X')
    {
        arr = currAttributes.computerStreaks;
        criticalPairs = &currAttributes.computerCriticalPairs;
    }
    else if (symbol == 'O')
    {
        arr = currAttributes.humanStreaks;
        criticalPairs = &currAttributes.humanCriticalPairs;
    }
    else
        return;

    int distFromSolution = 4 - len;

    if (distFromSolution <= 0)
        arr[0]++;
    else {
        int old2 = arr[2];
        int expandR;
        // explore up to find more opportunity.  EMPTY LOOP BODY. 
        // backwards can expand only into empty space, and not itself, in the situation - X - - X, to avoid double counting.
        int movesToWin = distFromSolution;
        for (expandR = startR - 1; (startR - expandR) <= distFromSolution && expandR >= 0 && (board[expandR][c] == '-' || board[expandR][c] == symbol); expandR--)
            if (board[expandR][c] == symbol)
                movesToWin -= 1;
        if ((startR - expandR) > distFromSolution)
            arr[movesToWin]++;

        movesToWin = distFromSolution;
        // explore down to find more opportunity.  EMPTY LOOP BODY. forward can expand into empty space OR into itself in the situation - X - - X
        for (expandR = startR + len; (expandR - startR) <= distFromSolution && expandR < BOARD_SIZE && (board[expandR][c] == '-' || board[expandR][c] == symbol); expandR++)
            if (board[expandR][c] == symbol)
                movesToWin -= 1;
        if ((expandR - startR) > distFromSolution && board[startR + 3][c] != symbol)  // exclude the case where the last character is already set, to avoid double counting
            arr[movesToWin]++;

        if (arr[2] - old2 == 2)
            *criticalPairs += 1;
    }
}

int evaluate(int depth, StateAttributes& oldAttributes, StateAttributes& currAttributes) {

    // check for horizontal winner
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        //int prefixSpace
        char lastSymbol = board[r][0];
        int length = 1;
        int startC = 0;
        for (int c = 1; c < BOARD_SIZE; c++)
        {
            char symbol = board[r][c];
            if (symbol != lastSymbol)
            {
                saveHorizontalStreak(lastSymbol, r, startC, length, currAttributes);
                lastSymbol = symbol;
                length = 1;
                startC = c;
            }
            else
            {
                length++;
            }
        }
        saveHorizontalStreak(lastSymbol, r, startC, length, currAttributes);
    }

    // check for vertical winner
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        char lastSymbol = board[0][c];
        int length = 1;
        int startR = 0;
        for (int r = 1; r < BOARD_SIZE; r++)
        {
            char symbol = board[r][c];
            if (symbol != lastSymbol)
            {
                saveVerticalStreak(lastSymbol, startR, c, length, currAttributes);
                lastSymbol = symbol;
                length = 1;
                startR = r;
            }
            else
            {
                length++;
            }
        }
        saveVerticalStreak(lastSymbol, startR, c, length, currAttributes);
    }

    int* computerStreaks = currAttributes.computerStreaks;
    int* humanStreaks = currAttributes.humanStreaks;

    int result = 0;
    int computerScore = 0;
    int humanScore = 0;
    computerScore += computerStreaks[0] * 1000000 * (MAX_DEPTH - depth + 1); // Prioritizes shallower wins. We'd rather win SOONER than later.
    humanScore += humanStreaks[0] * 1000000 * (MAX_DEPTH - depth + 1);  // Gives shallower losses a more negative score. We'd rather lose as late as possible.

    computerScore += computerStreaks[1] * 40; // 1 away
    humanScore += humanStreaks[1] * 40;

    computerScore += computerStreaks[2] * 20; // 2 away
    humanScore += humanStreaks[2] * 20;

    computerScore += computerStreaks[3] * 10; // 3 away
    humanScore += humanStreaks[3] * 10;

    if (currAttributes.computerCriticalPairs >= 2 || (currAttributes.computerCriticalPairs == 1 && computerStreaks[1] >= 1) || computerStreaks[1] >= 2 || (computerStreaks[1] == 1 && !isHumanMove))
        computerScore += 500000;  // INCOMING GAURANTEED WIN 

    if (currAttributes.humanCriticalPairs >= 2 || (currAttributes.humanCriticalPairs == 1 && humanStreaks[1] >= 1) || humanStreaks[1] >= 2 || (humanStreaks[1] == 1 && isHumanMove))
        humanScore += 500000;  // INCOMING GAURANTEED WIN 

    if (isHumanMove)
        humanScore *= 1.1;
    else
        computerScore *= 1.1;

    return computerScore - humanScore;
}

char checkForWinner() {
    // check for horizontal winner
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        char lastSymbol = board[r][0];
        int length = 1;
        for (int c = 1; c < BOARD_SIZE; c++)
        {
            char symbol = board[r][c];
            if (symbol != lastSymbol)
            {
                lastSymbol = symbol;
                length = 1;
            }
            else
            {
                length++;
                if (symbol != '-' && length >= 4)
                    return symbol;
            }
        }
    }

    // check for vertical winner
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        char lastSymbol = board[0][c];
        int length = 1;
        for (int r = 1; r < BOARD_SIZE; r++)
        {
            char symbol = board[r][c];
            if (symbol != lastSymbol)
            {
                lastSymbol = symbol;
                length = 1;
            }
            else
            {
                length++;
                if (symbol != '-' && length >= 4)
                    return symbol;
            }
        }
    }
    return '-';
}

chrono::high_resolution_clock::time_point endTime;
bool timeLimitExceeded; // keep track of whether time limit was exceeded by inner functions in order to validate the results for IDS.
bool checkTimeLimit()
{
    if (chrono::high_resolution_clock::now() > endTime)
    {
        timeLimitExceeded = true;
        return false;
    }
    return true;
}

void computerMove()
{
    cout << "Computer thinking... " << endl;
    endTime = chrono::high_resolution_clock::now() + chrono::milliseconds(TIME_LIMIT_MS);
    timeLimitExceeded = false;
    //isHumanMove = false;

    int depth = 1;
    int resultRow = -1;
    int resultCol = -1;
    while (true)           // Iterative Deepening Search
    {
        int row, col;
        minimax(depth, row, col);
        if (timeLimitExceeded)
            break;

        resultRow = row;
        resultCol = col;
        depth += 1;
    }

    board[resultRow][resultCol] = 'X';
    cout << "Selected " << (char)('A' + resultRow) << resultCol + 1 << " with depth " << depth - 1;
    slotsLeft--;
}

void minimax(int depth, int& resultRow, int& resultCol)
{
    StateAttributes currAttributes;  // make struct on the stack.
    MAX_DEPTH = depth;
    int best = -99999999;
    int alpha = -99999999;
    int beta = 99999999;

    int bestR, bestC;
    bestR = bestC = -1;
    isHumanMove = true;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE && checkTimeLimit(); c++) {
            if (board[r][c] == '-') { // free slot
                board[r][c] = 'X';
                slotsLeft--;
                int score = minValue(1, alpha, beta, currAttributes);
                board[r][c] = '-'; // reset to old value
                slotsLeft++;
                if (score > best)
                {
                    best = score;
                    bestR = r;
                    bestC = c;
                }
                alpha = max(alpha, best);
            }
        }
    }
    //isHumanMove = !isHumanMove;
    resultRow = bestR;
    resultCol = bestC;
}

int minValue(int depth, int alpha, int beta, StateAttributes& oldAttributes)
{
    StateAttributes currAttributes;  // make struct on the stack.
    int best;
    int evaluation = evaluate(depth, oldAttributes, currAttributes);//valueOf(checkForWinner());
    if (abs(evaluation) > 1000000 || depth == MAX_DEPTH)  // if theres a winner, OR if we have reached the max depth
        return evaluation;
    else if (slotsLeft == 0)
        return 0;  // return 0 if its a tie

    best = 99999999;
    isHumanMove = !isHumanMove;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE && checkTimeLimit(); c++) {
            if (board[r][c] == '-') { // free slot
                board[r][c] = 'O';
                slotsLeft--;
                int score = maxValue(depth + 1, alpha, beta, currAttributes);
                board[r][c] = '-'; // reset to old value
                slotsLeft++;
                best = min(best, score);
                if (best <= alpha)
                    return best;
                beta = min(beta, best);
            }
        }
    }
    isHumanMove = !isHumanMove;
    return best;
}

int maxValue(int depth, int alpha, int beta, StateAttributes& oldAttributes)
{
    StateAttributes currAttributes;  // make struct on the stack.
    int best;
    int evaluation = evaluate(depth, oldAttributes, currAttributes);//valueOf(checkForWinner());
    if (abs(evaluation) > 1000000 || depth == MAX_DEPTH)  // if theres a winner, OR if we have reached the max depth
        return evaluation;
    else if (slotsLeft == 0)
        return 0;  // return 0 if its a tie

    best = -99999999;
    isHumanMove = !isHumanMove;
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE && checkTimeLimit(); c++) {
            if (board[r][c] == '-') { // free slot.  THIS is each successor
                board[r][c] = 'X';
                slotsLeft--;
                int score = minValue(depth + 1, alpha, beta, currAttributes);
                board[r][c] = '-'; // reset to old value
                slotsLeft++;
                best = max(best, score);
                if (best >= beta)
                    return best;
                alpha = max(alpha, best);
            }
        }
    }
    isHumanMove = !isHumanMove;
    return best;
}

void checkGameOver() {
    char winner = checkForWinner();

    if (winner == 'O')
    {
        cout << "YOU WIN!" << endl;
        exit(0);
    }
    else if (winner == 'X')
    {
        cout << "YOU LOSE!" << endl;
        exit(0);
    }
    else if (slotsLeft == 0) // check for tie
    {
        cout << "TIE!" << endl;
        exit(0);
    }
}

int main(int argc, char const* argv[]) {
    memset(&board, '-', sizeof(board));  // initialize board to all '-' characters.

    bool humanMovesFirst;
    while (true) {
        cout << "CS 4200 Project 3 - Johnathan Mitri" << endl << endl;
        cout << "Would you like to go first? (y/n): ";
        char choice;
        cin >> choice;
        cin.ignore(256, '\n'); // remove (up to 256) extra, unread characters, until the newline

        if (choice == 'y' || choice == 'Y') {
            humanMovesFirst = true;
            break;
        }
        else if (choice == 'n' || choice == 'N') {
            humanMovesFirst = false;
            break;
        }
        else {
            cout << "Invalid Input." << endl;
        }
    }

    cout << "How long should the computer think in seconds? ";
    double choice;
    cin >> choice;
    TIME_LIMIT_MS = (int)(choice * 1000);


    if (humanMovesFirst)
    {
        printBoard();
        humanMove();
        printBoard();
    }

    while (true) {
        computerMove();
        printBoard();
        checkGameOver();

        humanMove();
        printBoard();
        checkGameOver();
    }

    return 0;
}
