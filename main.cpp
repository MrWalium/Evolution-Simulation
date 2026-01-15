/*
	Huuuuge Game of Life using Sparse Encoding
	"It's gonna be a fun one tomorrow boss" - javidx9

	Video: https://youtu.be/OqfHIujOvnE

	Use Left mouse button to stimulate a cell, right mouse button to
	stimulate many cells, space bar to run simulation and middle mouse
	to pan & zoom.

	License (OLC-3)
	~~~~~~~~~~~~~~~

	Copyright 2018 - 2025 OneLoneCoder.com

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.

	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Video:
	~~~~~~
	https://youtu.be/OqfHIujOvnE

	Links
	~~~~~
	YouTube:	https://www.youtube.com/javidx9
				https://www.youtube.com/javidx9extra
	Discord:	https://discord.gg/WhwHUMV
	X:			https://www.x.com/javidx9
	Twitch:		https://www.twitch.tv/javidx9
	GitHub:		https://www.github.com/onelonecoder
	Homepage:	https://www.onelonecoder.com

	Author
	~~~~~~
	David Barr, aka javidx9, �OneLoneCoder 2025
*/

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "olcPGEX_UI.h"

#define OLC_PGEX_TRANSFORMEDVIEW
#include "olcPGEX_TransformedView.h"

#include <unordered_set>
#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <string>
#include <limits>
#include <chrono>
#include <thread>
#include "wtypes.h"

class Animal {
	public:
	Animal(int xpos, int ypos, olc::Pixel newColor) : pos(olc::vi2d(xpos, ypos)), prevPos(olc::vi2d(xpos, ypos)), itersSinceRepro(0), color(newColor), isDead(false) {}
	Animal(olc::vi2d newPos, olc::Pixel newColor) : pos(newPos), prevPos(newPos), itersSinceRepro(0), color(newColor), isDead(false) {}
	virtual ~Animal() = default;
	
	virtual void update() = 0;
	virtual std::string getType() = 0;
	bool canReproduce() {return itersSinceRepro > 5;}
	void reproduced() {itersSinceRepro = 0;}
	olc::Pixel getColor() {return color;}
	int getX() { return pos.x;}
	int getY() { return pos.y;}
	int getPrevX() { return prevPos.x;}
	int getPrevY() { return prevPos.y;}
	olc::vi2d getPos() {return pos;}
	olc::vi2d getPrevPos() {return prevPos;}
	void move(int xpos, int ypos) {prevPos = pos; pos = olc::vi2d(xpos, ypos);}
	void move(olc::vi2d newPos) {prevPos = pos; pos = newPos;}
	void die() {isDead = true;}
	bool isAlive() {return !isDead;}

	protected:
	olc::vi2d pos;
	olc::vi2d prevPos;
	int itersSinceRepro;
	bool isDead;
	olc::Pixel color;

};

class Predator : public Animal {
	public:
	int const RADIUS = 5;

	Predator(int xpos, int ypos, olc::Pixel newColor) : Animal(xpos, ypos, newColor), itersSinceFood(0) {}
	Predator(olc::vi2d newPos, olc::Pixel newColor) : Animal(newPos, newColor), itersSinceFood(0) {}
	
	void update() override {
		itersSinceFood++;
		itersSinceRepro++;
		isDead = itersSinceFood > 5;
	}

	void eat() {
		itersSinceFood = 0;
	}

	std::string getType() {
		return "Predator";
	}
	
	private:
	int itersSinceFood;
};

class Prey : public Animal {
	public:
	Prey(int xpos, int ypos, olc::Pixel newColor) : Animal(xpos, ypos, newColor) {}
	Prey(olc::vi2d newPos, olc::Pixel newColor) : Animal(newPos, newColor) {}

	void update() {
		itersSinceRepro++;
	}

	std::string getType() {
		return "Prey";
	}

};

struct HASH_OLC_VI2D
{
	std::size_t operator()(const olc::vi2d &v) const
	{
		return int64_t(v.y << sizeof(int32_t) | v.x);
	}
};

class SparseEncodedGOL : public olc::PixelGameEngine
{
public:
	SparseEncodedGOL()
	{
		sAppName = "Huge Game Of Life";
		getDesktopResolution();
	}

