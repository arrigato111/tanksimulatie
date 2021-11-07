#pragma once

namespace Tmpl8
{
//forward declarations
class Tank;
class Rocket;
class Smoke;
class Particle_beam;

class Game
{
  public:
    void set_target(Surface* surface) { screen = surface; }
    void init();
    void shutdown();
    void update_tanks_blue();
    void update_tanks_red();
    void update_tanks_template( int start, int end);
    void update_rocket_blue();
    void update_rocket_red();
    void update_beam();
    void update_smoke();
    void update_explosion();
    void draw();
    void draw_tank();
    void draw_rocket_red();
    void draw_rocket_blue();
    void draw_rest();
    void draw_healthbar();
    void tick(float deltaTime);
    void merge_sort_tanks_health(std::vector<Tank*>& tanks_to_sort, int l, int r);
    void merge(std::vector<Tank*>& tanks_to_sort, int l, int m, int r);
    void measure_performance();

    Tank& find_closest_enemy(Tank& current_tank);
    Tank& find_closest_blue(Tank& current_tank);
    Tank& find_closest_red(Tank& current_tank);

  private:
    Surface* screen;

    vector<Tank> tanks;
    vector<Tank> BlueTanks; //unused
    vector<Tank> RedTanks; //unused
    vector<Rocket> RedRockets;
    vector<Rocket> BlueRockets;
    vector<Smoke> smokes;
    vector<Explosion> explosions;
    vector<Particle_beam> particle_beams;

    Font* frame_count_font;
    long long frame_count = 0;

    bool lock_update = false;
};

}; // namespace Tmpl8