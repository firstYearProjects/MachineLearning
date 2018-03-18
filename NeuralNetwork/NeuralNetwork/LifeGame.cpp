﻿#include "LifeGame.h"
#include "Player.h"

int getMax(int a, int b)
{
	return ((a > b) ? a : b);
}

bool cmpDead(const deadPlayer* a, const deadPlayer* b)
{
	return (a->lastStep > b->lastStep);
}

////////////////////////////////////////////////////////////////////

LifeGame::LifeGame(int n, int m, int  numberOfPlayers, unsigned int evolution) :_n(n), _m(m), _evolution(evolution)
{
	this->_field = new Field(n, m);
	for (int i = 0; i < numberOfPlayers; i++)
	{
		Player* tmpPlayer = new Player(n, m, i, this);
		this->players.push_back(tmpPlayer);
	}
}

LifeGame::LifeGame(int n, int m, int numberOfPlayers, unsigned int evolution, bool echo) :_n(n), _m(m), _evolution(evolution)
{
	this->_echo = echo;
	this->_field = new Field(n, m);
	for (int i = 0; i < numberOfPlayers; i++)
	{
		Player* tmpPlayer = new Player(n, m, i, this, echo);
		this->players.push_back(tmpPlayer);
	}
}

LifeGame::~LifeGame()
{
	vector<NeuralNetwork*> neurals;

	for (auto it = this->players.begin();
		it != this->players.end(); ++it)
	{

		if (*it != nullptr)
		{
			NeuralNetwork* tmp = (*it)->neuro;
			if (find(neurals.begin(), neurals.end(), tmp) == neurals.end())
			{
				neurals.push_back(tmp);
			}
			delete (*it);
		}
	}
	for (auto it = this->deadPlayers.begin();
		it != this->deadPlayers.end(); ++it)
	{
		if ((*it)->_player != nullptr)
		{
			NeuralNetwork* tmp = (*it)->_player->neuro;
			if (find(neurals.begin(), neurals.end(), tmp) == neurals.end())
			{
				neurals.push_back(tmp);
			}
			delete (*it)->_player;
		}
	}

	for (auto neural = neurals.begin(); neural != neurals.end(); ++neural) // HERE
	{
		delete (*neural);
	}

	for (auto it = this->food.begin();
		it != this->food.end(); ++it)
	{
		delete (*it);
	}
}

void LifeGame::setPause(unsigned int time_)
{
	this->_pause = time_;
}

void LifeGame::initializeAllWithRnd(float a, float b)
{
	for (auto it = this->players.begin(); it != this->players.end(); ++it)
	{
		(*it)->neuro->setLayerWeights("input", 0.0f);
		(*it)->neuro->setLayerWeights("between1", 0.0f);
		(*it)->neuro->setLayerWeights("between2", 0.0f);
		(*it)->mutate(a, b);
	}
}

void LifeGame::echo(bool value)
{
	this->_echo = value;
}

void LifeGame::echoPrint(bool value)
{
	this->_echoPrint = value;
}

