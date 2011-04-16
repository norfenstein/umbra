/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"

static  vec3_t  forward, right, up;
static  vec3_t  muzzle;

/*
================
G_ForceWeaponChange
================
*/
void G_ForceWeaponChange( gentity_t *ent, weapon_t weapon )
{
  playerState_t *ps = &ent->client->ps;

  // stop a reload in progress
  if( ps->weaponstate == WEAPON_RELOADING )
  {
    ps->torsoAnim = ( ( ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | TORSO_RAISE;
    ps->weaponTime = 250;
    ps->weaponstate = WEAPON_READY;
  }
  
  if( weapon == WP_NONE ||
      !BG_InventoryContainsWeapon( weapon, ps->stats ) )
  {
    ps->persistant[ PERS_NEWWEAPON ] = ent->client->ps.stats[ STAT_WEAPON1 ];
  }
  else
    ps->persistant[ PERS_NEWWEAPON ] = weapon;

  // force this here to prevent flamer effect from continuing
  ps->generic1 = WPM_NOTFIRING;

  // The PMove will do an animated drop, raise, and set the new weapon
  ps->pm_flags |= PMF_WEAPON_SWITCH;
}

/*
=================
G_GiveClientMaxAmmo
=================
*/
void G_GiveClientMaxAmmo( gentity_t *ent, qboolean buyingEnergyAmmo )
{
  int i;
  qboolean restoredAmmo = qfalse;

  for( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
  {
    if( !BG_InventoryContainsWeapon( i, ent->client->ps.stats ) ||
        !BG_Weapon( i )->usesAmmo ||
        BG_WeaponIsFull( i, ent->client->ps.stats,
                         ent->client->ps.ammo, ent->client->ps.clips ) )
      continue;
      
    ent->client->ps.ammo = BG_Weapon( i )->maxAmmo;
    ent->client->ps.clips = BG_Weapon( i )->maxClips;

    restoredAmmo = qtrue;
  }

  if( restoredAmmo )
    G_ForceWeaponChange( ent, ent->client->ps.weapon );
}

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout )
{
  vec3_t v, newv;
  float dot;

  VectorSubtract( impact, start, v );
  dot = DotProduct( v, dir );
  VectorMA( v, -2 * dot, dir, newv );

  VectorNormalize(newv);
  VectorMA(impact, 8192, newv, endout);
}

/*
================
G_WideTrace

Trace a bounding box against entities, but not the world
Also check there is a line of sight between the start and end point
================
*/
static void G_WideTrace( trace_t *tr, gentity_t *ent, float range,
                         float width, float height, gentity_t **target )
{
  vec3_t    mins, maxs;
  vec3_t    end;

  VectorSet( mins, -width, -width, -height );
  VectorSet( maxs, width, width, width );

  *target = NULL;

  if( !ent->client )
    return;

  // Try a linear trace first
  VectorMA( muzzle, range + width, forward, end );

  G_UnlaggedOn( ent, muzzle, range + width );

  trap_Trace( tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
  if( tr->entityNum != ENTITYNUM_NONE )
  {
    // We hit something with the linear trace
    *target = &g_entities[ tr->entityNum ];
  }
  else
  {
    // The linear trace didn't hit anything, so retry with a wide trace
    VectorMA( muzzle, range, forward, end );

    // Trace against entities
    trap_Trace( tr, muzzle, mins, maxs, end, ent->s.number, CONTENTS_BODY );
    if( tr->entityNum != ENTITYNUM_NONE )
    {
      *target = &g_entities[ tr->entityNum ];

      // Set range to the trace length plus the width, so that the end of the
      // LOS trace is close to the exterior of the target's bounding box
      range = Distance( muzzle, tr->endpos ) + width;
      VectorMA( muzzle, range, forward, end );

      // Trace for line of sight against the world
      trap_Trace( tr, muzzle, NULL, NULL, end, 0, CONTENTS_SOLID );
      if( tr->fraction < 1.0f )
        *target = NULL;
    }
  }

  G_UnlaggedOff( );
}

/*
======================
SnapVectorTowards
SnapVectorNormal

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to )
{
  int   i;

  for( i = 0 ; i < 3 ; i++ )
  {
    if( v[ i ] >= 0 )
      v[ i ] = (int)( v[ i ] + ( to[ i ] <= v[ i ] ? 0 : 1 ) );
    else
      v[ i ] = (int)( v[ i ] + ( to[ i ] <= v[ i ] ? -1 : 0 ) );
  }
}

void SnapVectorNormal( vec3_t v, vec3_t normal )
{
  int i;

  for( i = 0 ; i < 3 ; i++ )
  {
    if( v[ i ] >= 0 )
      v[ i ] = (int)( v[ i ] + ( normal[ i ] <= 0 ? 0 : 1 ) );
    else
      v[ i ] = (int)( v[ i ] + ( normal[ i ] <= 0 ? -1 : 0 ) );
  }
}

/*
===============
BloodSpurt

Generates a blood spurt event for traces with accurate end points
===============
*/
static void BloodSpurt( gentity_t *attacker, gentity_t *victim, trace_t *tr )
{
  gentity_t *tent;

  if( !attacker->client )
    return;

  if( victim->health <= 0 )
    return;

  tent = G_TempEntity( tr->endpos, EV_MISSILE_HIT );
  tent->s.otherEntityNum = victim->s.number;
  tent->s.eventParm = DirToByte( tr->plane.normal );
  tent->s.weapon = attacker->s.weapon;
  tent->s.generic1 = attacker->s.generic1; // weaponMode
}

/*
===============
WideBloodSpurt

Calculates the position of a blood spurt for wide traces and generates an event
===============
*/
static void WideBloodSpurt( gentity_t *attacker, gentity_t *victim, trace_t *tr )
{
  gentity_t *tent;
  vec3_t normal, origin;
  float mag, radius;

  if( !attacker->client )
    return;

  if( victim->health <= 0 )
    return;

  if( tr )
    VectorSubtract( tr->endpos, victim->s.origin, normal );
  else
    VectorSubtract( attacker->client->ps.origin,
                    victim->s.origin, normal );

  // Normalize the horizontal components of the vector difference to the
  // "radius" of the bounding box
  mag = sqrt( normal[ 0 ] * normal[ 0 ] + normal[ 1 ] * normal[ 1 ] );
  radius = victim->r.maxs[ 0 ] * 1.21f;
  if( mag > radius )
  {
    normal[ 0 ] = normal[ 0 ] / mag * radius;
    normal[ 1 ] = normal[ 1 ] / mag * radius;
  }

  // Clamp origin to be within bounding box vertically
  if( normal[ 2 ] > victim->r.maxs[ 2 ] )
    normal[ 2 ] = victim->r.maxs[ 2 ];
  if( normal[ 2 ] < victim->r.mins[ 2 ] )
    normal[ 2 ] = victim->r.mins[ 2 ];

  VectorAdd( victim->s.origin, normal, origin );
  VectorNegate( normal, normal );
  VectorNormalize( normal );

  // Create the blood spurt effect entity
  tent = G_TempEntity( origin, EV_MISSILE_HIT );
  tent->s.eventParm = DirToByte( normal );
  tent->s.otherEntityNum = victim->s.number;
  tent->s.weapon = attacker->s.weapon;
  tent->s.generic1 = attacker->s.generic1; // weaponMode
}

/*
===============
meleeAttack
===============
*/
void meleeAttack( gentity_t *ent, float range, float width, float height,
                  int damage, meansOfDeath_t mod )
{
  trace_t   tr;
  gentity_t *traceEnt;

  G_WideTrace( &tr, ent, range, width, height, &traceEnt );
  if( traceEnt == NULL || !traceEnt->takedamage )
    return;

  WideBloodSpurt( ent, traceEnt, &tr );
  G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, 0, mod );
}

/*
======================================================================

MACHINEGUN

======================================================================
*/

void bulletFire( gentity_t *ent, float spread, int damage, int knockback, int mod )
{
  trace_t   tr;
  vec3_t    end;
  float   r;
  float   u;
  gentity_t *tent;
  gentity_t *traceEnt;

  r = random( ) * M_PI * 2.0f;
  u = sin( r ) * crandom( ) * spread * 16;
  r = cos( r ) * crandom( ) * spread * 16;
  VectorMA( muzzle, 8192 * 16, forward, end );
  VectorMA( end, r, right, end );
  VectorMA( end, u, up, end );

  // don't use unlagged if this is not a client (e.g. turret)
  if( ent->client )
  {
    G_UnlaggedOn( ent, muzzle, 8192 * 16 );
    trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
    G_UnlaggedOff( );
  }
  else
    trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );

  if( tr.surfaceFlags & SURF_NOIMPACT )
    return;

  traceEnt = &g_entities[ tr.entityNum ];

  // snap the endpos to integers, but nudged towards the line
  SnapVectorTowards( tr.endpos, muzzle );

  // send bullet impact
  if( traceEnt->takedamage &&
      (traceEnt->s.eType == ET_PLAYER ||
       traceEnt->s.eType == ET_BUILDABLE ) )
  {
    tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
    tent->s.eventParm = traceEnt->s.number;
  }
  else
  {
    tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
    tent->s.eventParm = DirToByte( tr.plane.normal );
  }
  tent->s.otherEntityNum = ent->s.number;

  if( traceEnt->takedamage )
  {
    G_Damage( traceEnt, ent, ent, forward, tr.endpos,
      damage, knockback, 0, mod );
  }
}

/*
======================================================================

SCATTERGUN

======================================================================
*/

// this should match CG_ScattergunPattern
void ScattergunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent )
{
  int        i;
  float      r, u;
  vec3_t    end;
  vec3_t    forward, right, up;
  trace_t    tr;
  gentity_t  *traceEnt;

  // derive the right and up vectors from the forward vector, because
  // the client won't have any other information
  VectorNormalize2( origin2, forward );
  PerpendicularVector( right, forward );
  CrossProduct( forward, right, up );

  // generate the "random" spread pattern
  for( i = 0; i < SHOTGUN_PELLETS; i++ )
  {
    r = Q_crandom( &seed ) * SCATTERGUN_SPREAD * 16;
    u = Q_crandom( &seed ) * SCATTERGUN_SPREAD * 16;
    VectorMA( origin, SCATTERGUN_RANGE, forward, end );
    VectorMA( end, r, right, end );
    VectorMA( end, u, up, end );

    trap_Trace( &tr, origin, NULL, NULL, end, ent->s.number, MASK_SHOT );
    traceEnt = &g_entities[ tr.entityNum ];

    // send bullet impact
    if( !( tr.surfaceFlags & SURF_NOIMPACT ) )
    {
      if( traceEnt->takedamage )
        G_Damage( traceEnt, ent, ent, forward, tr.endpos, SCATTERGUN_DMG, SCATTERGUN_KNOCKBACK, 0, MOD_SCATTERGUN );
    }
  }
}

