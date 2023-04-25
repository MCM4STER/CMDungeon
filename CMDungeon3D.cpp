#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <conio.h>
#elif defined(__linux__)
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif
#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <limits>
#include <stack>

#define PI 3.14159265359
#define P2 PI/2
#define P3 3*PI/2
#define DEG 0.0174533

using namespace std;

// ----------[ STRUCT ]--------------

enum Tile
{
	AIR,
	ROOM_AIR,
	DOOR,

	WALL,
	CORRIDOR_WALL,
	UNDESTRUCT_WALL,
};

enum Direction
{
	NORTH,
	WEST,
	EAST,
	SOUTH,
};

struct Room
{
	int sizeX{}, sizeY{};
	int mapX{}, mapY{};
	int offsetX{}, offsetY{};
	vector<vector<Tile>> tiles;

	int maxSizeX = 12, maxSizeY = 10;
};

struct Player {
	float x{}, y{}, deltaX{}, deltaY{}, angle{};
};

struct Map {
	int roomsX = 5, roomsY = 5;

	int sizeX{}, sizeY{};
	int tileSize = 64;

	Player player;

	vector<vector<Room>> roomArray;
	vector<vector<Tile>> tileArray;
};

// ----------[ RANDOM FUNCTIONS ]--------------

void cls() {
	system("cls");
}

void getTerminalSize(int& width, int& height) {
#if defined(_WIN32)
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	width = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1) / 2;
	height = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1) - 1;
#elif defined(__linux__)
	struct winsize w;
	ioctl(fileno(stdout), TIOCGWINSZ, &w);
	width = (int)(w.ws_col) / 2;
	height = (int)(w.ws_row) - 1;
#endif // Windows/Linux
}