	int getScreenWidth() { return screen_width; }

	int getScreenHeight() { return screen_height; }

protected:
	enum class landType {
		NONE,
		OCEAN,
		BEACH,
		FOREST,
		MOUNTAIN,
		SNOW
	};

	std::unordered_set<olc::vi2d, HASH_OLC_VI2D> setActive;
	std::unordered_set<olc::vi2d, HASH_OLC_VI2D> setActiveNext;
	std::unordered_set<olc::vi2d, HASH_OLC_VI2D> setPotential;
	std::unordered_set<olc::vi2d, HASH_OLC_VI2D> setPotentialNext;

	std::unordered_set<std::unique_ptr<Predator>> predators;
	std::unordered_set<std::unique_ptr<Prey>> preys;
	std::unordered_map<olc::vi2d, Animal*, HASH_OLC_VI2D> occupancy;

	std::vector<std::vector<float>> terrain;
	std::vector<std::vector<landType>> land;
	olc::TransformedView tv;
	olc::UI_CONTAINER myUI;

	std::unique_ptr<olc::Sprite> terrainSprite;
	std::unique_ptr<olc::Decal> terrainDecal;

	std::chrono::system_clock::time_point currTime = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point prevTime = std::chrono::system_clock::now();

	int screen_width = 0;
	int screen_height = 0;
	int terrainSize;
	bool terrainDisplayed = false;

	float const OCEAN_LIM = -0.2;
	float const BEACH_LIM = 0.0;
	float const MOUNT_LIM = 0.3;
	float const SNOW_LIM = 0.5;
	int FPS = 3;

	// Will be used to obtain a seed for the random number engine
	// Standard mersenne_twister_engine seeded with rd()
	std::mt19937 generator{std::random_device{}()};

protected:
	bool OnUserCreate() override
	{

		tv.Initialise(GetScreenSize());
		// 137 w
		// 77 h
		tv.SetWorldScale({10.0f, 10.0f});
		// myUI.ToggleDEBUGMODE();

		terrainSize = std::pow(2, std::ceil(std::log2(static_cast<float>(std::max(screen_width, screen_height)) / 10))) + 1;
		float roughnessDelta = 0.6; // from 0 - 1, the smaller the smoother the results
		makeTerrain(roughnessDelta);
		cacheTerrain();
		displayTerrain();

		preys.insert(std::make_unique<Prey>(20, 20, olc::Pixel(255, 0, 0)));

		myUI.addNewButton(UIStyle::UI_RED, olc::Key::Q, false, "EXIT", screen_width - 40, 0, 40, 20, "EXIT");
		// myUI.addNewDropDown(UI_BLACK, UI_BLACK, screen_width - 15, 20, 15, "<", "FIRST,SECOND,EXIT", "CMD_1,CMD_2,EXIT");

		return true;
	}

	void fixedAvg(int i, int j, int v, float roughness, int (&offsets)[4][2])
	{
		float sum = 0.0f;
		int count = 0;

		for (auto &offset : offsets)
		{
			int x = i + offset[0] * v;
			int y = j + offset[1] * v;
			if (0 <= x && x < terrainSize && 0 <= y && y < terrainSize)
			{
				sum += terrain[x][y];
				count++;
			}
		}

		std::uniform_real_distribution<float> randomness(-roughness, roughness);
		terrain[i][j] = ((sum / static_cast<float>(count)) + randomness(generator));
	}

	void diamondSquareStep(int cellLen, float roughness)
	{
		// distance from new cell to nbs to average over
		int v = std::floor(cellLen / 2);

		// offsets
		int diamondOffsets[4][2] = {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}};
		int squareOffsets[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

		// Diamond Step
		for (int i = v; i < terrainSize; i += cellLen)
		{
			for (int j = v; j < terrainSize; j += cellLen)
			{
				fixedAvg(i, j, v, roughness, diamondOffsets);
			}
		}

		// Square Step with rows
		for (int i = v; i < terrainSize; i += cellLen)
		{
			for (int j = 0; j < terrainSize; j += cellLen)
			{
				fixedAvg(i, j, v, roughness, squareOffsets);
			}
		}

		// Square Step with columns
		for (int i = 0; i < terrainSize; i += cellLen)
		{
			for (int j = v; j < terrainSize; j += cellLen)
			{
				fixedAvg(i, j, v, roughness, squareOffsets);
			}
		}
	}

