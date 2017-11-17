//
//		Chess Board
//
//�ܼƸ���:
//x,y,z: �@�ɮy��
//i,j: �}�C�y�� �����ѽL�W���I �_�Ƭ��W�� ���Ƭ����
//direct: �����V 1�����k 0���W�U
//id: �Ѥl��l��m
//order: �C������

#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <string>
#include <vector>
#include <sstream>
#include <windows.h>
#include <glut.h>
using namespace std;

const float EDGE_HIGH = 0.3; //�ѽL��t����
const float SWELL_HIGH = 0.6; //�Y�X��氪��
const float NOTCH_HIGH = 0.45; //����W�Ѱ���
const float WALL_HIGH = 1; //�������
const float CHESS_HIGH = 0.8; //�Ѥl����

struct Player {
	int id;
	string name;
	int lastWall;
	POINT pos;
};

class Chessboard
{
private:
	vector<Player> m_player;
	bool m_boardMap[17][17];
	int m_gameOrder;
	string m_winnerName;

	string m_myName;
	bool m_isCanMove;
	int m_tempWall[3];
	vector<vector<int>> m_moveableList;
	vector<vector<int>> m_wallList;
public:
	Chessboard();
	void SetName(string name);
	void Init(vector<string> names);
	bool NextPlayer(int index);
	bool RemovePlayer(int index);
	bool IsMyTurn();
	void Win(string name);

	//glut Display
	void ShowGame();
	void ShowChessboard();
	void ShowWall(float x, float y, bool direct, bool isTemp);
	void ShowChess(float x, float y, int id, bool isTemp);

	//Check Possible Action
	bool CheckWallSettable(int i, int j, int direct);
	bool DFS(int i, int j, int order);
	void CheckChessMoveable();

	//Player Action
	void ReadMouse(float x, float y, float z, int* cmd);
	void SetTempWall(float x, float y, float z);
	void SetWall(int i, int j, bool direct);
	void MoveChess(int i, int j);

	//Static Function
	static void LoadTexture();
	static void SetTextureObj(char* fileName, GLuint &id);
	static unsigned char *LoadBitmapFile(char *fileName, BITMAPINFO *bitmapInfo);
	static bool World2BoardPos(float x, float y, float z, int &i, int &j);

	//Get Function
	string GetPlayerName(int order);
	string GetWinnerName();
	int GetOrder();
	int GetMyId();
};

static bool m_isLoadTexture;
static GLuint* m_textures;

Chessboard::Chessboard()
{
	m_myName = "";
}

void Chessboard::SetName(string name)
{
	m_myName = name;
}

void Chessboard::Init(vector<string> names)
{
	m_gameOrder = 0;

	m_player.clear();
	switch (names.size()) {
	case 2:
		m_player.push_back({ 0, names[0], 10 , POINT{ 8, 0 } });
		m_player.push_back({ 2, names[1], 10 , POINT{ 8, 16 } });
		break;
	case 3:
		m_player.push_back({ 0, names[0], 7 , POINT{ 8, 0 } });
		m_player.push_back({ 1, names[1], 7 , POINT{ 16, 8 } });
		m_player.push_back({ 2, names[2], 7 , POINT{ 8, 16 } });
		break;
	case 4:
		m_player.push_back({ 0, names[0], 5 , POINT{ 8, 0 } });
		m_player.push_back({ 1, names[1], 5 , POINT{ 16, 8 } });
		m_player.push_back({ 2, names[2], 5 , POINT{ 8, 16 } });
		m_player.push_back({ 3, names[3], 5 , POINT{ 0, 8 } });
		break;
	}
	memset(m_boardMap, 0, sizeof m_boardMap[0][0] * 17 * 17);

	m_winnerName = "";
	m_tempWall[2] = -1;
	m_moveableList.clear();
	m_wallList.clear();

	//glutPostRedisplay();
}

bool Chessboard::NextPlayer(int index)
{
	m_gameOrder = index;
	if (IsMyTurn()) {
		CheckChessMoveable();
		if (m_moveableList.size() == 0 && m_player[m_gameOrder].lastWall == 0) {
			m_isCanMove = false;
			return false;
		}
		else {
			m_isCanMove = true;
		}
		PlaySound("sound/notice.wav", NULL, SND_ASYNC | SND_FILENAME);
	}
	else {
		m_isCanMove = false;
	}
	return true;
}

