// WindowsProject78.cpp : Defines the entry point for the application.
//
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "Shapes.h"
#include "framework.h"

#define ID_TIMER  1
#define TIME_INTERVAL 1000   // Fall time interval 1 second

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID    CALLBACK TimerProc(HWND, UINT, UINT, DWORD);

/*--------------Macro-------------------*/
#define BOARD_WIDTH 180
#define BOARD_HEIGHT 400
#define LONG_SLEEP 300

#define COLS 15  //Number of columns
#define ROWS 30  // Rows
#define EXTENDED_COLS 23  // Including the number of columns that are not displayed
#define EXTENDED_ROWS 34  // Including the number of rows that are not displayed

// The actual grid position of the game interface
#define BOARD_LEFT 4
#define BOARD_RIGHT 18
#define BOARD_TOP 0
#define BOARD_BOTTOM 29
/*-----------------------------------*/


/*-------------Parameter declaration---------------*/
// static int shapes[7][4][4];
static int shape[4][4];
static int score = 0;

static int shape_row = 0;  // The row of the current shape
static int shape_col = EXTENDED_COLS / 2 - 2; // The column of the current shape
static int** gBoard;

static int lattices_top = 40;   // Blank above
static int lattices_left = 20;  // Left blank
static int width = BOARD_WIDTH / COLS;                    //The width of each grid
static int height = (BOARD_HEIGHT - lattices_top) / ROWS; //Height of each grid

static HBRUSH grey_brush = CreateSolidBrush(RGB(210, 210, 210));
static HBRUSH white_brush = CreateSolidBrush(RGB(255, 255, 255));
static HBRUSH red_brush = CreateSolidBrush(RGB(255, 0, 0));
static HBRUSH blue_brush = CreateSolidBrush(RGB(0, 0, 255));
static HPEN hPen = CreatePen(PS_SOLID, 1, RGB(147, 155, 166));

static bool gIsPause = false;  // Determine whether to pause
/*-----------------------------------*/

/*-------------Function declaration---------------*/

void InitGame(HWND);
void InitData();

void TypeInstruction(HWND);

void RandShape();  // Randomly select a shape

void AddScore();   //Add 100 points after clearing a line

void UpdateShapeRect(HWND hwnd); // Update the rectangular area of the falling shape
void UpdateAllBoard(HWND hwnd);  // Update game range

void FallToGround();
void MoveDown(HWND hwnd);   // Drop one bar
void RePaintBoard(HDC hdc); // Redraw the game interface
void PaintCell(HDC hdc, int x, int y, int color); // Draw a grid
void ClearFullLine();       // Empty full line

void RotateShape(HWND hwnd);  //Graphic distortion
void MoveHori(HWND hwnd, int direction);  // Move horizontally
void RotateMatrix();          // Flip the graph counterclockwise
void ReRotateMatrix();        // Flip the graph clockwise
bool IsLegel();               // Check whether the graphics are out of range

void RespondKey(HWND hwnd, WPARAM wParam); // Respond to the button

void PauseGame(HWND hwnd); // Pause the game
void WakeGame(HWND hwnd);  // Continue the game


bool JudgeLose();          // Judge whether to lose
void LoseGame(HWND hwnd);  // Deal with after losing the game
void ExitGame(HWND hwnd);  // game over
/*-----------------------------------*/



