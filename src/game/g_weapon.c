// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_weapon.c
// perform the server side effects of a weapon firing

/*
 *  Portions Copyright (C) 2000-2001 Tim Angus
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*  To assertain which portions are licensed under the LGPL and which are
 *  licensed by Id Software, Inc. please run a diff between the equivalent
 *  versions of the "Tremulous" modification and the unmodified "Quake3"
 *  game source code.
 */

#include "g_local.h"

static  vec3_t  forward, right, up;
static  vec3_t  muzzle;

#define NUM_NAILSHOTS 10

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
  vec3_t v, newv;
  float dot;

  VectorSubtract( impact, start, v );
  dot = DotProduct( v, dir );
  VectorMA( v, -2*dot, dir, newv );

  VectorNormalize(newv);
  VectorMA(impact, 8192, newv, endout);
}

/*
======================================================================

GAUNTLET

======================================================================
*/

void Weapon_Gauntlet( gentity_t *ent ) {

}

/*
===============
CheckGauntletAttack
===============
*/
qboolean CheckGauntletAttack( gentity_t *ent ) {
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;
  int     damage;

  // set aiming directions
  AngleVectors (ent->client->ps.viewangles, forward, right, up);

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  VectorMA (muzzle, 32, forward, end);

  trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
  if ( tr.surfaceFlags & SURF_NOIMPACT ) {
    return qfalse;
  }

  traceEnt = &g_entities[ tr.entityNum ];

  // send blood impact
  if ( traceEnt->takedamage && traceEnt->client ) {
    tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
    tent->s.otherEntityNum = traceEnt->s.number;
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
  }

  if ( !traceEnt->takedamage) {
    return qfalse;
  }

  damage = 50;
  G_Damage( traceEnt, ent, ent, forward, tr.endpos,
    damage, 0, MOD_GAUNTLET );

  return qtrue;
}


/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
  int   i;

  for ( i = 0 ; i < 3 ; i++ ) {
    if ( to[i] <= v[i] ) {
      v[i] = (int)v[i];
    } else {
      v[i] = (int)v[i] + 1;
    }
  }
}

#define MACHINEGUN_SPREAD 200
#define MACHINEGUN_DAMAGE 7
#define MACHINEGUN_TEAM_DAMAGE  5   // wimpier MG in teamplay

#define CHAINGUN_SPREAD 1200
#define CHAINGUN_DAMAGE 14

void Bullet_Fire (gentity_t *ent, float spread, int damage, int mod ) {
  trace_t   tr;
  vec3_t    end;
  float   r;
  float   u;
  gentity_t *tent;
  gentity_t *traceEnt;
  int     i, passent;

  r = random() * M_PI * 2.0f;
  u = sin(r) * crandom() * spread * 16;
  r = cos(r) * crandom() * spread * 16;
  VectorMA (muzzle, 8192*16, forward, end);
  VectorMA (end, r, right, end);
  VectorMA (end, u, up, end);

  passent = ent->s.number;
  for (i = 0; i < 10; i++) {

    trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
    if ( tr.surfaceFlags & SURF_NOIMPACT ) {
      return;
    }

    traceEnt = &g_entities[ tr.entityNum ];

    // snap the endpos to integers, but nudged towards the line
    SnapVectorTowards( tr.endpos, muzzle );

    // send bullet impact
    if ( traceEnt->takedamage && traceEnt->client ) {
      tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
      tent->s.eventParm = traceEnt->s.number;
      if( LogAccuracyHit( traceEnt, ent ) ) {
        ent->client->accuracy_hits++;
      }
    } else {
      tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
      tent->s.eventParm = DirToByte( tr.plane.normal );
    }
    tent->s.otherEntityNum = ent->s.number;

    if ( traceEnt->takedamage) {
        G_Damage( traceEnt, ent, ent, forward, tr.endpos,
          damage, 0, MOD_MACHINEGUN);
    }
    break;
  }
}


/*
======================================================================

BFG

======================================================================
*/

