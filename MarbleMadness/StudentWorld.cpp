#include "StudentWorld.h"
#include "Actor.h"
#include "GameConstants.h"
#include "GraphObject.h"
#include "Level.h"
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

GameWorld* createStudentWorld(string assetPath)
{
	return new StudentWorld(assetPath);
}

//Constructor
StudentWorld::StudentWorld(string assetPath)
: GameWorld(assetPath)
{
    m_player = nullptr;
    calledClean = false;
    levelDone = false;
    m_crystals = 0;
    m_bonus = 1000;
}

//Destructor
StudentWorld::~StudentWorld()
{
    if (!calledClean)
        cleanUp();
}

//Loads the current level's maze from a data file
int StudentWorld::init()
{
    //Reset m_crystals, m_bonus, and calledClean
    calledClean = false;
    m_crystals = 0;
    m_bonus = 1000;
    
    ostringstream oss;
    oss << "level";
    if (getLevel() < 10)
        oss << "0";
    oss << getLevel() << ".txt";
    string curLevel = oss.str();
    Level lev(assetPath());
    Level::LoadResult res = lev.loadLevel(curLevel);
    
    if (getLevel() > 99 || res == Level::load_fail_file_not_found)
        return GWSTATUS_PLAYER_WON;
    if (res == Level::load_fail_bad_format)
        return GWSTATUS_LEVEL_ERROR;
    
    //If an actor exists at location (c,r), create a new actor object
    Level::MazeEntry item;
    for (int c = 0; c < VIEW_WIDTH; c++)
    {
        for (int r = 0; r < VIEW_HEIGHT; r++)
        {
            item = lev.getContentsOf(c, r);
            switch (item)
            {
                case Level::empty:
                    break;
                case Level::exit:
                    addActor(new Exit(this, c, r));
                    break;
                case Level::player:
                    m_player = new Player(this, c, r);
                    addActor(m_player);
                    break;
                case Level::horiz_ragebot:
                    addActor(new RageBot(this, c, r, 0));
                    break;
                case Level::vert_ragebot:
                    addActor(new RageBot(this, c, r, 270));
                    break;
                case Level::thiefbot_factory:
                    addActor(new ThiefBotFactory(this, c, r, false));
                    break;
                case Level::mean_thiefbot_factory:
                    addActor(new ThiefBotFactory(this, c, r, true));
                    break;
                case Level::wall:
                    addActor(new Wall(this, c, r));
                    break;
                case Level::marble:
                    addActor(new Marble(this, c, r));
                    break;
                case Level::pit:
                    addActor(new Pit(this, c, r));
                    break;
                case Level::crystal:
                    m_crystals++;
                    addActor(new Crystal(this, c, r));
                    break;
                case Level::restore_health:
                    addActor(new RestoreHealthGoodie(this, c, r));
                    break;
                case Level::extra_life:
                    addActor(new ExtraLifeGoodie(this, c, r));
                    break;
                case Level::ammo:
                    addActor(new AmmoGoodie(this, c, r));
                    break;
            }
        }
    }
    return GWSTATUS_CONTINUE_GAME;
}

//Make the actor do something
int StudentWorld::doSomething(Actor* a)
{
    a->doSomething();
    //Player died (decrement lives)
    if (!m_player->isAlive())
    {
        playSound(SOUND_PLAYER_DIE);
        decLives();
        return GWSTATUS_PLAYER_DIED;
    }
    //Level has been finished (increase score)
    if (levelDone)
    {
        levelDone = false;
        playSound(SOUND_FINISHED_LEVEL);
        increaseScore(2000 + m_bonus);
        return GWSTATUS_FINISHED_LEVEL;
    }
    return GWSTATUS_CONTINUE_GAME;
}

//Updates the game text header
void StudentWorld::updateGameText()
{
    ostringstream oss;
    oss << "Score: ";
    oss.fill('0');
    oss << setw(7) << getScore();
    oss.fill('0');
    oss << "  Level: " << setw(2) << getLevel();
    oss.fill(' ');
    oss << "  Lives: " << setw(2) << getLives();
    oss << "  Health: " << setw(3) << m_player->getHealthPct();
    oss << "%  Ammo: " << setw(3) << m_player->getAmmo();
    oss << "  Bonus: " << setw(4) << m_bonus;
    string s = oss.str();
    setGameStatText(s);
}

