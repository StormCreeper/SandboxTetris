#include "raylib.h"

#include <future>
#include <thread>
#include <chrono>
#include <random>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

bool DEBUG = false;

const int cell_size = 10;
const int pixel_size = 4;

const int num_cells_x = 10;
const int num_cells_y = 20;

const int grid_width = num_cells_x * cell_size;
const int grid_height = num_cells_y * cell_size;

const int ui_width = 300;

const int screen_width = grid_width * pixel_size + ui_width;
const int screen_height = grid_height * pixel_size;

struct TetrisPiece {
    int w;
    int h;
    std::vector<bool> data;

    TetrisPiece(int w, int h) : w(w), h(h) {
        data.resize(w * h);
        data.assign(w * h, false);
    }

    bool get(int x, int y) {
        return data[y * w + x];
    }
};
enum Type {
    VOID_T,
    YELLOW_T,
    GREEN_T,
    BLUE_T,
    RED_T,
    PURPLE_T,

};
struct PieceDesc {
    TetrisPiece piece{0, 0};
    Type type;
    int rot;  // 0, 1, 2, 3
    int x = grid_width / 2;
    int y = -cell_size * 2;
};

PieceDesc next_piece{};
PieceDesc current_piece{};

std::vector<TetrisPiece>
    pieces;

void initPieces(std::string filename) {
    // File
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file " << filename << std::endl;
        return;
    }

    while (!file.eof()) {
        int s;
        file >> s;
        TetrisPiece piece(s, s);
        for (int i = 0; i < s * s; i++) {
            int val;
            file >> val;
            piece.data[i] = val;
        }
        pieces.push_back(piece);
    }
}

// Rotate a square grid to the right
void rotate_left(std::vector<bool> &tab, int n) {
    bool *buffer = new bool[n * n];
    for (int i = 0; i < n * n; i++) {
        buffer[i] = tab[i];
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int index_rot = i * n + j;
            int index = (n - 1 - j) * n + (i);
            tab[index_rot] = buffer[index];
        }
    }

    delete[] buffer;
}

const Color colors[] = {
    BLACK,                // VOID
    {150, 150, 50, 255},  // YELLOW
    {50, 150, 50, 255},   // GREEN
    {50, 50, 150, 255},   // BLUE
    {150, 50, 50, 255},   // RED
    {150, 50, 150, 255}   // PURPLE
};

char *grid;
char *bufferGrid;

int index(int x, int y) {
    return y * grid_width + x;
}

Image grid_image;
Texture2D grid_texture;

bool modified = false;
bool stop = false;

int score = 0;

void update_grid() {
    bool line_move[grid_width] = {true};
    for (int y = grid_height - 2; y >= 0; y--) {
        // Reset line_move
        for (int i = 0; i < grid_width; i++) {
            line_move[i] = true;
        }
        bool cont = true;
        while (cont) {
            int x = rand() % grid_width;
            if (!line_move[x]) {
                continue;
            }

            int i = index(x, y);
            if (grid[i] != VOID_T) {
                if (y < grid_height - 1) {
                    int below = index(x, y + 1);
                    if (bufferGrid[below] == VOID_T) {
                        bufferGrid[below] = grid[i];
                        bufferGrid[i] = VOID_T;
                    } else {
                        int dir = rand() % 2 == 0 ? -1 : 1;
                        if (x - dir >= 0 && x - dir < grid_width && bufferGrid[below - dir] == VOID_T) {
                            bufferGrid[below - dir] = grid[i];
                            bufferGrid[i] = VOID_T;
                        } else if (x + dir >= 0 && x + dir < grid_width && bufferGrid[below + dir] == VOID_T) {
                            bufferGrid[below + dir] = grid[i];
                            bufferGrid[i] = VOID_T;
                        }
                    }
                }
            }
            line_move[x] = false;

            for (int i = 0; i < grid_width; i++) {
                cont = false;
                if (line_move[i]) {
                    cont = true;
                    break;
                }
            }
        }
    }

    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            int i = index(x, y);
            grid[i] = bufferGrid[i];
        }
    }

    // Update the texture
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            int i = index(x, y);
            ImageDrawPixel(&grid_image, x, y, colors[grid[i]]);
        }
    }

    modified = true;
}