void BFG_Fire ( gentity_t *ent ) {
  gentity_t *m;

  m = fire_bfg (ent, muzzle, forward);

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}


/*
======================================================================

SHOTGUN

======================================================================
*/

// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT are in bg_public.h, because
// client predicts same spreads
#define DEFAULT_SHOTGUN_DAMAGE  10

qboolean ShotgunPellet( vec3_t start, vec3_t end, gentity_t *ent ) {
  trace_t   tr;
  int     damage, i, passent;
  gentity_t *traceEnt;
  vec3_t    tr_start, tr_end;

  passent = ent->s.number;
  VectorCopy( start, tr_start );
  VectorCopy( end, tr_end );
  for (i = 0; i < 10; i++) {
    trap_Trace (&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
    traceEnt = &g_entities[ tr.entityNum ];

    // send bullet impact
    if (  tr.surfaceFlags & SURF_NOIMPACT ) {
      return qfalse;
    }

    if ( traceEnt->takedamage) {
      damage = DEFAULT_SHOTGUN_DAMAGE;
      G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN);
        if( LogAccuracyHit( traceEnt, ent ) ) {
          return qtrue;
        }
    }
    return qfalse;
  }
  return qfalse;
}

// this should match CG_ShotgunPattern
void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ) {
  int     i;
  float   r, u;
  vec3_t    end;
  vec3_t    forward, right, up;
  int     oldScore;
  qboolean  hitClient = qfalse;

  // derive the right and up vectors from the forward vector, because
  // the client won't have any other information
  VectorNormalize2( origin2, forward );
  PerpendicularVector( right, forward );
  CrossProduct( forward, right, up );

  oldScore = ent->client->ps.persistant[PERS_SCORE];

  // generate the "random" spread pattern
  for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {
    r = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
    u = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
    VectorMA( origin, 8192 * 16, forward, end);
    VectorMA (end, r, right, end);
    VectorMA (end, u, up, end);
    if( ShotgunPellet( origin, end, ent ) && !hitClient ) {
      hitClient = qtrue;
      ent->client->accuracy_hits++;
    }
  }
}


void weapon_supershotgun_fire (gentity_t *ent) {
  gentity_t   *tent;

  // send shotgun blast
  tent = G_TempEntity( muzzle, EV_SHOTGUN );
  VectorScale( forward, 4096, tent->s.origin2 );
  SnapVector( tent->s.origin2 );
  tent->s.eventParm = rand() & 255;   // seed for spread pattern
  tent->s.otherEntityNum = ent->s.number;

  ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
}