void LifeGame::step()
{
	for (auto it = this->players.begin(); it != this->players.end(); ++it)
	{
		float* params = formInputVector((*it));
		(*it)->activateNeuro(params);

		delete[] params;
	}
	//check on repeats in pair of coords
		//if so - >
			// kill player with lesser health
	for (int i = 0; i < this->players.size(); i++)
	{
		for (int k = i + 1; k < this->players.size(); k++)
		{
			Player* iPlayer = this->players[i];
			Player* kPlayer = this->players[k];
			if (iPlayer->get_X() == kPlayer->get_X() &&
				iPlayer->get_Y() == kPlayer->get_Y())
			{
				if (this->players[i]->getHealth() >= this->players[k]->getHealth())
				{
					bool alreadyExists = 0;
					for (auto dead = this->deadPlayers.begin(); dead != this->deadPlayers.end(); ++dead)
					{
						if ((*dead)->_player == this->players[k])
							alreadyExists = 1;
					}
					if (!alreadyExists)
					{
						deadPlayer* tmpDead = new deadPlayer();
						tmpDead->_player = this->players[k];
						tmpDead->lastStep = this->_step;

						this->deadPlayers.push_back(tmpDead);
					}
					this->players[i]->addHealth(FOODVALUE);
				}
				else
				{
					bool alreadyExists = 0;
					for (auto dead = this->deadPlayers.begin(); dead != this->deadPlayers.end(); ++dead)
					{
						if ((*dead)->_player == this->players[i])
							alreadyExists = 1;
					}
					if (!alreadyExists)
					{
						deadPlayer* tmpDead = new deadPlayer();
						tmpDead->_player = this->players[i];
						tmpDead->lastStep = this->_step;

						this->deadPlayers.push_back(tmpDead);
					}
					this->players[k]->addHealth(FOODVALUE);
				}
			}
		}
	}

	for (auto it = this->deadPlayers.begin(); it != this->deadPlayers.end(); ++it)
	{
		auto tmp = find(this->players.begin(), this->players.end(), (*it)->_player);
		if (tmp != this->players.end())
			tmp = this->players.erase(tmp);
	}

	// if alive player stays on food cell health++
	for (auto pl = this->players.begin(); pl != this->players.end(); ++pl) // S(0)
	{
		for (auto fd = this->food.begin(); fd != this->food.end(); ++fd)
		{
			if ((*pl)->get_X() == (*fd)->pos_x)
			{
				if ((*pl)->get_Y() == (*fd)->pos_y)
				{
					(*pl)->addHealth(FOODVALUE);
					delete (*fd);
					fd = this->food.erase(fd);

					break; // moves to next iteration of S(0) after decreasing
				}
			}
		}
		// every player health --
		(*pl)->addHealth(-5.0f);
		// check if dead
		// put all dead players into vector of dead with mark of current step number
		if ((*pl)->getHealth() <= 0)
		{
			deadPlayer* tmpDead = new deadPlayer();
			tmpDead->_player = (*pl);
			tmpDead->lastStep = this->_step;

			this->deadPlayers.push_back(tmpDead);
		}
	}

	for (auto it = this->deadPlayers.begin(); it != this->deadPlayers.end(); ++it)
	{
		auto tmp = find(this->players.begin(), this->players.end(), (*it)->_player);
		if (tmp != this->players.end())
			tmp = this->players.erase(tmp);
	}

	this->_field->clearField(); // make all cells ' ' then we will rewrite every symbol

	// Reassigne field symbols
	assigneCells();

	// generate new food
	int foodMax = (this->_m * this->_n) / 10;
	while (this->food.size() < foodMax)
	{
		generateFood();
	}

	this->_step++;

	if (_echoPrint == 1)
	{
		system("cls");
		this->_field->printField();
		Sleep(this->_pause);
	}
	// Go to beginning
}

void LifeGame::play()
{
	while (this->players.size() != 0 && this->_step < MAXSTEPS)
	{
		step();
	}
	system("cls");
	cout << '\n' << "     *Results*\n";
	for (int i = 0; i < this->deadPlayers.size(); i++)
	{
		deadPlayer* tmp = this->deadPlayers[i];
		cout << " player_" << tmp->_player->getID() << "  " << tmp->lastStep << "\n";
	}
	cout << "evolution #" << this->_evolution << "\n";
	if (this->_pause > 0)
		Sleep(PAUSE_BETWEEN_GAMES * this->_pause);
	this->_evolution++;
	teach();

	for (int i = 0; i < this->deadPlayers.size(); i++)
	{
		this->deadPlayers[i]->_player->saveWeights();
	}
}

void LifeGame::teach()
{
	if (this->deadPlayers.size() < 5)
		return;
	for (int i = 5; i < 7; i++)
	{
		this->deadPlayers[i]->_player->mutatePartly();
	}
	for (int i = 7; i < this->deadPlayers.size(); i++)
	{
		//this->deadPlayers[i]->_player->copyNeuro(*this->deadPlayers[i - 7]->_player); // copy neuro from best guys
		NeuralNetwork* deadiNet = this->deadPlayers[i]->_player->neuro;
		NeuralNetwork* deadi_7Net = this->deadPlayers[i - 7]->_player->neuro;
		deadiNet->crossLayers("input",    *deadi_7Net->getLayer("input"), 1);
		deadiNet->crossLayers("between1", *deadi_7Net->getLayer("between1"), 30);
		deadiNet->crossLayers("between2", *deadi_7Net->getLayer("between2"), 10);
	}
}

char LifeGame::getXY(int X, int Y)
{
	return (this->_field->getXY(X, Y));
}

vector<int> LifeGame::getResult()
{
	vector<int> res(this->deadPlayers.size());
	for (auto it = this->deadPlayers.begin(); it != this->deadPlayers.end(); ++it)
	{
		res[(*it)->_player->getID()] = (*it)->lastStep;
	}
	return res;
}

bool LifeGame::checkMove(int new_x, int new_y)
{
	if (new_x > 0 && new_y > 0)
	{
		if (new_x < this->_n && new_y < this->_m)
			return 1;
	}
	return 0;
}