	void initCorners(int arrSize)
	{
		terrain[0][0] = 0.0f;
		terrain[0][arrSize - 1] = 0.0f;
		terrain[arrSize - 1][0] = 0.0f;
		terrain[arrSize - 1][arrSize - 1] = 0.0f;
	}

	void makeTerrain(float roughnessDelta)
	{
		terrain.clear();
		terrain.resize(terrainSize, std::vector<float>(terrainSize, 0));
		initCorners(terrainSize);

		int cellLen = terrainSize - 1;
		float roughness = 1.0;

		while (cellLen > 1)
		{
			diamondSquareStep(cellLen, roughness);

			cellLen = std::floor(cellLen / 2);
			roughness *= roughnessDelta;
		}

		// printTerrainArray();
	}

	void printTerrainArray()
	{
		for (auto &row : terrain)
		{
			for (float value : row)
			{
				std::cout << value << " ";
			}
			std::cout << std::endl;
		}
	}

	void displayTerrain(bool usingOld = false)
	{
		if (!usingOld)
		{
			if (!terrainDecal)
				return;

			tv.DrawDecal({0, 0}, terrainDecal.get(), {1.0f, 1.0f});
		}
		else
		{

			int rows = std::ceil(static_cast<float>(screen_width) / 10);
			int columns = std::ceil(static_cast<float>(screen_height) / 10);
			for (int i = 0; i < rows; i++)
			{
				for (int j = 0; j < columns; j++)
				{
					int luminosity = std::clamp(static_cast<int>(std::abs(terrain[i][j]) * 255), 0, 255);
					tv.FillRectDecal(olc::vi2d(i, j), olc::vi2d(1, 1), olc::Pixel(luminosity, luminosity, luminosity));
				}
			}
		}
	}

	void cacheTerrain()
	{
		int rows = std::ceil(static_cast<float>(screen_width) / 10);
		int columns = std::ceil(static_cast<float>(screen_height) / 10);

		land.clear();
		land.resize(rows, std::vector<landType>(columns, landType::NONE));

		terrainSprite.reset(new olc::Sprite(rows, columns));

		for (int i = 0; i < rows; i++)
		{
			for (int j = 0; j < columns; j++)
			{
				int luminosity = /*std::clamp()*/ static_cast<int>(terrain[i][j] * 255) /*, 0, 255)*/;
				// --- OCEAN DIVIDER --- //
				if (terrain[i][j] < OCEAN_LIM)
				{
					float value = (terrain[i][j] + 1) / (OCEAN_LIM + 1);
					int darkBlue[3] = {0, 0, 53};
					int lightBlue[3] = {135, 206, 250};

					int r = int(darkBlue[0] + value * (lightBlue[0] - darkBlue[0]));
					int g = int(darkBlue[1] + value * (lightBlue[1] - darkBlue[1]));
					int b = int(darkBlue[2] + value * (lightBlue[2] - darkBlue[2]));
					terrainSprite->SetPixel(i, j, olc::Pixel(r, g, b, 200));
					land[i][j] = landType::OCEAN;
				}
				// --- BEACH BIOM --- //
				else if (terrain[i][j] >= OCEAN_LIM && terrain[i][j] <= BEACH_LIM)
				{
					std::uniform_real_distribution<float> randomness(0, 1);
					float value = randomness(generator);
					int r = 255;
    				int g = 200 + static_cast<int>(55 * value);
    				int b = static_cast<int>(20.0 * (1.0 - value));
					terrainSprite->SetPixel(i, j, olc::Pixel(r, g, b, 200));
					land[i][j] = landType::BEACH;
				// --- MOUNTAIN BIOM --- //
				} else if(terrain[i][j] > MOUNT_LIM && terrain[i][j] < SNOW_LIM) {
					float value = terrain[i][j] / MOUNT_LIM;
					int darkGrey[3] = {51, 51, 51};
					int lightGrey[3] = {170, 170, 170};

					int r = int(darkGrey[0] + value * (lightGrey[0] - darkGrey[0]));
					int g = int(darkGrey[1] + value * (lightGrey[1] - darkGrey[1]));
					int b = int(darkGrey[2] + value * (lightGrey[2] - darkGrey[2]));
					terrainSprite->SetPixel(i, j, olc::Pixel(r, g, b, 200));
					land[i][j] = landType::MOUNTAIN;
				// --- SNOW BIOM --- //
				} else if(terrain[i][j] >= SNOW_LIM) {
					terrainSprite->SetPixel(i, j, olc::Pixel(255, 250, 250, 200));
					land[i][j] = landType::SNOW;
				// --- FORREST BIOM --- //
				} else {
					float value = terrain[i][j] / MOUNT_LIM;
					int darkGreen[3] = {0, 100, 0};
					int lightGreen[3] = {0, 186, 0};

					int r = int(darkGreen[0] + value * (lightGreen[0] - darkGreen[0]));
					int g = int(darkGreen[1] + value * (lightGreen[1] - darkGreen[1]));
					int b = int(darkGreen[2] + value * (lightGreen[2] - darkGreen[2]));
					terrainSprite->SetPixel(i, j, olc::Pixel(r, g, b, 200));
					land[i][j] = landType::FOREST;
				}
			}
		}
		terrainDecal.reset(new olc::Decal(terrainSprite.get()));
	}
	
