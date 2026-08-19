#include "raylib.h"
#include <cmath>

// ============================================================
// KNIFE FIGHTS
// ============================================================
//
// A simple local multiplayer sword game.
//
// P1:
// A / D       Move
// W / S       Sword level
// SPACE       Thrust
// LEFT SHIFT  Charge overhead
//
// P2:
// LEFT/RIGHT   Move
// UP/DOWN      Sword level
// RIGHT SHIFT  Thrust
// ENTER        Charge overhead
//
// TITLE SCREEN:
// W / S        Select option
// A / D        Change option
// ENTER        Start
// Q / E        P1 colour
// I / O        P2 colour
// M            Toggle sound
// N            Toggle music
//
// ============================================================


// ============================================================
// WINDOW
// ============================================================

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 600;

const float GROUND_Y = 470.0f;


// ============================================================
// PLAYER
// ============================================================

const float PLAYER_WIDTH = 30.0f;
const float PLAYER_HEIGHT = 50.0f;

const float NORMAL_SPEED = 4.0f;


// ============================================================
// SWORD
// ============================================================

const float SWORD_LENGTH = 82.0f;
const float SWORD_THICKNESS = 7.0f;


// ============================================================
// NORMAL THRUST
// ============================================================

const float THRUST_DURATION = 0.24f;
const float THRUST_DISTANCE = 65.0f;

// How far the actual PLAYER moves forward during a thrust.
// This is separate from THRUST_DISTANCE, which controls
// the sword's forward movement.
const float THRUST_STEP_DISTANCE = 28.0f;

// Additional recovery after the thrust ends.
// This gives the opponent a punish window without requiring
// a parry.
const float THRUST_RECOVERY = 0.22f;

const float THRUST_COOLDOWN = 0.40f;


// ============================================================
// CHARGE
// ============================================================

const float FULL_CHARGE_TIME = 1.10f;

const float EXHAUSTION_TIME = 2.40f;

const float EXHAUSTED_DURATION = 1.50f;


// ============================================================
// OVERHEAD
// ============================================================

const float OVERHEAD_DURATION = 0.52f;

const float OVERHEAD_DASH_TIME = 0.42f;

const float OVERHEAD_DASH_DISTANCE = 250.0f;

// Reduced from the previous attack range.
const float OVERHEAD_RANGE = 130.0f;

const float OVERHEAD_RECOVERY = 1.20f;

// Hit stop: briefly freezes the game on contact.
// The overhead gets a stronger hit stop.
const float HIT_STOP_NORMAL = 0.20f;
const float HIT_STOP_OVERHEAD = 0.38f;

// Impact camera punch. The camera zooms toward the point of impact
// when a player scores, then smoothly returns to normal.
const float IMPACT_ZOOM_NORMAL = 1.18f;
const float IMPACT_ZOOM_OVERHEAD = 1.32f;
const float IMPACT_ZOOM_HOLD_NORMAL = 0.22f;
const float IMPACT_ZOOM_HOLD_OVERHEAD = 0.40f;


// ============================================================
// STUN / PARRY
// ============================================================

const float STUN_DURATION = 0.65f;

const float PERFECT_PARRY_DURATION = 1.10f;

const float PERFECT_PARRY_WINDOW = 0.14f;


// ============================================================
// MENU
// ============================================================

const int SCORE_OPTIONS[] =
{
    1,
    3,
    5,
    7,
    10
};

const int SCORE_OPTION_COUNT = 5;


const int TIME_OPTIONS[] =
{
    0,
    30,
    60,
    90,
    120,
    180
};

const int TIME_OPTION_COUNT = 6;


// ============================================================
// COLOUR PALETTE
// ============================================================

struct PaletteColour
{
    const char* name;
    Color colour;
};


const PaletteColour COLOURS[] =
{
    { "BLUE",     SKYBLUE },
    { "RED",      RED },
    { "GREEN",    LIME },
    { "PURPLE",   PURPLE },
    { "YELLOW",   GOLD },
    { "ORANGE",   ORANGE },
    { "PINK",     PINK },
    { "CYAN",     SKYBLUE },
    { "WHITE",    RAYWHITE },
    { "MAGENTA",  MAGENTA }
};


const int COLOUR_COUNT =
sizeof(COLOURS) /
sizeof(COLOURS[0]);


// ============================================================
// UTILITY
// ============================================================

float ClampFloat(
    float value,
    float minimum,
    float maximum)
{
    if (value < minimum)
        return minimum;

    if (value > maximum)
        return maximum;

    return value;
}


// ============================================================
// PLAYER
// ============================================================

struct Player
{
    float x;
    float y;

    bool facingRight;

    int swordLevel;


    // --------------------------------------------------------
    // Thrust
    // --------------------------------------------------------

    bool thrusting;

    float thrustTimer;

    float thrustCooldown;

    float thrustRecoveryTimer;

    float thrustStartX;


    // --------------------------------------------------------
    // Charge
    // --------------------------------------------------------

    bool charging;

    float chargeTimer;

    bool fullyCharged;


    // --------------------------------------------------------
    // Exhaustion
    // --------------------------------------------------------

    bool exhausted;

    float exhaustedTimer;


    // --------------------------------------------------------
    // Overhead
    // --------------------------------------------------------

    bool overhead;

    float overheadTimer;

    float overheadStartX;

    bool overheadHit;


    // --------------------------------------------------------
    // Recovery
    // --------------------------------------------------------

    float recoveryTimer;


    // --------------------------------------------------------
    // Stun
    // --------------------------------------------------------

    float stunTimer;


    // --------------------------------------------------------
    // Perfect parry
    // --------------------------------------------------------

    float swordChangeTimer;


    // --------------------------------------------------------
    // Score
    // --------------------------------------------------------

    int score;


    // --------------------------------------------------------
    // Colours
    // --------------------------------------------------------

    Color bodyColor;
    Color swordColor;
};


// ============================================================
// SWORD
// ============================================================

struct Sword
{
    Vector2 handle;
    Vector2 bladeStart;
    Vector2 tip;
};


// ============================================================
// COMBAT EFFECT
// ============================================================

struct CombatEffect
{
    bool active;

    Vector2 position;

    float timer;

    float duration;

    bool perfect;
};


// ============================================================
// SCORE POPUP
// ============================================================

struct ScorePopup
{
    bool active;

    int player;

    Color color;

    float timer;

    float duration;
};


// ============================================================
// OVERHEAD IMPACT FLASH
// ============================================================

struct ImpactFlash
{
    bool active;

    Vector2 position;

    float timer;

    float duration;
};


// ============================================================
// SOUND SYSTEM
// ============================================================

struct GameAudio
{
    Sound thrust;
    Sound parry;
    Sound perfectParry;
    Sound hit;
    Sound overhead;
    Sound score;
    Sound menu;

    bool soundsMuted;
    bool musicMuted;

    float musicTimer;

    int musicNote;
};


// ============================================================
// CREATE SIMPLE TONE
// ============================================================

Sound CreateTone(
    float frequency,
    float duration,
    float volume)
{
    const int sampleRate = 44100;

    int sampleCount =
        (int)(
            duration *
            sampleRate
            );


    short* samples =
        (short*)MemAlloc(
            sampleCount *
            sizeof(short)
        );


    for (int i = 0; i < sampleCount; i++)
    {
        float t =
            (float)i /
            sampleRate;


        float envelope = 1.0f;


        if (t < 0.015f)
        {
            envelope =
                t / 0.015f;
        }


        if (t >
            duration - 0.04f)
        {
            envelope =
                (duration - t) /
                0.04f;
        }


        float sample =
            sinf(
                2.0f *
                PI *
                frequency *
                t
            );


        samples[i] =
            (short)(
                sample *
                32767.0f *
                volume *
                envelope
                );
    }


    Wave wave{};

    wave.frameCount =
        sampleCount;

    wave.sampleRate =
        sampleRate;

    wave.sampleSize =
        16;

    wave.channels =
        1;

    wave.data =
        samples;


    Sound sound =
        LoadSoundFromWave(wave);


    UnloadWave(wave);

    return sound;
}


// ============================================================
// INITIALISE AUDIO
// ============================================================

void InitGameAudio(
    GameAudio& audio)
{
    InitAudioDevice();


    audio.thrust =
        CreateTone(
            220.0f,
            0.08f,
            0.25f
        );


    audio.parry =
        CreateTone(
            500.0f,
            0.12f,
            0.35f
        );


    audio.perfectParry =
        CreateTone(
            900.0f,
            0.18f,
            0.40f
        );


    audio.hit =
        CreateTone(
            120.0f,
            0.15f,
            0.35f
        );


    audio.overhead =
        CreateTone(
            160.0f,
            0.22f,
            0.35f
        );


    audio.score =
        CreateTone(
            700.0f,
            0.20f,
            0.30f
        );


    audio.menu =
        CreateTone(
            440.0f,
            0.08f,
            0.20f
        );


    audio.soundsMuted = false;
    audio.musicMuted = false;

    audio.musicTimer = 0.0f;
    audio.musicNote = 0;
}