bool Chessboard::RemovePlayer(int index)
//�^��:�p�G�������a�����ʪ�=true
{
	m_player.erase(m_player.begin() + index);
	if (index == m_gameOrder) {
		return true;
	}
	else {
		return false;
	}
}

bool Chessboard::IsMyTurn() {
	if (m_player[m_gameOrder].name == m_myName) {
		return true;
	}
	return false;
}

void Chessboard::Win(string name)
{
	m_winnerName = name;
	m_isCanMove = false;
}

//glut Display
void Chessboard::ShowGame()
{
	if (!m_isLoadTexture) {
		LoadTexture();
	}
	ShowChessboard();
	//Show Wall
	for (vector<int> wall : m_wallList) {
		ShowWall(wall[0] / 2.0 - 4, wall[1] / 2.0 - 4, wall[2], false);
	}
	for (Player p : m_player) {
		//Show Chess
		ShowChess(p.pos.x / 2.0 - 4, p.pos.y / 2.0 - 4, p.id, false);
		//Show LastWall
		glPushMatrix();
		glRotatef(90 * p.id, 0, 0, 1);
		glTranslatef(-4, -5, 0);
		glScalef(0.5, 0.5, 0.5);
		for (int j = 0; j < p.lastWall; j++) {
			glTranslatef(0.5, 0, 0);
			ShowWall(0, 0, 0, false);
		}
		glPopMatrix();
	}
	if (m_isCanMove) { //����ۤv
		//Show Temp Wall
		if (m_tempWall[2] != -1) {
			ShowWall(m_tempWall[0] / 2.0 - 4, m_tempWall[1] / 2.0 - 4, m_tempWall[2], true);
		}
		//Show Temp Chess
		for (vector<int> chess : m_moveableList) {
			ShowChess(chess[0] / 2.0 - 4, chess[1] / 2.0 - 4, chess[2], true);
		}
	}
}

