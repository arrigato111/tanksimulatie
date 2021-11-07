#include "precomp.h"
#include <thread>
#include <mutex>
#include "thread_pool.h"

#define NUM_TANKS_BLUE 1279
#define NUM_TANKS_RED 1279

#define TANK_MAX_HEALTH 1000
#define ROCKET_HIT_VALUE 60
#define PARTICLE_BEAM_HIT_VALUE 50

#define TANK_MAX_SPEED 1.5

#define HEALTH_BARS_OFFSET_X 0
#define HEALTH_BAR_HEIGHT 70
#define HEALTH_BAR_WIDTH 1
#define HEALTH_BAR_SPACING 0

#define MAX_FRAMES 2000

//Global performance timer
#define REF_PERFORMANCE 20800
static timer perf_timer;
static float duration;

//Load sprite files and initialize sprites
static Surface* background_img = new Surface("assets/Background_Grass.png");
static Surface* tank_red_img = new Surface("assets/Tank_Proj2.png");
static Surface* tank_blue_img = new Surface("assets/Tank_Blue_Proj2.png");
static Surface* rocket_red_img = new Surface("assets/Rocket_Proj2.png");
static Surface* rocket_blue_img = new Surface("assets/Rocket_Blue_Proj2.png");
static Surface* particle_beam_img = new Surface("assets/Particle_Beam.png");
static Surface* smoke_img = new Surface("assets/Smoke.png");
static Surface* explosion_img = new Surface("assets/Explosion.png");

static Sprite background(background_img, 1);
static Sprite tank_red(tank_red_img, 12);
static Sprite tank_blue(tank_blue_img, 12);
static Sprite rocket_red(rocket_red_img, 12);
static Sprite rocket_blue(rocket_blue_img, 12);
static Sprite smoke(smoke_img, 4);
static Sprite explosion(explosion_img, 9);
static Sprite particle_beam_sprite(particle_beam_img, 3);

const static vec2 tank_size(14, 18);
const static vec2 rocket_size(25, 24);

const static float tank_radius = 8.5f;
const static float rocket_radius = 10.f;

//timer for functions
static double deltatime = 0.0;
timer t;

//forward declarations threadpool objects
ThreadPool pool(100);
ThreadPool draw_pool(20);
std::vector< std::function<void()> > draw_vector;
std::vector< std::future<void> > future_draw_vector;
std::vector< std::function<void()> > processing_vector;
std::vector< std::future<void> > future_processing_vector;
//forward declarations locks
std::mutex RedRocketmtx;
std::mutex BlueRocketmtx;
std::mutex smokemtx;
std::mutex explosionmtx;
std::mutex tankmtx;