// program entry WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT("Tetris");
	HWND         hwnd;
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground =(HBRUSH) (CreateSolidBrush(RGB(255, 255, 0)));
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT(""),
			szAppName, MB_ICONERROR);
		return 0;
	}

	//Here the window is set to be unable to adjust the size and cannot be maximized
	hwnd = CreateWindow(szAppName, TEXT("Tetris"),
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		BOARD_WIDTH + 290, BOARD_HEIGHT + 150,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	// Print instructions
	TypeInstruction(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hdc;
	static HDC hdcBuffer;
	static HBITMAP hBitMap;
	static PAINTSTRUCT ps;

	switch (message)
	{
	case WM_CREATE:
		SetTimer(hwnd, ID_TIMER, TIME_INTERVAL, TimerProc);
		InitGame(hwnd);
		TypeInstruction(hwnd);
		return 0;

		//Need to redraw instructions after minimizing recovery
	case WM_SIZE:
		TypeInstruction(hwnd);
		return 0;

	case WM_KEYDOWN:
		RespondKey(hwnd, wParam);
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		RePaintBoard(hdc);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		KillTimer(hwnd, ID_TIMER);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

// Timer response event
VOID CALLBACK TimerProc(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{
	// The timer moves down every second
	MoveDown(hwnd);
}

// Initialize the game
void InitGame(HWND hwnd) {

	gBoard = new int* [EXTENDED_ROWS];
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		gBoard[i] = new int[EXTENDED_COLS];
	}

	srand(time(0));

	InitData();

	UpdateAllBoard(hwnd);
}

//Initialize game data
void InitData() {

	// Clear the game panel
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		for (int j = 0; j < EXTENDED_COLS; j++) {
			gBoard[i][j] = 0;
		}
	}

	// Fill the periphery with 1, in order to determine whether it is out of range
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		for (int j = 0; j < BOARD_LEFT; j++) {
			gBoard[i][j] = 1;
		}
	}
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		for (int j = BOARD_RIGHT + 1; j < EXTENDED_COLS; j++) {
			gBoard[i][j] = 1;
		}
	}
	for (int i = BOARD_BOTTOM + 1; i < EXTENDED_ROWS; i++) {
		for (int j = 0; j < EXTENDED_COLS; j++) {
			gBoard[i][j] = 1;
		}
	}

	gIsPause = false;

	// Initialize the score
	score = 0;

		// Randomly generate graphics
		RandShape();

	return;
}

// Print instructions
void TypeInstruction(HWND hwnd) {

	TEXTMETRIC  tm;
	int cxChar, cxCaps, cyChar, cxClient, cyClient, iMaxWidth;

	HDC hdc = GetDC(hwnd);

	//Save line height and word width information
	GetTextMetrics(hdc, &tm);
	cxChar = tm.tmAveCharWidth;
	cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;
	cyChar = tm.tmHeight + tm.tmExternalLeading;

	int startX = 180;
	int startY = 40;

	TCHAR Instruction[100];

	//Print instructions
	wsprintf(Instruction, TEXT("INSTRUCTION "));
	TextOut(hdc, startX + 40, startY, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("↑  - Change Shape"));
	TextOut(hdc, startX + 40, startY + cyChar * 3, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("←  - Move Left"));
	TextOut(hdc, startX + 40, startY + cyChar * 5, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("→  - Move Right"));
	TextOut(hdc, startX + 40, startY + cyChar * 7, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("↓  - Move Down"));
	TextOut(hdc, startX + 40, startY + cyChar * 9, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("Space - Pause the game"));
	TextOut(hdc, startX + 40, startY + cyChar * 11, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("Esc  - Exit the game"));
	TextOut(hdc, startX + 40, startY + cyChar * 13, Instruction, lstrlen(Instruction));


	ReleaseDC(hwnd, hdc);
}

//Randomly select a graph
void RandShape() {

	int shape_num = rand() % 7;

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			shape[i][j] = shapes[shape_num][i][j];

}

// Update the entire game interface
void UpdateAllBoard(HWND hwnd) {

	static RECT rect;

	rect.left = lattices_left;
	rect.right = lattices_left + COLS * width + width;
	rect.top = lattices_top - 30;
	rect.bottom = lattices_top + ROWS * height;

	// This rectangle includes the game interface grid, but does not include the description on the right
	InvalidateRect(hwnd, &rect, false);

}

// Update the rectangular area where the falling shape is located
void UpdateShapeRect(HWND hwnd) {

	static RECT rect;

	rect.left = lattices_left;
	rect.right = lattices_left + COLS * width + width;
	rect.top = lattices_top + (shape_row - 1) * height;
	rect.bottom = lattices_top + (shape_row + 4) * height;

	InvalidateRect(hwnd, &rect, false);
}