void Chessboard::ShowChessboard()
{
	//-----�ୱ------
	glBindTexture(GL_TEXTURE_2D, m_textures[3]);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0); glVertex3d(-15, -15, 0);
	glTexCoord2f(5, 0); glVertex3d(15, -15, 0);
	glTexCoord2f(5, 5); glVertex3d(15, 15, 0);
	glTexCoord2f(0, 5); glVertex3d(-15, 15, 0);
	glEnd();

	//----�ѽL�D��----
	glBindTexture(GL_TEXTURE_2D, m_textures[0]);
	glBegin(GL_QUADS);
	//�k
	glNormal3f(1, 0, 0);
	glTexCoord2f(0, 0); glVertex3d(5.5, 5.5, 0);
	glTexCoord2f(0, .3); glVertex3d(5.5, 5.5, EDGE_HIGH);
	glTexCoord2f(3, .3); glVertex3d(5.5, -5.5, EDGE_HIGH);
	glTexCoord2f(3, 0); glVertex3d(5.5, -5.5, 0);
	//��
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0, 0); glVertex3d(-5.5, 5.5, 0);
	glTexCoord2f(3, 0); glVertex3d(-5.5, -5.5, 0);
	glTexCoord2f(3, 0.3); glVertex3d(-5.5, -5.5, EDGE_HIGH);
	glTexCoord2f(0, 0.3); glVertex3d(-5.5, 5.5, EDGE_HIGH);
	//�W
	glNormal3f(0, 1, 0);
	glTexCoord2f(0, 0); glVertex3d(5.5, 5.5, 0);
	glTexCoord2f(0, 3); glVertex3d(-5.5, 5.5, 0);
	glTexCoord2f(.3, 3); glVertex3d(-5.5, 5.5, EDGE_HIGH);
	glTexCoord2f(.3, 0); glVertex3d(5.5, 5.5, EDGE_HIGH);
	//�U
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0); glVertex3d(5.5, -5.5, 0);
	glTexCoord2f(.3, 0); glVertex3d(5.5, -5.5, EDGE_HIGH);
	glTexCoord2f(.3, 3); glVertex3d(-5.5, -5.5, EDGE_HIGH);
	glTexCoord2f(0, 3); glVertex3d(-5.5, -5.5, 0);
	//��
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0); glVertex3d(5.5, 5.5, EDGE_HIGH);
	glTexCoord2f(3, 0); glVertex3d(-5.5, 5.5, EDGE_HIGH);
	glTexCoord2f(3, 3); glVertex3d(-5.5, -5.5, EDGE_HIGH);
	glTexCoord2f(0, 3); glVertex3d(5.5, -5.5, EDGE_HIGH);

	//----�Y�X��t----
	//�k
	glNormal3f(1, 0, 0);
	glTexCoord2f(0, 0); glVertex3d(4.4, 4.4, NOTCH_HIGH);
	glTexCoord2f(3, 0); glVertex3d(4.4, -4.4, NOTCH_HIGH);
	glTexCoord2f(3, .3); glVertex3d(4.4, -4.4, EDGE_HIGH);
	glTexCoord2f(0, .3); glVertex3d(4.4, 4.4, EDGE_HIGH);
	//��
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0, 0); glVertex3d(-4.4, 4.4, NOTCH_HIGH);
	glTexCoord2f(.3, 0); glVertex3d(-4.4, 4.4, EDGE_HIGH);
	glTexCoord2f(.3, 3); glVertex3d(-4.4, -4.4, EDGE_HIGH);
	glTexCoord2f(0, 3); glVertex3d(-4.4, -4.4, NOTCH_HIGH);
	//�W
	glNormal3f(0, 1, 0);
	glTexCoord2f(0, 0); glVertex3d(4.4, 4.4, NOTCH_HIGH);
	glTexCoord2f(.3, 0); glVertex3d(4.4, 4.4, EDGE_HIGH);
	glTexCoord2f(.3, 3); glVertex3d(-4.4, 4.4, EDGE_HIGH);
	glTexCoord2f(0, 3); glVertex3d(-4.4, 4.4, NOTCH_HIGH);
	//�U
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0); glVertex3d(4.4, -4.4, NOTCH_HIGH);
	glTexCoord2f(3, 0);  glVertex3d(-4.4, -4.4, NOTCH_HIGH);
	glTexCoord2f(3, .3); glVertex3d(-4.4, -4.4, EDGE_HIGH);
	glTexCoord2f(0, .3); glVertex3d(4.4, -4.4, EDGE_HIGH);

	//----�Y�X���9*9----
	for (int i = -4; i <= 4; i++) {
		for (int j = -4; j <= 4; j++) {
			//�k
			glNormal3f(1, 0, 0);
			glTexCoord2f(0, 0); glVertex3d(i + 0.4, j + 0.4, SWELL_HIGH);
			glTexCoord2f(1, 0); glVertex3d(i + 0.4, j - 0.4, SWELL_HIGH);
			glTexCoord2f(1, 1); glVertex3d(i + 0.4, j - 0.4, NOTCH_HIGH);
			glTexCoord2f(0, 1); glVertex3d(i + 0.4, j + 0.4, NOTCH_HIGH);
			//��
			glNormal3f(-1, 0, 0);
			glTexCoord2f(0, 0); glVertex3d(i - 0.4, j + 0.4, SWELL_HIGH);
			glTexCoord2f(1, 0); glVertex3d(i - 0.4, j + 0.4, NOTCH_HIGH);
			glTexCoord2f(1, 1); glVertex3d(i - 0.4, j - 0.4, NOTCH_HIGH);
			glTexCoord2f(0, 1); glVertex3d(i - 0.4, j - 0.4, SWELL_HIGH);
			//�W
			glNormal3f(0, 1, 0);
			glTexCoord2f(0, 0); glVertex3d(i + 0.4, j + 0.4, SWELL_HIGH);
			glTexCoord2f(1, 0); glVertex3d(i + 0.4, j + 0.4, NOTCH_HIGH);
			glTexCoord2f(1, 1); glVertex3d(i - 0.4, j + 0.4, NOTCH_HIGH);
			glTexCoord2f(0, 1); glVertex3d(i - 0.4, j + 0.4, SWELL_HIGH);
			//�U
			glNormal3f(0, -1, 0);
			glTexCoord2f(0, 0); glVertex3d(i + 0.4, j - 0.4, SWELL_HIGH);
			glTexCoord2f(1, 0); glVertex3d(i - 0.4, j - 0.4, SWELL_HIGH);
			glTexCoord2f(1, 1); glVertex3d(i - 0.4, j - 0.4, NOTCH_HIGH);
			glTexCoord2f(0, 1); glVertex3d(i + 0.4, j - 0.4, NOTCH_HIGH);
			//��
			glNormal3f(0, 0, 1);
			glTexCoord2f(0.3, 0); glVertex3d(i - 0.4, j - 0.4, SWELL_HIGH);
			glTexCoord2f(0.5, 0); glVertex3d(i + 0.4, j - 0.4, SWELL_HIGH);
			glTexCoord2f(0.5, 0.2); glVertex3d(i + 0.4, j + 0.4, SWELL_HIGH);
			glTexCoord2f(0.3, 0.2); glVertex3d(i - 0.4, j + 0.4, SWELL_HIGH);
		}
	}
	glEnd();

	//----�ѽL�W��----
	glBindTexture(GL_TEXTURE_2D, m_textures[1]);
	glBegin(GL_QUADS);
	//��
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0); glVertex3d(4.4, 4.4, NOTCH_HIGH);
	glTexCoord2f(3, 0); glVertex3d(-4.4, 4.4, NOTCH_HIGH);
	glTexCoord2f(3, 3); glVertex3d(-4.4, -4.4, NOTCH_HIGH);
	glTexCoord2f(0, 3); glVertex3d(4.4, -4.4, NOTCH_HIGH);

	glEnd();
}

