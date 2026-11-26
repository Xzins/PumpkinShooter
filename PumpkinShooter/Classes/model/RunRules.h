#pragma once

enum class Difficulty { Easy, Normal, Hard };
enum class Weapon { Pea, MachineGun, Corn };

inline const char* bestScoreKey(Difficulty difficulty)
{
    switch (difficulty)
    {
        case Difficulty::Easy: return "best_score_easy";
        case Difficulty::Normal: return "best_score_normal";
        case Difficulty::Hard: return "best_score_hard";
    }
    return "best_score_easy";
}

struct Impact
{
    float x;
    float y;
    bool blast;
};

struct Pickup
{
    Weapon weapon;
    float x;
    float y;
};
