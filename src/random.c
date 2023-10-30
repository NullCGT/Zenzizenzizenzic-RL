/**
 * @file random.c
 * @author Kestrel (kestrelg@kestrelscry.com)
 * @brief Random number generation functions.
 * @version 1.0
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <time.h>

/* Seed the random number generator with a given value. */
/**
 * @brief Seed the random number generator with a given value
 * 
 * @param x The integer with which to seed the random number generator.
 */
void rndseed(int x) {
    srand(x);
    return;
}

/**
 * @brief Seed the random number generator with a randomized value determined
 by the current time.
 * 
 */
void rndseed_t() {
    rndseed((unsigned) time(NULL));
    return;
}

/**
 * @brief Return a random number greater than or equal to zero and less than x.
 * 
 * @param x An upper bound.
 * @return int An integer greater than or equal to zero and less than x.
 */
int rndmx(int x) {
    return (rand() % x);
}

/**
 * @brief Returns a random number greater than or equal to x and less than y.
 * 
 * @param x A lower bound.
 * @param y An upper bound.
 * @return int An integer greater than or equal to x and less than y.
 */
int rndrng(int x, int y) {
    if (y <= x) return x;
    return (x + (rand() % abs(y - x)));
}

/**
 * @brief Returns a random boolean falue.
 * 
 * @return int A boolean value; either zero or one.
 */
int rndbool() {
    return (rand() % 2);
}

/**
 * @brief Roll xdy and return the result.
 * 
 * @param x The number of dice to roll.
 * @param y The number of sides on said dice.
 * @return int The result of the dice roll.
 */
int d(int x, int y) {
    int ret = 0;
    for (int i = 0; i < x; i++) {
        ret += rndrng(1, y + 1);
    }
    return ret;
}