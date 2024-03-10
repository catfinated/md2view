#include "engine.hpp"

bool Engine::check_key_pressed(unsigned int key) 
{
    gsl_Expects(key < max_keys);

    if (keys_[key] && !keys_pressed_[key]) {
        keys_pressed_[key] = true;
        return true;
    }
    return false;
}