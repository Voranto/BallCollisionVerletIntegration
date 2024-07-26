#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

using namespace sf;
using namespace std;
using namespace std::chrono;

// Constants
Vector2f gravity = { 0.f, 1000.0f };
const float friction = 0.4f;
const float damping = 0.5f;
const int frameheight = 800;
const int framewidth = 800;
const int ballradius = 6;
const int substeps = 10;
const float numberofballs = 5000;
const float bigcircleradius = 400.0f;
const int framerate = 120;
const Vector2f bigcirclepos{ 400.0f, 400.0f };
const int griddivisions = 50;
const float numbergridwidth = framewidth / griddivisions;
const float numbergridheight = frameheight / griddivisions;
unsigned int Seed = 10;
Vector2f ballradiusvec = Vector2f(ballradius, ballradius);
int Random(int min, int max) {
    srand((unsigned)time(0));
    int val = min + rand() % max;
    return val;
}

// Ball class, with default attributes
class RGB {
public:
    int alpha, red, green, blue;
    int state = 0;

    RGB(int a, int r, int g, int b) : alpha(a), red(r), green(g), blue(b) {}

    Color next() {
        if (state == 0) { red -= 5; blue += 5; }
        if (blue == 255) state += 1;
        if (state == 1) { blue -= 5; green += 5; }
        if (green == 255) state += 1;
        if (state == 2) { green -= 5; red += 5; }
        if (red == 255) state = 0;
        return Color(red, green, blue, alpha);
    }
};
RGB colorbola(255, 0, 0, 0);

class Ball {
public:
    Vector2f currentpos;
    Vector2f oldpos;
    Vector2f acceleration = gravity;

    Ball(Vector2f pos, Vector2f oldpos2) : currentpos(pos), oldpos(oldpos2) {
        Color temp = colorbola.next();
    }

    void update(float dt) {
        const Vector2f displacement = currentpos - oldpos;
        oldpos = currentpos;
        currentpos += displacement + acceleration * (dt * dt);
    }

    void applyConstraint() {
        const Vector2f currentposcenter = currentpos + ballradiusvec;
        const Vector2f v = bigcirclepos - currentposcenter;
        const float dist2 = v.x * v.x + v.y * v.y;
        if (dist2 > (bigcircleradius - ballradius) * (bigcircleradius - ballradius)) {
            
            currentpos = bigcirclepos - v / sqrt(dist2) * (bigcircleradius - ballradius) - ballradiusvec;
            
        }
    }

    void checkCollisions(float dt, Ball& otherBall) {
        const Vector2f v = currentpos - otherBall.currentpos;
        const float dist2 = v.x * v.x + v.y * v.y;
        const float min_dist = ballradius * 2;
        if (dist2 < min_dist * min_dist) {
            
            const float dist = sqrt(dist2);
            const Vector2f n = v / dist;
            const float delta = 0.5f * (dist - min_dist);
            currentpos -= n * delta;
            otherBall.currentpos += n * delta;
        }
    }
};

vector<Ball> balls;
vector<vector<vector<int>>> grid(griddivisions+2, vector<vector<int>>(griddivisions+2, vector<int>(0)));

// Main loop
int main() {
    
    for (int i = 0; i < griddivisions + 2; ++i) {
        for (int j = 0; j < griddivisions + 2; ++j) {
            grid[i][j].clear();
        }
    }


    RenderWindow window(VideoMode(framewidth, frameheight), "Ball Simulation", Style::Close | Style::Titlebar);
    window.setFramerateLimit(60);
    Clock clock;
    CircleShape bigcircle(bigcircleradius, 80);
    bigcircle.setFillColor(Color(255, 255, 255, 255));
    float lastTime = 0;
    float time = 0;
    int counter = 0;

    // Load font for FPS display
    Font font;
    if (!font.loadFromFile("C:\\Windows\\Fonts\\impact.ttf")) {
        cerr << "Error loading font\n";
        return -1;
    }

    Text fps;
    Text object_count;
    object_count.setFont(font);
    object_count.setPosition(Vector2f(0, 40));
    object_count.setFillColor(Color::Red);
    fps.setFont(font);
    fps.setFillColor(Color::Red);
    bool key_pressed = false;
    
    Vector2f startpos(400, 20);
    Vector2f startvel(0, 1200);
    Event event;
    while (window.isOpen()) {
        if (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        auto start = std::chrono::high_resolution_clock::now(); // Start timing the frame
        float deltaTime = clock.restart().asSeconds();
        time += deltaTime;
        float subDelta = deltaTime / substeps;
        
        if (time >= 0.001f && counter < numberofballs) {
            
            balls.emplace_back(startpos,startpos- startvel * subDelta);
            time = 0;
            counter++;
        }
        float FPS = 1.f / deltaTime;
        object_count.setString("Object Count: " + to_string(counter));
        
        window.clear();

        
        
        for (int t = 0; t < substeps; t++) {
            
            for (int i = 0; i < griddivisions + 2; ++i) {
                for (int j = 0; j < griddivisions + 2; ++j) {
                    grid[i][j].clear();
                }
            }
            
            for (int idx = 0; idx < counter; idx++) {
                
                if (balls[idx].currentpos != balls[idx].oldpos) {
                    
                    
                    
                    balls[idx].update(subDelta);
                    balls[idx].applyConstraint();
                    int gridx = 1 + int(trunc((balls[idx].currentpos.x + ballradius) / numbergridwidth));
                    int gridy = 1 + int(trunc((balls[idx].currentpos.y + ballradius) / numbergridheight));
                    grid[gridx][gridy].push_back(idx);
                    for (int i = -1; i <= 1; ++i) {
                        for (int j = -1; j <= 1; ++j) {
                            if (gridx + i > 0 and gridx + i < griddivisions + 2 and gridy + j > 0 and gridy + j < griddivisions + 2) {
                                for (auto& otherBallidx : grid[gridx + i][gridy + j]) {
                                    Ball& otherBall = balls[otherBallidx];
                                    if (&balls[idx] != &otherBall) {
                                        
                                        balls[idx].checkCollisions(subDelta, otherBall);
                                        
                                    }

                                }
                            }
                        }
                    }
                    
                }
            }
            

        }
        
        CircleShape circle{ballradius};
        circle.setFillColor(Color(255, 0, 0, 255));
        window.draw(bigcircle);
        fps.setString("Simulation time: "+to_string(int(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count())));
        window.draw(fps);
        window.draw(object_count);
        float time = 0.0f;
        for (const auto& obj : balls) {
            
            circle.setPosition(obj.currentpos);
            auto drawstart = std::chrono::high_resolution_clock::now();
            window.draw(circle);
            auto drawend = std::chrono::high_resolution_clock::now();
            time += std::chrono::duration_cast<std::chrono::microseconds>(drawend - drawstart).count();
        }
        window.display();
       
        
    }
    return 0;
}