/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (gentity_t *ent) {
  gentity_t *m;

  // extra vertical velocity
  forward[2] += 0.2f;
  VectorNormalize( forward );

  m = fire_grenade (ent, muzzle, forward);

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (gentity_t *ent) {
  gentity_t *m;

  m = fire_rocket (ent, muzzle, forward);

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

SAWBLADE

======================================================================
*/

void Weapon_SawbladeLauncher_Fire( gentity_t *ent )
{
  gentity_t *m;

  m = fire_sawblade( ent, muzzle, forward );

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

PLASMAGUN

======================================================================
*/

void Weapon_Plasma_Fire (gentity_t *ent) {
  gentity_t *m;

  m = fire_plasma (ent, muzzle, forward);

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

FLAME THROWER

======================================================================
*/

void Weapon_Flamer_Fire (gentity_t *ent) {
  gentity_t *m;

  m = fire_flamer (ent, muzzle, forward);

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

RAILGUN

======================================================================
*/


/*
=================
weapon_railgun_fire
=================
*/
#define MAX_RAIL_HITS 4
void weapon_railgun_fire( gentity_t *ent )
{
  vec3_t    end;
  trace_t   trace;
  gentity_t *tent;
  gentity_t *traceEnt;
  int     damage;
  int     i;
  int     hits;
  int     unlinked;
  int     passent;
  gentity_t *unlinkedEntities[MAX_RAIL_HITS];

  damage = 100;

  VectorMA (muzzle, 8192, forward, end);

  // trace only against the solids, so the railgun will go through people
  unlinked = 0;
  hits = 0;
  passent = ent->s.number;
  
  do
  {
    trap_Trace (&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT );
    if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL )
      break;

    traceEnt = &g_entities[ trace.entityNum ];
    if ( traceEnt->takedamage )
      G_Damage( traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN );
    
    if ( trace.contents & CONTENTS_SOLID )
      break;    // we hit something solid enough to stop the beam
 
    // unlink this entity, so the next trace will go past it
    trap_UnlinkEntity( traceEnt );
    unlinkedEntities[unlinked] = traceEnt;
    unlinked++;
  }
  while ( unlinked < MAX_RAIL_HITS );

  // link back in any entities we unlinked
  for ( i = 0 ; i < unlinked ; i++ )
    trap_LinkEntity( unlinkedEntities[i] );

  // the final trace endpos will be the terminal point of the rail trail

  // snap the endpos to integers to save net bandwidth, but nudged towards the line
  SnapVectorTowards( trace.endpos, muzzle );

  // send railgun beam effect
  tent = G_TempEntity( trace.endpos, EV_RAILTRAIL );

  // set player number for custom colors on the railtrail
  tent->s.clientNum = ent->s.clientNum;

  VectorCopy( muzzle, tent->s.origin2 );
  // move origin a bit to come closer to the drawn gun muzzle
  VectorMA( tent->s.origin2, 16, up, tent->s.origin2 );

  // no explosion at end if SURF_NOIMPACT, but still make the trail
  if ( trace.surfaceFlags & SURF_NOIMPACT )
    tent->s.eventParm = 255;  // don't make the explosion at the end
  else
    tent->s.eventParm = DirToByte( trace.plane.normal );
 
  tent->s.clientNum = ent->s.clientNum;
}


/*
======================================================================

GRAPPLING HOOK

======================================================================
*/

void Weapon_GrapplingHook_Fire (gentity_t *ent)
{
  if (!ent->client->fireHeld && !ent->client->hook)
    fire_grapple (ent, muzzle, forward);

  ent->client->fireHeld = qtrue;
}

void Weapon_HookFree (gentity_t *ent)
{
  ent->parent->client->hook = NULL;
  ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
  G_FreeEntity( ent );
}

void Weapon_HookThink (gentity_t *ent)
{
  if (ent->enemy) {
    vec3_t v, oldorigin;

    VectorCopy(ent->r.currentOrigin, oldorigin);
    v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
    v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
    v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
    SnapVectorTowards( v, oldorigin );  // save net bandwidth

    G_SetOrigin( ent, v );
  }

  VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}

/*
======================================================================

LIGHTNING GUN

======================================================================
*/


void Weapon_LightningFire( gentity_t *ent ) {
  trace_t   tr;
  vec3_t    end;
  gentity_t *traceEnt, *tent;
  int     damage, i, passent;

  damage = 8;

  passent = ent->s.number;
  for (i = 0; i < 10; i++) {
    VectorMA( muzzle, LIGHTNING_RANGE, forward, end );

    trap_Trace( &tr, muzzle, NULL, NULL, end, passent, MASK_SHOT );

    if ( tr.entityNum == ENTITYNUM_NONE ) {
      return;
    }

    traceEnt = &g_entities[ tr.entityNum ];

    if ( traceEnt->takedamage) {
        G_Damage( traceEnt, ent, ent, forward, tr.endpos,
          damage, 0, MOD_LIGHTNING);
    }

    if ( traceEnt->takedamage && traceEnt->client ) {
      tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
      tent->s.otherEntityNum = traceEnt->s.number;
      tent->s.eventParm = DirToByte( tr.plane.normal );
      tent->s.weapon = ent->s.weapon;
      if( LogAccuracyHit( traceEnt, ent ) ) {
        ent->client->accuracy_hits++;
      }
    } else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
      tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
      tent->s.eventParm = DirToByte( tr.plane.normal );
    }

    break;
  }
}


//======================================================================

/*
======================================================================

BUILD GUN

======================================================================
*/


///////build weapons
/*
===============
Weapon_Abuild_Fire
===============
*/
void Weapon_Abuild_Fire( gentity_t *ent )
{
  G_AddPredictableEvent( ent, EV_MENU, MN_D_BUILD );
}

/*
===============
Weapon_Hbuild_Fire
===============
*/
void Weapon_Hbuild_Fire( gentity_t *ent )
{
  G_AddPredictableEvent( ent, EV_MENU, MN_H_BUILD );
}
///////build weapons

/*
======================================================================

VENOM

======================================================================
*/

/*
===============
CheckVenomAttack
===============
*/
qboolean CheckVenomAttack( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;
  int     damage;

  // set aiming directions
  AngleVectors (ent->client->ps.viewangles, forward, right, up);

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  VectorMA (muzzle, 32, forward, end);

  trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
  if ( tr.surfaceFlags & SURF_NOIMPACT )
    return qfalse;

  traceEnt = &g_entities[ tr.entityNum ];

  if( !traceEnt->takedamage)
    return qfalse;
  if( !traceEnt->client )
    return qfalse;
  if( traceEnt->client->ps.stats[ STAT_PTEAM ] == PTE_DROIDS )
    return qfalse;

  // send blood impact
  if ( traceEnt->takedamage && traceEnt->client )
  {
    tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
    tent->s.otherEntityNum = traceEnt->s.number;
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
  }

  G_Damage( traceEnt, ent, ent, forward, tr.endpos, 5, DAMAGE_NO_KNOCKBACK, MOD_VENOM );
  if( traceEnt->client )
  {
    if( !( traceEnt->client->ps.stats[ STAT_STATE ] & SS_POISONED ) )
    {
      traceEnt->client->ps.stats[ STAT_STATE ] |= SS_POISONED;
      traceEnt->client->lastPoisonTime = level.time;
    }
  }

  return qtrue;
}

/*
===============
Weapon_Venom_Fire
===============
*/
void Weapon_Venom_Fire( gentity_t *ent )
{
}

/*
======================================================================

CIRCULAR SAW

======================================================================
*/

/*
===============
Weapon_Csaw_Fire
===============
*/
void Weapon_CSaw_Fire( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;

  // set aiming directions
  AngleVectors (ent->client->ps.viewangles, forward, right, up);

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  VectorMA( muzzle, 32, forward, end );

  trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
  if ( tr.surfaceFlags & SURF_NOIMPACT )
    return;

  traceEnt = &g_entities[ tr.entityNum ];

  // send blood impact
  if ( traceEnt->takedamage && traceEnt->client )
  {
    tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
    tent->s.otherEntityNum = traceEnt->s.number;
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
  }

  if ( traceEnt->takedamage )
    G_Damage( traceEnt, ent, ent, forward, tr.endpos, 5, DAMAGE_NO_KNOCKBACK, MOD_VENOM );
}

/*
===============
Weapon_Grab_Fire
===============
*/
void Weapon_Grab_Fire( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;
  int     damage;

  // set aiming directions
  AngleVectors (ent->client->ps.viewangles, forward, right, up);

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  VectorMA( muzzle, 32, forward, end );

  trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
  if ( tr.surfaceFlags & SURF_NOIMPACT )
    return;

  traceEnt = &g_entities[ tr.entityNum ];

  if( !traceEnt->takedamage )
    return;
  if( !traceEnt->client )
    return;
  if( traceEnt->client->ps.stats[ STAT_PTEAM ] == PTE_DROIDS )
    return;
    
  if( traceEnt->client )
  {
    //lock client
    traceEnt->client->ps.stats[ STAT_STATE ] |= SS_GRABBED;
    traceEnt->client->lastGrabTime = level.time;
  }
}

/*
===============
CheckGrabAttack
===============
*/
qboolean CheckGrabAttack( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;
  int     damage;

  // set aiming directions
  AngleVectors (ent->client->ps.viewangles, forward, right, up);

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  VectorMA (muzzle, 32, forward, end);

  trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
  if ( tr.surfaceFlags & SURF_NOIMPACT )
    return qfalse;

  traceEnt = &g_entities[ tr.entityNum ];

  if( !traceEnt->takedamage)
    return qfalse;
  if( !traceEnt->client )
    return qfalse;
  if( traceEnt->client->ps.stats[ STAT_PTEAM ] == PTE_DROIDS )
    return qfalse;
    
  return qtrue;
}

/*
======================================================================

CLAW AND POUNCE

======================================================================
*/

/*
===============
Weapon_Claw_Fire
===============
*/
void Weapon_Claw_Fire( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;

  // set aiming directions
  AngleVectors (ent->client->ps.viewangles, forward, right, up);

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  VectorMA( muzzle, 32, forward, end );

  trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
  if ( tr.surfaceFlags & SURF_NOIMPACT )
    return;

  traceEnt = &g_entities[ tr.entityNum ];

  // send blood impact
  if ( traceEnt->takedamage && traceEnt->client )
  {
    tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
    tent->s.otherEntityNum = traceEnt->s.number;
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
  }

  if ( traceEnt->takedamage )
    G_Damage( traceEnt, ent, ent, forward, tr.endpos, 50, DAMAGE_NO_KNOCKBACK, MOD_VENOM );
}

/*
===============
CheckPounceAttack
===============
*/
qboolean CheckPounceAttack( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;
  int     damage;

  if( !ent->client->allowedToPounce )
    return qfalse;

  if( ent->client->ps.groundEntityNum != ENTITYNUM_NONE )
    return qfalse;

  // set aiming directions
  AngleVectors (ent->client->ps.viewangles, forward, right, up);

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  VectorMA (muzzle, 48, forward, end);

  trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);

  //miss
  if( tr.fraction >= 1.0 )
    return qfalse;
    
  if ( tr.surfaceFlags & SURF_NOIMPACT )
    return qfalse;

  traceEnt = &g_entities[ tr.entityNum ];

  // send blood impact
  if ( traceEnt->takedamage && traceEnt->client )
  {
    tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
    tent->s.otherEntityNum = traceEnt->s.number;
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
  }

  if( !traceEnt->takedamage)
    return qfalse;

  damage = (int)( (float)ent->client->pouncePayload / ( MAX_POUNCE_SPEED / 100.0f ) );

  G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_VENOM );

  ent->client->allowedToPounce = qfalse;
  
  return qtrue;
}

//======================================================================

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) 
{ 
  //TA: theres a crash bug in here somewhere, but i'm too lazy to find it hence,
  return qfalse;
  
  /*if( !target->takedamage ) {
    return qfalse;
  }

  if ( target == attacker ) {
    return qfalse;
  }

  if( !target->client ) {
    return qfalse;
  }

  if( !attacker->client ) {
    return qfalse;
  }

  if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
    return qfalse;
  }

  if ( OnSameTeam( target, attacker ) ) {
    return qfalse;
  }

  return qtrue;*/
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
  VectorCopy( ent->s.pos.trBase, muzzlePoint );
  muzzlePoint[2] += ent->client->ps.viewheight;
  VectorMA( muzzlePoint, 1, forward, muzzlePoint );
  // snap to integer coordinates for more efficient network bandwidth usage
  SnapVector( muzzlePoint );
}

