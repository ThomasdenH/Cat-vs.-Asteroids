#include "game.h"
#include <cstdlib>

Game::Game()
{
    boxes = {};
    reset();
}

void Game::reset()
{
    gameOver = false;
    score = 0;
    spawnTime = 0.5;

    jumping = false;
    catY = 0.0;
    catvY = 0.0;

    catX = 0;
    targetCatX = 0;
    interpolator = 0.0;
    totalTime = 0.0;
    catRotation = 0.0;
    boxes.clear();
}

void Game::update(double dt)
{
    if (gameOver) {
        catRotation += dt * 500.0;
        if (catRotation > 90)
            catRotation = 90;
        return;
    }

    totalTime += dt;

    score = (int) (totalTime / 10.0);

    // Character jumping & gravity
    if (jumping) {
        catvY -= dt * 100.0;
        catY += catvY * dt;
        if (catY < 0.0) {
            catY = 0.0;
            catvY = 0.0;
            jumping = false;
        }
    }

    // Character movement
    if (catX != targetCatX) {
        interpolator += dt * 20.0;
        if (interpolator >= 1.0) {
            catX = targetCatX;
            interpolator = 0.0;
        }
    }

    // Move boxes
    for (unsigned i = 0; i < boxes.size(); i++) {
        Box b = boxes.at(i);
        b.z += dt * (20.0 + totalTime / 1000.0);

        // Check for collision
        if (b.z > -10.5 && b.z < -9.5) {
            if (b.x == catX && catY < 1.0) {
                gameOver = true;
                return;
            }

            // Remove old boxes
            if (b.z > 0) {
                boxes.erase(boxes.begin() + i);
                i--;
                continue;
            }
        }
        boxes[i] = b;
    }

    // Add more boxes
    spawnTime -= dt;
    if (spawnTime < 0.0) {
        spawnTime = 0.5 + (double) (rand() % 100) / 100.0 * 2.0;

        spawnTime -= totalTime / 10000.0;
        if (spawnTime < 0.3)
            spawnTime = 0.3;

        Box b;
        b.x = (rand() % 3) - 1;
        b.z = -100.0;
        boxes.push_back(b);
    }
}

unsigned Game::getBoxCount() {
    return boxes.size();
}

QVector3D Game::getCatLocation() {
    double interpolatedX = catX + interpolator * double (targetCatX - catX);
    return QVector3D(interpolatedX * 3.0, catY - 4.0, -10.0);
}

QVector3D Game::getBoxLocation(unsigned index) {
    Box b = boxes.at(index);
    return QVector3D(b.x * 3.0, -4.0, b.z);
}

void Game::goLeft() {
    if (targetCatX == catX && targetCatX >= 0)
        targetCatX--;
}

void Game::goRight() {
    if (targetCatX == catX && targetCatX <= 0)
        targetCatX++;
}

void Game::jump() {
    if (gameOver) {
        reset();
    } else if (!jumping) {
        jumping = true;
        catvY = 30.0;
    }
}

unsigned Game::getScore() {
    return score;
}

double Game::getCatRotation() {
    return catRotation;
}