void scattergunShellFire( gentity_t *ent )
{
  gentity_t    *tent;

  // send shotgun blast
  tent = G_TempEntity( muzzle, EV_SCATTERGUN );
  VectorScale( forward, 4096, tent->s.origin2 );
  SnapVector( tent->s.origin2 );
  tent->s.eventParm = rand() & 255;    // seed for spread pattern
  tent->s.otherEntityNum = ent->s.number;
  G_UnlaggedOn( ent, muzzle, SCATTERGUN_RANGE );
  ScattergunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
  G_UnlaggedOff();
}

void scattergunBlastFire( gentity_t *ent )
{
  fire_scattergun( ent, muzzle, forward, ent->client->ps.stats[ STAT_MISC ] );

  ent->client->ps.stats[ STAT_MISC ] = 0;
}

/*
======================================================================

SHOTGUN

======================================================================
*/

// this should match CG_ShotgunPattern
void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent )
{
  int        i;
  float      r, u;
  vec3_t    end;
  vec3_t    forward, right, up;
  trace_t    tr;
  gentity_t  *traceEnt;

  // derive the right and up vectors from the forward vector, because
  // the client won't have any other information
  VectorNormalize2( origin2, forward );
  PerpendicularVector( right, forward );
  CrossProduct( forward, right, up );

  // generate the "random" spread pattern
  for( i = 0; i < SHOTGUN_PELLETS; i++ )
  {
    r = Q_crandom( &seed ) * SHOTGUN_SPREAD * 16;
    u = Q_crandom( &seed ) * SHOTGUN_SPREAD * 16;
    VectorMA( origin, SHOTGUN_RANGE, forward, end );
    VectorMA( end, r, right, end );
    VectorMA( end, u, up, end );

    trap_Trace( &tr, origin, NULL, NULL, end, ent->s.number, MASK_SHOT );
    traceEnt = &g_entities[ tr.entityNum ];

    // send bullet impact
    if( !( tr.surfaceFlags & SURF_NOIMPACT ) )
    {
      if( traceEnt->takedamage )
        G_Damage( traceEnt, ent, ent, forward, tr.endpos, SHOTGUN_DMG, SHOTGUN_KNOCKBACK, 0, MOD_SHOTGUN );
    }
  }
}