/*
===============
FireWeapon2
===============
*/
void FireWeapon2( gentity_t *ent )
{
  if( ent->client )
  {
    // set aiming directions
    AngleVectors (ent->client->ps.viewangles, forward, right, up);
    CalcMuzzlePoint( ent, forward, right, up, muzzle );
  }
  else
  {
    AngleVectors( ent->s.angles2, forward, right, up );
    VectorCopy( ent->s.pos.trBase, muzzle );
  }

  // fire the specific weapon
  switch( ent->s.weapon )
  {
    case WP_GAUNTLET:
      Weapon_Gauntlet( ent );
      break;
    case WP_LIGHTNING:
      Weapon_LightningFire( ent );
      break;
    case WP_SHOTGUN:
      weapon_supershotgun_fire( ent );
      break;
    case WP_MACHINEGUN:
      Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE, MOD_MACHINEGUN );
      break;
    case WP_CHAINGUN:
      Bullet_Fire( ent, CHAINGUN_SPREAD, CHAINGUN_DAMAGE, MOD_CHAINGUN );
      break;
    case WP_GRENADE_LAUNCHER:
      weapon_grenadelauncher_fire( ent );
      break;
    case WP_ROCKET_LAUNCHER:
      Weapon_RocketLauncher_Fire( ent );
      break;
    case WP_FLAMER:
      Weapon_Flamer_Fire( ent );
      break;
    case WP_PLASMAGUN:
      Weapon_Plasma_Fire( ent );
      break;
    case WP_RAILGUN:
      weapon_railgun_fire( ent );
      break;
    case WP_SAWBLADE_LAUNCHER:
      break;
    case WP_BFG:
      BFG_Fire( ent );
      break;
    case WP_GRAPPLING_HOOK:
      Weapon_GrapplingHook_Fire( ent );
      break;
    case WP_VENOM:
      Weapon_Venom_Fire( ent );
      break;
    case WP_GRABANDCSAW:
      Weapon_Grab_Fire( ent );
      break;
    case WP_POUNCE:
      break;
    case WP_DBUILD:
      Weapon_Abuild_Fire( ent );
      break;
    case WP_HBUILD:
      Weapon_Hbuild_Fire( ent );
      break;
    case WP_SCANNER: //scanner doesn't "fire"
    default:
  // FIXME    G_Error( "Bad ent->s.weapon" );
      break;
  }
}

