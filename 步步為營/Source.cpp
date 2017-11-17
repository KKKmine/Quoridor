#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"") //讓console視窗消失
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "Winmm.lib") 
#define _WINSOCKAPI_

#include <stdlib.h>
#include <windows.h>
#include <glut.h>
#include "Server.h"
#include "Client.h"
#include "Chessboard.h"

#define VERSION "ver 0.3.1"
#define PORT 8513
#define FRAME_DELAY 100
#define CHAT_MAX_LINE 6

enum Scene { SC_NAME, SC_SELECT, SC_IP, SC_JOIN, SC_GAME, SC_WIN };
enum Align { AL_LEFT, AL_CENTER, AL_RIGHT };

int windowsID = NULL;
Chessboard myBoard;
Server myServer;
Client myClient(&myBoard);
Scene scene = SC_NAME;
string errorMsg;
string nameStr, ipStr, chatStr;
int chatLine = 0;
bool isChatOpen = false;

void Display();
void MouseButton(int button, int state, int x, int y);
void MouseMove(int x, int y);
void KeyBoard(unsigned char key, int x, int y);
void SpecKeyBoard(int key, int x, int y);
void TimerFunc(int id);

void SetLight();
void SetMaterial();
void ShowUI();
void RenderBitmapString(float x, float y, Align align, const char *string, bool isRed);
double* Unproject(int x, int y);


int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	windowsID = glutCreateWindow("Quoridor 步步為營");

	SetLight();
	SetMaterial();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glutDisplayFunc(Display);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMove);
	glutPassiveMotionFunc(MouseMove);
	glutKeyboardFunc(KeyBoard);
	glutSpecialFunc(SpecKeyBoard);
	glutTimerFunc(FRAME_DELAY, TimerFunc, 1);

	glutMainLoop();
	return 0;
}

void Display()
{
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPolygonMode(GL_BACK, GL_LINE);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, double(viewport[2]) / viewport[3], 0.1, 50);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	switch (myBoard.GetMyId()) { //轉棋盤 使自己棋子在下面
	case 0:
		gluLookAt(0, -2, 15, 0, 0, 0, 0, 1, 0);
		break;
	case 1:
		gluLookAt(2, 0, 15, 0, 0, 0, -1, 0, 0);
		break;
	case 2:
		gluLookAt(0, 2, 15, 0, 0, 0, 0, -1, 0);
		break;
	case 3:
		gluLookAt(-2, 0, 15, 0, 0, 0, 1, 0, 0);
		break;
	}

	myBoard.ShowGame();
	ShowUI();

	glutSwapBuffers();
}

