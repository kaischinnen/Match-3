#include "raylib.h"
#include <stdio.h>
#include <stdbool.h> 
#include <time.h>
#include <math.h>
#include <stdlib.h>

#define BOARD_SIZE 8
#define TILE_SIZE 42
#define TILE_TYPES 5
#define SCORE_FONT_SIZE 32
#define MAX_SCORE_POPUPS 32

const char tile_chars[TILE_TYPES] = {'#', '@', '$', '%', '&'};

char board[BOARD_SIZE][BOARD_SIZE];
bool matched[BOARD_SIZE][BOARD_SIZE] = {0};
float fall_offset[BOARD_SIZE][BOARD_SIZE] = {0};

Vector2 grid_origin;

int score = 0;
Texture2D background;
Font score_font;
Vector2 selected_tile = {-1, -1};
float fall_speed = 8.0f;
float match_delay_timer = 0.0f;
const float match_delay_duration = 0.2f;
float volume = 0.5f;
bool sliderGrabbed = false;

float score_scale = 1.0f;
float score_scale_velocity = 0.0f;
bool score_animating = false;

Music background_music;
Sound match_sound;

typedef enum {
    STATE_IDLE,
    STATE_ANIMATING,
    STATE_MATCH_DELAY
} TileState;

TileState tile_state;

typedef struct {
    Vector2 position;
    int amount;
    float lifetime;
    float alpha;
    float active;
} ScorePopup;

ScorePopup score_popups[MAX_SCORE_POPUPS] = {0};

char random_tile() {
    return tile_chars[rand() % TILE_TYPES];
}

void swap_tiles(int x1, int y1, int x2, int y2) {
    char temp = board[y1][x1];
    board[y1][x1] = board[y2][x2];
    board[y2][x2] = temp;
}

bool are_tiles_adjacent(Vector2 tile1, Vector2 tile2) {
    return (abs((int)tile1.x - (int)tile2.x) + abs((int)tile1.y - (int)tile2.y)) == 1;
}

void add_score_popup(int x, int y, int amount, Vector2 grid_origin) {
    for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
        if (!score_popups[i].active) {
            score_popups[i].position = (Vector2) {
                grid_origin.x + x * TILE_SIZE + TILE_SIZE / 2,
                grid_origin.y + y * TILE_SIZE + TILE_SIZE / 2
            };
            score_popups[i].amount = amount;
            score_popups[i].lifetime = 1.0f;
            score_popups[i].alpha = 1.0f;
            score_popups[i].active = true;
            break;
        }
    }
}

bool find_matches() {
    bool found = false; 
    for (int y = 0; y < BOARD_SIZE; y++){
        for (int x = 0; x < BOARD_SIZE; x++) {
            matched[y][x] = false; // reset matched state
        }
    }

    // horizontal matches
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE - 2; x++) {
            char t = board[y][x];
            if (t == board[y][x + 1] && t == board[y][x + 2]) {
                matched[y][x] = matched[y][x + 1] = matched[y][x + 2] = true;

                score += 10;
                found = true;
                PlaySound(match_sound);

                score_animating = true;
                score_scale = 2.0f;
                score_scale_velocity = -2.5f;

                add_score_popup(x,y, 10, grid_origin);
            }
        }
    }

    // vertical matches
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE - 2; y++) {
            char t = board[y][x];
            if (t == board[y + 1][x] && t == board[y + 2][x]) {
                matched[y][x] = matched[y + 1][x] = matched[y + 2][x] = true;

                score += 10;
                found = true;
                PlaySound(match_sound);

                score_animating = true;
                score_scale = 2.0f;
                score_scale_velocity = -2.5f;

                add_score_popup(x,y, 10, grid_origin);
            }
        }
    }
    return found;
}