void update_task() {
    while (!stop) {
        update_grid();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void set_pixel(int x, int y, Type type) {
    if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
        return;
    int i = index(x, y);
    grid[i] = type;
}

void fill_rect(int x, int y, int w, int h, Type type) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            set_pixel(x + i, y + j, type);
        }
    }
}

void place_piece(int x, int y, TetrisPiece &piece, Type type) {
    int piece_w = piece.w * cell_size;
    int piece_h = piece.h * cell_size;
    for (int i = 0; i < piece.w; i++) {
        for (int j = 0; j < piece.h; j++) {
            if (piece.get(i, j)) {
                fill_rect(x + i * cell_size, y + j * cell_size, cell_size, cell_size, type);
            }
        }
    }
}

void draw_ui() {
    DrawText("NEXT", screen_width - ui_width + 80, 0, 50, WHITE);
    int size = pixel_size * cell_size * 4;
    int dec_x = screen_width - ui_width + (ui_width - size) / 2;
    int dec_y = 70;
    DrawRectangle(dec_x, dec_y, size, size, BLACK);

    int cell_real_size = cell_size * pixel_size;
    int piece_w = next_piece.piece.w * cell_real_size;
    int piece_h = next_piece.piece.h * cell_real_size;

    for (int i = 0; i < next_piece.piece.w; i++) {
        for (int j = 0; j < next_piece.piece.h; j++) {
            if (next_piece.piece.get(i, j)) {
                DrawRectangle(dec_x + i * cell_real_size + (size - piece_w) / 2,
                              dec_y + j * cell_real_size + (size - piece_h) / 2,
                              cell_real_size, cell_real_size,
                              colors[next_piece.type]);
            }
        }
    }

    DrawText("LINES CLEARED", screen_width - ui_width + 10, 300, 32, WHITE);
    DrawText(std::to_string(score).c_str(), screen_width - ui_width + 80, 350, 50, WHITE);
}

int *regionsGrid;
int regionNum;
std::vector<bool> connection_regions{};

void flood_fill(int x, int y, int region, char type) {
    if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
        return;
    int i = index(x, y);
    if (grid[i] == VOID_T || grid[i] != type || regionsGrid[i] != -1)
        return;
    if (x == grid_width - 1) {
        connection_regions[region] = true;
    }
    regionsGrid[i] = region;
    flood_fill(x - 1, y, region, type);
    flood_fill(x + 1, y, region, type);
    flood_fill(x, y - 1, region, type);
    flood_fill(x, y + 1, region, type);
}

void clear_region(int region) {
    for (int i = 0; i < grid_width * grid_height; i++) {
        if (regionsGrid[i] == region) {
            grid[i] = VOID_T;
            bufferGrid[i] = VOID_T;
        }
    }
    score++;
}

void check_lines() {
    regionNum = -1;
    // reset regionsGrid
    for (int i = 0; i < grid_width * grid_height; i++) {
        regionsGrid[i] = -1;
    }
    connection_regions.clear();

    for (int y = grid_height - 1; y >= 0; y--) {
        // Only start from the side
        if (regionsGrid[index(0, y)] == -1) {
            regionNum++;
            connection_regions.push_back(false);
            flood_fill(0, y, regionNum, grid[index(0, y)]);
            if (connection_regions[regionNum]) {
                // std::cout << "Region " << regionNum << " is connected" << std::endl;
                clear_region(regionNum);
                return;
            }
        }
    }
}

