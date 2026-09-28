// aim.h
// Human-motor aiming model for HL1 bots - interface.
//
// This header intentionally does NOT include extdll.h/util.h/cbase.h/
// player.h. Every type it touches is forward-declared and used only by
// pointer/reference, so any file can #include "aim.h" without inheriting
// the SDK's include-order requirements. The implementation (which DOES need
// those headers) lives in aim.cpp - see that file for how the aiming model
// actually works and what was dropped converting it from the TF2 aim.inc.
//
// Add aim.cpp to the project's source list; this header alone is not enough
// to link.

#ifndef AIM_H
#define AIM_H

class Vector;
class CBaseEntity;
class CBasePlayer;

/// PERCEIVE + PREDICT only: the world point this bot currently believes it
/// should aim at, without moving the view. Useful on its own for callers
/// that want the point itself - e.g. a visibility trace before deciding
/// whether to shoot at all.
void Bot_ComputeAimPoint(CBasePlayer* target, bool headAim, Vector* outAimPoint);

/// Track a live enemy: perceive + predict + turn. This is the one that can
/// be juked - see aim.cpp's file header for why that's a feature, not a bug.
///   headAim        - true for a headshot line, false for center mass.
///   responsiveness - per-call turn responsiveness; leave at the default
///                    for normal human tracking, or pass >= 0.99f for an
///                    instant, noise-free snap.
void Bot_AimAtEnemy(CBaseEntity* self, CBasePlayer* target, bool headAim = false, float responsiveness = 0.05f);

/// Turn toward a fixed point with the same human hand dynamics, but with no
/// perceive/predict stage - there's nothing to juke a waypoint or a health
/// kit. Use this for anything that isn't tracking a living target.
void Bot_AimAtPoint(CBaseEntity* self, const Vector& point, float responsiveness = 0.05f);

#endif // AIM_H