	int distSquared(olc::vi2d from, olc::vi2d to) {
		int dx = from.x - to.x;
		int dy = from.y - to.y;
		return dx * dx + dy * dy;
	}
	
	olc::vi2d stepTowords(olc::vi2d from, olc::vi2d to, bool forPred=true) {
		//olc::vi2d delta = from - to;
		std::vector<olc::vi2d> possibleMovements;

		for(int i=-1; i<=1; i++) {
			for(int j=-1; j<=1; j++) {
				if(i == 0 && j == 0) continue;
				if(forPred) {
					if(walkable(olc::vi2d(from.x + i, from.y + j), true)) {
						possibleMovements.push_back(olc::vi2d(from.x + i, from.y + j));
					}
				} else {
					if(walkable(olc::vi2d(from.x + i, from.y + j))) {
						possibleMovements.push_back(olc::vi2d(from.x + i, from.y + j));
					}
				}
			}
		}

		if(possibleMovements.size() == 0) {return from;}

		int best = 0;
		for(int i=0; i<possibleMovements.size(); i++) {
			if(distSquared(possibleMovements[best], to) > distSquared(possibleMovements[i], to)) {
				best = i;
			}
		}

		return possibleMovements[best];
		// return olc::vi2d(
		// 	delta.x == 0 ? 0 : delta.x > 0 ? 1 : -1,
		// 	delta.y == 0 ? 0 : delta.y > 0 ? 1 : -1
		// );
	}

	int colorDiff(olc::Pixel color1, olc::Pixel color2) {
		return std::floor(static_cast<float>(abs(color1.r - color2.r) + abs(color1.g - color2.g) + abs(color1.b - color2.b)) / 3.0);
	}
	
