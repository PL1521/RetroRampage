#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include "StudentWorld.h"

//Increments or decrements the xCoord or yCoord
void oneStep(int dir, int& xCoord, int& yCoord);

class StudentWorld;

class Actor : public GraphObject
{
  public:
    Actor(StudentWorld* world, int startX, int startY, int imageID);
    virtual ~Actor() {};
    
    // Action to perform each tick
    virtual void doSomething() = 0;
    
    // Is this actor alive?
    bool isAlive() const { return m_alive; };
    
    // Mark this actor as dead
    void setDead() { m_alive = false; setVisible(false); };
    
    // Get this actor's world
    StudentWorld* getWorld() const { return m_world; };
    
    // How many hit points does this actor have left?
    int getHitPoints() const { return m_hp; };
    
    // Set this actor's hit points to amt.
    void setHitPoints(int amt) { m_hp = amt; };
    
    // Make the actor sustain damage.  Return true if this kills the
    // actor, and false otherwise.
    bool tryToBeKilled(int damageAmt);
    
    // Can an agent occupy the same square as this actor?
    virtual bool allowsAgentColocation() const { return false; };
    
    // Can a marble occupy the same square as this actor?
    virtual bool allowsMarble() const { return false; };
    
    // Does this actor count when a factory counts items near it?
    virtual bool countsInFactoryCensus() const { return false; };
    
    // Does this actor stop peas from continuing?
    virtual bool stopsPea() const { return false; };
    
    // Can this actor be damaged by peas?
    virtual bool isDamageable() const { return false; };
    
    // Cause this Actor to sustain damageAmt hit points of damage.
    virtual void damage(int damageAmt) {};
    
    // Can this actor be pushed by a to location (x,y)?
    virtual bool bePushedBy(Agent* a) { return false; };
    
    // Can this actor be swallowed by a pit?
    virtual bool isSwallowable() const { return false; };
    
    // Can this actor be picked up by a ThiefBot?
    virtual bool isStealable() const { return false; };
    
    // Return true if this actor doesn't shoot unless there's an unobstructed
    // path to the player.
    virtual bool needsClearShot() const { return false; };
    
    // Added for ThiefBot/Goodie dynamic
    //These function will never be called by non-goodie actors
    virtual void setStolen(bool status) {};
    bool goodieIsHeld() { return goodieHeld; };
    void goodieHeldStatus(bool status) { goodieHeld = status; };

  private:
    StudentWorld* m_world;
    bool m_alive;
    int m_hp;
    //Added for ThiefBot/Goodie dynamic
    bool goodieHeld;
};

class Agent : public Actor
{
  public:
    Agent(StudentWorld* world, int startX, int startY, int imageID);
    
    // Move to the adjacent square in the direction the agent is facing
    // if it is not blocked, and return true.  Return false if the agent
    // can't move.
    bool moveIfPossible();
    
    // Return true if this agent can push marbles (which means it's the
    // player).
    virtual bool canPushMarbles() const { return false; };
    
    // Return the sound effect ID for a shot from this agent.
    virtual int shootingSound() const = 0;
};

class Player : public Agent
{
  public:
    Player(StudentWorld* world, int startX, int startY);
    virtual void doSomething();
    virtual bool isDamageable() const { return true; };
    virtual void damage(int damageAmt);
    virtual bool canPushMarbles() const { return true; };
    virtual bool needsClearShot() const { return false; };
    virtual int shootingSound() const { return SOUND_PLAYER_FIRE; };
    
    // Get player's health percentage
    int getHealthPct() const { return getHitPoints() * 5; };
    
    // Get player's amount of ammunition
    int getAmmo() const { return m_peas; };
    
    // Restore player's health to the full amount.
    void restoreHealth() { setHitPoints(20); };
    
    // Increase player's amount of ammunition.
    void increaseAmmo() { m_peas += 20; };
  
  private:
    int m_peas;
};

class PickupableItem : public Actor
{
  public:
    PickupableItem(StudentWorld* world, int startX, int startY, int imageID,
                            int score);
    virtual void doSomething();
    virtual void pickItemUp() = 0;
    virtual bool allowsAgentColocation() const { return true; };
  private:
    int m_score;
};