// ============================================================
// PLAY SOUND
// ============================================================

void PlayGameSound(
    GameAudio& audio,
    Sound& sound)
{
    if (!audio.soundsMuted)
    {
        PlaySound(sound);
    }
}


// ============================================================
// SIMPLE BACKGROUND MUSIC
// ============================================================

void UpdateMusic(
    GameAudio& audio)
{
    if (audio.musicMuted)
        return;


    audio.musicTimer -=
        GetFrameTime();


    if (audio.musicTimer > 0)
        return;


    const float notes[] =
    {
        261.63f,
        329.63f,
        392.00f,
        329.63f,
        293.66f,
        349.23f,
        440.00f,
        349.23f
    };


    Sound musicTone =
        CreateTone(
            notes[audio.musicNote],
            0.18f,
            0.035f
        );


    PlaySound(musicTone);


    audio.musicNote++;


    if (audio.musicNote >= 8)
        audio.musicNote = 0;


    audio.musicTimer =
        0.32f;
}


// ============================================================
// UNLOAD AUDIO
// ============================================================

void UnloadGameAudio(
    GameAudio& audio)
{
    UnloadSound(audio.thrust);
    UnloadSound(audio.parry);
    UnloadSound(audio.perfectParry);
    UnloadSound(audio.hit);
    UnloadSound(audio.overhead);
    UnloadSound(audio.score);
    UnloadSound(audio.menu);

    CloseAudioDevice();
}


// ============================================================
// SWORD LEVEL HEIGHT
// ============================================================

float GetSwordLevelY(
    const Player& player)
{
    if (player.swordLevel == 0)
        return player.y - 7.0f;

    if (player.swordLevel == 1)
        return player.y - 28.0f;

    return player.y - 49.0f;
}


// ============================================================
// NORMAL SWORD
// ============================================================

Sword GetNormalSword(
    const Player& player)
{
    Sword sword;


    float direction =
        player.facingRight
        ? 1.0f
        : -1.0f;


    float x =
        player.x;


    if (player.thrusting)
    {
        float progress =
            1.0f -
            player.thrustTimer /
            THRUST_DURATION;


        progress =
            ClampFloat(
                progress,
                0.0f,
                1.0f
            );


        x +=
            direction *
            THRUST_DISTANCE *
            progress;
    }


    float y =
        GetSwordLevelY(player);


    sword.handle =
    {
        x -
            direction * 8.0f,
        y
    };


    sword.bladeStart =
    {
        x +
            direction * 7.0f,
        y
    };


    sword.tip =
    {
        sword.bladeStart.x +
            direction *
            SWORD_LENGTH,

        sword.bladeStart.y
    };


    return sword;
}


// ============================================================
// OVERHEAD SWORD
// ============================================================

Sword GetOverheadSword(
    const Player& player)
{
    Sword sword;


    float progress =
        1.0f -
        player.overheadTimer /
        OVERHEAD_DURATION;


    progress =
        ClampFloat(
            progress,
            0.0f,
            1.0f
        );


    float direction =
        player.facingRight
        ? 1.0f
        : -1.0f;


    const float startAngle = -PI * 0.78f;
    const float endAngle = PI * 0.12f;

    float angle =
        startAngle +
        (endAngle -
            startAngle) *
        progress;


    Vector2 root =
    {
        player.x,
        player.y - 53.0f
    };


    float bladeDirectionX =
        cosf(angle) * direction;

    float bladeDirectionY =
        sinf(angle);

    sword.handle =
    {
        root.x -
            bladeDirectionX * 9.0f,

        root.y -
            bladeDirectionY * 9.0f
    };


    sword.bladeStart =
    {
        root.x +
            bladeDirectionX * 7.0f,

        root.y +
            bladeDirectionY * 7.0f
    };


    sword.tip =
    {
        sword.bladeStart.x +
            bladeDirectionX *
            SWORD_LENGTH,

        sword.bladeStart.y +
            bladeDirectionY *
            SWORD_LENGTH
    };


    return sword;
}


// ============================================================
// DRAW SWORD
// ============================================================

void DrawSword(
    const Sword& sword,
    Color bladeColor)
{
    DrawLineEx(
        sword.handle,
        sword.bladeStart,
        8.0f,
        DARKGRAY
    );


    DrawCircle(
        (int)sword.handle.x,
        (int)sword.handle.y,
        4,
        BLACK
    );


    Vector2 direction =
    {
        sword.tip.x -
            sword.bladeStart.x,

        sword.tip.y -
            sword.bladeStart.y
    };


    float length =
        sqrtf(
            direction.x *
            direction.x +
            direction.y *
            direction.y
        );


    if (length > 0.001f)
    {
        direction.x /= length;
        direction.y /= length;
    }


    Vector2 perpendicular =
    {
        -direction.y,
        direction.x
    };


    Vector2 guardA =
    {
        sword.bladeStart.x +
            perpendicular.x * 9.0f,

        sword.bladeStart.y +
            perpendicular.y * 9.0f
    };


    Vector2 guardB =
    {
        sword.bladeStart.x -
            perpendicular.x * 9.0f,

        sword.bladeStart.y -
            perpendicular.y * 9.0f
    };


    DrawLineEx(
        guardA,
        guardB,
        5.0f,
        DARKGRAY
    );


    DrawLineEx(
        sword.bladeStart,
        sword.tip,
        SWORD_THICKNESS,
        bladeColor
    );
}


// ============================================================
// PLAYER HITBOX
// ============================================================

Rectangle GetPlayerHitbox(
    const Player& player)
{
    return
    {
        player.x -
            PLAYER_WIDTH / 2.0f,

        player.y -
            PLAYER_HEIGHT,

        PLAYER_WIDTH,

        PLAYER_HEIGHT
    };
}


// ============================================================
// SWORD HITS PLAYER
// ============================================================

bool SwordHitsPlayer(
    const Sword& sword,
    const Player& target)
{
    Rectangle box =
        GetPlayerHitbox(target);


    if (CheckCollisionPointRec(
        sword.tip,
        box))
    {
        return true;
    }


    if (CheckCollisionPointRec(
        sword.bladeStart,
        box))
    {
        return true;
    }


    Vector2 hit;


    if (CheckCollisionLines(
        sword.bladeStart,
        sword.tip,
        {
            box.x,
            box.y
        },
        {
            box.x +
                box.width,
            box.y
        },
        &hit))
    {
        return true;
    }


    if (CheckCollisionLines(
        sword.bladeStart,
        sword.tip,
        {
            box.x +
                box.width,
            box.y
        },
        {
            box.x +
                box.width,
            box.y +
                box.height
        },
        &hit))
    {
        return true;
    }


    if (CheckCollisionLines(
        sword.bladeStart,
        sword.tip,
        {
            box.x +
                box.width,
            box.y +
                box.height
        },
        {
            box.x,
            box.y +
                box.height
        },
        &hit))
    {
        return true;
    }


    if (CheckCollisionLines(
        sword.bladeStart,
        sword.tip,
        {
            box.x,
            box.y +
                box.height
        },
        {
            box.x,
            box.y
        },
        &hit))
    {
        return true;
    }


    return false;
}


// ============================================================
// OVERHEAD HITBOX
// ============================================================


bool OverheadHitsPlayer(
    const Player& attacker,
    const Player& defender)
{
    Sword sword =
        GetOverheadSword(attacker);

    return SwordHitsPlayer(
        sword,
        defender
    );
}


// ============================================================
// PARRY ZONE
// ============================================================

Rectangle GetParryZone(
    const Player& player)
{
    Sword sword =
        GetNormalSword(player);


    float minX =
        fminf(
            sword.bladeStart.x,
            sword.tip.x
        );


    float maxX =
        fmaxf(
            sword.bladeStart.x,
            sword.tip.x
        );


    return
    {
        minX,
        sword.bladeStart.y - 13.0f,
        maxX - minX,
        26.0f
    };
}


// ============================================================
// CHECK PARRY
// ============================================================
//
// 0 = no parry
// 1 = normal parry
// 2 = perfect parry
//