// Initialize the application
void Tmpl8::Game::init()
{
    frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

    tanks.reserve(NUM_TANKS_BLUE + NUM_TANKS_RED);

    uint rows = (uint)sqrt(NUM_TANKS_BLUE + NUM_TANKS_RED);
    uint max_rows = 12;

    float start_blue_x = tank_size.x + 10.0f;
    float start_blue_y = tank_size.y + 80.0f;

    float start_red_x = 980.0f;
    float start_red_y = 100.0f;

    float spacing = 15.0f;

    //Spawn blue tanks
    for (int i = 0; i < NUM_TANKS_BLUE; i++)
    {
        tanks.push_back(Tank(start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing), BLUE, &tank_blue, &smoke, 1200, 600, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED));
    }
    //Spawn red tanks
    for (int i = 0; i < NUM_TANKS_RED; i++)
    {
        tanks.push_back(Tank(start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing), RED, &tank_red, &smoke, 80, 80, tank_radius, TANK_MAX_HEALTH, TANK_MAX_SPEED));
    }

    particle_beams.push_back(Particle_beam(vec2(SCRWIDTH / 2, SCRHEIGHT / 2), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));
    particle_beams.push_back(Particle_beam(vec2(80, 80), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));
    particle_beams.push_back(Particle_beam(vec2(1200, 600), vec2(100, 50), &particle_beam_sprite, PARTICLE_BEAM_HIT_VALUE));

    //Fill draw vector with bound function calls
    draw_vector.push_back(std::bind(&Game::draw_tank, this));
    draw_vector.push_back(std::bind(&Game::draw_healthbar, this));
    draw_vector.push_back(std::bind(&Game::draw_rocket_blue, this));
    draw_vector.push_back(std::bind(&Game::draw_rocket_red, this));
    draw_vector.push_back(std::bind(&Game::draw_rest, this));


    //Fill draw_future_vector with the returned futures from the threadpool
    for (int i = 0; i < draw_vector.size(); i++)
    {
        future_draw_vector.push_back(pool.enqueue(draw_vector.at(i)));
    }

    //Fill processing vector with bound function calls
    for (int i = 0; i < 10; i++)
    {
        processing_vector.push_back(std::bind(&Game::update_tanks_template, this, i, 10));
    }
    processing_vector.push_back(std::bind(&Game::update_rocket_blue, this));
    processing_vector.push_back(std::bind(&Game::update_rocket_red, this));
    processing_vector.push_back(std::bind(&Game::update_smoke, this));
    processing_vector.push_back(std::bind(&Game::update_explosion, this));
    processing_vector.push_back(std::bind(&Game::update_beam, this));

    //Fill future_processing_vector with the returned futures from the threadpool
    for (int i = 0; i < processing_vector.size(); i++)
    {
        future_processing_vector.push_back(pool.enqueue(processing_vector.at(i)));
    }
}

//Close down application
void Tmpl8::Game::shutdown()
{
}

//UNUSED || template find closest enemy ||
Tank& Tmpl8::Game::find_closest_enemy(Tank& current_tank)
{
    float closest_distance = numeric_limits<float>::infinity();
    int closest_index{};
    int mid = tanks.size() / 2;
    int i;
    int j;

    if (current_tank.allignment == BLUE)
    {
        i = mid;
        j = tanks.size();
    }
    else
    {
        i = 0;
        j = mid;
    }
    for (i; i < j; i++)
    {
        if (tanks.at(i).allignment != current_tank.allignment && tanks.at(i).active)
        {
            float sqr_dist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
            if (sqr_dist < closest_distance)
            {
                closest_distance = sqr_dist;
                closest_index = i;
            }
        }
    }

    return tanks.at(closest_index);
}

//Iterates through all tanks and returns the closest blue tank for the given tank
Tank& Tmpl8::Game::find_closest_blue(Tank& current_tank)
{
    float closest_distance = numeric_limits<float>::infinity();
    int closest_index{};

    for (int i = 0; i < tanks.size()/2; i++)
    {
        if (tanks.at(i).active)
        {
            float sqr_dist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
            if (sqr_dist < closest_distance)
            {
                closest_distance = sqr_dist;
                closest_index = i;
            }
        }
    }

    return tanks.at(closest_index);
}

//Iterates through all red tanks and returns the closest red tank for the given tank
Tank& Tmpl8::Game::find_closest_red(Tank& current_tank)
{
    float closest_distance = numeric_limits<float>::infinity();
    int closest_index{};

    for (int i = tanks.size()/2; i < tanks.size(); i++)
    {
        if (tanks.at(i).active)
        {
            float sqr_dist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
            if (sqr_dist < closest_distance)
            {
                closest_distance = sqr_dist;
                closest_index = i;
            }
        }
    }

    return tanks.at(closest_index);
}