void MouseButton(int button, int state, int x, int y)
{
	int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
	if (state == 1) {
		switch (scene) {
		case SC_NAME:
			if (x<w / 2 + 50 && x>w / 2 - 50 && y<h / 2 + 80 && y>h / 2 + 50) {
				if (nameStr == "") {
					errorMsg = "Error : Empty Name";
				} 
				else {
					scene = SC_SELECT;
					myClient.SetMyname(nameStr);
					myBoard.SetName(nameStr);
					errorMsg = "";
				}
				//glutPostRedisplay();
			}
			break;
		case SC_SELECT:
			if (x<w / 2 + 50 && x>w / 2 - 50 && y<h / 2 + 20&& y>h / 2 - 10) {
				if (!myServer.StartServer(PORT)) {
					break;
				}
				if (!myClient.Connect("127.0.0.1", PORT)) {
					myServer.CloseServer();
					break;
				}
				scene = SC_JOIN;
				errorMsg = "";
				//glutPostRedisplay();
			}
			else if (x<w / 2 + 50 && x>w / 2 - 50 && y<h / 2 + 80 && y>h / 2 + 50) {
				scene = SC_IP;
				errorMsg = "";
				//glutPostRedisplay();
			}
			break;
		case SC_IP:
			if (x<w / 2 + 50 && x>w / 2 - 50 && y<h / 2 + 80 && y>h / 2 + 50) {
				if (ipStr == "") {
					errorMsg = "Error : Empty IP";
				}
				else {
					if (!myClient.Connect(ipStr, PORT)) {
						ipStr = "";
						break;
					}
					scene = SC_JOIN;
					errorMsg = "";
				}
				//glutPostRedisplay();
			}
			else if (x<w / 2 + 140 && x>w / 2 + 120 && y<h / 2 - 55 && y>h / 2 - 88) {
				scene = SC_SELECT;
				errorMsg = "";
				//glutPostRedisplay();
			}
			break;
		case SC_JOIN:
			if (myServer.IsOpenServer() && x<w / 2 + 50 && x>w / 2 - 50 && y<h / 2 + 80 && y>h / 2 + 50) {
				if (myClient.GetPlayerName().size() < 2) {
					errorMsg = "Error : Player must more than 2";
				}
				else {
					myBoard.Init(myClient.GetPlayerName());
					myClient.SendStart();
					scene = SC_GAME;
					errorMsg = "";
					chatStr = "";
					chatLine = 0;
					myClient.SendDone();
				}
				//glutPostRedisplay();
			}
			else if (x<w / 2 + 140 && x>w / 2 + 120 && y<h / 2 - 55 && y>h / 2 - 88) {
				myClient.Disconnect();
				myServer.CloseServer();
				scene = SC_SELECT;
				errorMsg = "";
				//glutPostRedisplay();
			}
			break;
		case SC_WIN:
			if (x<w / 2 + 50 && x>w / 2 - 50 && y<h / 2 + 80 && y>h / 2 + 50) {
				scene = SC_JOIN;
				errorMsg = "";
				//glutPostRedisplay();
			}
			break;
		case SC_GAME:
			double* worldPos = Unproject(x, y);
			int action[4];
			myBoard.ReadMouse(worldPos[0], worldPos[1], worldPos[2], action);
			if (action[0] == 0) {
				myClient.SendMoveChess(action[1], action[2]);
				if (myBoard.GetWinnerName() == "") {
					myClient.SendDone();
				}
				else {
					myClient.SendGameOver();
				}
				//glutPostRedisplay();
			}
			else if (action[0] == 1) {
				myClient.SendSetWall(action[1], action[2], action[3]);
				myClient.SendDone();
				//glutPostRedisplay();
			}
			free(worldPos);
			break;
		}
	}
}

void MouseMove(int x, int y)
{
	if (scene == SC_GAME) {
		double* worldPos = Unproject(x, y);
		myBoard.SetTempWall(worldPos[0], worldPos[1], worldPos[2]);
		free(worldPos);
	}
}

void KeyBoard(unsigned char key, int x, int y)
{
	switch (scene) {
	case SC_NAME:
		if (key >= 32 && key < 127) {
			if (nameStr.size() < 12) {
				nameStr.push_back(key);
			}
		}
		else if (key == 8) { //Backspace
			if (nameStr.size() > 0) {
				nameStr.pop_back();
			}
		}
		else if (key == 13) { //Enter
			MouseButton(0, 1, glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2 + 70);
		}
		break;
	case SC_IP:
		if (key == '.' || key >= '0' &&key <= '9') {
			if (ipStr.size() < 20) {
				ipStr.push_back(key);
			}
		}
		else if (key == 8) { //Backspace
			if (ipStr.size() > 0) {
				ipStr.pop_back();
			}
		}
		else if (key == 13) { //Enter
			MouseButton(0, 1, glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2 + 70);
		}
		break;
	case SC_JOIN:
	case SC_GAME:
		if (key == 9) { //Tab
			isChatOpen = !isChatOpen;
		}
		else if (isChatOpen) {
			if (key >= 32 && key < 127) {
				chatStr.push_back(key);
			}
			else if (key == 8) { //Backspace
				if (chatStr.size() > 0) {
					chatStr.pop_back();
				}
			}
			else if (key == 13) { //Enter
				myClient.SendChat(chatStr);
				chatStr = "";
			}
		}
		break;
	}
	switch (key)
	{
	case 27: //ESC
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glutDestroyWindow(windowsID);
		exit(0);
		break;
	}
	//glutPostRedisplay();
}

void SpecKeyBoard(int key, int x, int y)
{
	switch (scene) {
	case SC_JOIN:
	case SC_GAME:
		if (isChatOpen) {
			if (key == GLUT_KEY_UP) {
				if (chatLine + CHAT_MAX_LINE < myClient.GetChatLines()) {
					chatLine++;
				}
			}
			else if (key == GLUT_KEY_DOWN) {
				if (chatLine > 0) {
					chatLine--;
				}
			}
		}
		break;
	}
}

void TimerFunc(int id)
{
	glutPostRedisplay();
	glutTimerFunc(FRAME_DELAY, TimerFunc, 1);
}

