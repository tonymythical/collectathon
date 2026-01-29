#include <bn_core.h>
#include <bn_backdrop.h>
#include <bn_display.h>
#include <bn_log.h>
#include <bn_keypad.h>
#include <bn_random.h>
#include <bn_rect.h>
#include <bn_sprite_ptr.h>
#include <bn_sprite_text_generator.h>
#include <bn_size.h>
#include <bn_string.h>

#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_square.h"
#include "bn_sprite_items_common_fixed_8x16_font.h"

// Pixels / Frame player moves at - Anthony
// Speed boost when A is pressed - Yousif
static constexpr bn::fixed SPEED = 2;
static constexpr bn::fixed SPEED_BOOST = 4;
int SPEED_BOOST_TIMER = 0;
bn::fixed speed = SPEED;
int boost_left = 3;

// Width and height of the the player and treasure bounding boxes, and now the follower.
static constexpr bn::size PLAYER_SIZE = {8, 8};
static constexpr bn::size TREASURE_SIZE = {8, 8};
static constexpr bn::size FOLLOWER_SIZE = {8, 8};

// Full bounds of the screen
static constexpr int MIN_Y = -bn::display::height() / 2;
static constexpr int MAX_Y = bn::display::height() / 2;
static constexpr int MIN_X = -bn::display::width() / 2;
static constexpr int MAX_X = bn::display::width() / 2;

// Number of characters required to show the longest numer possible in an int (-2147483647)
static constexpr int MAX_SCORE_CHARS = 24;

static constexpr int MAX_BOOST_CHARS = 24;

// Score location
static constexpr int SCORE_X = 85;
static constexpr int SCORE_Y = -70;

// Boost indicator location
static constexpr int BOOST_X = -65;
static constexpr int BOOST_Y = -70;

// Player location and follower - Anthony
static constexpr bn::fixed PLAYER_Y = 40;
static constexpr bn::fixed PLAYER_X = 40;
static constexpr bn::fixed FOLLOWER_Y = 60;
static constexpr bn::fixed FOLLOWER_X = 60;

int main()
{
    bn::core::init();

    //enumerator for different state in game/menu - Anthony
    enum class GameState
    {
        MENU,
        GAME
    };
    GameState current_state = GameState::MENU;

    bn::sprite_text_generator text_generator(bn::sprite_font(bn::sprite_items::common_fixed_8x16_font));
    // Changed backdrop color - Yousif
    bn::backdrop::set_color(bn::color(15, 0, 31));

    bn::random rng = bn::random();

    bn::vector<bn::sprite_ptr, 32> menu_sprites;

    // Will hold the sprites for the score
    bn::vector<bn::sprite_ptr, MAX_SCORE_CHARS> score_sprites = {};

    // amount of boosts left display
    bn::string<MAX_SCORE_CHARS> boost_string = {};
    bn::vector<bn::sprite_ptr, MAX_BOOST_CHARS> boost_sprites = {};

    int score = 0;
    bn::sprite_ptr follower = bn::sprite_items::dot.create_sprite(FOLLOWER_X, FOLLOWER_Y);
    bn::sprite_ptr player = bn::sprite_items::square.create_sprite(PLAYER_X, PLAYER_Y);
    bn::sprite_ptr treasure = bn::sprite_items::dot.create_sprite(0, 0);

    bool menu_text_generated = false;

    // Main game loop
    while (true)
    {
        // main menu loop
        if (current_state == GameState::MENU)
        {
            if (!menu_text_generated)
            {
                text_generator.set_center_alignment();
                text_generator.generate(0, -50, "GRABBER", menu_sprites);
                text_generator.generate(0, 50, "PRESS START TO PLAY", menu_sprites);
                menu_text_generated = true;
            }

            // Fun flicker effect for the treasure - Anthony
            treasure.set_position(15 + rng.get_fixed(-1, 1), 15 + rng.get_fixed(-1, 1));

            if (bn::keypad::start_pressed())
            {
                current_state = GameState::GAME;
                menu_sprites.clear();
            }
            bn::core::update();
            continue;
        }
        else if (current_state == GameState::GAME)
        {
            // speed boost when a is pressed, stays for 3 seconds - Yousif
            if (bn::keypad::a_pressed() && SPEED_BOOST_TIMER == 0 && boost_left > 0)
            {
                speed = SPEED_BOOST;
                SPEED_BOOST_TIMER = 180; // 3 seconds
                boost_left--;
            }

            if (SPEED_BOOST_TIMER > 0)
            {
                SPEED_BOOST_TIMER--;
                // Jutter player position to simulate speed/running effect for fun - Anthony
                player.set_x(player.x() + rng.get_fixed(-0.5, 0.5));
                player.set_y(player.y() + rng.get_fixed(-0.5, 0.5));
                if (SPEED_BOOST_TIMER == 0)
                {
                    speed = SPEED;
                }
            }
            // game resets when start is pressed - Yousif
            if (bn::keypad::start_pressed())
            {
                score = 0;
                player.set_position(-50, 50);
                treasure.set_position(0, 0);
                score_sprites.clear();
                boost_left = 3;
            }
            // Move player with d-pad
            if (bn::keypad::left_held())
            {
                player.set_x(player.x() - speed);
            }
            if (bn::keypad::right_held())
            {
                player.set_x(player.x() + speed);
            }
            if (bn::keypad::up_held())
            {
                player.set_y(player.y() - speed);
            }
            if (bn::keypad::down_held())
            {
                player.set_y(player.y() + speed);
            }

            // The bounding boxes of the player and treasure, snapped to integer pixels
            bn::rect player_rect = bn::rect(player.x().round_integer(),
                                            player.y().round_integer(),
                                            PLAYER_SIZE.width(),
                                            PLAYER_SIZE.height());
            bn::rect treasure_rect = bn::rect(treasure.x().round_integer(),
                                              treasure.y().round_integer(),
                                              TREASURE_SIZE.width(),
                                              TREASURE_SIZE.height());

            // If the bounding boxes overlap, set the treasure to a new location an increase score
            if (player_rect.intersects(treasure_rect))
            {
                // Jump to any random point in the screen
                int new_x = rng.get_int(MIN_X, MAX_X);
                int new_y = rng.get_int(MIN_Y, MAX_Y);
                treasure.set_position(new_x, new_y);

                score++;
            }

            // Player loops through x - Anthony
            if (player.x() < MIN_X)
            {
                player.set_x(MAX_X);
            }
            else if (player.x() > MAX_X)
            {
                player.set_x(MIN_X);
            }

            // Player loops through y - Anthony
            if (player.y() < MIN_Y)
            {
                player.set_y(MAX_Y);
            }
            else if (player.y() > MAX_Y)
            {
                player.set_y(MIN_Y);
            }

            // Update score display
            bn::string<MAX_SCORE_CHARS> score_string = "SCORE: " + bn::to_string<MAX_SCORE_CHARS>(score);
            score_sprites.clear();
            text_generator.generate(SCORE_X, SCORE_Y,
                                    score_string,
                                    score_sprites);

            // Update boost display
            boost_string.clear();
            boost_string = "BOOST LEFT: " + bn::to_string<MAX_BOOST_CHARS>(boost_left);
            boost_sprites.clear();
            text_generator.generate(BOOST_X, BOOST_Y,
                                    boost_string,
                                    boost_sprites);
        }

        // Update RNG seed every frame so we don't get the same sequence of positions every time
        rng.update();

        bn::core::update();
    }
}