//Updates blue tanks UNUSED
void Tmpl8::Game::update_tanks_blue()
{
    for (int i = 0; i < tanks.size()/2; i++)
    {
        Tank& tank = tanks.at(i);
        if (tank.active)
        {
            //Check tank collision and nudge tanks away from each other
            for (int i = 0; i < tanks.size(); i++)
            {
                Tank& o_tank = tanks.at(i);
                if (&tank == &o_tank) continue;

                vec2 dir = tank.get_position() - o_tank.get_position();
                float dir_squared_len = dir.sqr_length();

                float col_squared_len = (tank.get_collision_radius() + o_tank.get_collision_radius());
                col_squared_len *= col_squared_len;

                if (dir_squared_len < col_squared_len)
                {
                    tank.push(dir.normalized(), 1.f);
                }
            }

            //Move tanks according to speed and nudges (see above) also reload
            tank.tick();
        }
        //Shoot at closest target if reloaded
        if (tank.rocket_reloaded())
        {
            Tank& target = find_closest_red(tank);
            //rocketmtx.lock();
            BlueRockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment,&rocket_blue));
            //rocketmtx.unlock();
            tank.reload_rocket();
        }
    }
}

//Updates red tanks UNUSED
void Tmpl8::Game::update_tanks_red()
{
    for (int i = tanks.size()/2; i < tanks.size(); i++)
    {
        Tank& tank = tanks.at(i);
        if (tank.active)
        {
            //Check tank collision and nudge tanks away from each other
            for (int i = 0; i < tanks.size(); i++)
            {
                Tank& o_tank = tanks.at(i);
                if (&tank == &o_tank) continue;

                vec2 dir = tank.get_position() - o_tank.get_position();
                float dir_squared_len = dir.sqr_length();

                float col_squared_len = (tank.get_collision_radius() + o_tank.get_collision_radius());
                col_squared_len *= col_squared_len;

                if (dir_squared_len < col_squared_len)
                {
                    tank.push(dir.normalized(), 1.f);
                }
            }

            //Move tanks according to speed and nudges (see above) also reload
            tank.tick();
        }
        //Shoot at closest target if reloaded
        if (tank.rocket_reloaded())
        {
            Tank& target = find_closest_blue(tank);
            //rocketmtx.lock();
            RedRockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment,&rocket_red));
            //rocketmtx.unlock();
            tank.reload_rocket();
        }
    }
}

//Updates tanks base template. Can divide into any amount of seperate threads
void Tmpl8::Game::update_tanks_template(int n, int total)
{
    for (int i = tanks.size() / total*n; i < tanks.size()/total*(n+1); i++)
    {
        Tank& tank = tanks.at(i);
        if (tank.active)
        {
            //Check tank collision and nudge tanks away from each other
            for (int j = 0; j < tanks.size(); j++)
            {
                Tank& o_tank = tanks.at(j);
                if (&tank == &o_tank) continue;

                vec2 dir = tank.get_position() - o_tank.get_position();
                float dir_squared_len = dir.sqr_length();

                float col_squared_len = (tank.get_collision_radius() + o_tank.get_collision_radius());
                col_squared_len *= col_squared_len;

                if (dir_squared_len < col_squared_len)
                {
                    tank.push(dir.normalized(), 1.f);
                }
            }
            //Move tanks according to speed and nudges (see above) also reload
            tank.tick();
        }

        if (tank.allignment == BLUE)
        {
            //Shoot at closest target if reloaded for BLUE
            if (tank.rocket_reloaded())
            {
                Tank& target = find_closest_red(tank);
                BlueRocketmtx.lock();
                BlueRockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment, &rocket_blue));
                BlueRocketmtx.unlock();
                tank.reload_rocket();
            }
        }
        else
        {
            //Shoot at closest target if reloaded for RED
            if (tank.rocket_reloaded())
            {
                Tank& target = find_closest_blue(tank);
                RedRocketmtx.lock();
                RedRockets.push_back(Rocket(tank.position, (target.get_position() - tank.position).normalized() * 3, rocket_radius, tank.allignment, &rocket_red));
                RedRocketmtx.unlock();
                tank.reload_rocket();
            }
        }

    }
}