void SetLight()
{
	float light_ambient[] = { 0.2, 0.2, 0.2, 1.0 }; //環境光
	float light_diffuse[] = { 0.8, 0.8, 0.8, 1.0 }; //散射光
	float light_specular[] = { 1.0, 1.0, 1.0, 1.0 }; //反射光
	float light_position[] = { 0.0, 0.0, 5.0, 0.4 }; //光的座標 

	glEnable(GL_LIGHTING);

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);
}

void SetMaterial()
{
	float material_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
	float material_diffuse[] = { 0.7, 0.7, 0.7, 1.0 };
	float material_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	float material_shininess = 0.25;

	glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, material_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, material_shininess * 128.0);
}

void ShowUI()
{
	int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
	string bufStr;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, w, 0, h);
	glScalef(1, -1, 1);
	glTranslatef(0, -h, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, 0);
	if (!myClient.IsOpened() && (scene == SC_JOIN || scene == SC_GAME)) {
		scene = SC_SELECT;
	}
	else if (myClient.IsPlaying() && scene != SC_GAME) {
		scene = SC_GAME;
	}
	else if (myBoard.GetWinnerName() != "" && scene == SC_GAME) {
		scene = SC_WIN;
		myClient.SendGameOver();
	}
	//Dialog Box
	if (scene != SC_GAME) {
		glPushMatrix();
		glTranslated(w / 2, h / 2, 0);
		switch (scene) {
		case SC_NAME:
			//Title
			RenderBitmapString(0, -60, AL_CENTER, "Input Your Name :", false);
			//Input Name
			RenderBitmapString(0, 0, AL_CENTER, nameStr.c_str(), false);
			//Button Confirm
			glBegin(GL_LINE_LOOP);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glVertex2d(-50, 80);
			glVertex2d(-50, 50);
			glVertex2d(50, 50);
			glVertex2d(50, 80);
			glEnd();
			RenderBitmapString(0, 70, AL_CENTER, "Confirm", false);
			break;
		case SC_SELECT:
			//Title
			RenderBitmapString(0, -60, AL_CENTER, "Welcome to Quoridor !!", false);
			RenderBitmapString(0, -30, AL_CENTER, nameStr.c_str(), false);
			//Button Server
			glBegin(GL_LINE_LOOP);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glVertex2d(-50, 20);
			glVertex2d(-50, -10);
			glVertex2d(50, -10);
			glVertex2d(50, 20);
			glEnd();
			RenderBitmapString(0, 10, AL_CENTER, "Server", false);
			//Button Client button
			glBegin(GL_LINE_LOOP);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glVertex2d(-50, 80);
			glVertex2d(-50, 50);
			glVertex2d(50, 50);
			glVertex2d(50, 80);
			glEnd();
			RenderBitmapString(0, 70, AL_CENTER, "Client", false);
			//Label version
			RenderBitmapString(142, 95, AL_RIGHT, VERSION, false);
			break;
		case SC_IP:
			//Title
			RenderBitmapString(0, -60, AL_CENTER, "Input Server IP :", false);
			//Input Name
			RenderBitmapString(0, 0, AL_CENTER, ipStr.c_str(), false);
			//Button Connect
			glBegin(GL_LINE_LOOP);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glVertex2d(-50, 80);
			glVertex2d(-50, 50);
			glVertex2d(50, 50);
			glVertex2d(50, 80);
			glEnd();
			RenderBitmapString(0, 70, AL_CENTER, "Connect", false);
			break;
		case SC_JOIN:
			//Title
			RenderBitmapString(0, -60, AL_CENTER, "Game Lobby :", false);
			//Player Names
			for (int i = 0; i < 4; i++) {
				if (i < myClient.GetPlayerName().size()) {
					RenderBitmapString(0, -30 + 20 * i, AL_CENTER, myClient.GetPlayerName()[i].c_str(), false);
				}
				else {
					RenderBitmapString(0, -30 + 20 * i, AL_CENTER, "--", false);
				}
			}
			//Button Start
			if (myServer.IsOpenServer()) {
				glBegin(GL_LINE_LOOP);
				glColor4f(0.0, 0.0, 0.0, 1.0);
				glVertex2d(-50, 80);
				glVertex2d(-50, 50);
				glVertex2d(50, 50);
				glVertex2d(50, 80);
				glEnd();
				RenderBitmapString(0, 70, AL_CENTER, "Start", false);
			}
			break;
		case SC_WIN:
			//Title
			RenderBitmapString(0, -50, AL_CENTER, "Winner !!", false);
			//Label
			RenderBitmapString(0, 0, AL_CENTER, myBoard.GetWinnerName().c_str(), false);
			//Button Restart
			glBegin(GL_LINE_LOOP);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glVertex2d(-50, 80);
			glVertex2d(-50, 50);
			glVertex2d(50, 50);
			glVertex2d(50, 80);
			glEnd();
			RenderBitmapString(0, 70, AL_CENTER, "Restart", false);
			break;
		}
		//Error Msg
		string e = myClient.GetErrorMsg();
		if (e != "") {
			errorMsg = e;
		}
		else {
			e = myServer.GetErrorMsg();
			if (e != "") {
				errorMsg = e;
			}
		}
		if (errorMsg != "") {
			RenderBitmapString(0, -80, AL_CENTER, errorMsg.c_str(), true);
		}
		//Button back
		if (scene == SC_IP || scene == SC_JOIN) {
			char c[] = "X";
			RenderBitmapString(130, -61, AL_CENTER, c, false);
			glBegin(GL_LINE_LOOP);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glVertex2d(140, -80);
			glVertex2d(140, -55);
			glVertex2d(120, -55);
			glVertex2d(120, -80);
			glEnd();
		}
		//BackGround
		if (scene != SC_GAME) {
			glBegin(GL_QUADS);
			glColor4f(1.0, 1.0, 1.0, 0.7);
			glVertex2d(-150, -100);
			glVertex2d(-150, 100);
			glVertex2d(150, 100);
			glVertex2d(150, -100);
			glEnd();
		}
		glPopMatrix();
	}
	else {
		//Title
		RenderBitmapString(45, 27, AL_LEFT, "Player :", false);
		//Player Names
		for (int i = 0; i < 4; i++) {
			RenderBitmapString(10, 47 + 18 * i, AL_LEFT, myBoard.GetPlayerName(i).c_str(), false);
		}
		if (myBoard.IsMyTurn()) {
			RenderBitmapString(125, 47 + 18 * myBoard.GetOrder(), AL_LEFT, "Your Turn", false);
		}
		else {
			RenderBitmapString(125, 47 + 18 * myBoard.GetOrder(), AL_LEFT, "Thinking", false);
		}
		glBegin(GL_QUADS);
		glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
		glVertex2d(5, 10);
		glVertex2d(5, 110);
		glVertex2d(210, 110);
		glVertex2d(210, 10);
		glEnd();
	}
	//Chat Box
	if (scene == SC_JOIN || scene == SC_GAME) {
		if (isChatOpen) {
			RenderBitmapString(10, h - 15, AL_LEFT, chatStr.c_str(), false);
			for (int i = 0; i < 6; i++) {
				string msg = myClient.GetChatRecord(chatLine + i);
				if (msg != "") {
					RenderBitmapString(10, h - 30 - i * 15, AL_LEFT, msg.c_str(), false);
				}
				else {
					break;
				}
			}
			glBegin(GL_QUADS);
			//Background
			glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
			glVertex2d(5, h - (CHAT_MAX_LINE + 1) * 17);
			glVertex2d(5, h - 5);
			glVertex2d(375, h - 5);
			glVertex2d(375, h - (CHAT_MAX_LINE + 1) * 17);
			glEnd();
		}
	}
	glEnable(GL_LIGHTING);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void RenderBitmapString(float x, float y, Align align, const char *string, bool isRed)
{
	if (isRed) {
		glColor3f(1.0, 0.0, 0.0);
	}
	else {
		glColor3f(0.0, 0.0, 0.0);
	}
	switch (align) {
	case AL_LEFT:
		glRasterPos2f(x, y);
		break;
	case AL_CENTER:
		glRasterPos2f(x - strlen(string) * 4.5, y);
		break;
	case AL_RIGHT:
		glRasterPos2f(x - strlen(string) * 9, y);
		break;
	}
	for (const char *c = string; *c != '\0'; c++) {
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
	}
}

double* Unproject(int x, int y)
//說明:螢幕2D座標 轉 世界3D座標
{
	int viewport[4];
	double modelview[16], projection[16], *worldPos = new double[3];
	float winz;
	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glReadPixels(x, viewport[3] - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winz);
	gluUnProject(x, viewport[3] - y, winz, modelview, projection, viewport, &worldPos[0], &worldPos[1], &worldPos[2]);
	return worldPos;
}
