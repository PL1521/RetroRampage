#include "Actor.h"
#include "StudentWorld.h"
#include "GameConstants.h"

//Increments or decrements the xCoord or yCoord
void oneStep(int dir, int& xCoord, int& yCoord)
{
    switch (dir)
    {
        case 0:
            xCoord++;
            break;
        case 90:
            yCoord++;
            break;
        case 180:
            xCoord--;
            break;
        case 270:
            yCoord--;
            break;
    }
}

//ACTOR
Actor::Actor(StudentWorld* world, int startX, int startY, int imageID)
: GraphObject(imageID, startX, startY, none), m_world(world), m_hp(0), m_alive(true), goodieHeld(false)
{
    setVisible(true);
}

// Make the actor sustain damage.  Return true if this kills the
// actor, and false otherwise.
bool Actor::tryToBeKilled(int damageAmt)
{
    if (m_hp <= 0)
        return true;
    
    m_hp -= damageAmt;
    if (m_hp <= 0)
    {
        setDead();
        return true;
    }
    return false;
}

//AGENT (Any object that can move ==> i.e. player, robot)
Agent::Agent(StudentWorld* world, int startX, int startY, int imageID)
: Actor(world, startX, startY, imageID) {}

// Move to the adjacent square in the direction the agent is facing
// if it is not blocked, and return true.  Return false if the agent
// can't move.
bool Agent::moveIfPossible()
{
    int x = getX();
    int y = getY();
    oneStep(getDirection(), x, y);
    if (getWorld()->canAgentMoveTo(this, x, y))
    {
        moveTo(x, y);
        return true;
    }
    return false;
}

//PLAYER
Player::Player(StudentWorld* world, int startX, int startY)
: Agent(world, startX, startY, IID_PLAYER), m_peas(20)
{
    setHitPoints(20);
    setDirection(right);
}

void Player::doSomething()
{
    //Return if the player isn't alive
    if (!isAlive())
        return;
    
    //Gets the user's input
    int ch;
    if (getWorld()->getKey(ch))
    {
        switch (ch)
        {
            //Set the player's status to dead
            case KEY_PRESS_ESCAPE:
                setDead();
                break;
            //Shoots a new pea object
            case KEY_PRESS_SPACE:
                if (getAmmo() > 0)
                {
                    m_peas--;
                    int x = getX();
                    int y = getY();
                    oneStep(getDirection(), x, y);
                    getWorld()->addActor(new Pea(getWorld(), x, y, getDirection()));
                    getWorld()->playSound(shootingSound());
                }
                break;
            case KEY_PRESS_LEFT:
                setDirection(left);
                moveIfPossible();
                break;
            case KEY_PRESS_RIGHT:
                setDirection(right);
                moveIfPossible();
                break;
            case KEY_PRESS_UP:
                setDirection(up);
                moveIfPossible();
                break;
            case KEY_PRESS_DOWN:
                setDirection(down);
                moveIfPossible();
                break;
        }
    }
}

// Player sustains damageAmt hp of damage
void Player::damage(int damageAmt)
{
    bool check = tryToBeKilled(damageAmt);
    if (!check)
        //If check returns false, the player just took damage
        getWorld()->playSound(SOUND_PLAYER_IMPACT);
}

//PICKUPABLE ITEM
PickupableItem::PickupableItem(StudentWorld* world, int startX, int startY, int imageID, int score)
: Actor(world, startX, startY, imageID), m_score(score) {}

void PickupableItem::doSomething()
{
    //Return if the pickupable item is dead
    if (!isAlive())
        return;
    
    //If the player is on the same square as the goodie, pick it up
    //Set the goodie's status to dead
    if (getWorld()->isPlayerColocatedWith(getX(), getY()))
    {
        getWorld()->increaseScore(m_score);
        getWorld()->playSound(SOUND_GOT_GOODIE);
        pickItemUp();
        setDead();
    }
}

//GOODIE
Goodie::Goodie(StudentWorld* world, int startX, int startY, int imageID, int score)
: PickupableItem(world, startX, startY, imageID, score), isStolen(false) {}

void Goodie::doSomething()
{
    //Return if the goodie is currently held by a thiefbot
    if (isStolen)
        return;
    //Calls the base class's doSomething method
    PickupableItem::doSomething();
}

//Make the goodie visible if isStolen is false and invisible if isStolen is true
void Goodie::setStolen(bool status)
{
    isStolen = status;
    if (isStolen)
        setVisible(false);
    else
        setVisible(true);
}

//EXTRA LIFE GOODIE
ExtraLifeGoodie::ExtraLifeGoodie(StudentWorld* world, int startX, int startY)
: Goodie(world, startX, startY, IID_EXTRA_LIFE, 1000) {}

