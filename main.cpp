
#include <SDL.h>
#include <iostream>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>
#include <string>
#include <sstream>

/*

    Software SDL is the worst way to do this

    Screen Space is -1..1 x, -1..1 y
    For now camera should be at 0 (x) 0 (y) -1 (z)

    Fix aspect ratio change distortion

*/

const Uint32 WINDOW_FLAGS = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
const Uint32 RENDERER_FLAGS = SDL_RENDERER_ACCELERATED;

SDL_Window* WINDOW;
SDL_Renderer* RENDERER;

int WIDTH = 512;
int HEIGHT = 512;

class vec3 {
public:
    float x, y, z;
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3 operator+(const vec3 &vec) const{
        return vec3(x + vec.x, y + vec.y, z + vec.z);
    }
    void operator+=(const vec3 &vec)
    {
        x += vec.x;
        y += vec.y;
        z += vec.z;
    }
    vec3 operator-(const vec3 &vec) const{
        return vec3(x - vec.x, y - vec.y, z - vec.z);
    }
    void operator-=(const vec3 &vec){
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;
    }
    float dot(const vec3& other) {
        return x*other.x + y*other.y + z*other.z;
    }
    vec3 cross(const vec3& other) {
        return vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }
    void normalise() {
        float magnitude = sqrt((x * x) + (y * y) + (z * z));
        if (magnitude != 0){
            x /= magnitude;
            y /= magnitude;
            z /= magnitude;
        }
    }
};

struct tri {
    vec3 vertices[3];
    tri(vec3 a, vec3 b, vec3 c) : vertices({a, b, c}) {}
    vec3 normal() {
        return (vertices[1]).cross(vertices[0]);
    }
    void draw(SDL_Renderer* R, bool wire, bool fillin) {

        if (vertices[0].z <= 0.001 || vertices[0].z <= 0.001 || vertices[0].z <= 0.001) {
            return;
        }

        float v0x = WIDTH /2.0 * (vertices[0].x/vertices[0].z) + WIDTH /2.0;
        float v0y = HEIGHT/2.0 * (vertices[0].y/vertices[0].z) + HEIGHT/2.0;
        float v1x = WIDTH /2.0 * (vertices[1].x/vertices[1].z) + WIDTH /2.0;
        float v1y = HEIGHT/2.0 * (vertices[1].y/vertices[1].z) + HEIGHT/2.0;
        float v2x = WIDTH /2.0 * (vertices[2].x/vertices[2].z) + WIDTH /2.0;
        float v2y = HEIGHT/2.0 * (vertices[2].y/vertices[2].z) + HEIGHT/2.0;

        if (fillin) {

            std::vector<std::pair<float, float>> vxs = { {v0x, v0y} , { v1x, v1y} , { v2x, v2y} };

            std::sort(vxs.begin(), vxs.end(), [](auto &left, auto &right) {
                return left.second < right.second;
            });

            // fill
            SDL_SetRenderDrawColor(R, 0, 0, 255, 255);
            if (vxs[1].second == vxs[2].second) { // bottom flat, filltop
                filltop(R, vxs);
            }
            else if (vxs[0].second == vxs[1].second) { // top flat, fillbot
                fillbot(R, vxs);
            }
            else {
                std::pair<float, float> v = {(int)(vxs[0].first + ((float)(vxs[1].second - vxs[0].second) / (float)(vxs[2].second - vxs[0].second)) * (vxs[2].first - vxs[0].first)), vxs[1].second};
                std::vector<std::pair<float, float>> vxs_top = {vxs[0], vxs[1], v};
                std::vector<std::pair<float, float>> vxs_bot = {vxs[1], v, vxs[2]};
                filltop(R, vxs_top);
                fillbot(R, vxs_bot);
            }
        }

        // draw tri edges
        if (wire) {
            SDL_SetRenderDrawColor(R, 255, 255, 255, 255);
            SDL_RenderDrawLineF(R, v0x, v0y, v1x, v1y);
            SDL_RenderDrawLineF(R, v0x, v0y, v2x, v2y);
            SDL_RenderDrawLineF(R, v1x, v1y, v2x, v2y);
        }

    }
    void filltop(SDL_Renderer* R, std::vector<std::pair<float, float>> vxs) {
        float m1 = (vxs[1].first - vxs[0].first) / (vxs[1].second - vxs[0].second);
        float m2 = (vxs[2].first - vxs[0].first) / (vxs[2].second - vxs[0].second);

        float x1 = vxs[0].first;
        float x2 = vxs[0].first;

        for (int scanline = vxs[0].second; scanline <= vxs[1].second; scanline++) {
            SDL_RenderDrawLine(R,(int)x1, scanline, (int)x2, scanline);
            // for zbuffer
            //std::vector<SDL_Point> pts;
            //for (int x = (int) x1; x <= (int) x2; x++) {
            //     if (ZBUFF[x][scanline] > zed) pts.push_back( {x, scanline} );
            // }
            //SDL_RenderDrawPoints(R, pts.data(), abs(x2 - x1));
            x1 += m1;
            x2 += m2;
        }
    }
    void fillbot(SDL_Renderer* R, std::vector<std::pair<float, float>> vxs) {
        float m1 = (vxs[2].first - vxs[0].first) / (vxs[2].second - vxs[0].second);
        float m2 = (vxs[2].first - vxs[1].first) / (vxs[2].second - vxs[1].second);

        float x1 = vxs[2].first;
        float x2 = vxs[2].first;

        for (int scanline = vxs[2].second; scanline > vxs[0].second; scanline--) {
            SDL_RenderDrawLine(R,(int)x1, scanline, (int)x2, scanline);
            x1 -= m1;
            x2 -= m2;
        }
    }
};