int CheckParry(
    const Player& attacker,
    const Player& defender)
{
    if (attacker.overhead)
        return 0;


    if (defender.charging)
        return 0;


    if (defender.exhausted)
        return 0;


    if (defender.overhead)
        return 0;


    if (!attacker.thrusting)
        return 0;


    if (attacker.swordLevel !=
        defender.swordLevel)
    {
        return 0;
    }


    Sword attackSword =
        GetNormalSword(attacker);


    Rectangle parryZone =
        GetParryZone(defender);


    if (!CheckCollisionPointRec(
        attackSword.tip,
        parryZone))
    {
        return 0;
    }


    if (defender.swordChangeTimer > 0)
        return 2;


    return 1;
}


// ============================================================
// RESET PLAYER
// ============================================================

void ResetPlayer(
    Player& player,
    float x,
    bool facingRight)
{
    player.x = x;

    player.y = GROUND_Y;

    player.facingRight =
        facingRight;


    player.swordLevel = 1;


    player.thrusting = false;

    player.thrustTimer = 0;

    player.thrustCooldown = 0;

    player.thrustRecoveryTimer = 0;

    player.thrustStartX = x;


    player.charging = false;

    player.chargeTimer = 0;

    player.fullyCharged = false;


    player.exhausted = false;

    player.exhaustedTimer = 0;


    player.overhead = false;

    player.overheadTimer = 0;

    player.overheadStartX = x;

    player.overheadHit = false;


    player.recoveryTimer = 0;


    player.stunTimer = 0;


    player.swordChangeTimer = 0;
}


// ============================================================
// RESET ROUND
// ============================================================

void ResetRound(
    Player& p1,
    Player& p2)
{
    int score1 = p1.score;
    int score2 = p2.score;

    Color p1Body = p1.bodyColor;
    Color p1Sword = p1.swordColor;

    Color p2Body = p2.bodyColor;
    Color p2Sword = p2.swordColor;


    ResetPlayer(
        p1,
        250.0f,
        true
    );


    ResetPlayer(
        p2,
        950.0f,
        false
    );


    p1.score = score1;
    p2.score = score2;


    p1.bodyColor = p1Body;
    p1.swordColor = p1Sword;

    p2.bodyColor = p2Body;
    p2.swordColor = p2Sword;
}


// ============================================================
// START THRUST
// ============================================================

void StartThrust(
    Player& player)
{
    if (player.stunTimer > 0)
        return;

    if (player.charging)
        return;

    if (player.exhausted)
        return;

    if (player.overhead)
        return;

    if (player.recoveryTimer > 0)
        return;

    if (player.thrustRecoveryTimer > 0)
        return;

    if (player.thrustCooldown > 0)
        return;


    player.thrusting = true;

    player.thrustTimer =
        THRUST_DURATION;

    player.thrustCooldown =
        THRUST_COOLDOWN;

    player.thrustRecoveryTimer = 0;

    player.thrustStartX =
        player.x;
}


// ============================================================
// START CHARGE
// ============================================================

void StartCharge(
    Player& player)
{
    if (player.stunTimer > 0)
        return;

    if (player.thrusting)
        return;

    if (player.overhead)
        return;

    if (player.recoveryTimer > 0)
        return;

    if (player.thrustRecoveryTimer > 0)
        return;

    if (player.exhausted)
        return;


    player.charging = true;

    player.chargeTimer = 0;

    player.fullyCharged = false;
}


// ============================================================
// RELEASE CHARGE
// ============================================================

void ReleaseCharge(
    Player& player,
    GameAudio& audio)
{
    if (!player.charging)
        return;


    if (!player.fullyCharged)
    {
        player.charging = false;

        player.chargeTimer = 0;

        return;
    }


    player.charging = false;

    player.overhead = true;

    player.overheadTimer =
        OVERHEAD_DURATION;

    player.overheadStartX =
        player.x;

    player.overheadHit = false;


    player.swordLevel = 1;

    player.swordChangeTimer = 0;


    PlayGameSound(
        audio,
        audio.overhead
    );
}


// ============================================================
// UPDATE PLAYER
// ============================================================

void UpdatePlayer(
    Player& player)
{
    float dt =
        GetFrameTime();


    if (player.thrustCooldown > 0)
    {
        player.thrustCooldown -= dt;

        if (player.thrustCooldown < 0)
            player.thrustCooldown = 0;
    }


    // --------------------------------------------------------
    // Thrust
    // --------------------------------------------------------

    if (player.thrusting)
    {
        player.thrustTimer -= dt;

        if (player.thrustTimer <= 0)
        {
            player.thrustTimer = 0;

            player.thrusting = false;

            // The thrust is finished, but the player is
            // temporarily vulnerable during this recovery.
            player.thrustRecoveryTimer =
                THRUST_RECOVERY;
        }
    }


    if (player.thrustRecoveryTimer > 0)
    {
        player.thrustRecoveryTimer -= dt;

        if (player.thrustRecoveryTimer < 0)
            player.thrustRecoveryTimer = 0;
    }


    // --------------------------------------------------------
    // Charge
    // --------------------------------------------------------

    if (player.charging)
    {
        player.chargeTimer += dt;


        if (player.chargeTimer >=
            FULL_CHARGE_TIME)
        {
            player.fullyCharged = true;
        }


        if (player.chargeTimer >=
            EXHAUSTION_TIME)
        {
            player.charging = false;

            player.fullyCharged = false;

            player.chargeTimer = 0;

            player.exhausted = true;

            player.exhaustedTimer =
                EXHAUSTED_DURATION;
        }
    }


    // --------------------------------------------------------
    // Exhaustion
    // --------------------------------------------------------

    if (player.exhausted)
    {
        player.exhaustedTimer -= dt;

        if (player.exhaustedTimer <= 0)
        {
            player.exhaustedTimer = 0;

            player.exhausted = false;
        }
    }


    // --------------------------------------------------------
    // Overhead
    // --------------------------------------------------------

    if (player.overhead)
    {
        player.overheadTimer -= dt;

        if (player.overheadTimer <= 0)
        {
            player.overheadTimer = 0;

            player.overhead = false;

            player.recoveryTimer =
                OVERHEAD_RECOVERY;

            player.swordLevel = 1;
        }
    }


    // --------------------------------------------------------
    // Overhead recovery
    // --------------------------------------------------------

    if (player.recoveryTimer > 0)
    {
        player.recoveryTimer -= dt;

        if (player.recoveryTimer < 0)
            player.recoveryTimer = 0;
    }


    // --------------------------------------------------------
    // Stun
    // --------------------------------------------------------

    if (player.stunTimer > 0)
    {
        player.stunTimer -= dt;

        if (player.stunTimer < 0)
            player.stunTimer = 0;
    }


    // --------------------------------------------------------
    // Sword-change window
    // --------------------------------------------------------

    if (player.swordChangeTimer > 0)
    {
        player.swordChangeTimer -= dt;

        if (player.swordChangeTimer < 0)
            player.swordChangeTimer = 0;
    }
}


// ============================================================
// MOVEMENT SPEED
// ============================================================

float GetMovementSpeed(
    const Player& player)
{
    if (player.overhead)
        return 0;


    if (player.stunTimer > 0)
        return 0;


    if (player.exhausted)
        return NORMAL_SPEED * 0.10f;


    if (player.recoveryTimer > 0)
        return NORMAL_SPEED * 0.25f;


    if (player.thrustRecoveryTimer > 0)
        return NORMAL_SPEED * 0.15f;


    if (!player.charging)
        return NORMAL_SPEED;


    float progress =
        player.chargeTimer /
        FULL_CHARGE_TIME;


    progress =
        ClampFloat(
            progress,
            0.0f,
            1.0f
        );


    float multiplier =
        1.0f -
        progress * 0.85f;


    return NORMAL_SPEED *
        multiplier;
}


// ============================================================
// CHANGE SWORD LEVEL
// ============================================================

void ChangeSwordLevel(
    Player& player,
    int newLevel)
{
    if (newLevel < 0)
        newLevel = 0;


    if (newLevel > 2)
        newLevel = 2;


    if (newLevel ==
        player.swordLevel)
    {
        return;
    }


    player.swordLevel =
        newLevel;


    player.swordChangeTimer =
        PERFECT_PARRY_WINDOW;
}


// ============================================================
// CHARGE GLOW
// ============================================================