	olc::vi2d avoidPredators(olc::vi2d pos, olc::vi2d prevPos, olc::Pixel color) {
		std::vector<olc::vi2d> possibleMovements;
		std::vector<olc::vi2d> preds;

		for(int i=-1; i<=1; i++) {
			for(int j=-1; j<=1; j++) {
				if(i == 0 && j == 0) continue;
				olc::vi2d possiblePos = olc::vi2d(pos.x + j, pos.y + i);
				if(walkable(possiblePos)) {
					possibleMovements.push_back(possiblePos);
				}

				auto occupied = occupancy.find(possiblePos);
				if(occupied != occupancy.end()) {
					Predator* pred = dynamic_cast<Predator*>(occupied->second);
					if(pred && (*pred).isAlive()) {
						preds.push_back(possiblePos);
					}
				}
				
			}
		}

		if(possibleMovements.empty()) {return pos;}
		if(possibleMovements.size() == 1) {return possibleMovements[0];}

		if(preds.size() == 0) {
			//std::vector<int> colorDiffs;
			std::uniform_real_distribution<> wander(0.0f, 1.0f);
			if(wander(generator) <= 0.15) {
				std::uniform_int_distribution<> randMove(0, possibleMovements.size() - 1);
				return possibleMovements[randMove(generator)]; 
			}
			int best = 0;
			int bestDiff = std::numeric_limits<int>::max();
			for(int i=0; i<possibleMovements.size(); i++) {
				olc::vi2d possiblePos = possibleMovements[i];
				std::uniform_int_distribution<> randomness(-15, 15);
				int diffColor = abs(colorDiff(terrainSprite->GetPixel(possiblePos.x, possiblePos.y), color) + randomness(generator));
				std::cout << prevPos.x << " " << prevPos.y << std::endl;
				if(diffColor < bestDiff && (possiblePos.x != prevPos.x || possiblePos.y != prevPos.y)) {
					bestDiff = diffColor;
					best = i;
				}
			}
			return possibleMovements[best];
		}

		int best = 0;
		int bestDist = 0;
		for(int i=0; i<possibleMovements.size(); i++) {
			int smallestDist = std::numeric_limits<int>::max();
			for(const auto& pred: preds) {
				if(distSquared(pred, possibleMovements[i]) < smallestDist) {
					smallestDist = distSquared(pred, possibleMovements[i]);
				}
			}
			if(smallestDist > bestDist) {
				best = i;
				bestDist = smallestDist;
			}
		}

		return possibleMovements[best];
	}
	
	olc::vi2d moveRandom(olc::vi2d pos, bool isPrey=false) {
		std::vector<olc::vi2d> possibleMovements;

		for(int i=-1; i<=1; i++) {
			for(int j=-1; j<=1; j++) {
				if(walkable(olc::vi2d(pos.x + i, pos.y + j), !isPrey)) {
					possibleMovements.push_back(olc::vi2d(pos.x + i, pos.y + j));
				}
			}
		}

		if (possibleMovements.empty()) return pos;

		if(isPrey) {
			// move based on color
			std::uniform_int_distribution<> randMove(0, possibleMovements.size() - 1);
			return possibleMovements[randMove(generator)];
		} else {
			std::uniform_int_distribution<> randMove(0, possibleMovements.size() - 1);
			return possibleMovements[randMove(generator)];
		}
	}
	
	bool walkable(olc::vi2d cell, bool forPred=false) {
		if (cell.y < 0 || cell.y >= (int)land.size() || cell.x < 0 || cell.x >= (int)land[0].size() || land[cell.y][cell.x] == landType::OCEAN) {
			return false;
		}

		if(forPred) {
			auto occupied = occupancy.find(cell);
			if(occupied != occupancy.end()) {
				Prey* prey = dynamic_cast<Prey*>(occupied->second);
				return prey && (*prey).isAlive();
			}
			return true;
		}
		return occupancy.find(cell) == occupancy.end();
	}
	
	int GetCellState(const olc::vi2d &in)
	{
		return setActive.find(in) != setActive.end() ? 1 : 0;
	}

	void cleanCollections(bool clearPreds, bool clearPreys, bool doBoth=false) {
		if(clearPreds && clearPreys && doBoth) {
			for(auto it = occupancy.begin(); it != occupancy.end();) {
				if(!it->second->isAlive()) {
					it = occupancy.erase(it);
				} else {
					++it;
				}
			}
		}
		if(clearPreds) {
			for (auto it = predators.begin(); it != predators.end();) {
    			if (!(*(*it)).isAlive()) {
        			occupancy.erase((*it)->getPos());
        			it = predators.erase(it); // returns next iterator
				} else {
       				++it;
    			}
			}
		}

		
		if(clearPreys) {
			for (auto it = preys.begin(); it != preys.end();) {
    			if (!(*(*it)).isAlive()) {
        			occupancy.erase((*it)->getPos());
        			it = preys.erase(it); // returns next iterator
				} else {
       				++it;
    			}
			}
		}
		
	}