void resolve_matches() {
    for (int x = 0; x < BOARD_SIZE; x++) {
        int write_y = BOARD_SIZE - 1; // lowest column
        for (int y = BOARD_SIZE - 1; y >= 0; y--) {
            if (!matched[y][x]) {
                if (y != write_y) {
                    board[write_y][x] = board[y][x]; // move non-matching tile down
                    fall_offset[write_y][x] = (write_y - y) * TILE_SIZE;
                    board[y][x] = ' '; 
                }
                write_y--;
            }
        }

        // fill empty spaces with new random tiles
        while (write_y >= 0) {
            board[write_y][x] = random_tile();
            fall_offset[write_y][x] = (write_y + 1) * TILE_SIZE;
            write_y--;
        }
    }

    tile_state = STATE_ANIMATING;
}


void init_board() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = random_tile();
        }
    }

    int grid_width = BOARD_SIZE * TILE_SIZE;
    int grid_height = BOARD_SIZE * TILE_SIZE;

    grid_origin = (Vector2) {
        (GetScreenWidth() - grid_width) / 2,
        (GetScreenHeight() - grid_height) / 2
    };

    if (find_matches()) {
        resolve_matches();
    } else  {
        tile_state = STATE_IDLE;
    }
}

float Slider(float x, float y, float width, float minValue, float maxValue, float value)
{
    Rectangle sliderBar = { x, y + 10, width, 8 };
    float knobX = x + (value - minValue) / (maxValue - minValue) * width - 8;
    Rectangle sliderKnob = { knobX, y, 16, 30 };

    DrawRectangleRec(sliderBar, GRAY);

    Vector2 mousePos = GetMousePosition();
    bool mouseOnKnob = CheckCollisionPointRec(mousePos, sliderKnob);
    bool mouseOnSlider = CheckCollisionPointRec(mousePos, (Rectangle){ x, y, width, 30 });

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && mouseOnSlider)
    {
        sliderGrabbed = true;
    }
    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON))
    {
        sliderGrabbed = false;
    }

    if (sliderGrabbed)
    {
        float newValue = (mousePos.x - x) / width * (maxValue - minValue) + minValue;
        if (newValue < minValue) newValue = minValue;
        if (newValue > maxValue) newValue = maxValue;
        value = newValue;
    }

    if (sliderGrabbed)
    {
        DrawRectangleRec(sliderKnob, YELLOW);
        DrawRectangleLinesEx(sliderKnob, 2, DARKGRAY);
    }
    else
    {
        DrawRectangleRec(sliderKnob, DARKGRAY);
    }

    return value;
}