void shotgunFire( gentity_t *ent )
{
  gentity_t    *tent;

  // send shotgun blast
  tent = G_TempEntity( muzzle, EV_SHOTGUN );
  VectorScale( forward, 4096, tent->s.origin2 );
  SnapVector( tent->s.origin2 );
  tent->s.eventParm = rand() & 255;    // seed for spread pattern
  tent->s.otherEntityNum = ent->s.number;
  G_UnlaggedOn( ent, muzzle, SHOTGUN_RANGE );
  ShotgunPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
  G_UnlaggedOff();
}

/*
======================================================================

MASS DRIVER

======================================================================
*/

void massDriverFire( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;

  VectorMA( muzzle, 8192.0f * 16.0f, forward, end );

  G_UnlaggedOn( ent, muzzle, 8192.0f * 16.0f );
  trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
  G_UnlaggedOff( );

  if( tr.surfaceFlags & SURF_NOIMPACT )
    return;

  traceEnt = &g_entities[ tr.entityNum ];

  // snap the endpos to integers, but nudged towards the line
  SnapVectorTowards( tr.endpos, muzzle );

  // send impact
  if( traceEnt->takedamage && 
      (traceEnt->s.eType == ET_BUILDABLE || 
       traceEnt->s.eType == ET_PLAYER ) )
  {
    BloodSpurt( ent, traceEnt, &tr );
  }
  else
  {
    tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
    tent->s.generic1 = ent->s.generic1; //weaponMode
  }

  if( traceEnt->takedamage )
  {
    G_Damage( traceEnt, ent, ent, forward, tr.endpos,
      MDRIVER_DMG, MDRIVER_KNOCKBACK, 0, MOD_MDRIVER );
  }
}

/*
======================================================================

LOCKBLOB

======================================================================
*/