void DrawChargeGlow(
    const Player& player)
{
    if (!player.charging)
        return;


    float progress =
        player.chargeTimer /
        FULL_CHARGE_TIME;


    progress =
        ClampFloat(
            progress,
            0.0f,
            1.0f
        );


    float frequency =
        3.0f +
        progress * 17.0f;


    float pulse =
        (
            sinf(
                (float)GetTime() *
                frequency
            ) +
            1.0f
            ) *
        0.5f;


    float radius =
        20.0f +
        progress * 20.0f +
        pulse * 5.0f;


    float alpha =
        0.20f +
        progress * 0.55f;


    DrawCircleLines(
        (int)player.x,
        (int)player.y - 42,
        radius,
        ColorAlpha(
            player.swordColor,
            alpha
        )
    );


    if (player.fullyCharged)
    {
        float brightPulse =
            (
                sinf(
                    (float)GetTime() *
                    24.0f
                ) +
                1.0f
                ) *
            0.5f;


        if (brightPulse > 0.35f)
        {
            DrawCircleLines(
                (int)player.x,
                (int)player.y - 42,
                radius + 10.0f,
                player.swordColor
            );
        }
    }
}


// ============================================================
// DRAW PLAYER
// ============================================================

void DrawPlayer(
    const Player& player)
{
    DrawChargeGlow(player);


    Color body =
        player.bodyColor;


    if (player.stunTimer > 0)
    {
        if (sinf(
            (float)GetTime() * 20.0f
        ) > 0)
        {
            body = WHITE;
        }
    }


    if (player.exhausted)
    {
        body =
            ColorAlpha(
                GRAY,
                0.75f
            );
    }


    DrawRectangle(
        (int)player.x - 15,
        (int)player.y - 50,
        30,
        50,
        body
    );


    DrawCircle(
        (int)player.x,
        (int)player.y - 65,
        15,
        BEIGE
    );


    float direction =
        player.facingRight
        ? 1.0f
        : -1.0f;


    DrawCircle(
        (int)(
            player.x +
            direction * 5.0f
            ),
        (int)player.y - 68,
        3,
        BLACK
    );


    Sword sword;


    if (player.overhead)
    {
        sword =
            GetOverheadSword(player);
    }
    else
    {
        sword =
            GetNormalSword(player);
    }


    DrawSword(
        sword,
        player.swordColor
    );


    // --------------------------------------------------------
    // Charge bar
    // --------------------------------------------------------

    if (player.charging)
    {
        float progress =
            player.chargeTimer /
            FULL_CHARGE_TIME;


        progress =
            ClampFloat(
                progress,
                0.0f,
                1.0f
            );


        DrawRectangle(
            (int)player.x - 40,
            (int)player.y - 112,
            80,
            8,
            DARKGRAY
        );


        DrawRectangle(
            (int)player.x - 40,
            (int)player.y - 112,
            (int)(
                80.0f *
                progress
                ),
            8,
            player.swordColor
        );


        if (player.fullyCharged)
        {
            DrawText(
                "READY",
                (int)player.x - 26,
                (int)player.y - 140,
                16,
                player.swordColor
            );
        }
    }


    if (player.exhausted)
    {
        DrawText(
            "EXHAUSTED!",
            (int)player.x - 47,
            (int)player.y - 108,
            18,
            GRAY
        );
    }


    if (player.stunTimer > 0)
    {
        DrawText(
            "STUN!",
            (int)player.x - 27,
            (int)player.y - 105,
            18,
            YELLOW
        );
    }


    if (player.recoveryTimer > 0)
    {
        DrawText(
            "RECOVERY",
            (int)player.x - 40,
            (int)player.y - 105,
            15,
            GRAY
        );
    }


    if (player.thrustRecoveryTimer > 0)
    {
        DrawText(
            "RECOVERY",
            (int)player.x - 40,
            (int)player.y - 105,
            15,
            GRAY
        );
    }
}


// ============================================================
// DRAW PARRY ZONE
// ============================================================

void DrawParryZone(
    const Player& player,
    const Player& attacker)
{
    if (player.charging)
        return;


    if (player.exhausted)
        return;


    if (player.overhead)
        return;


    if (attacker.overhead)
        return;


    if (!attacker.thrusting)
        return;


    if (player.swordLevel !=
        attacker.swordLevel)
    {
        return;
    }


    Rectangle zone =
        GetParryZone(player);


    DrawRectangleRec(
        zone,
        ColorAlpha(
            player.swordColor,
            0.06f
        )
    );
}


// ============================================================
// COMBAT EFFECT
// ============================================================

void CreateCombatEffect(
    CombatEffect& effect,
    Vector2 position,
    bool perfect)
{
    effect.active = true;

    effect.position =
        position;

    effect.perfect =
        perfect;

    effect.duration =
        perfect
        ? 0.40f
        : 0.25f;

    effect.timer =
        effect.duration;
}


void DrawCombatEffect(
    const CombatEffect& effect)
{
    if (!effect.active)
        return;


    float progress =
        1.0f -
        effect.timer /
        effect.duration;


    float radius =
        10.0f +
        progress * 60.0f;


    Color color =
        effect.perfect
        ? GOLD
        : ORANGE;


    float alpha =
        1.0f -
        progress;


    DrawCircleLines(
        (int)effect.position.x,
        (int)effect.position.y,
        radius,
        ColorAlpha(
            color,
            alpha
        )
    );


    const char* text =
        effect.perfect
        ? "PERFECT!"
        : "PARRY!";


    int fontSize =
        effect.perfect
        ? 30
        : 24;


    int width =
        MeasureText(
            text,
            fontSize
        );


    DrawText(
        text,
        (int)effect.position.x -
        width / 2,
        (int)effect.position.y -
        50,
        fontSize,
        ColorAlpha(
            color,
            alpha
        )
    );
}


// ============================================================
// SCORE POPUP
// ============================================================

void CreateScorePopup(
    ScorePopup& popup,
    int player,
    Color color)
{
    popup.active = true;

    popup.player =
        player;

    popup.color =
        color;

    popup.duration =
        1.20f;

    popup.timer =
        popup.duration;
}


void DrawScorePopup(
    const ScorePopup& popup)
{
    if (!popup.active)
        return;


    float progress =
        1.0f -
        popup.timer /
        popup.duration;


    float y =
        160.0f -
        progress * 40.0f;


    float alpha = 1.0f;


    if (progress > 0.70f)
    {
        alpha =
            1.0f -
            (progress - 0.70f) /
            0.30f;
    }


    const char* text =
        popup.player == 1
        ? "PLAYER 1 SCORES!"
        : "PLAYER 2 SCORES!";


    int fontSize = 40;


    int width =
        MeasureText(
            text,
            fontSize
        );


    DrawText(
        text,
        SCREEN_WIDTH / 2 -
        width / 2,
        (int)y,
        fontSize,
        ColorAlpha(
            popup.color,
            alpha
        )
    );
}


// ============================================================
// OVERHEAD IMPACT FLASH
// ============================================================

void CreateImpactFlash(
    ImpactFlash& flash,
    Vector2 position)
{
    flash.active = true;
    flash.position = position;
    flash.duration = 0.14f;
    flash.timer = flash.duration;
}


void DrawImpactFlash(
    const ImpactFlash& flash)
{
    if (!flash.active)
        return;

    float progress =
        1.0f -
        flash.timer /
        flash.duration;

    float alpha =
        1.0f - progress;

    // Very brief, expanding white burst.
    float radius =
        35.0f +
        progress * 95.0f;

    DrawCircle(
        (int)flash.position.x,
        (int)flash.position.y,
        radius,
        ColorAlpha(WHITE, alpha * 0.16f)
    );

    DrawCircleLines(
        (int)flash.position.x,
        (int)flash.position.y,
        radius,
        ColorAlpha(WHITE, alpha)
    );

    for (int i = 0; i < 8; i++)
    {
        float angle =
            (float)i * (PI * 2.0f / 8.0f);

        float inner =
            30.0f + progress * 25.0f;

        float outer =
            70.0f + progress * 75.0f;

        Vector2 a =
        {
            flash.position.x + cosf(angle) * inner,
            flash.position.y + sinf(angle) * inner
        };

        Vector2 b =
        {
            flash.position.x + cosf(angle) * outer,
            flash.position.y + sinf(angle) * outer
        };

        DrawLineEx(
            a,
            b,
            4.0f,
            ColorAlpha(WHITE, alpha)
        );
    }
}


// ============================================================
// TITLE SCREEN - DRAW COLOUR
// ============================================================

void DrawColourChoice(
    int player,
    int colourIndex,
    int x,
    int y)
{
    Color colour =
        COLOURS[colourIndex].colour;


    const char* playerName =
        player == 1
        ? "P1 COLOUR"
        : "P2 COLOUR";


    DrawText(
        playerName,
        x,
        y,
        20,
        player == 1
        ? BLUE
        : RED
    );


    DrawRectangle(
        x,
        y + 30,
        70,
        35,
        colour
    );


    DrawRectangleLines(
        x,
        y + 30,
        70,
        35,
        WHITE
    );


    DrawText(
        COLOURS[colourIndex].name,
        x + 85,
        y + 35,
        20,
        colour
    );
}