// Redraw the game interface
void RePaintBoard(HDC hdc) {

	SetBkColor(hdc, RGB(255, 255, 255));
	SelectObject(hdc, hPen);   //Choose a brush
	TCHAR score_str[50];

	//Plot the current score
	wsprintf(score_str, TEXT("Score: %5d "), score);
	TextOut(hdc, 20, 15, score_str, lstrlen(score_str));


	//Draw the game interface background
	for (int i = BOARD_TOP; i <= BOARD_BOTTOM; i++) {
		for (int j = BOARD_LEFT; j <= BOARD_RIGHT; j++) {
			PaintCell(hdc, i, j, gBoard[i][j]);
		}
	}

	// Draw a graph that is falling
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (shape[i][j] == 1)
				PaintCell(hdc, shape_row + i, shape_col + j, shape[i][j]);
		}
	}
}

// Print the square of the specified color at the specified position
void PaintCell(HDC hdc, int x, int y, int color) {

	//End directly beyond the scope
	if (x < BOARD_TOP || x > BOARD_BOTTOM ||
		y < BOARD_LEFT || y > BOARD_RIGHT) {
		return;
	}

	x -= BOARD_TOP;
	y -= BOARD_LEFT;

	// Convert coordinates to actual pixels
	int _left = lattices_left + y * width;
	int _right = lattices_left + y * width + width;
	int _top = lattices_top + x * height;
	int _bottom = lattices_top + x * height + height;

	// Draw border
	MoveToEx(hdc, _left, _top, NULL);
	LineTo(hdc, _right, _top);
	MoveToEx(hdc, _left, _top, NULL);
	LineTo(hdc, _left, _bottom);
	MoveToEx(hdc, _left, _bottom, NULL);
	LineTo(hdc, _right, _bottom);
	MoveToEx(hdc, _right, _top, NULL);
	LineTo(hdc, _right, _bottom);


	if (color == 0) {
		SelectObject(hdc, white_brush);
	}
	else if (color == 1) {
		SelectObject(hdc, blue_brush);
	}
	else if (color == 2) {
		SelectObject(hdc, red_brush);
	}

	// filling
	Rectangle(hdc, _left, _top, _right, _bottom);
}

// Respond to the button
void RespondKey(HWND hwnd, WPARAM wParam) {

	if (wParam == VK_ESCAPE) {//ESC drop out
		ExitGame(hwnd);
		return;
	}
	if (wParam == VK_SPACE) {//Space pause
		gIsPause = !gIsPause;
		if (gIsPause == true) {
			PauseGame(hwnd);
			return;
		}
		else {
			WakeGame(hwnd);
			return;
		}
	}

	// If it is in a paused state, it will not respond to these movement operations
	if (!gIsPause) {
		if (wParam == VK_UP) {
			RotateShape(hwnd);
			return;
		}
		if (wParam == VK_DOWN) {
			MoveDown(hwnd);
			return;
		}
		if (wParam == VK_LEFT) {
			MoveHori(hwnd, 0);
			return;
		}
		if (wParam == VK_RIGHT) {
			MoveHori(hwnd, 1);
			return;
		}
	}
}

//Stop counter
void PauseGame(HWND hwnd) {
	KillTimer(hwnd, ID_TIMER);
}

// Restart counter
void WakeGame(HWND hwnd) {
	SetTimer(hwnd, ID_TIMER, TIME_INTERVAL, TimerProc);
}

// exit the game
void ExitGame(HWND hwnd) {

	// Pause the game first
	SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);

	// Whether to exit
	int flag = MessageBox(NULL, TEXT("exit the game?"), TEXT("EXIT"), MB_YESNO);

	if (flag == IDYES) {
		SendMessage(hwnd, WM_DESTROY, NULL, 0);
	}
	else if (flag == IDNO) {
		return;
	}

}

// Graphic distortion
void RotateShape(HWND hwnd) {

	RotateMatrix();

	if (!IsLegel()) {
		ReRotateMatrix();
	}

	UpdateShapeRect(hwnd);

	return;
}