//Updates all blue rockets
void Tmpl8::Game::update_rocket_blue()
{
    for (int i = 0; i < BlueRockets.size(); i++)
    {
        Rocket& rocket = BlueRockets.at(i);
        rocket.tick();
        //if ((rocket.position.x < 0) && (rocket.position.x > SCRWIDTH) && (rocket.position.y < 0) && (rocket.position.y > SCRHEIGHT))
        //{
        //    delete& rocket;
        //}

        //Check if rocket collides with a tank, spawn explosion and if tank is destroyed spawn a smoke plume
        for (int i = tanks.size() / 2; i < tanks.size(); i++)
        {
            Tank& tank = tanks.at(i);
            if (tank.active && rocket.intersects(tank.position, tank.collision_radius))
            {
                explosionmtx.lock();
                explosions.push_back(Explosion(&explosion, tank.position));
                explosionmtx.unlock();

                if (tank.hit(ROCKET_HIT_VALUE))
                {
                    smokemtx.lock();
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                    smokemtx.unlock();
                }

                rocket.active = false;
                break;
            }
        }
    }
}

//Updates all red rockets
void Tmpl8::Game::update_rocket_red()
{
    for (int i = 0; i < RedRockets.size(); i++)
    {
        Rocket& rocket = RedRockets.at(i);
        rocket.tick();
        //if ((rocket.position.x < 0) && (rocket.position.x > SCRWIDTH) && (rocket.position.y < 0) && (rocket.position.y > SCRHEIGHT))
        //{
        //    delete& rocket;
        //}

        //Check if rocket collides with a tank, spawn explosion and if tank is destroyed spawn a smoke plume
        for (int i = 0 / 2; i < tanks.size()/2; i++)
        {
            Tank& tank = tanks.at(i);
            if (tank.active && rocket.intersects(tank.position, tank.collision_radius))
            {
                explosionmtx.lock();
                explosions.push_back(Explosion(&explosion, tank.position));
                explosionmtx.unlock();

                if (tank.hit(ROCKET_HIT_VALUE))
                {
                    smokemtx.lock();
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                    smokemtx.unlock();
                }

                rocket.active = false;
                break;
            }
        }
    }
}

//Updates all smokes
void Tmpl8::Game::update_smoke()
{
    //Update smoke plumes
    for (Smoke& smoke : smokes)
    {
        if ((smoke.position.x >= 0) && (smoke.position.x < SCRWIDTH) && (smoke.position.y >= 0) && (smoke.position.y < SCRHEIGHT)) {
            smoke.tick();
        }
    }
}

//Updates particle beams
void Tmpl8::Game::update_beam()
{
    //Update particle beams
    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.tick(tanks);

        //Damage all tanks within the damage window of the beam (the window is an axis-aligned bounding box)
        for (Tank& tank : tanks)
        {
            if (tank.active && particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius()))
            {
                if (tank.hit(particle_beam.damage))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                }
            }
        }
    }
}

//Update explosion sprites and remove when done with remove erase idiom
void Tmpl8::Game::update_explosion()
{
    
    for (Explosion& explosion : explosions)
    {
        if ((explosion.position.x >= 0) && (explosion.position.x < SCRWIDTH) && (explosion.position.y >= 0) && (explosion.position.y < SCRHEIGHT))
        {
            explosion.tick();
        }
    }

    explosions.erase(std::remove_if(explosions.begin(), explosions.end(), [](const Explosion& explosion) { return explosion.done(); }), explosions.end());
}