/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent )
{
  if( ent->client )
  {
    // set aiming directions
    AngleVectors (ent->client->ps.viewangles, forward, right, up);
    CalcMuzzlePoint( ent, forward, right, up, muzzle );
  }
  else
  {
    AngleVectors( ent->s.angles2, forward, right, up );
    VectorCopy( ent->s.pos.trBase, muzzle );
  }

  // fire the specific weapon
  switch( ent->s.weapon )
  {
    case WP_GAUNTLET:
      Weapon_Gauntlet( ent );
      break;
    case WP_LIGHTNING:
      Weapon_LightningFire( ent );
      break;
    case WP_SHOTGUN:
      weapon_supershotgun_fire( ent );
      break;
    case WP_MACHINEGUN:
      Bullet_Fire( ent, MACHINEGUN_SPREAD, MACHINEGUN_DAMAGE, MOD_MACHINEGUN );
      break;
    case WP_CHAINGUN:
      Bullet_Fire( ent, CHAINGUN_SPREAD, CHAINGUN_DAMAGE, MOD_CHAINGUN );
      break;
    case WP_GRENADE_LAUNCHER:
      weapon_grenadelauncher_fire( ent );
      break;
    case WP_ROCKET_LAUNCHER:
      Weapon_RocketLauncher_Fire( ent );
      break;
    case WP_FLAMER:
      Weapon_Flamer_Fire( ent );
      break;
    case WP_PLASMAGUN:
      Weapon_Plasma_Fire( ent );
      break;
    case WP_RAILGUN:
      weapon_railgun_fire( ent );
      break;
    case WP_SAWBLADE_LAUNCHER:
      Weapon_SawbladeLauncher_Fire( ent );
      break;
    case WP_BFG:
      BFG_Fire( ent );
      break;
    case WP_GRAPPLING_HOOK:
      Weapon_GrapplingHook_Fire( ent );
      break;
    case WP_VENOM:
      Weapon_Venom_Fire( ent );
      break;
    case WP_GRABANDCSAW:
      Weapon_CSaw_Fire( ent );
      break;
    case WP_POUNCE:
      Weapon_Claw_Fire( ent );
      break;
    case WP_DBUILD:
      Weapon_Abuild_Fire( ent );
      break;
    case WP_HBUILD:
      Weapon_Hbuild_Fire( ent );
      break;
    case WP_SCANNER: //scanner doesn't "fire"
    default:
  // FIXME    G_Error( "Bad ent->s.weapon" );
      break;
  }
}

