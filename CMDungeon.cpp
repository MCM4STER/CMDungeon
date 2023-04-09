#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#elif defined(__linux__)
#include <sys/ioctl.h>
#endif // Windows/Linux

void getTerminalSize(int& width, int& height) {
#if defined(_WIN32)
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	width = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
	height = (int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
#elif defined(__linux__)
	struct winsize w;
	ioctl(fileno(stdout), TIOCGWINSZ, &w);
	width = (int)(w.ws_col);
	height = (int)(w.ws_row);
#endif // Windows/Linux
}

#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <limits>

using namespace std;

enum tileState
{
	AIR,
	ROOM_AIR,
	WALL,
	CORRIDOR_WALL,
	UNDESTRUCT_WALL,
	DOOR,
};
enum directions
{
	NORTH,
	WEST,
	EAST,
	SOUTH,
};

struct Tile
{
	int x{}, y{};
	tileState state = AIR;
};

struct Room
{
	int sizeX{}, sizeY{};
	int mapX{}, mapY{};
	int offsetX{}, offsetY{};
	vector<vector<Tile>> tiles;
	vector<directions> doors;

	int maxSizeX = 12, maxSizeY = 24;
};

struct Map {
	int viewSizeX{}, viewSizeY{};
	int mapSizeX{}, mapSizeY{};
	int playerX{}, playerY{};
	vector<vector<Room>> rooms;
	vector<vector<Tile>> map;

	int roomsX = 8, roomsY = 8;
};
//----------[ RANDOM FUNCTIONS ]--------------
int randInt(int min, int max)
{
	return (int)(rand() % (max - min)) + min;
}

directions randDirection(unsigned int seed)
{
	srand(seed);
	const directions allDir[4] = { NORTH, WEST, EAST, SOUTH };
	return allDir[randInt(0, 4)];
}

unsigned int getSeed(int x, int y, unsigned int globalSeed = 2137420)
{
	return (unsigned int)x + y + 420 * 2137 * 69 * globalSeed;
}

Tile randTile(Room room)
{
	return room.tiles[randInt(0, room.sizeX)][randInt(0, room.sizeY)];
}

char stateToChar(tileState s)
{
	switch (s)
	{
	case WALL:
	case CORRIDOR_WALL:
		return '#';
	case UNDESTRUCT_WALL:
		return 'W';
	default:
		return ' ';
	}
}

float getDistance(int x1, int y1, int x2, int y2) {
	return (float)sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

void cls() {
	system("cls");
}

bool isWalkthru(tileState state) {
	if (state == AIR || state == DOOR || state == ROOM_AIR) return true;
	return false;
}

//----------[ MAP FUNCTIONS ]-----------------
void placeEggs(int rX, int rY) {}

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

	for (int x = 0; x < randRoom.maxSizeX; x++)
	{
		vector<Tile> temp;
		for (int y = 0; y < randRoom.maxSizeY; y++)
		{
			Tile tile;
			tile.x = x;
			tile.y = y;

			if (
				(x == randRoom.offsetX && y < randRoom.sizeY + randRoom.offsetY && y >= randRoom.offsetY)
				|| (x == randRoom.sizeX + randRoom.offsetX - 1 && y < randRoom.sizeY + randRoom.offsetY && y> randRoom.offsetY)
				|| (y == randRoom.offsetY && x < randRoom.sizeX + randRoom.offsetX && x >= randRoom.offsetX)
				|| (y == randRoom.sizeY + randRoom.offsetY - 1 && x < randRoom.sizeX + randRoom.offsetX && x> randRoom.offsetX)
				)
				tile.state = WALL;
			if (x > randRoom.offsetX && x < randRoom.sizeX + randRoom.offsetX - 1 && y > randRoom.offsetY && y < randRoom.sizeY + randRoom.offsetY - 1)
				tile.state = ROOM_AIR;

			temp.push_back(tile);
		}
		randRoom.tiles.push_back(temp);
	}

	for (int i = 0; i < roomSeed % 3 + 2; i++)
	{
		directions randDir = randDirection(roomSeed + i);
		randRoom.doors.push_back(randDir);
		if (randDir == NORTH)
			randRoom.tiles[randRoom.offsetX][randInt(1 + randRoom.offsetY, randRoom.sizeY + randRoom.offsetY - 2)].state = DOOR;
		if (randDir == SOUTH)
			randRoom.tiles[randRoom.sizeX + randRoom.offsetX - 1][randInt(1 + randRoom.offsetY, randRoom.sizeY + randRoom.offsetY - 2)].state = DOOR;
		if (randDir == WEST)
			randRoom.tiles[randInt(1 + randRoom.offsetX, randRoom.sizeX + randRoom.offsetX - 2)][randRoom.offsetY].state = DOOR;
		if (randDir == EAST)
			randRoom.tiles[randInt(1 + randRoom.offsetX, randRoom.sizeX + randRoom.offsetX - 2)][randRoom.sizeY + randRoom.offsetY - 1].state = DOOR;
	}

	return randRoom;
}

void renderMap(Map& map)
{
	Room r;
	int view0X = map.playerX - map.viewSizeX / 2;
	int view0Y = map.playerY - map.viewSizeY / 2;

	if (view0X < 0) view0X = 0;
	if (view0Y < 0) view0Y = 0;
	if (view0X + map.viewSizeX >= map.mapSizeX) view0X = map.mapSizeX - map.viewSizeX;
	if (view0Y + map.viewSizeY >= map.mapSizeY) view0Y = map.mapSizeY - map.viewSizeY;

	cls();
	for (int x = view0X; x < view0X + map.viewSizeX; x++)
	{
		for (int y = view0Y; y < view0Y + map.viewSizeY; y++)
		{
			cout.width(2);
			if (map.playerX == x && map.playerY == y) { cout << "P"; continue; }
			cout << stateToChar(map.map[x][y].state);
			//cout <<map.map[x][y].state;
		}
		cout << endl;
	}
}

Tile findClosestDoor(Map& map, int sX, int sY) {
	Tile closest = map.map[sX][sY];
	float closestDist = FLT_MAX;

	for (int x = 0; x < map.mapSizeX; x++)
		for (int y = 0; y < map.mapSizeY; y++) {
			if (map.map[x][y].state != DOOR) continue;
			if (closestDist > getDistance(sX, sY, x, y)) {
				closestDist = getDistance(sX, sY, x, y);
				closest = map.map[x][y];
			}
		}

	return closest;
}

void connectRooms(Map& map) {
	//Pathfinding D:
	vector<vector<float>> tempTiles;

}

void generateMap(Map& map) {
	Room r;
	map.mapSizeX = map.roomsX * r.maxSizeX;
	map.mapSizeY = map.roomsY * r.maxSizeY;
	for (int x = 0; x < map.mapSizeX; x++) {
		vector<Tile> temp;
		for (int y = 0; y < map.mapSizeY; y++) {
			Tile tile;
			tile.x = x; tile.y = y;
			if (x == 0 || x == map.mapSizeX - 1 || y == 0 || y == map.mapSizeY - 1)
				tile.state = UNDESTRUCT_WALL;
			temp.push_back(tile);
		}
		map.map.push_back(temp);
	}

	for (int x = 0; x < map.roomsX; x++) {
		vector<Room> temp;
		for (int y = 0; y < map.roomsY; y++) {
			Room room = generateRandomRoom(x, y);

			int mapX = room.mapX * room.maxSizeX;
			int mapY = room.mapY * room.maxSizeY;

			for (int rX = 0; rX < room.maxSizeX; rX++)
				for (int rY = 0; rY < room.maxSizeY; rY++)
					if (map.map[mapX + rX][mapY + rY].state != UNDESTRUCT_WALL)
						map.map[mapX + rX][mapY + rY].state = room.tiles[rX][rY].state;

			temp.push_back(room);
		}
		map.rooms.push_back(temp);
	}

	Room centerRoom = map.rooms[map.roomsX / 2][map.roomsY / 2];
	map.playerX = centerRoom.mapX * r.maxSizeX + centerRoom.sizeX / 2;
	map.playerY = centerRoom.mapY * r.maxSizeY + centerRoom.sizeY / 2;
}

void movePlayer(Map& map, directions dir) {
	int newX = map.playerX, newY = map.playerY;
	switch (dir)
	{
	case NORTH:
		newX--;
		break;
	case WEST:
		newY--;
		break;
	case EAST:
		newY++;
		break;
	case SOUTH:
		newX++;
		break;
	}

	if (isWalkthru(map.map[newX][newY].state)) {
		map.playerX = newX;
		map.playerY = newY;
	}
}

void handleInput(Map& map) {
	char action;
	cin >> action;
	switch (action)
	{
	case 'w':
		movePlayer(map, NORTH);
		break;
	case 's':
		movePlayer(map, SOUTH);
		break;
	case 'a':
		movePlayer(map, WEST);
		break;
	case 'd':
		movePlayer(map, EAST);
		break;
	}
}

int main()
{
	Map map;
	getTerminalSize(map.viewSizeY, map.viewSizeX);
	map.viewSizeY /= 2;
	generateMap(map);
	renderMap(map);

	while (true) {
		handleInput(map);
		renderMap(map);
	}

	return 0;
}