int main() {
    const int screen_width = 800;
    const int screen_height = 450;
    InitWindow(screen_width, screen_height, "Raylib 2D Match 3");
    SetTargetFPS(60);
    srand(time(NULL)); // init random Seed

    InitAudioDevice();

    background = LoadTexture("assets/bg.jpg");
    score_font = LoadFontEx("assets/04B_03__.TTF", SCORE_FONT_SIZE, NULL, 0);

    background_music = LoadMusicStream("assets/bgm.mp3");
    match_sound = LoadSound("assets/match.mp3");

    PlayMusicStream(background_music);
    SetMusicVolume(background_music, volume);

    init_board();

    while (!WindowShouldClose()) {

        UpdateMusicStream(background_music);

        Vector2 mouse = GetMousePosition();
        if (tile_state == STATE_IDLE && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int x = (mouse.x - grid_origin.x) / TILE_SIZE;
            int y = (mouse.y - grid_origin.y) / TILE_SIZE;

            if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
                Vector2 current_tile = (Vector2) {x, y};
                if (selected_tile.x < 0) {
                    selected_tile = current_tile;
                }
                else {
                    if (are_tiles_adjacent(selected_tile, current_tile)) {
                        swap_tiles(selected_tile.x, selected_tile.y, current_tile.x, current_tile.y);
                        if (find_matches()) {
                            resolve_matches();
                        }
                        else {
                            // swap back if no matches found
                            swap_tiles(selected_tile.x, selected_tile.y, current_tile.x, current_tile.y);
                        }
                    }
                    selected_tile = (Vector2) {-1, -1}; // reset selection
                }
            }
        }

        if (tile_state == STATE_ANIMATING) {
            bool still_animating = false;
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (fall_offset[y][x] > 0) {
                        fall_offset[y][x] -= fall_speed;

                        if (fall_offset[y][x] < 0) {
                            fall_offset[y][x] = 0;
                        } else {
                            still_animating = true;
                        }
                    }
                }
            }
            if (!still_animating) {
                tile_state = STATE_MATCH_DELAY;
                match_delay_timer = match_delay_duration;
            }
        }

        if (tile_state == STATE_MATCH_DELAY) {
            match_delay_timer -= GetFrameTime();
            if (match_delay_timer <= 0) {
                if (find_matches()) {
                    resolve_matches();
                } else {
                    tile_state = STATE_IDLE; 
                }
            }
        }

        // update score popups array
        for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
            if (score_popups[i].active) {
                score_popups[i].lifetime -= GetFrameTime();
                score_popups[i].position.y -= 30 * GetFrameTime(); 
                score_popups[i].alpha = score_popups[i].lifetime;

                if (score_popups[i].lifetime <= 0.0f) {
                    score_popups[i].active = false;
                }
            }
        }

        // update score animation
        if (score_animating) {
            score_scale += score_scale_velocity * GetFrameTime(); 
            if (score_scale <= 1.0f) {
                score_scale = 1.0f;
                score_animating = false;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // draw Background
        DrawTexturePro(
            background, 
            (Rectangle) {
                0,0, background.width, background.height
            },
            (Rectangle) {
                0, 0, GetScreenWidth(), GetScreenHeight()
            },
            (Vector2) {
                0, 0
            }, 
            0.0f, WHITE
        );

        // grid bg
        DrawRectangle(
            grid_origin.x,
            grid_origin.y,
            BOARD_SIZE * TILE_SIZE,
            BOARD_SIZE * TILE_SIZE,
            Fade(DARKGRAY, 0.6f)
        );


        // grid
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                Rectangle rect = {
                    grid_origin.x + x * TILE_SIZE,
                    grid_origin.y + y * TILE_SIZE,
                    TILE_SIZE,
                    TILE_SIZE
                };

                DrawRectangleLinesEx(rect, 1, DARKGRAY);


                if (board[y][x] != ' ') {
                    // draw Tiles
                    DrawTextEx(
                        GetFontDefault(),
                        TextFormat("%c", board[y][x]),
                        (Vector2) {rect.x + 12, rect.y + 8 - fall_offset[y][x]},
                        20, 1 , 
                        matched[y][x] ? GREEN : WHITE
                    );
                }
            }
        }

        // draw selected tile
        if (selected_tile.x >= 0) {
            DrawRectangleLinesEx((Rectangle) {
                    grid_origin.x + selected_tile.x * TILE_SIZE,
                    grid_origin.y + selected_tile.y * TILE_SIZE,
                    TILE_SIZE,
                    TILE_SIZE
                },
                2, YELLOW
            );
        }

        // audio slider
        int slider_width = 200;
        int sliderX = screen_width - slider_width - 20;
        int sliderY = 20;
        volume = Slider(sliderX, sliderY, slider_width, 0.0f, 1.0f, volume);
        SetMusicVolume(background_music, volume);
        DrawText(
            TextFormat("Volume: %.2f", volume),
            sliderX, 
            sliderY + 40, 
            20, WHITE
        );

        // draw Score
        DrawTextEx(
            score_font,
            TextFormat("Score: %d", score),
            (Vector2) {20, 20},
            SCORE_FONT_SIZE * score_scale,
            1.0f,
            YELLOW
        );

    // draw score popups
        for (int i = 0; i < MAX_SCORE_POPUPS; i++) {
            if (score_popups[i].active) {
            Color c = Fade(YELLOW, score_popups[i].alpha);
            DrawText(
                TextFormat("+%d", score_popups[i].amount),
                score_popups[i].position.x,
                score_popups[i].position.y,
                20, c);
        }
    }

        EndDrawing();
    }
    StopMusicStream(background_music);
    UnloadMusicStream(background_music);
    UnloadSound(match_sound);
    UnloadTexture(background);
    UnloadFont(score_font);

    CloseAudioDevice();

    CloseWindow();
    return 0;
}