void Chessboard::ShowWall(float x, float y, bool direct, bool isTemp)
{
	glPushMatrix();
	glTranslatef(x, y, 0);
	if (direct) {
		glRotatef(90, 0, 0, 1);
	}
	glBindTexture(GL_TEXTURE_2D, m_textures[2]);
	if (isTemp) {
		glDisable(GL_LIGHTING);
		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	}
	glBegin(GL_QUADS);
	//�k
	glNormal3f(1, 0, 0);
	glTexCoord2f(0, 0); glVertex3d(0.08, 0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(1, 0); glVertex3d(0.08, -0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(1, 1); glVertex3d(0.08, -0.9, NOTCH_HIGH);
	glTexCoord2f(0, 1); glVertex3d(0.08, 0.9, NOTCH_HIGH);
	//��
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0, 0); glVertex3d(-0.08, 0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(0, 1); glVertex3d(-0.08, 0.9, NOTCH_HIGH);
	glTexCoord2f(1, 1); glVertex3d(-0.08, -0.9, NOTCH_HIGH);
	glTexCoord2f(1, 0); glVertex3d(-0.08, -0.9, NOTCH_HIGH + WALL_HIGH);
	//�W
	glNormal3f(0, 1, 0);
	glTexCoord2f(0, 0); glVertex3d(0.08, 0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(0, .5); glVertex3d(0.08, 0.9, NOTCH_HIGH);
	glTexCoord2f(.1, .5); glVertex3d(-0.08, 0.9, NOTCH_HIGH);
	glTexCoord2f(.1, 0); glVertex3d(-0.08, 0.9, NOTCH_HIGH + WALL_HIGH);
	//�U
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0); glVertex3d(0.08, -0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(.1, 0); glVertex3d(-0.08, -0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(.1, .5); glVertex3d(-0.08, -0.9, NOTCH_HIGH);
	glTexCoord2f(0, .5); glVertex3d(0.08, -0.9, NOTCH_HIGH);
	//��
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0); glVertex3d(0.08, 0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(0, 1); glVertex3d(-0.08, 0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(.1, 1); glVertex3d(-0.08, -0.9, NOTCH_HIGH + WALL_HIGH);
	glTexCoord2f(.1, 0); glVertex3d(0.08, -0.9, NOTCH_HIGH + WALL_HIGH);

	glEnd();
	if (isTemp) {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glEnable(GL_LIGHTING);
	}
	glPopMatrix();
}

void Chessboard::ShowChess(float x, float y, int id, bool isTemp)
{
	glPushMatrix();
	glTranslatef(x, y, 0);
	glRotatef(90 * id, 0, 0, 1);
	switch (id){
	case 0:
		glBindTexture(GL_TEXTURE_2D, m_textures[4]);
		break;
	case 1:
		glBindTexture(GL_TEXTURE_2D, m_textures[5]);
		break;
	case 2:
		glBindTexture(GL_TEXTURE_2D, m_textures[6]);
		break;
	case 3:
		glBindTexture(GL_TEXTURE_2D, m_textures[7]);
		break;
	}
	if (isTemp) {
		glDisable(GL_LIGHTING);
		glColor4f(1.0, 1.0, 1.0, 0.5);
	}
	//��
	glBegin(GL_QUADS);
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0); glVertex3d(0.3, -0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 0); glVertex3d(-0.3, -0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 1); glVertex3d(-0.3, -0.3, SWELL_HIGH);
	glTexCoord2f(0, 1); glVertex3d(0.3, -0.3, SWELL_HIGH);

	glNormal3f(2, 1, 0);
	glTexCoord2f(0, 0); glVertex3d(-0.3, -0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 0); glVertex3d(0, 0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 1); glVertex3d(0, 0.3, SWELL_HIGH);
	glTexCoord2f(0, 1); glVertex3d(-0.3, -0.3, SWELL_HIGH);

	glNormal3f(-2, 1, 0);
	glTexCoord2f(0, 0); glVertex3d(0, 0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 0); glVertex3d(0.3, -0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 1); glVertex3d(0.3, -0.3, SWELL_HIGH);
	glTexCoord2f(0, 1); glVertex3d(0, 0.3, SWELL_HIGH);
	glEnd();
	//��
	glBegin(GL_TRIANGLES);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0); glVertex3d(0, 0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 0); glVertex3d(-0.3, -0.3, CHESS_HIGH + SWELL_HIGH);
	glTexCoord2f(1, 1); glVertex3d(0.3, -0.3, CHESS_HIGH + SWELL_HIGH);
	glEnd();

	if (isTemp) {
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glEnable(GL_LIGHTING);
	}
	glPopMatrix();
}

//Check Possible Action
bool Chessboard::CheckWallSettable(int i, int j, int direct)
//�^��:false=���i��m true=�i��m
{
	bool isSettable = true;
	if (direct == 1) { //���p����
		if (m_boardMap[i + 1][j] || m_boardMap[i][j] || m_boardMap[i - 1][j]) {
			return false;
		}
		m_boardMap[i+1][j] = true;
		m_boardMap[i-1][j] = true;
	}
	else {
		if (m_boardMap[i][j + 1] || m_boardMap[i][j] || m_boardMap[i][j - 1]) {
			return false;
		}
		m_boardMap[i][j+1] = true;
		m_boardMap[i][j-1] = true;
	}
	for (Player p : m_player) {
		for (int a = 0; a < 17; a += 2) { //��l�Ƹ��|
			for (int b = 0; b < 17; b += 2) {
				m_boardMap[a][b] = false;
			}
		}
		if (!DFS(p.pos.x, p.pos.y, p.id)) { //�L�k�ӧQ
			isSettable = false;
			break;
		}
	}
	if (direct == 1) {
		m_boardMap[i + 1][j] = false;
		m_boardMap[i - 1][j] = false;
	}
	else {
		m_boardMap[i][j + 1] = false;
		m_boardMap[i][j - 1] = false;
	}
	return isSettable;
}

bool Chessboard::DFS(int i, int j, int id)
//�^��:false=���� , true=�i�H��F���I
{
	if (m_boardMap[i][j]) { //���L
		return false;
	}
	else if (id == 0 && j == 16 || id == 1 && i == 0 || id == 2 && j == 0 || id == 3 && i == 16) { //��F���I
		return true;
	}
	m_boardMap[i][j] = true;
	if (i + 2 < 17 && !m_boardMap[i + 1][j] && DFS(i + 2, j, id)) {
		return true;
	}
	if (i - 2 >= 0 && !m_boardMap[i - 1][j] && DFS(i - 2, j, id)) {
		return true;
	}
	if (j + 2 < 17 && !m_boardMap[i][j + 1] && DFS(i, j + 2, id)) {
		return true;
	}
	if (j - 2 >= 0 && !m_boardMap[i][j - 1] && DFS(i, j - 2, id)) {
		return true;
	}
	return false;
}

void Chessboard::CheckChessMoveable()
{
	int i = m_player[m_gameOrder].pos.x, j = m_player[m_gameOrder].pos.y;
	int x = i / 2, y = j / 2;
	int chessMap[9][9]; //0:null 1:chess 2:moveable place

	memset(chessMap, 0, sizeof(chessMap[0][0]) * 9 * 9);
	for (Player p : m_player) {
		chessMap[p.pos.x / 2][p.pos.y / 2] = 1;
	}
	if (x + 1 >= 0 && !m_boardMap[i + 1][j]) { //�i�H���k & �S�����
		if (chessMap[x + 1][y] == 0) { //�Ӧ�m�S���Ѥl
			chessMap[x + 1][y] = 2;
		}
		else {
			//�i�H�A���k & �S����� & �Ӧ�m�S���Ѥl
			if (x + 2 < 9 && !m_boardMap[i + 3][j] && chessMap[x + 2][y] == 0) {
				chessMap[x + 2][y] = 2;
			}
			else {
				//�i�H���W & �S����� & �Ӧ�m�S���Ѥl
				if (y + 1 < 9 && !m_boardMap[i + 2][j + 1] && chessMap[x + 1][y + 1] == 0) {
					chessMap[x + 1][y + 1] = 2;
				}
				//�i�H���U & �S����� & �Ӧ�m�S���Ѥl
				if (y - 1 >= 0 && !m_boardMap[i + 2][j - 1] && chessMap[x + 1][y - 1] == 0) {
					chessMap[x + 1][y - 1] = 2;
				}
			}
		}
	}
	if (x - 1 >= 0 && !m_boardMap[i - 1][j]) { //�i�H���� & �S�����
		if (chessMap[x - 1][y] == 0) { //�Ӧ�m�S���Ѥl
			chessMap[x - 1][y] = 2;
		}
		else {
			//�i�H�A���� & �S����� & �Ӧ�m�S���Ѥl
			if (x - 2 >= 0 && !m_boardMap[i - 3][j] && chessMap[x - 2][y] == 0) {
				chessMap[x - 2][y] = 2;
			}
			else {
				//�i�H���W & �S����� & �Ӧ�m�S���Ѥl
				if (y + 1 < 9 && !m_boardMap[i - 2][j + 1] && chessMap[x - 1][y + 1] == 0) {
					chessMap[x - 1][y + 1] = 2;
				}
				//�i�H���U & �S����� & �Ӧ�m�S���Ѥl
				if (y - 1 >= 0 && !m_boardMap[i - 2][j - 1] && chessMap[x - 1][y - 1] == 0) {
					chessMap[x - 1][y - 1] = 2;
				}
			}
		}
	}
	if (y + 1 < 9 && !m_boardMap[i][j + 1]) { //�i�H���W & �S�����
		if (chessMap[x][y + 1] == 0) { //�Ӧ�m�S���Ѥl
			chessMap[x][y + 1] = 2;
		}
		else {
			//�i�H�A���W & �S����� & �Ӧ�m�S���Ѥl
			if (y + 2 < 9 && !m_boardMap[i][j + 3] && chessMap[x][y + 2] == 0) {
				chessMap[x][y + 2] = 2;
			}
			else {
				//�i�H���k & �S����� & �Ӧ�m�S���Ѥl
				if (x + 1 < 9 && !m_boardMap[i + 1][j + 2] && chessMap[x + 1][y + 1] == 0) {
					chessMap[x + 1][y + 1] = 2;
				}
				//�i�H���� & �S����� & �Ӧ�m�S���Ѥl
				if (x - 1 >= 0 && !m_boardMap[i - 1][j + 2] && chessMap[x - 1][y + 1] == 0) {
					chessMap[x - 1][y + 1] = 2;
				}
			}
		}
	}
	if (y - 1 >= 0 && !m_boardMap[i][j - 1]) { //�i�H���U & �S�����
		if (chessMap[x][y - 1] == 0) { //�Ӧ�m�S���Ѥl
			chessMap[x][y - 1] = 2;
		}
		else {
			//�i�H�A���U & �S����� & �Ӧ�m�S���Ѥl
			if (y - 2 >= 0 && !m_boardMap[i][j - 3] && chessMap[x][y - 2] == 0) {
				chessMap[x][y - 2] = 2;
			}
			else {
				//�i�H���k & �S����� & �Ӧ�m�S���Ѥl
				if (x + 1 < 9 && !m_boardMap[i + 1][j - 2] && chessMap[x + 1][y - 1] == 0) {
					chessMap[x + 1][y - 1] = 2;
				}
				//�i�H���� & �S����� & �Ӧ�m�S���Ѥl
				if (x - 1 >= 0 && !m_boardMap[i - 1][j - 2] && chessMap[x - 1][y - 1] == 0) {
					chessMap[x - 1][y - 1] = 2;
				}
			}
		}
	}
	m_moveableList.clear();
	for (int x = 0; x < 9; x++) {
		for (int y = 0; y < 9; y++) {
			if (chessMap[x][y] == 2) {
				m_moveableList.push_back({ x * 2, y * 2, m_player[m_gameOrder].id });
			}
		}
	}
}

//Player Action
void Chessboard::ReadMouse(float x, float y, float z, int* cmd)
{
	int i, j;
	cmd[0] = -1;
	if (m_isCanMove) {
		if (World2BoardPos(x, y, z, i, j)) {
			//Move Chess ����:��m�b�i���ʲM�椤
			if (i % 2 == 0 && j % 2 == 0) { 
				for (vector<int> chess : m_moveableList) {
					if (chess[0] == i && chess[1] == j) {
						MoveChess(i, j);
						cmd[0] = 0;
						cmd[1] = i;
						cmd[2] = j;
						cmd[3] = -1;
						m_isCanMove = false;
						//glutPostRedisplay();
						break;
					}
				}
			}
			//Set Wall ����:�٦���� & �i�H����
			else { 
				if (m_player[m_gameOrder].lastWall > 0 && m_tempWall[2] != -1) {
					SetWall(m_tempWall[0], m_tempWall[1], m_tempWall[2]);
					cmd[0] = 1;
					cmd[1] = m_tempWall[0];
					cmd[2] = m_tempWall[1];
					cmd[3] = m_tempWall[2];
					m_tempWall[2] = -1;
					m_isCanMove = false;
					//glutPostRedisplay();
				}
			}
		}
	}
}

void Chessboard::SetTempWall(float x, float y, float z)
{
	int i, j;
	if (m_isCanMove && m_player[m_gameOrder].lastWall > 0) {
		if (World2BoardPos(x, y, z, i, j)) {
			int wall[3];
			if (i % 2 != 0 && j % 2 != 0) { //�I�b���W�Ѥ�
				wall[0] = i;
				wall[1] = j;
				wall[2] = m_tempWall[2];
				if (wall[2] == -1) {
					wall[2] = 0;
				}
			}
			else if (i % 2 != 0 && j % 2 == 0) { //�I�b�����W�Ѥ�
				wall[0] = i;
				j > 0 ? wall[1] = j - 1 : wall[1] = j + 1;
				wall[2] = 0;
			}
			else if (j % 2 != 0) { //�I�b�����W�Ѥ�
				i > 0 ? wall[0] = i - 1 : wall[0] = i + 1;
				wall[1] = j;
				wall[2] = 1;
			}
			else {
				wall[0] = -1;
				wall[1] = -1;
				wall[2] = -1;
			}
			//�򤧫e���@�� �ݭn�P�_�P����
			if (m_tempWall[2] != wall[2] || m_tempWall[0] == wall[0] || m_tempWall[1] == wall[1]) {
				if (wall[2] != -1 && CheckWallSettable(wall[0], wall[1], wall[2])) {
					m_tempWall[0] = wall[0];
					m_tempWall[1] = wall[1];
					m_tempWall[2] = wall[2];
				}
				else {
					m_tempWall[2] = -1;
				}
				//glutPostRedisplay();
			}
		}
		else if (m_tempWall[2] != -1) {
			m_tempWall[2] = -1;
			//glutPostRedisplay();
		}
	}
}

void Chessboard::SetWall(int i, int j, bool direct)
//����:��m��� ��ָӦ^�X���a������� �ô��U�쪱�a
{
	m_player[m_gameOrder].lastWall--;
	m_wallList.push_back({ i, j, direct });
	if (direct) { //���k��V
		m_boardMap[i + 1][j] = true;
		m_boardMap[i][j] = true;
		m_boardMap[i - 1][j] = true;
	}
	else { //�W�U��V
		m_boardMap[i][j + 1] = true;
		m_boardMap[i][j] = true;
		m_boardMap[i][j - 1] = true;
	}
}

void Chessboard::MoveChess(int i, int j)
//����:���ʸӦ^�X���a����w�y�� �ô��U�쪱�a
{
	if (i == -1 || j == -1) //������
		return;
	m_player[m_gameOrder].pos.x = i;
	m_player[m_gameOrder].pos.y = j;
	int id = m_player[m_gameOrder].id;
	if (id == 0 && j == 16 || id == 1 && i == 0 || id == 2 && j == 0 || id == 3 && i == 16) { //�ӧQ
		Win(m_player[m_gameOrder].name);
		return;
	}
}

//Static Function
void Chessboard::LoadTexture()
{
	free(m_textures);
	m_isLoadTexture = true;
	m_textures = new GLuint[8];
	glGenTextures(8, m_textures);
	SetTextureObj("image/ChessBoard.bmp", m_textures[0]);
	SetTextureObj("image/Notch.bmp", m_textures[1]);
	SetTextureObj("image/Wall.bmp", m_textures[2]);
	SetTextureObj("image/Table.bmp", m_textures[3]);
	SetTextureObj("image/Chess1.bmp", m_textures[4]);
	SetTextureObj("image/Chess2.bmp", m_textures[5]);
	SetTextureObj("image/Chess3.bmp", m_textures[6]);
	SetTextureObj("image/Chess4.bmp", m_textures[7]);

	glEnable(GL_TEXTURE_2D);
}

void Chessboard::SetTextureObj(char* fileName, GLuint &id)
{
	int width, height;
	unsigned char *image; //�o��ϮסA�O�ઽ����OpenGL�ϥΪ���ƤF
	BITMAPINFO bmpinfo; //�ΨӦs��HEADER��T

	glBindTexture(GL_TEXTURE_2D, id);
	image = LoadBitmapFile(fileName, &bmpinfo);
	width = bmpinfo.bmiHeader.biWidth;
	height = bmpinfo.bmiHeader.biHeight;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image);
}

unsigned char *Chessboard::LoadBitmapFile(char *fileName, BITMAPINFO *bitmapInfo)
{
	FILE *fp;
	BITMAPFILEHEADER   bitmapFileHeader; //Bitmap file header
	unsigned char *bitmapImage; //Bitmap image data
	unsigned int lInfoSize; //Size of information
	unsigned int lBitSize; //Size of bitmap

	fp = fopen(fileName, "rb");
	if (fp == NULL) {
		string str = "Can't Find Texture : ";
		str.append(fileName);
		MessageBox(NULL, str.c_str(), "ERROR", MB_OK);
		exit(4);
	}
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, fp); //Ū�� bitmap header

	lInfoSize = bitmapFileHeader.bfOffBits - sizeof(BITMAPFILEHEADER); //Info��size
	fread(bitmapInfo, lInfoSize, 1, fp);

	lBitSize = bitmapInfo->bmiHeader.biSizeImage; //�t�m�O����
	bitmapImage = new BYTE[lBitSize];
	fread(bitmapImage, 1, lBitSize, fp); //Ū���v����

	fclose(fp);

	return bitmapImage;
}

bool Chessboard::World2BoardPos(float x, float y, float z, int &i, int &j)
{
	if (z < EDGE_HIGH - 0.05 || z>5) {
		return false;
	}
	float temp_i = x + 4.5;
	float temp_j = y + 4.5;
	if (temp_i - (int)temp_i > 0.9) {
		temp_i = (int)temp_i + 0.5;
	}
	else if (temp_i - (int)temp_i <= 0.9 && temp_i - (int)temp_i > 0.1) {
		temp_i = (int)temp_i;
	}
	else {
		temp_i = (int)temp_i - 0.5;
	}
	if (temp_j - (int)temp_j  > 0.9) {
		temp_j = (int)temp_j + 0.5;
	}
	else if (temp_j - (int)temp_j <= 0.9 && temp_j - (int)temp_j > 0.1) {
		temp_j = (int)temp_j;
	}
	else {
		temp_j = (int)temp_j - 0.5;
	}
	i = temp_i * 2;
	j = temp_j * 2;
	if (i >= 0 && i < 17 && j >= 0 && j < 17) {
		return true;
	}
	return false;
}

//Get Function
string Chessboard::GetPlayerName(int order) {
	if (order < m_player.size()) {
		return m_player[order].name;
	}
	return "";
}

string Chessboard::GetWinnerName() {
	return m_winnerName;
}

int Chessboard::GetOrder() {
	for (int i = 0; i < m_player.size(); i++) {
		if (i == m_gameOrder) {
			return i;
		}
	}
	return -100;
}

int Chessboard::GetMyId()
{
	for (int i = 0; i < m_player.size(); i++) {
		if (m_player[i].name == m_myName) {
			return m_player[i].id;
		}
	}
	return 0;
}

#endif