//RESTORE HEALTH GOODIE
RestoreHealthGoodie::RestoreHealthGoodie(StudentWorld* world, int startX, int startY)
: Goodie(world, startX, startY, IID_RESTORE_HEALTH, 500) {}

//AMMO GOODIE
AmmoGoodie::AmmoGoodie(StudentWorld* world, int startX, int startY)
: Goodie(world, startX, startY, IID_AMMO, 100) {}

//CRYSTAL
Crystal::Crystal(StudentWorld* world, int startX, int startY)
: PickupableItem(world, startX, startY, IID_CRYSTAL, 50) {}

//ROBOT
Robot::Robot(StudentWorld* world, int startX, int startY, int imageID, int score, bool doesShoot)
: Agent(world, startX, startY, imageID), m_score(score), m_shoots(doesShoot), curr_ticks(0)
{
    max_ticks = (28 - getWorld()->getLevel()) / 4;
    if (max_ticks < 3)
        max_ticks = 3;
}

void Robot::doSomething()
{
    //Return if the robot is dead
    if (!isAlive())
        return;
    curr_ticks++;
    //Return if curr_ticks < max_ticks (the robot isn't allowed to do something on this tick)
    if (curr_ticks < max_ticks)
        return;
    curr_ticks = 0;
    
    int currX = getX();
    int currY = getY();
    //If the robot can shoot the player, do so
    if (isShootingRobot() && getWorld()->existsClearShotToPlayer(currX, currY, getDirection()))
    {
        oneStep(getDirection(), currX, currY);
        getWorld()->addActor(new Pea(getWorld(), currX, currY, getDirection()));
        getWorld()->playSound(shootingSound());
        return;
    //Otherwise, make the robot move
    } else
    {
        moveRobot();
    }
}

void Robot::damage(int damageAmt)
{
    bool check = tryToBeKilled(damageAmt);
    if (check)
    {
        //If check returns true, the robot died
        getWorld()->increaseScore(m_score);
        getWorld()->playSound(SOUND_ROBOT_DIE);
    }
    else
        //If check returns false, the robot just took damage
        getWorld()->playSound(SOUND_ROBOT_IMPACT);
}

//RAGEBOT
RageBot::RageBot(StudentWorld* world, int startX, int startY, int startDir)
: Robot(world, startX, startY, IID_RAGEBOT, 100, true)
{
    setDirection(startDir);
    setHitPoints(10);
}

void RageBot::moveRobot()
{
    bool check = moveIfPossible();
    if (!check)
    {
        //If the robot can't move, reverse its direction
        setDirection((getDirection() + 180) % 360);
    }
}

//THIEFBOT
ThiefBot::ThiefBot(StudentWorld* world, int startX, int startY, int imageID, int score, bool shoot)
: Robot(world, startX, startY, imageID, score, shoot)
{
    setDirection(right);
    max_steps = randInt(1, 6);
    curr_steps = 0;
    m_goodie = nullptr;
}

void ThiefBot::moveRobot()
{
    //Goodie is on the same square as a thiefbot and isn't currently being held by another thiefbot
    if (getWorld()->getColocatedStealable(getX(), getY()) != nullptr && !getWorld()->getColocatedStealable(getX(), getY())->goodieIsHeld())
    {
        //If chance allows, make the thiefbot pick up the goodie
        //Make the goodie invisible
        if (randInt(1, 10) == 1)
        {
            m_goodie = getWorld()->getColocatedStealable(getX(), getY());
            m_goodie->goodieHeldStatus(true);
            m_goodie->setStolen(true);
            getWorld()->playSound(SOUND_ROBOT_MUNCH);
            return;
        }
    }
    //If the thiefbot can move one square in its current direction and curr_steps < max_steps, make it move
    //If the thiefbot is carrying a goodie, make the goodie move too
    if ((curr_steps < max_steps) && moveIfPossible())
    {
        curr_steps++;
        if (m_goodie != nullptr)
            m_goodie->moveTo(getX(), getY());
    } else
    {
        //Reset the thiefbot's curr_steps, max_steps, and direction
        curr_steps = 0;
        max_steps = randInt(1, 6);
        int tempDir = randInt(0, 3);
        
        //0 = right, 1 = up, 2 = left, 3 = down
        int direction[4] = {right, up, left, down};
        setDirection(direction[tempDir]);
        //If the thiefbot can move one square in direction[tempDir], move it
        //If the thiefbot is carrying a goodie, make the goodie move too
        if (moveIfPossible())
        {
            if (m_goodie != nullptr)
                m_goodie->moveTo(getX(), getY());
            return;
        }
        
        //Otherwise, check if the thiefbot can move one square in the other directions
        for (int i = 0; i < 4; i++)
        {
            if (i == tempDir)
                continue;
            setDirection(direction[i]);
            if (moveIfPossible())
            {
                if (m_goodie != nullptr)
                    m_goodie->moveTo(getX(), getY());
                return;
            }
        }
        //If the thiefbot can't move in any direction, reset direction to direction[tempDir]
        setDirection(direction[tempDir]);
    }
}

