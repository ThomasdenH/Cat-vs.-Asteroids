#ifndef GAME_H
#define GAME_H

#include <QVector3D>
#include <vector>

typedef struct {
    int x;
    double z;
} Box;

class Game
{
public:
    Game();
    void reset();
    void update(double dt);
    QVector3D getCatLocation();
    double getCatRotation();
    unsigned getBoxCount();
    QVector3D getBoxLocation(unsigned index);
    unsigned getScore();

    void goLeft();
    void goRight();
    void jump();

private:
    /* The current obstacles. */
    std::vector<Box> boxes;

    /* The cat coordinates. */
    int catX;
    int targetCatX;
    double interpolator;

    bool jumping;
    double catY;
    double catvY;

    unsigned score;
    bool gameOver;
    double spawnTime;

    double totalTime;
    double catRotation;
};

#endif // GAME_H