float dist(int x1, int y1, int x2, int y2) {
	return (float)sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

int randInt(int min, int max)
{
	return (int)(rand() % (max - min)) + min;
}

Direction randDirection(unsigned int seed)
{
	srand(seed);
	const Direction allDir[4] = { NORTH, WEST, EAST, SOUTH };
	return allDir[randInt(0, 4)];
}

unsigned int getSeed(int x, int y, unsigned int globalSeed = 2137420)
{
	return (unsigned int)x + y + 420 * 2137 * 69 * globalSeed;
}

// ----------[ VIEW ]--------------

struct view {
	int sizeX{}, sizeY{};
	bool map = false;
	vector<vector<char>> viewArray;
};

view createView() {
	view v;
	getTerminalSize(v.sizeX, v.sizeY);
	for (int y = 0; y < v.sizeX; y++) {
		vector<char> temp;
		for (int x = 0; x < v.sizeX; x++)
			temp.push_back(' ');
		v.viewArray.push_back(temp);
	}
	return v;
}

void clearView(view& v) {
	for (int y = 0; y < v.sizeY; y++)
		for (int x = 0; x < v.sizeX; x++)
			v.viewArray[y][x] = ' ';
}

void renderView(view v) {
	cls();
	for (int y = 0; y < v.sizeY; y++) {
		for (int x = 0; x < v.sizeX; x++)
		{
			cout.width(2);
			cout << v.viewArray[y][x];
		}
		cout << endl;
	}
}

void placeWall(view& v, int x, int y, int height, char c) {
	if (x < 0 || y < 0 || x >= v.sizeX || y >= v.sizeY) return;
	for (int h = 0; h < height; h++)
		if (y + h < v.sizeY)
			v.viewArray[y + h][x] = c;
}

char stateToChar(Tile s)
{
	switch (s)
	{
	case WALL:
	case CORRIDOR_WALL:
		return '#';
	case UNDESTRUCT_WALL:
		return 'W';
	case AIR:
	default:
		return ' ';
	}
}

void viewMap2D(view& v, Map& map)
{
	Room r;
	int mapPlayerX = (int)(map.player.x / map.tileSize);
	int mapPlayerY = (int)(map.player.y / map.tileSize);

	int view0X = mapPlayerX - v.sizeX / 2;
	int view0Y = mapPlayerY - v.sizeY / 2;


	if (view0X < 0) view0X = 0;
	if (view0Y < 0) view0Y = 0;
	if (view0X + v.sizeX >= map.sizeX) view0X = map.sizeX - v.sizeX;
	if (view0Y + v.sizeY >= map.sizeY) view0Y = map.sizeY - v.sizeY;

	cls();
	for (int y = view0Y; y < view0Y + v.sizeY; y++)
	{
		for (int x = view0X; x < view0X + v.sizeX; x++)
		{
			cout.width(2);
			if (mapPlayerX == x && mapPlayerY == y) { cout << "P"; continue; }
			cout << stateToChar(map.tileArray[y][x]);
			// cout << map.tileArray[x][y];
		}
		cout << endl;
	}
}

// ----------[ MAP ]--------------

Room generateRandomRoom(int x, int y)
{
	unsigned int roomSeed = getSeed(x, y);
	Room randRoom;
	srand(roomSeed);

	randRoom.mapX = x;
	randRoom.mapY = y;

	randRoom.sizeX = randInt(8, randRoom.maxSizeX);
	randRoom.sizeY = randInt(8, randRoom.maxSizeY);

	randRoom.offsetX = randInt(0, randRoom.maxSizeX - randRoom.sizeX);
	randRoom.offsetY = randInt(0, randRoom.maxSizeY - randRoom.sizeY);

	for (int y = 0; y < randRoom.maxSizeY; y++)
	{
		vector<Tile> temp;
		for (int x = 0; x < randRoom.maxSizeX; x++)
		{
			Tile tile = AIR;

			if (
				(x == randRoom.offsetX && y < randRoom.sizeY + randRoom.offsetY && y >= randRoom.offsetY)
				|| (x == randRoom.sizeX + randRoom.offsetX - 1 && y < randRoom.sizeY + randRoom.offsetY && y> randRoom.offsetY)
				|| (y == randRoom.offsetY && x < randRoom.sizeX + randRoom.offsetX && x >= randRoom.offsetX)
				|| (y == randRoom.sizeY + randRoom.offsetY - 1 && x < randRoom.sizeX + randRoom.offsetX && x> randRoom.offsetX)
				)
				tile = WALL;
			if (x > randRoom.offsetX && x < randRoom.sizeX + randRoom.offsetX - 1 && y > randRoom.offsetY && y < randRoom.sizeY + randRoom.offsetY - 1)
				tile = ROOM_AIR;

			temp.push_back(tile);
		}
		randRoom.tiles.push_back(temp);
	}

	for (int i = 0; i < roomSeed % 3 + 2; i++)
	{
		Direction randDir = randDirection(roomSeed + i);
		if (randDir == NORTH)
			randRoom.tiles[randRoom.offsetY][randInt(1 + randRoom.offsetX, randRoom.sizeX + randRoom.offsetX - 2)] = DOOR;
		if (randDir == SOUTH)
			randRoom.tiles[randRoom.sizeY + randRoom.offsetY - 1][randInt(1 + randRoom.offsetX, randRoom.sizeX + randRoom.offsetX - 2)] = DOOR;
		if (randDir == WEST)
			randRoom.tiles[randInt(1 + randRoom.offsetY, randRoom.sizeY + randRoom.offsetY - 2)][randRoom.offsetX] = DOOR;
		if (randDir == EAST)
			randRoom.tiles[randInt(1 + randRoom.offsetY, randRoom.sizeY + randRoom.offsetY - 2)][randRoom.sizeX + randRoom.offsetX - 1] = DOOR;
	}

	return randRoom;
}

Map generateMap() {
	Map map;
	cout << "Generating map..." << endl;
	Room r;
	map.sizeX = map.roomsX * r.maxSizeX;
	map.sizeY = map.roomsY * r.maxSizeY;
	for (int y = 0; y < map.sizeY; y++) {
		vector<Tile> temp;
		for (int x = 0; x < map.sizeX; x++) {
			Tile state = AIR;
			if (x == 0 || x == map.sizeX - 1 || y == 0 || y == map.sizeY - 1)
				state = UNDESTRUCT_WALL;
			temp.push_back(state);
		}
		map.tileArray.push_back(temp);
	}

	cout << "Creating rooms..." << endl;
	for (int y = 0; y < map.roomsY; y++) {
		vector<Room> temp;
		for (int x = 0; x < map.roomsX; x++) {
			Room room = generateRandomRoom(x, y);

			int mapX = room.mapX * room.maxSizeX;
			int mapY = room.mapY * room.maxSizeY;

			for (int rY = 0; rY < room.maxSizeY; rY++)
				for (int rX = 0; rX < room.maxSizeX; rX++)
					if (map.tileArray[mapY + rY][mapX + rX] != UNDESTRUCT_WALL)
						map.tileArray[mapY + rY][mapX + rX] = room.tiles[rY][rX];

			temp.push_back(room);
		}
		map.roomArray.push_back(temp);
	}


	Room centerRoom = map.roomArray[map.roomsY / 2][map.roomsX / 2];
	map.player.x = (centerRoom.mapX * r.maxSizeX + centerRoom.sizeX / 2) * map.tileSize;
	map.player.y = (centerRoom.mapY * r.maxSizeY + centerRoom.sizeY / 2) * map.tileSize;

	return map;
}

void normalizeTiles(Map& map) {
	for (int y = 0; y < map.sizeY; y++)
		for (int x = 0; x < map.sizeX; x++)
		{
			if (map.tileArray[y][x] == CORRIDOR_WALL || map.tileArray[y][x] == UNDESTRUCT_WALL || map.tileArray[y][x] == WALL)
				map.tileArray[y][x] = WALL;
			else
				map.tileArray[y][x] = AIR;
		}
}

Room* getRoomFromMapCoords(Map& map, int x, int y) {
	Room r;
	return &map.roomArray[(int)y / r.maxSizeY][(int)x / r.maxSizeX];
}

Tile getTileFromPlayerCoords(Map& map, int x, int y) {
	int mapPlayerX = (int)(x / map.tileSize);
	int mapPlayerY = (int)(y / map.tileSize);
	return map.tileArray[mapPlayerY][mapPlayerX];
}

// ----------[ PATHFINDING ]--------------


// A*
// https://dev.to/jansonsa/a-star-a-path-finding-c-4a4h

struct Node {
	int x, y;
	int parentX = -1, parentY = -1;
	float gCost = FLT_MAX, hCost = FLT_MAX, fCost = FLT_MAX;
};

inline bool operator < (const Node& lhs, const Node& rhs)
{
	return lhs.fCost < rhs.fCost;
}

Node findClosestDoor(Map& map, int sX, int sY) {
	Node closest;
	closest.x = sX;
	closest.y = sY;
	float closestDist = FLT_MAX;

	for (int y = 0; y < map.sizeY; y++)
		for (int x = 0; x < map.sizeX; x++) {
			if (getRoomFromMapCoords(map, x, y) == getRoomFromMapCoords(map, sX, sY)) continue;
			if (map.tileArray[y][x] != DOOR || (x == sX && y == sY)) continue;
			if (closestDist > dist(sX, sY, x, y)) {
				closestDist = dist(sX, sY, x, y);
				closest.x = x;
				closest.y = y;
			}
		}

	return closest;
}

bool isValid(int x, int y, Map map) {
	if (x < 0 && y < 0 && x >= map.sizeX && y >= map.sizeY) return false;
	if (map.tileArray[y][x] == AIR || map.tileArray[y][x] == DOOR) return true;
	return false;
}

double calculateH(int x, int y, Node dest) {
	double H = (sqrt((x - dest.x) * (x - dest.x)
		+ (y - dest.y) * (y - dest.y)));
	return H;
}

bool isDestination(int x, int y, Node tile) {
	return (tile.x == x && tile.y == y);
}

vector<Node> makePath(vector<vector<Node>> map, Node destination, Map m) {
	try {
		int x = destination.x;
		int y = destination.y;
		stack<Node> path;
		vector<Node> usablePath;

		const int dx[] = { 0, -1, 0, 1 };
		const int dy[] = { -1, 0, 1, 0 };

		while (!(map[x][y].parentX == x && map[x][y].parentY == y) && map[x][y].x != -1 && map[x][y].y != -1) {
			path.push(map[x][y]);
			int tempX = map[x][y].parentX;
			int tempY = map[x][y].parentY;
			x = tempX;
			y = tempY;
		}
		path.push(map[x][y]);

		while (!path.empty()) {
			Node top = path.top();
			path.pop();
			usablePath.emplace_back(top);
			for (int i = 0; i < 4; i++) {
				int newX = top.x + dx[i];
				int newY = top.y + dy[i];
				if (newX >= 0 && newX < m.sizeX && newY >= 0 && newY < m.sizeY &&
					(map[newX][newY].parentX == top.x && map[newX][newY].parentY == top.y) &&
					!(dx[i] != 0 && dy[i] != 0)) {
					x = newX;
					y = newY;
					path.push(map[x][y]);
					break;
				}
			}
		}
		return usablePath;
	}
	catch (const exception& e) {}
}

vector<Node> aStar(Map map, Node start, Node destination) {
	vector<Node> empty;
	if (!isValid(destination.x, destination.y, map)) return empty;
	if (isDestination(start.x, start.y, destination)) return empty;

	bool** closedList = new bool* [map.sizeX];
	for (int i = 0; i < map.sizeX; i++)
		closedList[i] = new bool[map.sizeY];


	vector<vector<Node>> allMap;
	for (int x = 0; x < map.sizeX; x++) {
		vector<Node> temp;
		for (int y = 0; y < map.sizeY; y++) {
			Node tempNode;
			tempNode.x = x;
			tempNode.y = y;

			closedList[x][y] = false;

			temp.push_back(tempNode);
		}
		allMap.push_back(temp);
	}

	int x = start.x;
	int y = start.y;

	allMap[x][y].fCost = 0.0;
	allMap[x][y].gCost = 0.0;
	allMap[x][y].hCost = 0.0;
	allMap[x][y].parentX = x;
	allMap[x][y].parentY = y;

	vector<Node> openList;
	openList.emplace_back(allMap[x][y]);
	bool destinationFound = false;

	while (!openList.empty() && openList.size() < map.sizeX * map.sizeY) {
		Node node;
		do {
			float temp = FLT_MAX;
			vector<Node>::iterator itNode;
			for (auto i = openList.begin(); i != openList.end(); i++) {
				Node n = *i;
				if (n.fCost < temp) {
					temp = n.fCost;
					itNode = i;
				}
			}
			node = *itNode;
			openList.erase(itNode);
		} while (!isValid(node.x, node.y, map));

		x = node.x;
		y = node.y;
		closedList[x][y] = true;

		for (int nX = -1; nX <= 1; nX++)
			for (int nY = -1; nY <= 1; nY++)
			{
				double gNew, hNew, fNew;
				if (isValid(x + nX, y + nY, map))
					if (isDestination(x + nX, y + nY, destination)) {
						allMap[x + nX][y + nY].parentX = x;
						allMap[x + nX][y + nY].parentY = y;
						destinationFound = true;

						return makePath(allMap, destination, map);
					}
					else if (!closedList[x + nX][y + nY]) {
						gNew = node.gCost + 1.0;
						hNew = calculateH(x + nX, y + nY, destination);
						fNew = gNew + hNew;

						if (allMap[x + nX][y + nY].fCost == FLT_MAX || allMap[x + nX][y + nY].fCost > fNew) {
							allMap[x + nX][y + nY].fCost = fNew;
							allMap[x + nX][y + nY].gCost = gNew;
							allMap[x + nX][y + nY].hCost = hNew;
							allMap[x + nX][y + nY].parentX = x;
							allMap[x + nX][y + nY].parentY = y;

							openList.emplace_back(allMap[x + nX][y + nY]);
						}
					}
			}
	}
	if (!destinationFound) return empty;
}

void connectRooms(Map& map) {
	vector<vector<Node>> allPaths;

	cout << "Connecting rooms..." << endl;
	for (int y = 0; y < map.sizeY; y++) {
		for (int x = 0; x < map.sizeX; x++) {
			if (map.tileArray[y][x] != DOOR) continue;
			Node closestDoor = findClosestDoor(map, x, y);
			Node start; start.x = x; start.y = y;
			vector<Node> path = aStar(map, start, closestDoor);
			allPaths.push_back(path);
		}
	}

	cout << "Generating paths..." << endl;
	for (const vector<Node>& path : allPaths) {
		for (const Node& node : path) {
			for (int i = -1; i <= 1; i++) {
				for (int j = -1; j <= 1; j++) {
					if (j == 0 && i == 0) {
						map.tileArray[node.y + j][node.x + i] = ROOM_AIR;
						continue;
					}
					if (map.tileArray[node.y + j][node.x + i] != AIR) continue;
					map.tileArray[node.y + j][node.x + i] = CORRIDOR_WALL;
				}
			}
		}
	}

	cout << "end" << endl;
}


// ----------[ RAY CASTER ]--------------
// https://www.youtube.com/watch?v=gYRrGTC7GtA

#define PI 3.14159265359
#define P2 PI/2
#define P3 3*PI/2
#define DEG 0.0174533

void castRays(view& v, Map map) {
	clearView(v);
	int r{}, mx{}, my{}, dof{};
	float rx{}, ry{}, ra{}, xo{}, yo{}, disT{};

	ra = map.player.angle - DEG * v.sizeX / 2;
	if (ra < 0) ra += 2 * PI;
	if (ra > 2 * PI) ra -= 2 * PI;
	for (int r = 0; r < v.sizeX; r++)
	{
		// HORIZONTAL
		dof = 0;
		float disH = INT_MAX, hx = map.player.x, hy = map.player.y;

		float aTan = -1 / tan(ra);
		if (ra < PI) {
			ry = (((int)map.player.y >> 6) << 6) + 64;
			rx = (map.player.y - ry) * aTan + map.player.x;
			yo = 64; xo = -yo * aTan;
		}
		if (ra > PI) {
			ry = (((int)map.player.y >> 6) << 6) - 0.0001;
			rx = (map.player.y - ry) * aTan + map.player.x;
			yo = -64; xo = -yo * aTan;
		}
		if (ra == 0 || ra == PI) {
			rx = map.player.x; ry = map.player.y; dof = 16;
		}
		while (dof < 16) {
			mx = (int)(rx) >> 6;
			my = (int)(ry) >> 6;
			if (my >= 0 && mx >= 0 && my < map.sizeY && mx < map.sizeX && map.tileArray[my][mx] == WALL) {
				hx = rx; hy = ry;
				disH = dist(map.player.x, map.player.y, hx, hy);
				break;
			}
			else {
				rx += xo; ry += yo; dof += 1;
			}
		}

		// VERTICAL
		dof = 0;
		float disV = INT_MAX, vx = map.player.x, vy = map.player.y;

		float nTan = -tan(ra);
		if (ra < P2 || ra > P3) {
			rx = (((int)map.player.x >> 6) << 6) + 64;
			ry = (map.player.x - rx) * nTan + map.player.y;
			xo = 64; yo = -xo * nTan;
		}
		if (ra > P2 && ra < P3) {
			rx = (((int)map.player.x >> 6) << 6) - 0.0001;
			ry = (map.player.x - rx) * nTan + map.player.y;
			xo = -64; yo = -xo * nTan;
		}
		if (ra == 0 || ra == PI) {
			rx = map.player.x; ry = map.player.y; dof = 16;
		}
		while (dof < 16) {
			mx = (int)(rx) >> 6;
			my = (int)(ry) >> 6;
			if (my >= 0 && mx >= 0 && my < map.sizeY && mx < map.sizeX && map.tileArray[my][mx] == WALL) {
				vx = rx; vy = ry;
				disV = dist(map.player.x, map.player.y, vx, vy);
				break;
			}
			else {
				rx += xo; ry += yo; dof++;
			}
		}
		char c = ' ';
		if (disV < disH) { rx = vx; ry = vy; disT = disV; c = '#'; }
		if (disH < disV) { rx = hx; ry = hy; disT = disH; c = '*'; }

		float ca = map.player.angle - ra;
		if (ca < 0) ca += 2 * PI;
		if (ca > 2 * PI) ca -= 2 * PI;

		disT *= cos(ca) + 0.0001;

		int lineH = (map.tileSize * v.sizeY) / disT;
		if (lineH > v.sizeY) lineH = v.sizeY;


		placeWall(v, r, 0, lineH, c);

		ra += DEG;
		if (ra < 0) ra += 2 * PI;
		if (ra > 2 * PI) ra -= 2 * PI;
	}
}

// ----------[ MAIN ]--------------

void handleInput(char key, Map& map, view& v) {
	switch (key)
	{
	case 'a':
		map.player.angle -= 0.1;
		if (map.player.angle < 0) map.player.angle += 2 * PI;
		map.player.deltaX = cos(map.player.angle) * 5;
		map.player.deltaY = sin(map.player.angle) * 5;
		break;
	case 'd':
		map.player.angle += 0.1;
		if (map.player.angle > 2 * PI) map.player.angle -= 2 * PI;
		map.player.deltaX = cos(map.player.angle) * 5;
		map.player.deltaY = sin(map.player.angle) * 5;
		break;
	case 'w':
		if (getTileFromPlayerCoords(map, map.player.x + map.player.deltaX, map.player.y) != WALL)
			map.player.x += map.player.deltaX;
		if (getTileFromPlayerCoords(map, map.player.x, map.player.y + map.player.deltaY) != WALL)
			map.player.y += map.player.deltaY;
		break;
	case 's':
		if (getTileFromPlayerCoords(map, map.player.x - map.player.deltaX, map.player.y) != WALL)
			map.player.x -= map.player.deltaX;
		if (getTileFromPlayerCoords(map, map.player.x, map.player.y - map.player.deltaY) != WALL)
			map.player.y -= map.player.deltaY;
		break;
	case 'e':
		v.map = !v.map;
	}

}

void mainLoop(view& v, Map& map) {
	char key{};
#if defined(_WIN32)
	while (true) {
		if (_kbhit()) {
			key = _getch();
			handleInput(key, map, v);
			castRays(v, map);
			if (v.map)
				viewMap2D(v, map);
			else
				renderView(v);
		}
	}
#elif defined(__linux__)
	struct termios old_termios, new_termios;
	tcgetattr(STDIN_FILENO, &old_termios);
	new_termios = old_termios;
	new_termios.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
	while (true) {
		if (read(STDIN_FILENO, &key, 1) > 0) {
			handleInput(key, map, v);
			castRays(v, map);
			if (v.map)
				viewMap2D(v, map);
			else
				renderView(v);
		}
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
#endif // Windows/Linux 
}

void init(view& v, Map& map) {
	map = generateMap();
	connectRooms(map);
	normalizeTiles(map);
	map.player.deltaX = cos(map.player.angle) * 5;
	map.player.deltaY = sin(map.player.angle) * 5;
	v = createView();
	castRays(v, map);
	renderView(v);
}

int main()
{
	view v;
	Map map;
	init(v, map);

	mainLoop(v, map);
}