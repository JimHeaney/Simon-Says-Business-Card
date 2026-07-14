#ifndef SIMON_GAME_H
#define SIMON_GAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    STATE_WAIT_TO_START,
    STATE_PLAY_SEQUENCE,
    STATE_WAIT_FOR_INPUT,
    STATE_SUCCESS,
    STATE_GAME_OVER
} GameState_t;

typedef struct {
    uint8_t sequence[14];
    uint8_t currentScore;
    uint8_t highScore;
    uint8_t inputIndex;
    GameState_t state;
    bool seedGenerated;
    uint32_t stateTimer;

    // Hardware action callbacks
    void (*setGameLed)(uint8_t index, bool state);
    bool (*getButton)(uint8_t index);
    void (*updateScoreDisplay)(uint8_t score);
    void (*saveHighScore)(uint8_t score);
} SimonGame_t;

// Public Functions
void SimonGame_Init(SimonGame_t *game,
                    uint8_t initialHighScore,
                    void (*setGameLed)(uint8_t, bool),
                    bool (*getButton)(uint8_t),
                    void (*updateScoreDisplay)(uint8_t),
                    void (*saveHighScore)(uint8_t));

void SimonGame_Update(SimonGame_t *game);

#endif // SIMON_GAME_H