// ============================================================
// TITLE SCREEN
// ============================================================

void DrawTitleScreen(
    int menuSelection,
    int scoreOption,
    int timeOption,
    int p1Colour,
    int p2Colour,
    bool soundsMuted,
    bool musicMuted)
{
    ClearBackground(
        Color{
            18,
            20,
            25,
            255
        }
    );


    const char* title =
        "KNIFE FIGHTS";


    int titleWidth =
        MeasureText(
            title,
            64
        );


    DrawText(
        title,
        SCREEN_WIDTH / 2 -
        titleWidth / 2,
        28,
        64,
        WHITE
    );


    const char* subtitle =
        "A TWO PLAYER SWORD DUEL";


    int subtitleWidth =
        MeasureText(
            subtitle,
            20
        );


    DrawText(
        subtitle,
        SCREEN_WIDTH / 2 -
        subtitleWidth / 2,
        95,
        20,
        GRAY
    );


    DrawText(
        "MATCH SETTINGS",
        60,
        145,
        25,
        WHITE
    );


    const char* settings[] =
    {
        "SCORE LIMIT",
        "TIME LIMIT",
        "START MATCH"
    };


    for (int i = 0; i < 3; i++)
    {
        Color colour =
            menuSelection == i
            ? YELLOW
            : LIGHTGRAY;


        DrawText(
            settings[i],
            70,
            200 + i * 55,
            22,
            colour
        );


        if (i == 0)
        {
            DrawText(
                TextFormat(
                    "%d",
                    SCORE_OPTIONS[
                        scoreOption
                    ]
                ),
                300,
                200 + i * 55,
                22,
                colour
            );
        }


        if (i == 1)
        {
            if (TIME_OPTIONS[
                timeOption
            ] == 0)
            {
                DrawText(
                    "OFF",
                    300,
                    255,
                    22,
                    colour
                );
            }
            else
            {
                DrawText(
                    TextFormat(
                        "%d SEC",
                        TIME_OPTIONS[
                            timeOption
                        ]
                    ),
                    300,
                    255,
                    22,
                    colour
                );
            }
        }


        if (i == 2)
        {
            DrawText(
                "> PRESS ENTER <",
                265,
                310,
                20,
                colour
            );
        }
    }


    DrawColourChoice(
        1,
        p1Colour,
        60,
        390
    );


    DrawColourChoice(
        2,
        p2Colour,
        60,
        470
    );


    DrawText(
        "P1: Q / E",
        330,
        425,
        16,
        GRAY
    );


    DrawText(
        "P2: I / O",
        330,
        505,
        16,
        GRAY
    );


    DrawRectangle(
        490,
        145,
        650,
        330,
        Color{
            25,
            28,
            35,
            255
        }
    );


    DrawRectangleLines(
        490,
        145,
        650,
        330,
        DARKGRAY
    );


    DrawText(
        "RULES",
        520,
        165,
        28,
        WHITE
    );


    DrawText(
        "THRUST",
        520,
        215,
        19,
        SKYBLUE
    );


    DrawText(
        "Attack with your sword. Matching sword levels",
        620,
        215,
        16,
        LIGHTGRAY
    );


    DrawText(
        "can intercept an incoming thrust.",
        620,
        238,
        16,
        LIGHTGRAY
    );


    DrawText(
        "PARRY",
        520,
        275,
        19,
        GOLD
    );


    DrawText(
        "Raise or lower your sword into the attacker's",
        620,
        275,
        16,
        LIGHTGRAY
    );


    DrawText(
        "blade to stun them and counterattack.",
        620,
        298,
        16,
        LIGHTGRAY
    );


    DrawText(
        "PERFECT PARRY",
        520,
        335,
        19,
        YELLOW
    );


    DrawText(
        "Change sword level at exactly the right moment",
        680,
        335,
        16,
        LIGHTGRAY
    );


    DrawText(
        "for a longer stun and bigger impact.",
        680,
        358,
        16,
        LIGHTGRAY
    );


    DrawText(
        "OVERHEAD",
        520,
        395,
        19,
        ORANGE
    );


    DrawText(
        "Hold the charge until READY, then release.",
        650,
        395,
        16,
        LIGHTGRAY
    );


    DrawText(
        "It cannot be parried, but is slow and risky.",
        650,
        418,
        16,
        LIGHTGRAY
    );


    DrawText(
        TextFormat(
            "SFX: %s",
            soundsMuted
            ? "MUTED"
            : "ON"
        ),
        510,
        500,
        18,
        soundsMuted
        ? GRAY
        : WHITE
    );


    DrawText(
        TextFormat(
            "MUSIC: %s",
            musicMuted
            ? "MUTED"
            : "ON"
        ),
        700,
        500,
        18,
        musicMuted
        ? GRAY
        : WHITE
    );


    DrawText(
        "M / N to toggle",
        890,
        500,
        15,
        GRAY
    );


    DrawText(
        "W/S SELECT     A/D CHANGE     ENTER START",
        350,
        555,
        18,
        GRAY
    );
}


// ============================================================
// DRAW GAME TIMER
// ============================================================