void lockBlobLauncherFire( gentity_t *ent )
{
  gentity_t *m;

  m = fire_lockblob( ent, muzzle, forward );

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

HIVE

======================================================================
*/

void hiveFire( gentity_t *ent )
{
  vec3_t origin;

  // Fire from the hive tip, not the center
  VectorMA( muzzle, ent->r.maxs[ 2 ], ent->s.origin2, origin );
  
  fire_hive( ent, origin, forward );
}

/*
======================================================================

BLASTER PISTOL

======================================================================
*/

void blasterFire( gentity_t *ent )
{
  gentity_t *m;

  m = fire_blaster( ent, muzzle, forward );

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

PULSE RIFLE

======================================================================
*/

void pulseRifleFire( gentity_t *ent )
{
  gentity_t *m;

  m = fire_pulseRifle( ent, muzzle, forward );

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

GRENADE

======================================================================
*/

void throwGasGrenade( gentity_t *ent )
{
  gentity_t *m;

  m = launch_gasGrenade( ent, muzzle, forward );
}

void throwSporeGrenade( gentity_t *ent )
{
  gentity_t *m;

  m = launch_sporeGrenade( ent, muzzle, forward );
}

void throwSpikeGrenade( gentity_t *ent )
{
  gentity_t *m;

  m = launch_spikeGrenade( ent, muzzle, forward );
}

void throwShockGrenade( gentity_t *ent )
{
  gentity_t *m;

  m = launch_shockGrenade( ent, muzzle, forward );
}

void throwNerveGrenade( gentity_t *ent )
{
  gentity_t *m;

  m = launch_nerveGrenade( ent, muzzle, forward );
}

void throwFragGrenade( gentity_t *ent )
{
  gentity_t *m;

  m = launch_fragGrenade( ent, muzzle, forward );
}

/*
======================================================================

LAS GUN

======================================================================
*/

/*
===============
lasGunFire
===============
*/
void lasGunFire( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end;
  gentity_t *tent;
  gentity_t *traceEnt;

  VectorMA( muzzle, 8192 * 16, forward, end );

  G_UnlaggedOn( ent, muzzle, 8192 * 16 );
  trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
  G_UnlaggedOff( );

  if( tr.surfaceFlags & SURF_NOIMPACT )
    return;

  traceEnt = &g_entities[ tr.entityNum ];

  // snap the endpos to integers, but nudged towards the line
  SnapVectorTowards( tr.endpos, muzzle );

  // send impact
  if( traceEnt->takedamage && 
      (traceEnt->s.eType == ET_BUILDABLE || 
       traceEnt->s.eType == ET_PLAYER ) )
  {
    BloodSpurt( ent, traceEnt, &tr );
  }
  else
  {
    tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
    tent->s.generic1 = ent->s.generic1; //weaponMode
  }

  if( traceEnt->takedamage )
    G_Damage( traceEnt, ent, ent, forward, tr.endpos, LASGUN_DAMAGE, LASGUN_KNOCKBACK, 0, MOD_LASGUN );
}

/*
======================================================================

PAIN SAW

======================================================================
*/

void painSawFire( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    temp;
  gentity_t *tent, *traceEnt;

  G_WideTrace( &tr, ent, PAINSAW_RANGE, PAINSAW_WIDTH, PAINSAW_HEIGHT,
               &traceEnt );
  if( !traceEnt || !traceEnt->takedamage )
    return;

  // hack to line up particle system with weapon model
  tr.endpos[ 2 ] -= 5.0f;

  // send blood impact
  if( traceEnt->s.eType == ET_PLAYER || traceEnt->s.eType == ET_BUILDABLE )
  {
      BloodSpurt( ent, traceEnt, &tr );
  }
  else
  {
    VectorCopy( tr.endpos, temp );
    tent = G_TempEntity( temp, EV_MISSILE_MISS );
    tent->s.eventParm = DirToByte( tr.plane.normal );
    tent->s.weapon = ent->s.weapon;
    tent->s.generic1 = ent->s.generic1; //weaponMode
  }

  G_Damage( traceEnt, ent, ent, forward, tr.endpos, PAINSAW_DAMAGE, PAINSAW_KNOCKBACK, 0, MOD_PAINSAW );
}

/*
======================================================================

LUCIFER CANNON

======================================================================
*/

/*
===============
lcannonFire
===============
*/
void lcannonFire( gentity_t *ent )
{
  gentity_t *m;
  vec3_t    dir;

  //fire "drunkenly" if not crouching
  if( !( ent->client->ps.pm_flags & PMF_DUCKED ) )
  {
    vec3_t reverse;

    VectorMA( forward, random() * -0.25, up, dir );
    VectorMA( dir, crandom() * 0.5, right, dir );

    VectorCopy( dir, reverse );
    VectorInverse( reverse );

    G_Damage( ent, ent, ent, reverse, muzzle, LCANNON_KICKBACK_DAMAGE, LCANNON_KICKBACK_KNOCKBACK, 0, MOD_LCANNON_SPLASH);
  }
  else
  {
    VectorCopy( forward, dir );
  }

  m = fire_luciferCannon( ent, muzzle, dir );

  ent->client->ps.stats[ STAT_MISC ] = 0;
}

/*
======================================================================

TESLA GENERATOR

======================================================================
*/


void teslaFire( gentity_t *self )
{
  trace_t tr;
  vec3_t origin, target;
  gentity_t *tent;
  
  if( !self->enemy )
    return;

  // Move the muzzle from the entity origin up a bit to fire over turrets
  VectorMA( muzzle, self->r.maxs[ 2 ], self->s.origin2, origin );

  // Don't aim for the center, aim at the top of the bounding box
  VectorCopy( self->enemy->s.origin, target );
  target[ 2 ] += self->enemy->r.maxs[ 2 ];

  // Trace to the target entity
  trap_Trace( &tr, origin, NULL, NULL, target, self->s.number, MASK_SHOT );
  if( tr.entityNum != self->enemy->s.number )
    return;

  // Client side firing effect
  self->s.eFlags |= EF_FIRING;

  // Deal damage
  if( self->enemy->takedamage )
  {
    vec3_t dir;
    
    VectorSubtract( target, origin, dir );
    G_Damage( self->enemy, self, self, dir, tr.endpos,
              TESLAGEN_DMG, TESLAGEN_KNOCKBACK, 0, MOD_TESLAGEN );
  }

  // Send tesla zap trail
  tent = G_TempEntity( tr.endpos, EV_TESLATRAIL );
  tent->s.generic1 = self->s.number; // src
  tent->s.clientNum = self->enemy->s.number; // dest
}


/*
======================================================================

BUILD GUN

======================================================================
*/
void CheckCkitRepair( gentity_t *ent )
{
  vec3_t      forward, end;
  trace_t     tr;
  gentity_t   *traceEnt;
  int         bHealth;

  if( ent->client->ps.weaponTime > 0 ||
      ent->client->ps.stats[ STAT_MISC ] > 0 )
	return;

  // Construction kit repair
  AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
  VectorMA( ent->client->ps.origin, 100, forward, end );
  
  trap_Trace( &tr, ent->client->ps.origin, NULL, NULL, end, ent->s.number,
              MASK_PLAYERSOLID );
  traceEnt = &g_entities[ tr.entityNum ];
  
  if( tr.fraction < 1.0f && traceEnt->spawned && traceEnt->health > 0 &&
      traceEnt->s.eType == ET_BUILDABLE && traceEnt->buildableTeam == TEAM_HUMANS )
  {
    bHealth = BG_Buildable( traceEnt->s.modelindex )->health;
    if( traceEnt->health < bHealth )
    {
      HealEntity( traceEnt, bHealth, HBUILD_HEALRATE );
      if( traceEnt->health == bHealth )
        G_AddEvent( ent, EV_BUILD_REPAIRED, 0 );
      else
        G_AddEvent( ent, EV_BUILD_REPAIR, 0 );

      ent->client->ps.weaponTime += BG_Weapon( ent->client->ps.weapon )->repeatRate1;
    }
  }
}

/*
===============
cancelBuildFire
===============
*/
void cancelBuildFire( gentity_t *ent )
{
  // Cancel ghost buildable
  if( ent->client->ps.stats[ STAT_BUILDABLE ] != BA_NONE )
  {
    ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
    return;
  }

  if( ent->client->ps.weapon == WP_ABUILD )
    meleeAttack( ent, ABUILDER_CLAW_RANGE, ABUILDER_CLAW_WIDTH,
                 ABUILDER_CLAW_WIDTH, ABUILDER_CLAW_DMG, MOD_ABUILDER_CLAW );
}

/*
===============
buildFire
===============
*/
void buildFire( gentity_t *ent, dynMenu_t menu )
{
  buildable_t buildable = ( ent->client->ps.stats[ STAT_BUILDABLE ]
                            & ~SB_VALID_TOGGLEBIT );

  if( buildable > BA_NONE )
  {
    if( ent->client->ps.stats[ STAT_MISC ] > 0 )
    {
      G_AddEvent( ent, EV_BUILD_DELAY, ent->client->ps.clientNum );
      return;
    }

    if( G_BuildIfValid( ent, buildable ) )
    {
      if( !g_cheats.integer )
      {
        ent->client->ps.stats[ STAT_MISC ] +=
          BG_Buildable( buildable )->buildTime;
      }

      ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
    }

    return;
  }

  G_TriggerMenu( ent->client->ps.clientNum, menu );
}


/*
======================================================================

ALEVEL0

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
  gentity_t *traceEnt;
  int       damage = ALEVEL0_BITE_DMG;

  if( ent->client->ps.weaponTime )
	return qfalse;

  // Calculate muzzle point
  AngleVectors( ent->client->ps.viewangles, forward, right, up );
  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  G_WideTrace( &tr, ent, ALEVEL0_BITE_RANGE, ALEVEL0_BITE_WIDTH,
               ALEVEL0_BITE_WIDTH, &traceEnt );

  if( traceEnt == NULL )
    return qfalse;

  if( !traceEnt->takedamage )
    return qfalse;

  if( traceEnt->health <= 0 )
      return qfalse;

  if( !traceEnt->client && !( traceEnt->s.eType == ET_BUILDABLE ) )
    return qfalse;

  // only allow bites to work against buildings as they are constructing
  if( traceEnt->s.eType == ET_BUILDABLE )
  {
    /*
    if( traceEnt->spawned )
      return qfalse;
      */

    if( traceEnt->buildableTeam == TEAM_ALIENS )
      return qfalse;
  }

  if( traceEnt->client )
  {
    if( traceEnt->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
      return qfalse;
    if( traceEnt->client->ps.stats[ STAT_HEALTH ] <= 0 )
      return qfalse;
  }

  // send blood impact
  WideBloodSpurt( ent, traceEnt, &tr );

  G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, 0, MOD_ALEVEL0_BITE );
  ent->client->ps.weaponTime += ALEVEL0_BITE_REPEAT;
  return qtrue;
}

/*
===============
grappleFire
===============
*/
void grappleFire( gentity_t *ent )
{
  if( !ent->client->fireHeld && !ent->client->hook )
    fire_grapple( ent, muzzle, forward );

  ent->client->fireHeld = qtrue;
}

/*
===============
G_HookFree
===============
*/
void G_HookFree( gentity_t *ent )
{
  ent->parent->client->hook = NULL;
  ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
  G_FreeEntity( ent );
}

/*
===============
G_HookThink
===============
*/
void G_HookThink( gentity_t *ent )
{
  if( ent->enemy )
  {
    vec3_t v, oldorigin;

    VectorCopy( ent->r.currentOrigin, oldorigin );
    v[0] = ent->enemy->r.currentOrigin[0] + ( ent->enemy->r.mins[0] + ent->enemy->r.maxs[0] ) * 0.5;
    v[1] = ent->enemy->r.currentOrigin[1] + ( ent->enemy->r.mins[1] + ent->enemy->r.maxs[1] ) * 0.5;
    v[2] = ent->enemy->r.currentOrigin[2] + ( ent->enemy->r.mins[2] + ent->enemy->r.maxs[2] ) * 0.5;
    SnapVectorTowards( v, oldorigin );	// save net bandwidth

    G_SetOrigin( ent, v );
  }

  VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint );
}