void LifeGame::printField()
{
	this->_field->printField();
}

float * LifeGame::formInputVector(Player * formFor)
{
	float* mas = new float[6];
	mas[0] = formFor->getHealth();

	//** ENEMY **//
	if (this->players.size() == 1)
	{
		mas[1] = MAXLEN;
		mas[2] = MAXLEN;
		mas[3] = MAXLEN; // enemy far away & has big health
	}
	else
	{
		int etalonLen = 10000;
		Player* etalonPlayer = nullptr;
		for (auto it = this->players.begin(); it != this->players.end(); ++it)
		{
			if ((*it) != formFor)
			{
				int tmp = getMax(abs(formFor->get_X() - (*it)->get_X()),
					abs(formFor->get_Y() - (*it)->get_Y()));
				if (etalonLen > tmp)
				{
					etalonLen = tmp;
					etalonPlayer = (*it);
				}
			}
		}
		mas[1] = float(etalonPlayer->get_X() - formFor->get_X());
		mas[2] = float(etalonPlayer->get_Y() - formFor->get_Y());
		mas[3] = float(etalonPlayer->getHealth());
	}

	//** FOOD **//

	int etalonLen = 10000;
	Food* etalonFood = nullptr;
	for (auto it = this->food.begin(); it != this->food.end(); ++it)
	{
		int tmp = getMax(abs(formFor->get_X() - (*it)->pos_x),
			abs(formFor->get_Y() - (*it)->pos_y));
		if (etalonLen > tmp)
		{
			etalonLen = tmp;
			etalonFood = (*it);
		}
	}
	if (etalonFood == nullptr)
	{
		mas[4] = MAXLEN;
		mas[5] = MAXLEN;
	}
	else
	{
		mas[4] = float(etalonFood->pos_x - formFor->get_X());
		mas[5] = float(etalonFood->pos_y - formFor->get_Y());
	}

	return mas;
}

void LifeGame::generateFood()
{
	mt19937 gen(unsigned int(time(0)));
	uniform_int_distribution<int> range_n{ 0,this->_n - 1 };
	uniform_int_distribution<int> range_m{ 0,this->_m - 1 };

	int tmp_x = range_n(gen);
	int tmp_y = range_m(gen);
	while (this->_field->getXY(tmp_x, tmp_y) != ' ')
	{
		tmp_x = range_n(gen);
		tmp_y = range_m(gen);
	}
	Food* tmpFood = new Food();
	tmpFood->pos_x = tmp_x;
	tmpFood->pos_y = tmp_y;
	this->food.push_back(tmpFood);
	this->_field->setXY(tmp_x, tmp_y, '+');
}

void LifeGame::assigneCells()
{
	for (auto pl = this->players.begin(); pl != this->players.end(); ++pl)
	{
		this->_field->setXY((*pl)->get_X(), (*pl)->get_Y(), '±');
	}
	for (auto fd = this->food.begin(); fd != this->food.end(); ++fd)
	{
		this->_field->setXY((*fd)->pos_x, (*fd)->pos_y, '+');
	}
}

Field::Field(int n, int m) :_n(n), _m(m)
{
	this->_field = new char*[n];
	for (int i = 0; i < n; i++)
	{
		this->_field[i] = new char[m];
		for (int k = 0; k < m; k++)
			this->_field[i][k] = ' ';
	}
}

Field::~Field()
{
	for (int i = 0; i < this->_n; i++)
	{
		delete[] (this->_field[i]);
	}
	delete[] (this->_field);
}

void Field::setXY(int X, int Y, char symbol)
{
	if (X < this->_n && Y < this->_m)
		this->_field[X][Y] = symbol;
}

char Field::getXY(int X, int Y)
{
	return (this->_field[X][Y]);
}

int LifeGame::getN()
{
	return this->_n;
}

int LifeGame::getM()
{
	return this->_m;
}

void Field::printField()
{
	for (int i = 0; i < this->_n; i++)
	{
		for (int k = 0; k < this->_m; k++)
			cout << this->_field[i][k] << ' ';
		cout << '\n';
	}
}

void Field::clearField()
{
	for (int i = 0; i < this->_n; i++)
	{
		for (int j = 0; j < this->_m; j++)
		{
			this->_field[i][j] = ' ';
		}
	}
}

void setXY(int X, int Y, char symbol, LifeGame& game)
{
	game._field->setXY(X, Y, symbol);
}

void movePlayer(int pl_x, int pl_y, int new_x, int new_y, LifeGame& game)
{
	game._field->setXY(pl_x, pl_y, ' ');
}