void DrawGameTimer(
    float gameTimer,
    int timeLimit)
{
    if (timeLimit <= 0)
        return;


    int totalSeconds =
        (int)ceilf(gameTimer);


    int minutes =
        totalSeconds / 60;


    int seconds =
        totalSeconds % 60;


    DrawText(
        TextFormat(
            "%02d:%02d",
            minutes,
            seconds
        ),
        SCREEN_WIDTH / 2 - 38,
        25,
        30,
        gameTimer <= 10
        ? RED
        : WHITE
    );
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Knife Fights"
    );


    SetTargetFPS(60);


    // ========================================================
    // AUDIO
    // ========================================================

    GameAudio audio{};

    InitGameAudio(audio);


    // ========================================================
    // GAME STATE
    // ========================================================

    bool titleScreen = true;

    bool gameOver = false;

    bool suddenDeath = false;

    int winner = 0;


    // ========================================================
    // SETTINGS
    // ========================================================

    int scoreOption = 2;

    int timeOption = 2;


    int p1Colour = 0;

    int p2Colour = 1;


    int menuSelection = 0;


    // ========================================================
    // PLAYERS
    // ========================================================

    Player p1{};

    Player p2{};


    p1.score = 0;

    p2.score = 0;


    // ========================================================
    // EFFECTS
    // ========================================================

    CombatEffect combatEffect{};

    ScorePopup scorePopup{};

    ImpactFlash impactFlash{};


    float shakeTimer = 0.0f;

    float shakeAmount = 0.0f;


    float gameTimer = 0.0f;

    // When > 0, gameplay is temporarily frozen after a hit.
    float hitStopTimer = 0.0f;
    bool roundResetPending = false;

    // Camera impact state.
    float impactZoomTimer = 0.0f;
    float impactZoomDuration = 0.0f;
    float impactZoomStrength = 1.0f;
    float impactX = SCREEN_WIDTH / 2.0f;
    float impactY = GROUND_Y - 40.0f;
    bool impactIsOverhead = false;

    // Smooth camera state.  The target changes instantly, but the
    // actual zoom accelerates toward it and then springs back to 1.0.
    float currentImpactZoom = 1.0f;
    float impactZoomVelocity = 0.0f;

    // Camera target also eases back to centre after impacts.
    Vector2 currentCameraTarget =
    {
        SCREEN_WIDTH / 2.0f,
        SCREEN_HEIGHT / 2.0f
    };

    // 0 = normal world, 0.45 = substantially darker overhead impact.
    float overheadDarkness = 0.0f;


    // ========================================================
    // GAME LOOP
    // ========================================================

    while (!WindowShouldClose())
    {
        float dt =
            GetFrameTime();


        // ====================================================
        // TITLE SCREEN
        // ====================================================

        if (titleScreen)
        {
            if (IsKeyPressed(KEY_W))
            {
                menuSelection--;

                if (menuSelection < 0)
                    menuSelection = 2;
            }


            if (IsKeyPressed(KEY_S))
            {
                menuSelection++;

                if (menuSelection > 2)
                    menuSelection = 0;
            }


            if (menuSelection == 0)
            {
                if (IsKeyPressed(KEY_A))
                {
                    scoreOption--;

                    if (scoreOption < 0)
                        scoreOption =
                        SCORE_OPTION_COUNT - 1;
                }


                if (IsKeyPressed(KEY_D))
                {
                    scoreOption++;

                    if (scoreOption >=
                        SCORE_OPTION_COUNT)
                    {
                        scoreOption = 0;
                    }
                }
            }


            if (menuSelection == 1)
            {
                if (IsKeyPressed(KEY_A))
                {
                    timeOption--;

                    if (timeOption < 0)
                        timeOption =
                        TIME_OPTION_COUNT - 1;
                }


                if (IsKeyPressed(KEY_D))
                {
                    timeOption++;

                    if (timeOption >=
                        TIME_OPTION_COUNT)
                    {
                        timeOption = 0;
                    }
                }
            }


            if (IsKeyPressed(KEY_ENTER) &&
                menuSelection == 2)
            {
                titleScreen = false;

                gameOver = false;

                suddenDeath = false;

                winner = 0;

                hitStopTimer = 0.0f;
                roundResetPending = false;

                currentImpactZoom = 1.0f;
                impactZoomVelocity = 0.0f;
                impactZoomTimer = 0.0f;
                impactZoomStrength = 1.0f;
                overheadDarkness = 0.0f;
                impactIsOverhead = false;


                p1.score = 0;

                p2.score = 0;


                p1.bodyColor =
                    COLOURS[
                        p1Colour
                    ].colour;

                p1.swordColor =
                    COLOURS[
                        p1Colour
                    ].colour;


                p2.bodyColor =
                    COLOURS[
                        p2Colour
                    ].colour;

                p2.swordColor =
                    COLOURS[
                        p2Colour
                    ].colour;


                ResetRound(
                    p1,
                    p2
                );


                if (TIME_OPTIONS[
                    timeOption
                ] > 0)
                {
                    gameTimer =
                        (float)
                        TIME_OPTIONS[
                            timeOption
                        ];
                }
                else
                {
                    gameTimer = 0;
                }
            }


            if (IsKeyPressed(KEY_Q))
            {
                p1Colour--;

                if (p1Colour < 0)
                    p1Colour =
                    COLOUR_COUNT - 1;
            }


            if (IsKeyPressed(KEY_E))
            {
                p1Colour++;

                if (p1Colour >=
                    COLOUR_COUNT)
                {
                    p1Colour = 0;
                }
            }


            if (IsKeyPressed(KEY_I))
            {
                p2Colour--;

                if (p2Colour < 0)
                    p2Colour =
                    COLOUR_COUNT - 1;
            }


            if (IsKeyPressed(KEY_O))
            {
                p2Colour++;

                if (p2Colour >=
                    COLOUR_COUNT)
                {
                    p2Colour = 0;
                }
            }


            if (IsKeyPressed(KEY_M))
            {
                audio.soundsMuted =
                    !audio.soundsMuted;
            }


            if (IsKeyPressed(KEY_N))
            {
                audio.musicMuted =
                    !audio.musicMuted;
            }


            UpdateMusic(audio);


            BeginDrawing();


            DrawTitleScreen(
                menuSelection,
                scoreOption,
                timeOption,
                p1Colour,
                p2Colour,
                audio.soundsMuted,
                audio.musicMuted
            );


            EndDrawing();


            continue;
        }


        // ====================================================
        // GAME
        // ====================================================

        if (!gameOver)
        {
            // ------------------------------------------------
            // HIT STOP
            // ------------------------------------------------
            //
            // Audio, screen effects and drawing continue while
            // the actual game state is briefly frozen.
            //

            if (hitStopTimer > 0)
            {
                hitStopTimer -= dt;

                if (hitStopTimer < 0)
                    hitStopTimer = 0;
            }
            else
            {
                if (roundResetPending)
                {
                    ResetRound(
                        p1,
                        p2
                    );

                    roundResetPending = false;
                }

                // ------------------------------------------------
                // MUSIC DURING GAMEPLAY
                // ------------------------------------------------

                UpdateMusic(audio);


                UpdatePlayer(p1);

                UpdatePlayer(p2);


                // ------------------------------------------------
                // TIME LIMIT
                // ------------------------------------------------

                if (TIME_OPTIONS[
                    timeOption
                ] > 0 &&
                    !suddenDeath)
                {
                    gameTimer -= dt;


                    if (gameTimer <= 0)
                    {
                        gameTimer = 0;


                        if (p1.score >
                            p2.score)
                        {
                            gameOver = true;

                            winner = 1;
                        }
                        else if (
                            p2.score >
                            p1.score)
                        {
                            gameOver = true;

                            winner = 2;
                        }
                        else
                        {
                            suddenDeath = true;
                        }
                    }
                }


                // ------------------------------------------------
                // MOVEMENT
                // ------------------------------------------------

                if (p1.stunTimer <= 0 &&
                    !p1.overhead)
                {
                    float speed =
                        GetMovementSpeed(p1);


                    if (IsKeyDown(KEY_A))
                        p1.x -= speed;


                    if (IsKeyDown(KEY_D))
                        p1.x += speed;
                }


                if (p2.stunTimer <= 0 &&
                    !p2.overhead)
                {
                    float speed =
                        GetMovementSpeed(p2);


                    if (IsKeyDown(KEY_LEFT))
                        p2.x -= speed;


                    if (IsKeyDown(KEY_RIGHT))
                        p2.x += speed;
                }


                // ------------------------------------------------
                // REGULAR THRUST FORWARD MOVEMENT
                // ------------------------------------------------
                //
                // The player physically inches toward the opponent
                // during the jab. This makes attacking a positioning
                // tool, but also means a whiff puts you closer to
                // danger.
                //

                if (p1.thrusting)
                {
                    float progress =
                        1.0f -
                        p1.thrustTimer /
                        THRUST_DURATION;


                    progress =
                        ClampFloat(
                            progress,
                            0.0f,
                            1.0f
                        );


                    p1.x =
                        p1.thrustStartX +
                        THRUST_STEP_DISTANCE *
                        progress;
                }


                if (p2.thrusting)
                {
                    float progress =
                        1.0f -
                        p2.thrustTimer /
                        THRUST_DURATION;


                    progress =
                        ClampFloat(
                            progress,
                            0.0f,
                            1.0f
                        );


                    p2.x =
                        p2.thrustStartX -
                        THRUST_STEP_DISTANCE *
                        progress;
                }


                // ------------------------------------------------
                // SWORD LEVEL
                // ------------------------------------------------

                if (p1.stunTimer <= 0 &&
                    !p1.charging &&
                    !p1.exhausted &&
                    !p1.overhead &&
                    p1.recoveryTimer <= 0 &&
                    p1.thrustRecoveryTimer <= 0)
                {
                    if (IsKeyPressed(KEY_W))
                    {
                        ChangeSwordLevel(
                            p1,
                            p1.swordLevel + 1
                        );
                    }


                    if (IsKeyPressed(KEY_S))
                    {
                        ChangeSwordLevel(
                            p1,
                            p1.swordLevel - 1
                        );
                    }
                }


                if (p2.stunTimer <= 0 &&
                    !p2.charging &&
                    !p2.exhausted &&
                    !p2.overhead &&
                    p2.recoveryTimer <= 0 &&
                    p2.thrustRecoveryTimer <= 0)
                {
                    if (IsKeyPressed(KEY_UP))
                    {
                        ChangeSwordLevel(
                            p2,
                            p2.swordLevel + 1
                        );
                    }


                    if (IsKeyPressed(KEY_DOWN))
                    {
                        ChangeSwordLevel(
                            p2,
                            p2.swordLevel - 1
                        );
                    }
                }


                // ------------------------------------------------
                // THRUST
                // ------------------------------------------------

                if (IsKeyPressed(KEY_SPACE))
                {
                    StartThrust(p1);

                    if (p1.thrusting)
                    {
                        PlayGameSound(
                            audio,
                            audio.thrust
                        );
                    }
                }


                if (IsKeyPressed(KEY_RIGHT_SHIFT))
                {
                    StartThrust(p2);

                    if (p2.thrusting)
                    {
                        PlayGameSound(
                            audio,
                            audio.thrust
                        );
                    }
                }


                // ------------------------------------------------
                // CHARGE
                // ------------------------------------------------

                if (IsKeyPressed(KEY_LEFT_SHIFT))
                {
                    StartCharge(p1);
                }


                if (IsKeyReleased(KEY_LEFT_SHIFT))
                {
                    ReleaseCharge(
                        p1,
                        audio
                    );
                }


                if (IsKeyPressed(KEY_ENTER))
                {
                    StartCharge(p2);
                }


                if (IsKeyReleased(KEY_ENTER))
                {
                    ReleaseCharge(
                        p2,
                        audio
                    );
                }


                // ------------------------------------------------
                // OVERHEAD LEAP
                // ------------------------------------------------

                if (p1.overhead)
                {
                    float dashSpeed =
                        OVERHEAD_DASH_DISTANCE /
                        OVERHEAD_DASH_TIME;

                    float nextX =
                        p1.x +
                        dashSpeed * dt;

                    float stopX =
                        p2.x - PLAYER_WIDTH;

                    if (nextX > stopX)
                        nextX = stopX;

                    if (nextX > p1.x)
                        p1.x = nextX;
                }


                if (p2.overhead)
                {
                    float dashSpeed =
                        OVERHEAD_DASH_DISTANCE /
                        OVERHEAD_DASH_TIME;

                    float nextX =
                        p2.x -
                        dashSpeed * dt;

                    float stopX =
                        p1.x + PLAYER_WIDTH;

                    if (nextX < stopX)
                        nextX = stopX;

                    if (nextX < p2.x)
                        p2.x = nextX;
                }


                // ------------------------------------------------
                // BOUNDS
                // ------------------------------------------------

                p1.x =
                    ClampFloat(
                        p1.x,
                        30.0f,
                        SCREEN_WIDTH - 30.0f
                    );


                p2.x =
                    ClampFloat(
                        p2.x,
                        30.0f,
                        SCREEN_WIDTH - 30.0f
                    );


                // ------------------------------------------------
                // PLAYER COLLISION
                // ------------------------------------------------

                if (!p1.overhead &&
                    !p2.overhead)
                {
                    float distance =
                        p2.x - p1.x;


                    if (distance < 30.0f)
                    {
                        float midpoint =
                            (p1.x + p2.x) /
                            2.0f;


                        p1.x =
                            midpoint - 15.0f;


                        p2.x =
                            midpoint + 15.0f;
                    }
                }


                // ------------------------------------------------
                // P1 THRUST
                // ------------------------------------------------

                if (p1.thrusting)
                {
                    int parry =
                        CheckParry(
                            p1,
                            p2
                        );


                    if (parry > 0)
                    {
                        p1.thrusting = false;

                        p1.thrustTimer = 0;


                        bool perfect =
                            parry == 2;


                        p1.stunTimer =
                            perfect
                            ? PERFECT_PARRY_DURATION
                            : STUN_DURATION;


                        CreateCombatEffect(
                            combatEffect,
                            {
                                (p1.x + p2.x) /
                                    2.0f,

                                GetSwordLevelY(p1)
                            },
                            perfect
                        );


                        shakeTimer =
                            perfect
                            ? 0.32f
                            : 0.22f;


                        shakeAmount =
                            perfect
                            ? 30.0f
                            : 18.0f;


                        PlayGameSound(
                            audio,
                            perfect
                            ? audio.perfectParry
                            : audio.parry
                        );
                    }
                    else
                    {
                        Sword sword =
                            GetNormalSword(p1);


                        if (SwordHitsPlayer(
                            sword,
                            p2))
                        {
                            p1.score++;


                            CreateScorePopup(
                                scorePopup,
                                1,
                                p1.bodyColor
                            );


                            shakeTimer = 0.12f;

                            shakeAmount = 8.0f;


                            PlayGameSound(
                                audio,
                                audio.hit
                            );

                            hitStopTimer = HIT_STOP_NORMAL;
                            impactZoomStrength = IMPACT_ZOOM_NORMAL;
                            impactZoomDuration = IMPACT_ZOOM_HOLD_NORMAL;
                            impactZoomTimer = impactZoomDuration;
                            impactX = (p1.x + p2.x) * 0.5f;
                            impactY = GetSwordLevelY(p1);
                            impactIsOverhead = false;


                            if (suddenDeath)
                            {
                                gameOver = true;
                                winner = 1;
                            }
                            else if (p1.score >=
                                SCORE_OPTIONS[
                                    scoreOption
                                ])
                            {
                                gameOver = true;

                                winner = 1;
                            }
                            else
                            {
                                roundResetPending = true;
                            }
                        }
                    }
                }


                // ------------------------------------------------
                // P2 THRUST
                // ------------------------------------------------

                if (!gameOver &&
                    p2.thrusting)
                {
                    int parry =
                        CheckParry(
                            p2,
                            p1
                        );


                    if (parry > 0)
                    {
                        p2.thrusting = false;

                        p2.thrustTimer = 0;


                        bool perfect =
                            parry == 2;


                        p2.stunTimer =
                            perfect
                            ? PERFECT_PARRY_DURATION
                            : STUN_DURATION;


                        CreateCombatEffect(
                            combatEffect,
                            {
                                (p1.x + p2.x) /
                                    2.0f,

                                GetSwordLevelY(p2)
                            },
                            perfect
                        );


                        shakeTimer =
                            perfect
                            ? 0.32f
                            : 0.22f;


                        shakeAmount =
                            perfect
                            ? 30.0f
                            : 18.0f;


                        PlayGameSound(
                            audio,
                            perfect
                            ? audio.perfectParry
                            : audio.parry
                        );
                    }
                    else
                    {
                        Sword sword =
                            GetNormalSword(p2);


                        if (SwordHitsPlayer(
                            sword,
                            p1))
                        {
                            p2.score++;


                            CreateScorePopup(
                                scorePopup,
                                2,
                                p2.bodyColor
                            );


                            shakeTimer = 0.12f;

                            shakeAmount = 8.0f;


                            PlayGameSound(
                                audio,
                                audio.hit
                            );

                            hitStopTimer = HIT_STOP_NORMAL;
                            impactZoomStrength = IMPACT_ZOOM_NORMAL;
                            impactZoomDuration = IMPACT_ZOOM_HOLD_NORMAL;
                            impactZoomTimer = impactZoomDuration;
                            impactX = (p1.x + p2.x) * 0.5f;
                            impactY = GetSwordLevelY(p1);
                            impactIsOverhead = false;


                            if (suddenDeath)
                            {
                                // In sudden death, the first point wins.
                                gameOver = true;
                                winner = 2;
                            }
                            else if (p2.score >=
                                SCORE_OPTIONS[
                                    scoreOption
                                ])
                            {
                                gameOver = true;

                                winner = 2;
                            }
                            else
                            {
                                roundResetPending = true;
                            }
                        }
                    }
                }


                // ------------------------------------------------
                // P1 OVERHEAD
                // ------------------------------------------------

                if (!gameOver &&
                    p1.overhead &&
                    !p1.overheadHit)
                {


                    if (OverheadHitsPlayer(
                        p1,
                        p2))
                    {
                        p1.overheadHit = true;

                        p1.score++;


                        CreateScorePopup(
                            scorePopup,
                            1,
                            p1.bodyColor
                        );

                        CreateImpactFlash(
                            impactFlash,
                            {
                                (p1.x + p2.x) / 2.0f,
                                p1.y - 45.0f
                            }
                        );


                        shakeTimer = 0.30f;

                        shakeAmount = 24.0f;


                        PlayGameSound(
                            audio,
                            audio.hit
                        );

                        hitStopTimer = HIT_STOP_OVERHEAD;
                        impactZoomStrength = IMPACT_ZOOM_OVERHEAD;
                        impactZoomDuration = IMPACT_ZOOM_HOLD_OVERHEAD;
                        impactZoomTimer = impactZoomDuration;
                        impactX = (p1.x + p2.x) * 0.5f;
                        impactY = (p1.y + p2.y) * 0.5f - 35.0f;
                        impactIsOverhead = true;


                        if (suddenDeath)
                        {
                            gameOver = true;
                            winner = 1;
                        }
                        else if (p1.score >=
                            SCORE_OPTIONS[
                                scoreOption
                            ])
                        {
                            gameOver = true;

                            winner = 1;
                        }
                        else
                        {
                            roundResetPending = true;
                        }
                    }
                }


                // ------------------------------------------------
                // P2 OVERHEAD
                // ------------------------------------------------

                if (!gameOver &&
                    p2.overhead &&
                    !p2.overheadHit)
                {


                    if (OverheadHitsPlayer(
                        p2,
                        p1))
                    {
                        p2.overheadHit = true;

                        p2.score++;


                        CreateScorePopup(
                            scorePopup,
                            2,
                            p2.bodyColor
                        );

                        CreateImpactFlash(
                            impactFlash,
                            {
                                (p1.x + p2.x) / 2.0f,
                                p2.y - 45.0f
                            }
                        );


                        shakeTimer = 0.30f;

                        shakeAmount = 24.0f;


                        PlayGameSound(
                            audio,
                            audio.hit
                        );

                        hitStopTimer = HIT_STOP_OVERHEAD;
                        impactZoomStrength = IMPACT_ZOOM_OVERHEAD;
                        impactZoomDuration = IMPACT_ZOOM_HOLD_OVERHEAD;
                        impactZoomTimer = impactZoomDuration;
                        impactX = (p1.x + p2.x) * 0.5f;
                        impactY = (p1.y + p2.y) * 0.5f - 35.0f;
                        impactIsOverhead = true;


                        if (suddenDeath)
                        {
                            gameOver = true;
                            winner = 2;
                        }
                        else if (p2.score >=
                            SCORE_OPTIONS[
                                scoreOption
                            ])
                        {
                            gameOver = true;

                            winner = 2;
                        }
                        else
                        {
                            roundResetPending = true;
                        }
                    }
                }
            }
        }


        // ====================================================
        // EFFECT TIMERS
        // ====================================================

        if (shakeTimer > 0)
            shakeTimer -= dt;

        if (impactZoomTimer > 0)
        {
            impactZoomTimer -= dt;
            if (impactZoomTimer < 0)
                impactZoomTimer = 0;
        }

        float targetZoom =
            (impactZoomTimer > 0)
            ? impactZoomStrength
            : 1.0f;

        const float zoomAcceleration = 55.0f;
        const float zoomDamping = 14.0f;

        impactZoomVelocity +=
            (targetZoom - currentImpactZoom) *
            zoomAcceleration * dt;

        impactZoomVelocity -=
            impactZoomVelocity *
            zoomDamping * dt;

        currentImpactZoom +=
            impactZoomVelocity * dt;

        if (fabsf(currentImpactZoom - 1.0f) < 0.0005f &&
            impactZoomTimer <= 0 &&
            fabsf(impactZoomVelocity) < 0.01f)
        {
            currentImpactZoom = 1.0f;
            impactZoomVelocity = 0.0f;
        }

        Vector2 desiredTarget =
            (impactZoomTimer > 0)
            ? Vector2{ impactX, impactY }
            : Vector2{
                SCREEN_WIDTH / 2.0f,
                SCREEN_HEIGHT / 2.0f
        };

        currentCameraTarget.x +=
            (desiredTarget.x - currentCameraTarget.x) *
            ClampFloat(10.0f * dt, 0.0f, 1.0f);

        currentCameraTarget.y +=
            (desiredTarget.y - currentCameraTarget.y) *
            ClampFloat(10.0f * dt, 0.0f, 1.0f);

        float targetDarkness =
            (impactIsOverhead && impactZoomTimer > 0)
            ? 0.45f
            : 0.0f;

        overheadDarkness +=
            (targetDarkness - overheadDarkness) *
            ClampFloat(12.0f * dt, 0.0f, 1.0f);

        if (impactZoomTimer <= 0)
            impactIsOverhead = false;


        if (combatEffect.active)
        {
            combatEffect.timer -= dt;

            if (combatEffect.timer <= 0)
                combatEffect.active = false;
        }


        if (scorePopup.active)
        {
            scorePopup.timer -= dt;

            if (scorePopup.timer <= 0)
                scorePopup.active = false;
        }


        if (impactFlash.active)
        {
            impactFlash.timer -= dt;

            if (impactFlash.timer <= 0)
                impactFlash.active = false;
        }


        // ====================================================
        // GAME OVER
        // ====================================================

        if (gameOver)
        {
            if (IsKeyPressed(KEY_BACKSPACE))
            {
                titleScreen = true;

                gameOver = false;

                suddenDeath = false;

                winner = 0;

                p1.score = 0;

                p2.score = 0;

                
                audio.musicTimer = 0.0f;
                audio.musicNote = 0;
            }
        }


        // ====================================================
        // DRAW
        // ====================================================

        BeginDrawing();


        float shakeX = 0;

        float shakeY = 0;


        if (shakeTimer > 0)
        {
            shakeX =
                (float)GetRandomValue(
                    -(int)shakeAmount,
                    (int)shakeAmount
                );


            shakeY =
                (float)GetRandomValue(
                    -(int)shakeAmount,
                    (int)shakeAmount
                );
        }


        unsigned char skyR =
            (unsigned char)(220.0f * (1.0f - overheadDarkness));

        unsigned char skyG =
            (unsigned char)(225.0f * (1.0f - overheadDarkness));

        unsigned char skyB =
            (unsigned char)(220.0f * (1.0f - overheadDarkness));

        ClearBackground(
            Color{
                skyR,
                skyG,
                skyB,
                255
            }
        );


        // ====================================================
        // CAMERA / IMPACT ZOOM
        // ====================================================

        Camera2D impactCamera{};
        impactCamera.target = currentCameraTarget;
        impactCamera.offset = {
            SCREEN_WIDTH / 2.0f + shakeX,
            SCREEN_HEIGHT / 2.0f + shakeY
        };
        impactCamera.rotation = 0.0f;
        impactCamera.zoom = currentImpactZoom;

        BeginMode2D(impactCamera);

        // ====================================================
        // TERRAIN
        // ====================================================

        Color impactTerrainColor =
        {
            (unsigned char)(DARKGREEN.r* (1.0f - overheadDarkness)),
            (unsigned char)(DARKGREEN.g* (1.0f - overheadDarkness)),
            (unsigned char)(DARKGREEN.b* (1.0f - overheadDarkness)),
            255
        };

        const int TERRAIN_PADDING = 2000;

        DrawRectangle(
            -TERRAIN_PADDING,
            (int)GROUND_Y,
            SCREEN_WIDTH + TERRAIN_PADDING * 2,
            SCREEN_HEIGHT + TERRAIN_PADDING -
            (int)GROUND_Y,
            impactTerrainColor
        );


        DrawLine(
            -TERRAIN_PADDING,
            (int)GROUND_Y,
            SCREEN_WIDTH + TERRAIN_PADDING,
            (int)(GROUND_Y + shakeY),
            BLACK
        );


        // ====================================================
        // PARRY VISUALS
        // ====================================================

        DrawParryZone(
            p1,
            p2
        );


        DrawParryZone(
            p2,
            p1
        );


        // ====================================================
        // PLAYERS
        // ====================================================

        DrawPlayer(p1);

        DrawPlayer(p2);


        EndMode2D();

        // ====================================================
        // SCORE
        // ====================================================

        DrawText(
            TextFormat(
                "P1  %d",
                p1.score
            ),
            30,
            25,
            30,
            p1.bodyColor
        );


        DrawText(
            TextFormat(
                "P2  %d",
                p2.score
            ),
            SCREEN_WIDTH - 100,
            25,
            30,
            p2.bodyColor
        );


        // ====================================================
        // TIME
        // ====================================================

        if (TIME_OPTIONS[
            timeOption
        ] > 0)
        {
            if (suddenDeath)
            {
                DrawText(
                    "SUDDEN DEATH",
                    SCREEN_WIDTH / 2 - 100,
                    25,
                    25,
                    RED
                );
            }
            else
            {
                DrawGameTimer(
                    gameTimer,
                    TIME_OPTIONS[
                        timeOption
                    ]
                );
            }
        }


        // ====================================================
        // EFFECTS
        // ====================================================

        DrawCombatEffect(
            combatEffect
        );


        DrawScorePopup(
            scorePopup
        );


        DrawImpactFlash(
            impactFlash
        );


        // ====================================================
        // CONTROLS
        // ====================================================

        DrawText(
            "P1: A/D Move   W/S Sword   SPACE Thrust   LSHIFT Overhead",
            20,
            550,
            16,
            DARKGRAY
        );


        DrawText(
            "P2: Arrows Move   UP/DOWN Sword   RSHIFT Thrust   ENTER Overhead",
            650,
            550,
            14,
            DARKGRAY
        );


        // ====================================================
        // GAME OVER
        // ====================================================

        if (gameOver)
        {
            DrawRectangle(
                0,
                0,
                SCREEN_WIDTH,
                SCREEN_HEIGHT,
                ColorAlpha(
                    BLACK,
                    0.70f
                )
            );


            const char* winnerText =
                winner == 1
                ? "PLAYER 1 WINS!"
                : "PLAYER 2 WINS!";


            Color winnerColor =
                winner == 1
                ? p1.bodyColor
                : p2.bodyColor;


            int width =
                MeasureText(
                    winnerText,
                    55
                );


            DrawText(
                winnerText,
                SCREEN_WIDTH / 2 -
                width / 2,
                180,
                55,
                winnerColor
            );


            DrawText(
                TextFormat(
                    "%d  -  %d",
                    p1.score,
                    p2.score
                ),
                SCREEN_WIDTH / 2 - 50,
                250,
                35,
                WHITE
            );


            DrawText(
                "BACKSPACE: RETURN TO MENU",
                SCREEN_WIDTH / 2 - 150,
                320,
                20,
                LIGHTGRAY
            );
        }


        EndDrawing();
    }


    // ========================================================
    // CLEANUP
    // ========================================================

    UnloadGameAudio(audio);

    CloseWindow();

    return 0;
}