int StudentWorld::move()
{
    //Update the game text header every tick
    updateGameText();
    list<Actor*>::iterator itr;
    
    //Make actors that don't shoot do something
    for (itr = m_actors.begin(); itr != m_actors.end(); itr++)
    {
        //Skip the player and robots that shoot (fixes the pea problem)
        if ((*itr) == m_player || (*itr)->needsClearShot())
            continue;
        int res = doSomething((*itr));
        //Return if the player dies or the level is finished
        if (res != GWSTATUS_CONTINUE_GAME)
            return res;
    }
    
    //Make robots that shoot do something
    for (itr = m_actors.begin(); itr != m_actors.end(); itr++)
    {
        //Skip the player and actors that don't shoot
        if ((*itr) == m_player || !(*itr)->needsClearShot())
            continue;
        int res = doSomething((*itr));
        //Return if the player dies or the level is finished
        if (res != GWSTATUS_CONTINUE_GAME)
            return res;
    }
    
    //Make the player do something
    m_player->doSomething();
    
    //Delete any dead actors
    list<Actor*>::iterator deadItr = m_actors.begin();
    while (deadItr != m_actors.end())
    {
        if (!(*deadItr)->isAlive())
        {
            delete (*deadItr);
            deadItr = m_actors.erase(deadItr);
        } else
            deadItr++;
    }
    //Decrement the bonus by one every tick
    if (m_bonus > 0)
        m_bonus--;
	return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp()
{
    //Delete all remaining actors currently in the game
    list<Actor*>::iterator it;
    it = m_actors.begin();
    while (it != m_actors.end())
    {
        delete (*it);
        it = m_actors.erase(it);
    }
    calledClean = true;
}

// Restore player's health to the full amount
void StudentWorld::restorePlayerHealth()
{
    m_player->restoreHealth();
}

// Increase the amount of ammunition the player has
void StudentWorld::increaseAmmo()
{
    m_player->increaseAmmo();
}

// Can an agent move to x,y?
bool StudentWorld::canAgentMoveTo(Agent* agent, int x, int y) const
{
    list<Actor*>::const_iterator it;
    for (it = m_actors.begin(); it != m_actors.end(); it++)
    {
        if ((*it)->isAlive() && !(*it)->allowsAgentColocation() && (*it)->getX() == x && (*it)->getY() == y)
            return (*it)->bePushedBy(agent);
    }
    return true;
}

// Can a marble move to x,y?
bool StudentWorld::canMarbleMoveTo(int x, int y) const
{
    list<Actor*>::const_iterator it;
    for (it = m_actors.begin(); it != m_actors.end(); it++)
    {
        if ((*it)->isAlive() && !(*it)->allowsMarble() && (*it)->getX() == x && (*it)->getY() == y)
            return false;
    }
    return true;
}

// Is the player on the same square as an actor?
bool StudentWorld::isPlayerColocatedWith(int x, int y) const
{
    return m_player->getX() == x && m_player->getY() == y;
}

// Try to cause damage to something at a's location.  (a is only ever
// going to be a pea.)  Return true if something stops a -- something
// at this location prevents a pea from continuing.
bool StudentWorld::damageSomething(Actor* a, int damageAmt)
{
    bool res = false;
    list<Actor*>::iterator it;
    for (it = m_actors.begin(); it != m_actors.end(); it++)
    {
        if ((*it)->isAlive() && (*it)->getX() == a->getX() && (*it)->getY() == a->getY())
        {
            //If the obstruction is damageable, damage it and set the pea's state to dead
            if ((*it)->isDamageable())
            {
                (*it)->damage(damageAmt);
                a->setDead();
                return true;
            }
            //If the obstruction stops peas, set the pea's state to dead
            else if ((*it)->stopsPea())
            {
                a->setDead();
                res = true;
            }
        }
    }
    return res;
}

// Swallow any swallowable object at a's location.  (a is only ever
// going to be a pit.)
bool StudentWorld::swallowSwallowable(Actor* a)
{
    int x = a->getX();
    int y = a->getY();
    list<Actor*>::iterator it;
    for (it = m_actors.begin(); it != m_actors.end(); it++)
    {
        if ((*it)->isAlive() && (*it)->isSwallowable() && (*it)->getX() == x && (*it)->getY() == y)
        {
            (*it)->setDead();
            return true;
        }
    }
    return false;
}

// If a pea were at x,y moving in direction dx,dy, could it hit the
// player without encountering any obstructions?
bool StudentWorld::existsClearShotToPlayer(int x, int y, int dir) const
{
    if (x != m_player->getX() && y != m_player->getY())
        return false;
    
    //Pea is either on the same row or column of the player
    int currX = x;
    int currY = y;
    int dx = 0;
    int dy = 0;
    oneStep(dir, dx, dy);
    
    list<Actor*>::const_iterator it;
    while (currX != m_player->getX() || currY != m_player->getY())
    {
        //Move the pea one square in its current direction
        currX += dx;
        currY += dy;
        // Return true if the pea's position matches the player's position
        if (currX == m_player->getX() && currY == m_player->getY())
            return true;
        //Return false if the pea is on the same square as an obstruction (i.e. any agent or wall or factory)
        for (it = m_actors.begin(); it != m_actors.end(); it++)
        {
            if ((*it)->isAlive() && (*it)->getX() == currX && (*it)->getY() == currY)
            {
                if ((*it)->stopsPea() || (*it)->isDamageable())
                    return false;
            }
        }
        //Bounds check
        if (currX > VIEW_HEIGHT || currX < 0 || currY > VIEW_WIDTH || currY < 0)
            return false;
    }
    return true;
}

// If an item that can be stolen is at x,y, return a pointer to it;
// otherwise, return a null pointer.  (Stealable items are only ever
// going be goodies.)
Actor* StudentWorld::getColocatedStealable(int x, int y) const
{
    list<Actor*>::const_iterator it;
    for (it = m_actors.begin(); it != m_actors.end(); it++)
    {
        if ((*it)->isStealable() && (*it)->getX() == x && (*it)->getY() == y)
            return (*it);
    }
    return nullptr;
}

// If a factory is at x,y, how many items of the type that should be
// counted are in the rectangle bounded by x-distance,y-distance and
// x+distance,y+distance?  Set count to that number and return true,
// unless an item is on the factory itself, in which case return false
// and don't care about count.  (The items counted are only ever going
// ThiefBots.)
bool StudentWorld::doFactoryCensus(int x, int y, int distance, int& count) const
{
    count = 0;
    list<Actor*>::const_iterator it;
    for (it = m_actors.begin(); it != m_actors.end(); it++)
    {
        if ((*it)->isAlive() && (*it)->countsInFactoryCensus())
        {
            //Return false if a thiefbot is on the same square as the factory
            if ((*it)->getX() == x && (*it)->getY())
                return false;
            //Increment count if the thiefbot is in the specified area of the factory
            else if (abs((*it)->getX() - x) <= 3 && abs((*it)->getY() - y) <= 3)
                count++;
        }
    }
    return true;
}