//Draw all objects and total healthbar
//racing condition with update tanks
//if tanks update faster than next draw -> access reading violation. <4 update threads == crash
void Tmpl8::Game::draw()
{
    // clear the graphics window
    screen->clear(0);

    //Draw background
    background.draw(screen, 0, 0);

    //Draw sprites
    for (int i = 0; i < NUM_TANKS_BLUE + NUM_TANKS_RED; i++)
    {
        tanks.at(i).draw(screen);

        vec2 tank_pos = tanks.at(i).get_position();
        // tread marks
        if ((tank_pos.x >= 0) && (tank_pos.x < SCRWIDTH) && (tank_pos.y >= 0) && (tank_pos.y < SCRHEIGHT))
            background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH] = sub_blend(background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH], 0x808080);
    }

    for (Rocket& rocket : BlueRockets)
    {
        if ((rocket.position.x >= 0) && (rocket.position.x < SCRWIDTH) && (rocket.position.y >= 0) && (rocket.position.y < SCRHEIGHT))
        {
            rocket.draw(screen);
        }
    }

    for (Rocket& rocket : RedRockets)
    {
        if ((rocket.position.x >= 0) && (rocket.position.x < SCRWIDTH) && (rocket.position.y >= 0) && (rocket.position.y < SCRHEIGHT))
        {
            rocket.draw(screen);
        }
    }

    for (Smoke& smoke : smokes)
    {
        if ((smoke.position.x >= 0) && (smoke.position.x < SCRWIDTH) && (smoke.position.y >= 0) && (smoke.position.y < SCRHEIGHT))
        {
            smoke.draw(screen);
        }

    }

    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.draw(screen);
    }

    for (Explosion& explosion : explosions)
    {
        if ((explosion.position.x >= 0) && (explosion.position.x < SCRWIDTH) && (explosion.position.y >= 0) && (explosion.position.y < SCRHEIGHT))
        {
            explosion.draw(screen);
        }
    }
   
    //Vector for each color
	std::vector<Tank*> tanks_to_sort_blue(NUM_TANKS_BLUE + NUM_TANKS_RED);
    std::vector<Tank*> tanks_to_sort_red(NUM_TANKS_BLUE + NUM_TANKS_RED);

    //Fill vector with tanks
	for (int i = 0; i < NUM_TANKS_BLUE; i++)
	{
		tanks_to_sort_blue[i] = &tanks[i];
	}

    for (int i = NUM_TANKS_BLUE; i < NUM_TANKS_RED+ NUM_TANKS_BLUE; i++)
    {
        tanks_to_sort_red[i] = &tanks[i];
    }

    //Merge sort vectors
	merge_sort_tanks_health(tanks_to_sort_blue, 0, NUM_TANKS_BLUE - 1);
    merge_sort_tanks_health(tanks_to_sort_red, NUM_TANKS_BLUE, NUM_TANKS_BLUE + NUM_TANKS_RED - 1);


    //Draw sorted health bars
	for (int i = 0; i < NUM_TANKS_BLUE; i++)
	{
        int t = 0;
		int health_bar_start_x = i * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X;
		int health_bar_start_y = (t < 1) ? 0 : (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
		int health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
		int health_bar_end_y = (t < 1) ? HEALTH_BAR_HEIGHT : SCRHEIGHT - 1;

		screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
		screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - ((double)tanks_to_sort_blue.at(i)->health / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
	}
    for (int i = 0; i < NUM_TANKS_RED; i++)
    {
        int t = 1;
        int health_bar_start_x = i * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X;
        int health_bar_start_y = (t < 1) ? 0 : (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
        int health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
        int health_bar_end_y = (t < 1) ? HEALTH_BAR_HEIGHT : SCRHEIGHT - 1;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
        screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - ((double)tanks_to_sort_red.at(i+NUM_TANKS_RED)->health / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
    }
}

void Tmpl8::Game::draw_tank()
{
    //Draw sprites
    for (int i = 0; i < NUM_TANKS_BLUE + NUM_TANKS_RED; i++)
    {
        tanks.at(i).draw(screen);

        vec2 tank_pos = tanks.at(i).get_position();
        // tread marks
        if ((tank_pos.x >= 0) && (tank_pos.x < SCRWIDTH) && (tank_pos.y >= 0) && (tank_pos.y < SCRHEIGHT))
            background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH] = sub_blend(background.get_buffer()[(int)tank_pos.x + (int)tank_pos.y * SCRWIDTH], 0x808080);
    }
}

void Tmpl8::Game::draw_rocket_red()
{
    for (Rocket& rocket : RedRockets)
    {
        if ((rocket.position.x >= 0) && (rocket.position.x < SCRWIDTH) && (rocket.position.y >= 0) && (rocket.position.y < SCRHEIGHT))
        {
            rocket.draw(screen);
        }
    }
}

void Tmpl8::Game::draw_rocket_blue()
{
    for (Rocket& rocket : BlueRockets)
    {
        if ((rocket.position.x >= 0) && (rocket.position.x < SCRWIDTH) && (rocket.position.y >= 0) && (rocket.position.y < SCRHEIGHT))
        {
            rocket.draw(screen);
        }
    }
}

void Tmpl8::Game::draw_rest()
{
    for (Smoke& smoke : smokes)
    {
        if ((smoke.position.x >= 0) && (smoke.position.x < SCRWIDTH) && (smoke.position.y >= 0) && (smoke.position.y < SCRHEIGHT))
        {
            smoke.draw(screen);
        }

    }

    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.draw(screen);
    }

    for (Explosion& explosion : explosions)
    {
        if ((explosion.position.x >= 0) && (explosion.position.x < SCRWIDTH) && (explosion.position.y >= 0) && (explosion.position.y < SCRHEIGHT))
        {
            explosion.draw(screen);
        }
    }
}

void Tmpl8::Game::draw_healthbar()
{
    //Vector for each color
    std::vector<Tank*> tanks_to_sort_blue(NUM_TANKS_BLUE + NUM_TANKS_RED);
    std::vector<Tank*> tanks_to_sort_red(NUM_TANKS_BLUE + NUM_TANKS_RED);

    //Fill vector with tanks
    for (int i = 0; i < NUM_TANKS_BLUE; i++)
    {
        tanks_to_sort_blue[i] = &tanks[i];
    }

    for (int i = NUM_TANKS_BLUE; i < NUM_TANKS_RED + NUM_TANKS_BLUE; i++)
    {
        tanks_to_sort_red[i] = &tanks[i];
    }

    //Merge sort vectors
    merge_sort_tanks_health(tanks_to_sort_blue, 0, NUM_TANKS_BLUE - 1);
    merge_sort_tanks_health(tanks_to_sort_red, NUM_TANKS_BLUE, NUM_TANKS_BLUE + NUM_TANKS_RED - 1);


    //Draw sorted health bars
    for (int i = 0; i < NUM_TANKS_BLUE; i++)
    {
        int t = 0;
        int health_bar_start_x = i * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X;
        int health_bar_start_y = (t < 1) ? 0 : (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
        int health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
        int health_bar_end_y = (t < 1) ? HEALTH_BAR_HEIGHT : SCRHEIGHT - 1;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
        screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - ((double)tanks_to_sort_blue.at(i)->health / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
    }
    for (int i = 0; i < NUM_TANKS_RED; i++)
    {
        int t = 1;
        int health_bar_start_x = i * (HEALTH_BAR_WIDTH + HEALTH_BAR_SPACING) + HEALTH_BARS_OFFSET_X;
        int health_bar_start_y = (t < 1) ? 0 : (SCRHEIGHT - HEALTH_BAR_HEIGHT) - 1;
        int health_bar_end_x = health_bar_start_x + HEALTH_BAR_WIDTH;
        int health_bar_end_y = (t < 1) ? HEALTH_BAR_HEIGHT : SCRHEIGHT - 1;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
        screen->bar(health_bar_start_x, health_bar_start_y + (int)((double)HEALTH_BAR_HEIGHT * (1 - ((double)tanks_to_sort_red.at(i + NUM_TANKS_RED)->health / (double)TANK_MAX_HEALTH))), health_bar_end_x, health_bar_end_y, GREENMASK);
    }
}

//Sort tanks by health value using merge sort
void Tmpl8::Game::merge_sort_tanks_health(std::vector<Tank*>& tanks_to_sort, int l, int r)
{
    if (l >= r)
    {
        return; //returns recursively
    }
    int m = (l + r - 1) / 2;
    merge_sort_tanks_health(tanks_to_sort, l, m);
    merge_sort_tanks_health(tanks_to_sort, m + 1, r);
    merge(tanks_to_sort, l, m, r);
}

//Merge sort for health bar
void Tmpl8::Game::merge(std::vector<Tank*>& tanks_to_sort, int l, int m, int r)
{

    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    std::vector<Tank*> left_tanks(n1);
    std::vector<Tank*> right_tanks(n2);


    for (int i = 0; i < n1; i++)
        left_tanks[i] = tanks_to_sort[l + i];
    for (int j = 0; j < n2; j++)
        right_tanks[j] = tanks_to_sort[m + 1 + j];

    i = 0;
    j = 0;
    k = l;

    while (i < n1 && j < n2) {

        Tank* left_tank = left_tanks[i];
        Tank* right_tank = right_tanks[j];
        if ((right_tank->compare_health(*left_tank) > 0))
        {
            tanks_to_sort[k] = left_tank;
            i++;
        }
        else
        {
            tanks_to_sort[k] = right_tank;
            j++;
        }
        k++;
    }

    while (i < n1) {
        tanks_to_sort[k] = left_tanks[i];
        i++;
        k++;
    }

    while (j < n2) {
        tanks_to_sort[k] = right_tanks[j];
        j++;
        k++;
    }
}

//When we reach MAX_FRAMES print the duration and speedup multiplier
void Tmpl8::Game::measure_performance()
{
    char buffer[128];
    if (frame_count >= MAX_FRAMES)
    {
        if (!lock_update)
        {
            duration = perf_timer.elapsed();
            cout << "Duration was: " << duration << " (Replace REF_PERFORMANCE with this value)" << endl;
            cout << "deltaTime was: " << deltatime << endl;
            lock_update = true;
        }

        frame_count--;
    }

    if (lock_update)
    {
        screen->bar(420, 170, 870, 430, 0x030000);
        int ms = (int)duration % 1000, sec = ((int)duration / 1000) % 60, min = ((int)duration / 60000);
        sprintf(buffer, "%02i:%02i:%03i", min, sec, ms);
        frame_count_font->centre(screen, buffer, 200);
        sprintf(buffer, "SPEEDUP: %4.1f", REF_PERFORMANCE / duration);
        frame_count_font->centre(screen, buffer, 340);
    }
}

// Main application tick function
void Tmpl8::Game::tick(float deltaTime)
{
    // TODO screen flickering 
    // wacht tot alle threads klaar zijn en dan nieuwe tick
    // krijg status van de future met de wait_for functie
    // als status van desbetreffende functie == ready dan Enqueue hem opnieuw
    if (!lock_update)
    {
        // clear the graphics window
        screen->clear(0);
        //Draw background
        background.draw(screen, 0, 0);
        for (int i = 0; i < future_draw_vector.size(); i++)
        {
            while (future_draw_vector.at(i).wait_for(0ms) != std::future_status::ready) {}
            future_draw_vector.at(i) = pool.enqueue(draw_vector.at(i));

        }
        for (int i = 0; i < future_processing_vector.size(); i++)
        {
            while (future_processing_vector.at(i).wait_for(0ms) != std::future_status::ready){}
            future_processing_vector.at(i) = pool.enqueue(processing_vector.at(i));
        }
        //Remove exploded rockets with remove erase idiom
        BlueRockets.erase(std::remove_if(BlueRockets.begin(), BlueRockets.end(), [](const Rocket& rocket) { return !rocket.active; }), BlueRockets.end());
        RedRockets.erase(std::remove_if(RedRockets.begin(), RedRockets.end(), [](const Rocket& rocket) { return !rocket.active; }), RedRockets.end());
        //draw();



    }
    else {
        draw();
    }

    //measure time
    deltatime += t.elapsed();
    t.reset();
    measure_performance();

    //Print frame count
    frame_count++;
    string frame_count_string = "FRAME: " + std::to_string(frame_count);
    frame_count_font->print(screen, frame_count_string.c_str(), 350, 580);
}