int main(int argc, char* argv[]) {

    SDL_Init(SDL_INIT_VIDEO);

    WINDOW = SDL_CreateWindow("Software3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, WINDOW_FLAGS);
    RENDERER = SDL_CreateRenderer(WINDOW, -1, RENDERER_FLAGS);

    SDL_Event event;

    bool running = true;
    vec3 POS = {0, 0, 0};
    while (running) {

        SDL_GetWindowSize(WINDOW, &WIDTH, &HEIGHT);

        std::stringstream t;
        t << POS.x << " " << POS.y << " " << POS.z << std::endl;

        SDL_SetWindowTitle(WINDOW, t.str().c_str());

        while (SDL_PollEvent(&event) != 0) {
            switch (event.type)
            {
                case SDL_MOUSEMOTION:
                    break;
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                {
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_LEFT:  POS.x += 0.01; break;
                        case SDLK_RIGHT: POS.x -= 0.01; break;
                        case SDLK_UP:    POS.z -= 0.01; break;
                        case SDLK_DOWN:  POS.z += 0.01; break;
                        case SDLK_LALT:  POS.y -= 0.01; break;
                        case SDLK_RALT:  POS.y += 0.01; break;
                    }
                    break;
                }
            }
        }

        std::vector<tri> triangles;

        triangles.push_back(tri(POS + vec3( 0.25 , -0.5 , 1.0 ),
                                POS + vec3( 0.25 ,  0.5 , 1.0 ),
                                POS + vec3( 0.25 , -0.5 , 1.5 )));

        triangles.push_back(tri(POS + vec3( 0.25 ,  0.5 , 1.0 ),
                                POS + vec3( 0.25 , -0.5 , 1.5 ),
                                POS + vec3( 0.25 ,  0.5 , 1.5 )));

        triangles.push_back(tri(POS + vec3(  - 0.25 , -0.5 , 1.0 ),
                                POS + vec3(  - 0.25 ,  0.5 , 1.0 ),
                                POS + vec3(  - 0.25 , -0.5 , 1.5 )));

        triangles.push_back(tri(POS + vec3(  - 0.25 ,  0.5 , 1.0 ),
                                POS + vec3(  - 0.25 , -0.5 , 1.5 ),
                                POS + vec3(  - 0.25 ,  0.5 , 1.5 )));

        triangles.push_back(tri(POS + vec3(  - sin(1/2),  0.6 , 4.0 ),
                                POS + vec3(  - cos(1/2), -0.6 , 3.5 ),
                                POS + vec3(  - tan(1/2),  0.6 , 2.5 )));

        SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 255);
        SDL_RenderClear(RENDERER);

        std::sort(triangles.begin(), triangles.end(), [](auto &first, auto &second) {
            return (first.vertices[0].z + first.vertices[1].z + first.vertices[2].z)/3 > (second.vertices[0].z + second.vertices[1].z + second.vertices[2].z)/3;
        });

        for (auto t : triangles) {t.draw(RENDERER, true, true);}

        // Ray Trace fill?
        /*for (int x = 0; x < WIDTH; x++) for (int y = 0; y < HEIGHT; y++){
            for (auto t : triangles) {

            }
        }*/

        SDL_RenderPresent(RENDERER);
    }

    SDL_Quit();

    return 0;
}