void ThiefBot::damage(int damageAmt)
{
    Robot::damage(damageAmt);
    if (!isAlive() && m_goodie != nullptr)
    {
        //ThiefBot drops the goodie it was holding onto
        //Make the goodie visible on the square the thiefbot died
        m_goodie->setStolen(false);
        m_goodie->goodieHeldStatus(false);
        m_goodie = nullptr;
    }
}

//REGULAR THIEFBOT
RegularThiefBot::RegularThiefBot(StudentWorld* world, int startX, int startY)
: ThiefBot(world, startX, startY, IID_THIEFBOT, 10, false)
{
    setHitPoints(5);
}

//MEAN THIEFBOT
MeanThiefBot::MeanThiefBot(StudentWorld* world, int startX, int startY)
: ThiefBot(world, startX, startY, IID_MEAN_THIEFBOT, 20, true)
{
    setHitPoints(8);
}

//THIEFBOT FACTORY
ThiefBotFactory::ThiefBotFactory(StudentWorld* world, int startX, int startY, bool type)
: Actor(world, startX, startY, IID_ROBOT_FACTORY), meanThief(type) {}

void ThiefBotFactory::doSomething()
{
    int count = 0;
    bool check = getWorld()->doFactoryCensus(getX(), getY(), 3, count);
    //If no thiefbot is on the factory square and less than 3 exist in the specified area of the factory
    if (check && count < 3)
    {
        //If chance allows, create a new thiefbot
        if (randInt(1, 50) == 1)
        {
            if (meanThief)
                getWorld()->addActor(new MeanThiefBot(getWorld(), getX(), getY()));
            else
                getWorld()->addActor(new RegularThiefBot(getWorld(), getX(), getY()));
            getWorld()->playSound(SOUND_ROBOT_BORN);
        }
    }
}

//WALL
Wall::Wall(StudentWorld* world, int startX, int startY)
: Actor(world, startX, startY, IID_WALL) {}

//MARBLE
Marble::Marble(StudentWorld* world, int startX, int startY)
: Actor(world, startX, startY, IID_MARBLE)
{
    setHitPoints(10);
}

void Marble::damage(int damageAmt)
{
    tryToBeKilled(damageAmt);
}

bool Marble::bePushedBy(Agent* a)
{
    //Return if the actor can't push marbles
    if (!a->canPushMarbles())
        return false;
    
    //If the marble can move to the square next to it, move it there
    int currX = getX();
    int currY = getY();
    oneStep(a->getDirection(), currX, currY);
    if (getWorld()->canMarbleMoveTo(currX, currY))
    {
        moveTo(currX, currY);
        return true;
    }
    return false;
}

//PIT
Pit::Pit(StudentWorld* world, int startX, int startY)
: Actor(world, startX, startY, IID_PIT) {}

void Pit::doSomething()
{
    //Return if the pit is dead
    if (!isAlive())
        return;
    //Swallowable object (only ever a marble) is on the same square as a pit
    //Calls swallowSwallowable (which sets the marble's state to dead) and sets the pit's state to dead
    if (getWorld()->swallowSwallowable(this))
        setDead();
}

//PEA
Pea::Pea(StudentWorld* world, int startX, int startY, int startDir)
: Actor(world, startX, startY, IID_PEA)
{
    setDirection(startDir);
}

void Pea::doSomething()
{
    //Return if the pea is dead
    if (!isAlive())
        return;
    //If the pea encounters an obstruction (i.e. any agent or a wall), damage it (if possible)
    //Set the pea's state to dead
    if (getWorld()->damageSomething(this, 2))
        setDead();
    else
    {
        //Move the pea to the adjacent square in its current direction
        int x = getX();
        int y = getY();
        oneStep(getDirection(), x, y);
        moveTo(x, y);
        //If the pea encounters an obstruction (i.e. any agent or wall or factory), damage it (if possible)
        //Set the pea's state to dead
        if (getWorld()->damageSomething(this, 2))
            setDead();
    }
}

//EXIT
Exit::Exit(StudentWorld* world, int startX, int startY)
: Actor(world, startX, startY, IID_EXIT)
{
    revealExit = false;
    setVisible(false);
}

void Exit::doSomething()
{
    //If the level isn't finished but all the crystals have been collected, reveal the exit
    if (!revealExit)
    {
        if (!getWorld()->anyCrystals())
        {
            getWorld()->playSound(SOUND_REVEAL_EXIT);
            revealExit = true;
            setVisible(true);
        }
    } else
    {
        //If the level is finished and the player is standing on the exit, finish the level
        if (getWorld()->isPlayerColocatedWith(getX(), getY()))
            getWorld()->setLevelFinished();
    }
}