	void rebuildOccupancy() {
		occupancy.clear();

		for(const auto& preyPtr: preys) {
			Prey& prey = *preyPtr;
			if(prey.isAlive()) {
				occupancy.insert_or_assign(prey.getPos(), &prey);
			}
		}

		for(const auto& predator: predators) {
			Predator& pred = *predator;
			if(pred.isAlive()) {
			occupancy.insert_or_assign(pred.getPos(), &pred);
			}
		}
	}
	
	olc::Pixel addColorVariance(olc::Pixel color, int strength) {
		std::uniform_int_distribution colorVariance(-strength, strength);
		int r = color.r + colorVariance(generator);
		int g = color.g + colorVariance(generator);
		int b = color.b + colorVariance(generator);
		return olc::Pixel(r, g, b);
	}
	
	void reproduce(olc::vi2d pos, olc::Pixel color=olc::Pixel(255, 0, 0), bool forPred=true) {
		std::vector<olc::vi2d> possibleMovements;
		for(int i=-1; i<=1; i++) {
			for(int j=-1; j<=1; j++) {
				if(walkable(olc::vi2d(pos.x + i, pos.y + j))) {
					possibleMovements.push_back(olc::vi2d(pos.x + i, pos.y + j));
				}
			}
		}

		if(possibleMovements.empty()) {return;}

		if(forPred) {
			std::uniform_int_distribution reproRand(1, 3);
			int reproTimes = reproRand(generator);
			if(possibleMovements.size() < reproTimes) {reproTimes = possibleMovements.size();}
			for(int i=0; i<reproTimes; i++) {
				std::uniform_int_distribution<> randMove(0, possibleMovements.size() - 1);
				olc::vi2d reproPos = possibleMovements[randMove(generator)];
				auto babyPred = std::make_unique<Predator>(reproPos, color);

				Predator* babyPtr = babyPred.get();

				predators.insert(std::move(babyPred));
				occupancy.insert_or_assign(reproPos, babyPtr);
			}
		} else {
			std::uniform_int_distribution reproRand(1, 7);
			int reproTimes = reproRand(generator);
			if(possibleMovements.size() < reproTimes) {reproTimes = possibleMovements.size();}
			for(int i=0; i<reproTimes; i++) {
				std::uniform_int_distribution<> randMove(0, possibleMovements.size() - 1);
				olc::vi2d reproPos = possibleMovements[randMove(generator)];
				auto babyPrey = std::make_unique<Prey>(reproPos, addColorVariance(color, 5));

				Prey* babyPtr = babyPrey.get();

				preys.insert(std::move(babyPrey));
				occupancy.insert_or_assign(reproPos, babyPtr);
			}
		}
	}
	
	void updatePredators() {
		for(const auto& predator: predators) {
			Predator& pred = *predator;
			int x = pred.getX();
			int y = pred.getY();
			const int RADIUS = pred.RADIUS;
			std::vector<std::array<int, 3>> listPreys;
			pred.update();
			if(!pred.isAlive()) {continue;}
			for(int i=y-RADIUS; i<=y+RADIUS; i++) {
				for(int j=x-RADIUS; j<=x+RADIUS; j++) {
					int dx = j-x;
					int dy = i-y;
					if(dx*dx + dy*dy <= RADIUS*RADIUS) {
						auto cell = occupancy.find(olc::vi2d(j, i));
						if(cell != occupancy.end()) {
							Prey* prey = dynamic_cast<Prey*>(cell->second);
							if(prey && (*prey).isAlive()) {
								listPreys.push_back(std::array<int, 3>{cell->second->getX(), cell->second->getY(), 0});
							}
						}
					}
				}
			}

			occupancy.erase(pred.getPos());
			if(listPreys.size() == 0) {
				pred.move(moveRandom(pred.getPos(), false));
			} else {
				std::sort(listPreys.begin(), listPreys.end(), 
					[](const std::array<int, 3>& a, const std::array<int, 3>& b) {
						return a.back() < b.back(); // Use .back() to access the last element
					});
				if(listPreys.size() != 0) {
					pred.move(stepTowords(olc::vi2d(x, y), olc::vi2d(listPreys[0][0], listPreys[0][1])));
					
				}
			}
			auto cell = occupancy.find(pred.getPos());
			if(cell != occupancy.end()) {
				Prey* prey = dynamic_cast<Prey*>(cell->second);
				if(prey && (*prey).isAlive()) {
					pred.eat();
					(*prey).die();
				}
			}
			occupancy.insert_or_assign(pred.getPos(), &pred);
		}
	}