void update() {
    if (current_piece.piece.w == 0) {
        current_piece = next_piece;
        next_piece = PieceDesc{};
        next_piece.piece = pieces[rand() % pieces.size()];
        next_piece.type = static_cast<Type>(rand() % 5 + 1);
        next_piece.rot = 0;
    }
    current_piece.y += 1;
    if (IsKeyPressed(KEY_UP)) {
        current_piece.rot = (current_piece.rot + 1) % 4;
        rotate_left(current_piece.piece.data, current_piece.piece.w);
    }

    if (IsKeyDown(KEY_LEFT)) {
        current_piece.x -= 1;
    }

    if (IsKeyDown(KEY_RIGHT)) {
        current_piece.x += 1;
    }

    if (IsKeyDown(KEY_DOWN)) {
        current_piece.y += 1;
    }

    for (int i = 0; i < current_piece.piece.w; i++) {
        for (int j = 0; j < current_piece.piece.h; j++) {
            if (current_piece.piece.get(i, j)) {
                if (current_piece.x + i * cell_size < 0) {
                    current_piece.x = -i * cell_size;
                }
                if (current_piece.x + (i + 1) * cell_size >= grid_width) {
                    current_piece.x = grid_width - (i + 1) * cell_size;
                }
                if (current_piece.y + (j + 1) * cell_size > grid_height) {
                    place_piece(current_piece.x, current_piece.y, current_piece.piece, current_piece.type);
                    current_piece.piece.w = 0;
                    i = current_piece.piece.w;
                    j = current_piece.piece.h;
                }
            }
        }
    }

    {
        for (int i = 0; i < current_piece.piece.w; i++) {
            for (int j = 0; j < current_piece.piece.h; j++) {
                if (current_piece.piece.get(i, j)) {
                    for (int x = i * cell_size; x < (i + 1) * cell_size; x++) {
                        for (int y = j * cell_size; y < (j + 1) * cell_size; y++) {
                            if (x + current_piece.x < 0 || x + current_piece.x >= grid_width || y + current_piece.y < 0 || y + current_piece.y >= grid_height)
                                continue;
                            int i = index(x + current_piece.x, y + current_piece.y);
                            if (grid[i] != VOID_T) {
                                place_piece(current_piece.x, current_piece.y, current_piece.piece, current_piece.type);
                                current_piece.piece.w = 0;
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
}

int main(void) {
    InitWindow(screen_width, screen_height, "Sandbox :D");
    grid = new char[grid_width * grid_height];
    bufferGrid = new char[grid_width * grid_height];
    regionsGrid = new int[grid_width * grid_height];
    for (int i = 0; i < grid_width * grid_height; i++) {
        grid[i] = VOID_T;
        bufferGrid[i] = VOID_T;
    }

    grid_image = GenImageColor(grid_width, grid_height, BLUE);
    grid_texture = LoadTextureFromImage(grid_image);

    initPieces("../pieces.txt");
    next_piece.piece = pieces[rand() % pieces.size()];
    next_piece.type = static_cast<Type>(rand() % 5 + 1);
    next_piece.rot = 0;

    // auto result = std::async(std::launch::async, update_task);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (DEBUG) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int x = GetMouseX() * grid_width / screen_width;
                int y = GetMouseY() * grid_height / screen_height;
                int i = index(x, y);
                place_piece(x, y, next_piece.piece, next_piece.type);

                next_piece = PieceDesc{};
                next_piece.piece = pieces[rand() % pieces.size()];
                next_piece.type = static_cast<Type>(rand() % 5 + 1);
                next_piece.rot = 0;
            }
        }

        update();

        BeginDrawing();
        ClearBackground(GRAY);
        // Update texture
        update_grid();
        check_lines();

        if (modified) {
            Color *pixels = LoadImageColors(grid_image);
            UpdateTexture(grid_texture, pixels);
            UnloadImageColors(pixels);
            modified = false;
        }
        // Draw texture (scaled)
        DrawTextureEx(grid_texture, {0, 0}, 0, pixel_size, WHITE);

        // Show regions

        if (DEBUG) {
            for (int y = 0; y < grid_height; y += 3) {
                for (int x = 0; x < grid_width; x += 3) {
                    int i = index(x, y);
                    int region = regionsGrid[i];

                    DrawText(std::to_string(region).c_str(), x * pixel_size, y * pixel_size, 10, WHITE);
                }
            }
        }

        draw_ui();

        {
            int cell_real_size = cell_size * pixel_size;
            int piece_w = current_piece.piece.w * cell_real_size;
            int piece_h = current_piece.piece.h * cell_real_size;

            for (int i = 0; i < current_piece.piece.w; i++) {
                for (int j = 0; j < current_piece.piece.h; j++) {
                    if (current_piece.piece.get(i, j)) {
                        DrawRectangle(current_piece.x * pixel_size + i * cell_real_size,
                                      current_piece.y * pixel_size + j * cell_real_size,
                                      cell_real_size, cell_real_size,
                                      colors[current_piece.type]);
                    }
                }
            }
        }

        EndDrawing();
    }
    // stop = true;
    // result.wait();

    delete[] regionsGrid;
    delete[] bufferGrid;
    delete[] grid;
    CloseWindow();
    return 0;
}