// Determine if the shape is in the game interface
bool IsLegel() {

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			if (shape[i][j] == 1 &&
				(gBoard[shape_row + i][shape_col + j] == 1 ||
					gBoard[shape_row + i][shape_col + j] == 2))
				return false;

	return true;
}

// Rotate the current falling shape counterclockwise
void RotateMatrix() {

	int(*a)[4] = shape;

	int s = 0;

	for (int n = 4; n >= 1; n -= 2) {
		for (int i = 0; i < n - 1; i++) {
			int t = a[s + i][s];
			a[s + i][s] = a[s][s + n - i - 1];
			a[s][s + n - i - 1] = a[s + n - i - 1][s + n - 1];
			a[s + n - i - 1][s + n - 1] = a[s + n - 1][s + i];
			a[s + n - 1][s + i] = t;
		}
		s++;
	}

}

//If it exceeds the range, restore the shape (rotate clockwise)
void ReRotateMatrix() {
	int(*a)[4] = shape;
	int s = 0;
	for (int n = 4; n >= 1; n -= 2) {
		for (int i = 0; i < n - 1; i++) {
			int t = a[s + i][s];
			a[s + i][s] = a[s + n - 1][s + i];
			a[s + n - 1][s + i] = a[s + n - i - 1][s + n - 1];
			a[s + n - i - 1][s + n - 1] = a[s][s + n - i - 1];
			a[s][s + n - i - 1] = t;
		}
		s++;
	}
}

// The drop shape drops one square
void MoveDown(HWND hwnd) {

	shape_row++;

	if (!IsLegel()) {
		shape_row--;

		if (JudgeLose()) {
			LoseGame(hwnd);
			return;
		}
		FallToGround();
		ClearFullLine();
		UpdateAllBoard(hwnd);

		// Reset the drop shape position
		shape_row = 0;
		shape_col = EXTENDED_COLS / 2 - 2;

		RandShape();
	}

	UpdateShapeRect(hwnd);
}

// Judge if you lose
bool JudgeLose() {

	if (shape_row == 0)
		return true;

	return false;

}

// game over
void LoseGame(HWND hwnd) {

	SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);

	TCHAR words[100];
	wsprintf(words, TEXT("You lost the Game. Your score is %d. \nDo you want try again?"), score);

	int flag = MessageBox(NULL, words, TEXT("EXIT"), MB_YESNO);

	if (flag == IDYES) {
		SendMessage(hwnd, WM_CREATE, NULL, 0);
		return;
	}
	else if (flag == IDNO) {
		SendMessage(hwnd, WM_DESTROY, NULL, 0);
		return;
	}

}

// Graphics landing, update the background array
void FallToGround() {

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			gBoard[shape_row + i][shape_col + j] = shape[i][j] == 1 ? 2 : gBoard[shape_row + i][shape_col + j];
		}
	}
}

// Clear the entire row of squares
void ClearFullLine() {
	for (int i = shape_row; i <= shape_row + 3; i++) {
		if (i > BOARD_BOTTOM)
			continue;

		bool there_is_blank = false;

		// Determine whether there are spaces in a line
		for (int j = BOARD_LEFT; j <= BOARD_RIGHT; j++) {
			if (gBoard[i][j] == 0) {
				there_is_blank = true;
				break;
			}
		}
		if (!there_is_blank) {
			AddScore();
			for (int r = i; r >= 1; r--) {
				for (int c = BOARD_LEFT; c <= BOARD_RIGHT; c++) {
					gBoard[r][c] = gBoard[r - 1][c];
				}
			}
		}
	}
}

// Clear a line to add 100 points
void AddScore() {
	score += 100;

}

// The falling shape moves horizontally
void MoveHori(HWND hwnd, int direction) {

	int temp = shape_col;

	// direction 0, move left, otherwise move right
	if (direction == 0)
		shape_col--;
	else
		shape_col++;

	//If the position exceeds the boundary after moving
	if (!IsLegel()) {
		shape_col = temp;
	}

	UpdateShapeRect(hwnd);

	return;
}