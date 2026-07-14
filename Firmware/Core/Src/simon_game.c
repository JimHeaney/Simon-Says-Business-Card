#include "simon_game.h"
#include "stm32g0xx_hal.h" // Gives us access to HAL_GetTick() and HAL_Delay()

// Private helper prototypes
static void SimonGame_AddStep(SimonGame_t *game);
static void SimonGame_PlaySequence(SimonGame_t *game);
static void SimonGame_LightAll(SimonGame_t *game, bool state);
static int8_t SimonGame_GetButtonPressed(SimonGame_t *game);

void SimonGame_Init(SimonGame_t *game,
                    uint8_t initialHighScore,
                    void (*setGameLed)(uint8_t, bool),
                    bool (*getButton)(uint8_t),
                    void (*updateScoreDisplay)(uint8_t),
                    void (*saveHighScore)(uint8_t))
{
    game->highScore = initialHighScore;
    game->setGameLed = setGameLed;
    game->getButton = getButton;
    game->updateScoreDisplay = updateScoreDisplay;
    game->saveHighScore = saveHighScore;

    game->state = STATE_WAIT_TO_START;
    game->currentScore = 0;
    game->seedGenerated = false;
}

void SimonGame_Update(SimonGame_t *game) {
    switch (game->state) {
        case STATE_WAIT_TO_START: {
            game->updateScoreDisplay(game->highScore);

            // Flash LEDs at 1Hz (500ms intervals)
            if ((HAL_GetTick() / 500) % 2 == 0) {
                SimonGame_LightAll(game, true);
            } else {
                SimonGame_LightAll(game, false);
            }

            if (SimonGame_GetButtonPressed(game) != -1) {
                if (!game->seedGenerated) {
                    srand(HAL_GetTick()); // Seed software RNG using ticks
                    game->seedGenerated = true;
                }

                SimonGame_LightAll(game, false);
                HAL_Delay(1000);

                game->currentScore = 0;
                game->updateScoreDisplay(game->currentScore);

                SimonGame_AddStep(game);
                game->state = STATE_PLAY_SEQUENCE;
            }
            break;
        }

        case STATE_PLAY_SEQUENCE: {
            HAL_Delay(500);
            SimonGame_PlaySequence(game);
            game->inputIndex = 0;
            game->stateTimer = HAL_GetTick();
            game->state = STATE_WAIT_FOR_INPUT;
            break;
        }

        case STATE_WAIT_FOR_INPUT: {
            int8_t btn = SimonGame_GetButtonPressed(game);

            if (btn != -1) {
                game->setGameLed(btn, true);
                while (game->getButton(btn)) { HAL_Delay(10); } // Debounce/wait for release
                game->setGameLed(btn, false);

                if (btn == game->sequence[game->inputIndex]) {
                    game->inputIndex++;
                    if (game->inputIndex >= game->currentScore) {
                        game->state = STATE_SUCCESS;
                    }
                } else {
                    game->state = STATE_GAME_OVER;
                }
            }

            // 5 second inactivity timeout
            if (HAL_GetTick() - game->stateTimer > 5000) {
                game->state = STATE_GAME_OVER;
            }
            break;
        }

        case STATE_SUCCESS: {
            HAL_Delay(500);
            game->updateScoreDisplay(game->currentScore);

            if (game->currentScore >= 14) {
                if (game->currentScore > game->highScore) {
                    game->highScore = game->currentScore;
                    game->saveHighScore(game->highScore);
                }

                for (int i = 0; i < 3; i++) {
                    SimonGame_LightAll(game, true); HAL_Delay(300);
                    SimonGame_LightAll(game, false); HAL_Delay(300);
                }
                game->state = STATE_WAIT_TO_START;
            } else {
                SimonGame_AddStep(game);
                game->state = STATE_PLAY_SEQUENCE;
            }
            break;
        }

        case STATE_GAME_OVER: {
            if (game->currentScore - 1 > game->highScore) {
                game->highScore = game->currentScore - 1;
                game->saveHighScore(game->highScore);
            }

            for (int i = 0; i < 3; i++) {
                game->setGameLed(game->sequence[game->inputIndex], true); HAL_Delay(200);
                game->setGameLed(game->sequence[game->inputIndex], false); HAL_Delay(200);
            }
            HAL_Delay(1000);
            game->state = STATE_WAIT_TO_START;
            break;
        }
    }
}

// --- Private Helpers ---

static void SimonGame_AddStep(SimonGame_t *game) {
    game->sequence[game->currentScore] = rand() % 4;
    game->currentScore++;
}

static void SimonGame_PlaySequence(SimonGame_t *game) {
    for (uint8_t i = 0; i < game->currentScore; i++) {
        game->setGameLed(game->sequence[i], true);
        HAL_Delay(400);
        game->setGameLed(game->sequence[i], false);
        HAL_Delay(200);
    }
}

static void SimonGame_LightAll(SimonGame_t *game, bool state) {
    for (int i = 0; i < 4; i++) {
        game->setGameLed(i, state);
    }
}

static int8_t SimonGame_GetButtonPressed(SimonGame_t *game) {
    for (int i = 0; i < 4; i++) {
        if (game->getButton(i)) {
            return i;
        }
    }
    return -1;
}