	void updatePreys() {
		for(const auto& preyptr: preys) {
			Prey& prey = *preyptr;
			int x = prey.getX();
			int y = prey.getY();
			prey.update();
			if(!prey.isAlive()) {continue;}

			occupancy.erase(prey.getPos());
			prey.move(avoidPredators(prey.getPos(), prey.getPrevPos(), prey.getColor()));
			occupancy.insert_or_assign(prey.getPos(), &prey);

			if(prey.canReproduce()) {reproduce(prey.getPos(), prey.getColor(), false);}
		}
		cleanCollections(false, true);
	}

	void drawAnimals() {
		for(const auto& preyPtr: preys) {
			Prey& prey = *preyPtr;
			if(prey.isAlive()) {
				if (tv.IsRectVisible(prey.getPos(), olc::vi2d(1, 1))) {
					tv.FillRectDecal(prey.getPos(), olc::vi2d(1, 1), prey.getColor());
				}
			}
		}

		for(const auto& predator: predators) {
			Predator& pred = *predator;
			if(pred.isAlive()) {
				if (tv.IsRectVisible(pred.getPos(), olc::vi2d(1, 1))) {
					tv.FillRectDecal(pred.getPos(), olc::vi2d(1, 1), pred.getColor());
				}
			}
		}
	}
	
	void limitFPS(bool debug=false) {
		currTime = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> work_time = currTime - prevTime;

		if(work_time.count() < 1000.0 / static_cast<float>(FPS)) {
			std::chrono::duration<double, std::milli> delta_ms(1000.0 / static_cast<float>(FPS) - work_time.count());
            auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
		}

		prevTime = std::chrono::system_clock::now();
		if(debug) {
			std::chrono::duration<double, std::milli> sleep_time = prevTime - currTime;
			printf("Time: %f \n", (work_time + sleep_time).count());
		}
	}
	
	std::string landTypeToString(landType landt) {
		switch(landt) {
			case landType::OCEAN:
				return "OCEAN";
			case landType::BEACH:
				return "BEACH";
			case landType::FOREST:
				return "FOREST";
			case landType::MOUNTAIN:
				return "MOUNTAIN";
			case landType::SNOW:
				return "SNOW";
			default:
				return "NONE";
		}
		return "NONE";
	}
	
	void mouseDebug() {
		auto m = tv.ScreenToWorld(GetMousePos());
		std::cout << landTypeToString(land[std::floor(m.x)][std::floor(m.y)]) << " (" << m.x << ", " << m.y << ")" << std::endl;
	}
	
