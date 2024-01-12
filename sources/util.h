#pragma once

inline float clamp_float (float value, float min, float max) {
    float result  = value < min ? min : value;
    return result > max ? max : result;
}

inline int clamp_int (int value, int min, int max)
{
    int result  = value < min ? min : value;
    return result > max ? max : result;
}