/*
======================================================================

ALEVEL1_1

======================================================================
*/

void spitFire( gentity_t *ent )
{
  gentity_t *m;

  m = fire_spit( ent, muzzle, forward );

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
======================================================================

ALEVEL2

======================================================================
*/

void kamikazeExplode( gentity_t *ent )
{
  G_RadiusDamage( ent->r.currentOrigin, ent, ALEVEL2_KAMIKAZE_DAMAGE, ALEVEL2_KAMIKAZE_KNOCKBACK,
                  ALEVEL2_KAMIKAZE_RANGE, ent, MOD_ALEVEL2_KAMIKAZE );

  G_Damage( ent, ent, ent, NULL, ent->r.currentOrigin, ent->health, 0, 0, MOD_ALEVEL2_KAMIKAZE);
}

/*
======================================================================

ALEVEL3

======================================================================
*/

void flamerFire( gentity_t *ent )
{
  vec3_t origin;

  // Correct muzzle so that the missile does not start in the ceiling 
  VectorMA( muzzle, -7.0f, up, origin );

  // Correct muzzle so that the missile fires from the player's hand
  VectorMA( origin, 4.5f, right, origin );

  fire_flamer( ent, origin, forward );
}

/*
======================================================================

ALEVEL4

======================================================================
*/

/*
===============
CheckGrabAttack
===============
*/
void CheckGrabAttack( gentity_t *ent )
{
  trace_t   tr;
  vec3_t    end, dir;
  gentity_t *traceEnt;

  // set aiming directions
  AngleVectors( ent->client->ps.viewangles, forward, right, up );

  CalcMuzzlePoint( ent, forward, right, up, muzzle );

  if( ent->client->ps.weapon == WP_ALEVEL4 )
    VectorMA( muzzle, ALEVEL4_GRAB_RANGE, forward, end );

  trap_Trace( &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );
  if( tr.surfaceFlags & SURF_NOIMPACT )
    return;

  traceEnt = &g_entities[ tr.entityNum ];

  if( !traceEnt->takedamage )
    return;

  if( traceEnt->client )
  {
    if( traceEnt->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
      return;

    if( traceEnt->client->ps.stats[ STAT_HEALTH ] <= 0 )
      return;

    if( !( traceEnt->client->ps.stats[ STAT_STATE ] & SS_GRABBED ) )
    {
      AngleVectors( traceEnt->client->ps.viewangles, dir, NULL, NULL );
      traceEnt->client->ps.stats[ STAT_VIEWLOCK ] = DirToByte( dir );

      //event for client side grab effect
      G_AddPredictableEvent( ent, EV_ALEV4_GRAB, 0 );
    }

    traceEnt->client->ps.stats[ STAT_STATE ] |= SS_GRABBED;

    if( ent->client->ps.weapon == WP_ALEVEL4 )
      traceEnt->client->grabExpiryTime = level.time + ALEVEL4_GRAB_TIME;
  }
}


/*
======================================================================

ALEVEL5

======================================================================
*/

void bounceBallFire( gentity_t *ent )
{
  gentity_t *m;

  m = fire_bounceBall( ent, muzzle, forward );

//  VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );  // "real" physics
}

/*
===============
G_TrampleAttack
===============
*/
void G_TrampleAttack( gentity_t *ent, gentity_t *victim )
{
  int       damage, knockback;
  vec3_t    forward, normal;

  if( ent->client->ps.stats[ STAT_MISC ] <= 0 ||
      !( ent->client->ps.stats[ STAT_STATE ] & SS_CHARGING ) ||
      ent->client->ps.weaponTime )
    return;

  VectorSubtract( victim->s.origin, ent->s.origin, forward );
  VectorNormalize( forward );
  VectorNegate( forward, normal );

  if( !victim->takedamage )
    return;

  ent->s.generic1 = WPM_NOTFIRING;
  WideBloodSpurt( ent, victim, NULL );

  damage = ALEVEL5_TRAMPLE_DMG * ent->client->ps.stats[ STAT_MISC ] /
           ALEVEL5_TRAMPLE_DURATION;

  knockback = ALEVEL5_TRAMPLE_KNOCKBACK * ent->client->ps.stats[ STAT_MISC ] /
           ALEVEL5_TRAMPLE_DURATION;

  G_Damage( victim, ent, ent, forward, victim->s.origin, damage,
            knockback, 0, MOD_ALEVEL5_TRAMPLE );

  ent->client->ps.weaponTime += ALEVEL5_TRAMPLE_REPEAT;

  if( !victim->client )
    ent->client->ps.stats[ STAT_MISC ] = 0;
}

/*
===============
G_StompAttack
===============
*/
void G_StompAttack( gentity_t *ent, gentity_t *victim, int baseDamage, float velDamage )
{
  vec3_t dir;
  float jump;
  int damage;

  if( !victim->takedamage ||
      ent->client->ps.origin[ 2 ] + ent->r.mins[ 2 ] <
      victim->s.origin[ 2 ] + victim->r.maxs[ 2 ] ||
      ( victim->client &&
        victim->client->ps.groundEntityNum == ENTITYNUM_NONE ) )
    return;

  // Deal velocity based damage to target
  jump = BG_Class( ent->client->ps.stats[ STAT_CLASS ] )->jumpMagnitude;
  damage = ( ent->client->pmext.fallVelocity + jump ) *
           -velDamage;

  if( damage < 0 )
    damage = 0;
    
  // Players also get damaged periodically
  if( victim->client &&
      ent->client->lastStompTime + STOMP_REPEAT < level.time )
  {
    ent->client->lastStompTime = level.time;
    damage += baseDamage;
  }
  
  if( damage < 1 )
    return;

  // Crush the victim over a period of time
  VectorSubtract( victim->s.origin, ent->client->ps.origin, dir );
  G_Damage( victim, ent, ent, dir, victim->s.origin,
            damage, 0, 0, MOD_STOMP );
}

//======================================================================

/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint )
{
  vec3_t normal;

  VectorCopy( ent->client->ps.origin, muzzlePoint );
  BG_GetClientNormal( &ent->client->ps, normal );
  VectorMA( muzzlePoint, ent->client->ps.viewheight, normal, muzzlePoint );
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
    AngleVectors( ent->client->ps.viewangles, forward, right, up );
    CalcMuzzlePoint( ent, forward, right, up, muzzle );
  }
  else
  {
    AngleVectors( ent->s.angles2, forward, right, up );
    VectorCopy( ent->s.pos.trBase, muzzle );
  }

  if( BG_Weapon( ent->s.weapon )->usesAmmo & ( 1 << WPM_SECONDARY ) )
  {
    if( BG_Weapon( ent->s.weapon )->ammoRegenDelay > 0 )
      ent->nextAmmoRegenTime = level.time + BG_Weapon( ent->s.weapon )->ammoRegenDelay;
    else if( ent->nextAmmoRegenTime < level.time )
      ent->nextAmmoRegenTime = level.time + BG_Weapon( ent->s.weapon )->ammoRegen;
  }

  // fire the specific weapon
  switch( ent->s.weapon )
  {
    case WP_ABUILD:
    case WP_HBUILD:
      cancelBuildFire( ent );
      break;

    case WP_ALEVEL0:
    case WP_ALEVEL2:
      grappleFire( ent );
      break;

    case WP_ALEVEL1_1:
      spitFire( ent );
      break;

    case WP_ALEVEL3:
      flamerFire( ent );
      break;

    case WP_ALEVEL5:
      bounceBallFire( ent );
      break;

    case WP_SCATTERGUN:
      scattergunBlastFire( ent );
      break;

    default:
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
    AngleVectors( ent->client->ps.viewangles, forward, right, up );
    CalcMuzzlePoint( ent, forward, right, up, muzzle );    
  }
  else
  {
    AngleVectors( ent->turretAim, forward, right, up );
    VectorCopy( ent->s.pos.trBase, muzzle );
  }

  if( BG_Weapon( ent->s.weapon )->usesAmmo & ( 1 << WPM_PRIMARY ) )
  {
    if( BG_Weapon( ent->s.weapon )->ammoRegenDelay > 0 )
      ent->nextAmmoRegenTime = level.time + BG_Weapon( ent->s.weapon )->ammoRegenDelay;
    else if( ent->nextAmmoRegenTime < level.time )
      ent->nextAmmoRegenTime = level.time + BG_Weapon( ent->s.weapon )->ammoRegen;
  }

  // fire the specific weapon
  switch( ent->s.weapon )
  {
    case WP_ALEVEL1_0:
      meleeAttack( ent, ALEVEL1_1_CLAW_RANGE, ALEVEL1_1_CLAW_WIDTH, ALEVEL1_1_CLAW_WIDTH,
                   ALEVEL1_1_CLAW_DMG, MOD_ALEVEL1_1_CLAW );
      break;
    case WP_ALEVEL1_1:
      meleeAttack( ent, ALEVEL1_1_CLAW_RANGE, ALEVEL1_1_CLAW_WIDTH, ALEVEL1_1_CLAW_WIDTH,
                   ALEVEL1_1_CLAW_DMG, MOD_ALEVEL1_1_CLAW );
      break;
    case WP_ALEVEL2:
      kamikazeExplode( ent );
      break;
    case WP_ALEVEL3:
      meleeAttack( ent, ALEVEL3_CLAW_RANGE, ALEVEL3_CLAW_WIDTH, ALEVEL3_CLAW_WIDTH,
                   ALEVEL3_CLAW_DMG, MOD_ALEVEL3_CLAW );
      break;
    case WP_ALEVEL4:
      meleeAttack( ent, ALEVEL4_CLAW_RANGE, ALEVEL4_CLAW_WIDTH, ALEVEL4_CLAW_WIDTH,
                   ALEVEL4_CLAW_DMG, MOD_ALEVEL4_CLAW );
      break;
    case WP_ALEVEL5:
      meleeAttack( ent, ALEVEL5_CLAW_RANGE, ALEVEL5_CLAW_WIDTH,
                   ALEVEL5_CLAW_HEIGHT, ALEVEL5_CLAW_DMG, MOD_ALEVEL5_CLAW );
      break;

    case WP_BLASTER:
      blasterFire( ent );
      break;
    case WP_HANDGUN:
      bulletFire( ent, HANDGUN_SPREAD, HANDGUN_DMG, HANDGUN_KNOCKBACK, MOD_HANDGUN );
      break;
    case WP_MACHINEGUN:
      bulletFire( ent, RIFLE_SPREAD, RIFLE_DMG, RIFLE_KNOCKBACK, MOD_MACHINEGUN );
      break;
    case WP_SCATTERGUN:
      scattergunShellFire( ent );
      break;
    case WP_SHOTGUN:
      shotgunFire( ent );
      break;
    case WP_CHAINGUN:
      bulletFire( ent, CHAINGUN_SPREAD, CHAINGUN_DMG, CHAINGUN_KNOCKBACK, MOD_CHAINGUN );
      break;
    case WP_PULSE_RIFLE:
      pulseRifleFire( ent );
      break;
    case WP_MASS_DRIVER:
      massDriverFire( ent );
      break;
    case WP_LUCIFER_CANNON:
      lcannonFire( ent );
      break;
    case WP_LAS_GUN:
      lasGunFire( ent );
      break;
    case WP_PAIN_SAW:
      painSawFire( ent );
      break;
    case WP_GAS_GRENADE:
      throwGasGrenade( ent );
      break;
    case WP_SPORE_GRENADE:
      throwSporeGrenade( ent );
      break;
    case WP_SPIKE_GRENADE:
      throwSpikeGrenade( ent );
      break;
    case WP_SHOCK_GRENADE:
      throwShockGrenade( ent );
      break;
    case WP_NERVE_GRENADE:
      throwNerveGrenade( ent );
      break;
    case WP_FRAG_GRENADE:
      throwFragGrenade( ent );
      break;

    case WP_LOCKBLOB_LAUNCHER:
      lockBlobLauncherFire( ent );
      break;
    case WP_HIVE:
      hiveFire( ent );
      break;
    case WP_TESLAGEN:
      teslaFire( ent );
      break;
    case WP_MGTURRET:
      bulletFire( ent, MGTURRET_SPREAD, MGTURRET_DMG, MGTURRET_KNOCKBACK, MOD_MGTURRET );
      break;

    case WP_ABUILD:
      buildFire( ent, MN_A_BUILD );
      break;
    case WP_HBUILD:
      buildFire( ent, MN_H_BUILD );
      break;
    default:
      break;
  }
}