	bool OnUserUpdate(float fElapsedTime) override
	{
		// // tv.HandlePanAndZoom();

		Clear(olc::BLACK);

		displayTerrain();

		if (GetKey(olc::Key::SPACE).bHeld)
		{
			

			setActive = setActiveNext;
			setActiveNext.clear();
			setActiveNext.reserve(setActive.size());

			setPotential = setPotentialNext;

			// We know all active cells this epoch have potential to stimulate next epoch
			setPotentialNext = setActive;

			for (const auto &c : setPotential)
			{
				// Cell has changed, apply rules

				// The secret of artificial life =================================================
				int nNeighbours =
					GetCellState(olc::vi2d(c.x - 1, c.y - 1)) +
					GetCellState(olc::vi2d(c.x - 0, c.y - 1)) +
					GetCellState(olc::vi2d(c.x + 1, c.y - 1)) +
					GetCellState(olc::vi2d(c.x - 1, c.y + 0)) +
					GetCellState(olc::vi2d(c.x + 1, c.y + 0)) +
					GetCellState(olc::vi2d(c.x - 1, c.y + 1)) +
					GetCellState(olc::vi2d(c.x + 0, c.y + 1)) +
					GetCellState(olc::vi2d(c.x + 1, c.y + 1));

				if (GetCellState(c) == 1)
				{
					// if cell is alive and has 2 or 3 neighbours, cell lives on
					int s = (nNeighbours == 2 || nNeighbours == 3);

					if (s == 0)
					{
						// Kill cell through activity omission

						// Neighbours are stimulated for computation next epoch
						for (int y = -1; y <= 1; y++)
							for (int x = -1; x <= 1; x++)
								setPotentialNext.insert(c + olc::vi2d(x, y));
					}
					else
					{
						// No Change - Keep cell alive
						setActiveNext.insert(c);
					}
				}
				else
				{
					int s = (nNeighbours == 3);
					if (s == 1)
					{
						// Spawn cell
						setActiveNext.insert(c);

						// Neighbours are stimulated for computation next epoch
						for (int y = -1; y <= 1; y++)
							for (int x = -1; x <= 1; x++)
								setPotentialNext.insert(c + olc::vi2d(x, y));
					}
					else
					{
						// No Change - Keep cell dead
					}
				}
				// ===============================================================================
			}
		}

		if (GetMouse(0).bHeld)
		{
			auto m = tv.ScreenToWorld(GetMousePos());

			setActiveNext.insert(m);
			setActive.insert(m);

			for (int y = -1; y <= 1; y++)
				for (int x = -1; x <= 1; x++)
					setPotentialNext.insert(m + olc::vi2d(x, y));
		}

		if (GetMouse(1).bHeld)
		{
			auto m = tv.ScreenToWorld(GetMousePos());

			for (int i = -50; i <= 50; i++)
				for (int j = -50; j < 50; j++)
				{
					std::uniform_int_distribution<> randomness(0, 2);
					if (randomness(generator))
					{
						setActiveNext.insert(m + olc::vi2d(i, j));
						setActive.insert(m + olc::vi2d(i, j));

						for (int y = -1; y <= 1; y++)
							for (int x = -1; x <= 1; x++)
								setPotentialNext.insert(m + olc::vi2d(i + x, j + y));
					}
				}
		}

		if (GetKey(olc::Key::C).bPressed)
		{
			setActive.clear();
			setActiveNext.clear();
			setPotential.clear();
			setPotentialNext.clear();
		}

		size_t nDrawCount = 0;
		for (const auto &c : setActive)
		{
			if (tv.IsRectVisible(olc::vi2d(c), olc::vi2d(1, 1)))
			{
				tv.FillRectDecal(olc::vi2d(c), olc::vi2d(1, 1), olc::Pixel(255, 30, 75));
				nDrawCount++;
			}
		}

		updatePreys();
		updatePredators();
		
		drawAnimals();
		cleanCollections(true, true);
		rebuildOccupancy();
		
		DrawStringDecal({2, 2}, "Active: " + std::to_string(setActiveNext.size()) + " / " + std::to_string(setPotentialNext.size()) + " : " + std::to_string(nDrawCount));

		// this handles the update of all items
		myUI.Update(fElapsedTime);
		// if any button in the current UI sends the command EXIT, letsd exit
		if (myUI.hasCommand("EXIT", false))
			return 0;
		// if (myUI.getbtnPressed() == 1)
		// {
		// 	myUI.setW(1, 50);
		// 	myUI.setX(1, screen_width - 50);
		// 	myUI.setText(1, ">");
		// 	myUI.setText(1, ">");
		// }
		// This draws all items in the UI
		myUI.drawUIObjects();
		// lets also draw all current commands to the screen
		std::string myOut = myUI.getAllCmds();

		limitFPS(true);

		return !GetKey(olc::Key::ESCAPE).bPressed;
	}

	// Get the horizontal and vertical screen sizes in pixel
	void getDesktopResolution()
	{
		RECT desktop;
		// Get a handle to the desktop window
		const HWND hDesktop = GetDesktopWindow();
		// Get the size of screen to the variable desktop
		GetWindowRect(hDesktop, &desktop);
		// The top left corner will have coordinates (0,0)
		// and the bottom right corner will have coordinates
		// (horizontal, vertical)
		screen_width = desktop.right;
		screen_height = desktop.bottom;
	}
};

int main()
{
	SparseEncodedGOL demo;
	if (demo.Construct(/*1280, 960*/ demo.getScreenWidth(), demo.getScreenHeight(), 1, 1, true))
		demo.Start();

	return 0;
}