class Goodie : public PickupableItem
{
  public:
    Goodie(StudentWorld* world, int startX, int startY, int imageID,
                            int score);
    virtual void doSomething();
    virtual bool isStealable() const { return true; };
    // Set whether this goodie is currently stolen.
    virtual void setStolen(bool status);
    
  private:
    bool isStolen;
};

class ExtraLifeGoodie : public Goodie
{
  public:
    ExtraLifeGoodie(StudentWorld* world, int startX, int startY);
    virtual void pickItemUp() { getWorld()->incLives(); };
};

class RestoreHealthGoodie : public Goodie
{
  public:
    RestoreHealthGoodie(StudentWorld* world, int startX, int startY);
    virtual void pickItemUp() { getWorld()->restorePlayerHealth(); };
};

class AmmoGoodie : public Goodie
{
  public:
    AmmoGoodie(StudentWorld* world, int startX, int startY);
    virtual void pickItemUp() { getWorld()->increaseAmmo(); };
};

class Crystal : public PickupableItem
{
  public:
    Crystal(StudentWorld* world, int startX, int startY);
    virtual void pickItemUp() { getWorld()->decCrystals(); };
};

class Robot : public Agent
{
  public:
    Robot(StudentWorld* world, int startX, int startY, int imageID,
          int score, bool doesShoot);
    virtual void doSomething();
    virtual bool isDamageable() const { return true; };
    virtual void damage(int damageAmt);
    virtual bool canPushMarbles() const { return false; };
    virtual bool needsClearShot() const { return true; };
    virtual int shootingSound() const { return SOUND_ENEMY_FIRE; };
    // Does this robot shoot?
    virtual bool isShootingRobot() const { return m_shoots; };
    virtual void moveRobot() {};
    
  private:
    int m_score;
    bool m_shoots;
    int max_ticks;
    int curr_ticks;
};

class RageBot : public Robot
{
  public:
    RageBot(StudentWorld* world, int startX, int startY, int startDir);
    virtual void moveRobot();
};

class ThiefBot : public Robot
{
  public:
    ThiefBot(StudentWorld* world, int startX, int startY, int imageID,
                         int score, bool shoot);
    virtual void moveRobot();
    virtual bool countsInFactoryCensus() const { return true; };
    virtual void damage(int damageAmt);
    
  private:
    int max_steps;
    int curr_steps;
    //Goodie* m_goodie;
    Actor* m_goodie;
};

class RegularThiefBot : public ThiefBot
{
  public:
    RegularThiefBot(StudentWorld* world, int startX, int startY);
};

class MeanThiefBot : public ThiefBot
{
  public:
    MeanThiefBot(StudentWorld* world, int startX, int startY);
};

class Exit : public Actor
{
  public:
    Exit(StudentWorld* world, int startX, int startY);
    virtual void doSomething();
    virtual bool allowsAgentColocation() const { return true; };
    
  private:
    bool revealExit;
};

class Wall : public Actor
{
  public:
    Wall(StudentWorld* world, int startX, int startY);
    virtual void doSomething() {};
    virtual bool stopsPea() const { return true; };
};

class Marble : public Actor
{
  public:
    Marble(StudentWorld* world, int startX, int startY);
    virtual void doSomething() {};
    virtual bool isDamageable() const { return true; };
    virtual void damage(int damageAmt);
    virtual bool isSwallowable() const { return true; };
    virtual bool bePushedBy(Agent* a);
};

class Pit : public Actor
{
  public:
    Pit(StudentWorld* world, int startX, int startY);
    virtual void doSomething();
    virtual bool allowsMarble() const { return true; };
};

class Pea : public Actor
{
  public:
    Pea(StudentWorld* world, int startX, int startY, int startDir);
    virtual void doSomething();
    virtual bool allowsAgentColocation() const { return true; };
};

class ThiefBotFactory : public Actor
{
  public:
    ThiefBotFactory(StudentWorld* world, int startX, int startY, bool type);
    virtual void doSomething();
    virtual bool stopsPea() const { return true; };
    
  private:
    bool meanThief;
};

#endif // ACTOR_H_
