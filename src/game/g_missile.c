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

#include "g_local.h"

#define MISSILE_PRESTEP_TIME  50

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace )
{
  vec3_t  velocity;
  float dot;
  int   hitTime;

  // reflect the velocity on the trace plane
  hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
  BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
  dot = DotProduct( velocity, trace->plane.normal );
  VectorMA( velocity, -2 * dot, trace->plane.normal, ent->s.pos.trDelta );

  if( ent->s.eFlags & EF_BOUNCE_HALF )
  {
    VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
    // check for stop
    if( trace->plane.normal[ 2 ] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 )
    {
      G_SetOrigin( ent, trace->endpos );
      return;
    }
  }

  VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin );
  VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
  ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent )
{
  vec3_t    dir;
  vec3_t    origin;

  BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
  SnapVector( origin );
  G_SetOrigin( ent, origin );

  // we don't have a valid direction, so just point straight up
  dir[ 0 ] = dir[ 1 ] = 0;
  dir[ 2 ] = 1;

  ent->s.eType = ET_GENERAL;

  if( ent->s.weapon != WP_LOCKBLOB_LAUNCHER &&
      ent->s.weapon != WP_ALEVEL3 )
    G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

  ent->freeAfterEvent = qtrue;

  // splash damage
  if( ent->splashDamage || ent->splashKnockback )
    G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashKnockback,
                    ent->splashRadius, ent, ent->splashMethodOfDeath );

  trap_LinkEntity( ent );
}

void AHive_ReturnToHive( gentity_t *self );

/*
================
G_MissileImpact

================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace )
{
  gentity_t   *other, *attacker;
  qboolean    returnAfterDamage = qfalse;
  vec3_t      dir;

  other = &g_entities[ trace->entityNum ];
  attacker = &g_entities[ ent->r.ownerNum ];

  // check for bounce
  if( !other->takedamage &&
      ( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) )
  {
    G_BounceMissile( ent, trace );

    //only play a sound if requested
    if( !( ent->s.eFlags & EF_NO_BOUNCE_SOUND ) )
      G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );

    return;
  }

  if( !strcmp( ent->classname, "grenade" ) )
  {
    //grenade doesn't explode on impact
    G_BounceMissile( ent, trace );

    //only play a sound if requested
    if( !( ent->s.eFlags & EF_NO_BOUNCE_SOUND ) )
      G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );

    return;
  }
  else if( !strcmp( ent->classname, "lockblob" ) )
  {
    if( other->client && other->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
    {
      other->client->ps.stats[ STAT_STATE ] |= SS_BLOBLOCKED;
      other->client->lastLockTime = level.time;
      AngleVectors( other->client->ps.viewangles, dir, NULL, NULL );
      other->client->ps.stats[ STAT_VIEWLOCK ] = DirToByte( dir );
    }
  }
  else if( !strcmp( ent->classname, "hive" ) )
  {
    if( other->s.eType == ET_BUILDABLE && other->s.modelindex == BA_A_HIVE )
    {
      if( !ent->parent )
        G_Printf( S_COLOR_YELLOW "WARNING: hive entity has no parent in G_MissileImpact\n" );
      else
        ent->parent->active = qfalse;

      G_FreeEntity( ent );
      return;
    }
    else
    {
      //prevent collision with the client when returning
      ent->r.ownerNum = other->s.number;

      ent->think = G_ExplodeMissile;
      ent->nextthink = level.time + FRAMETIME;

      //only damage humans
      if( other->client && other->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
        returnAfterDamage = qtrue;
      else
        return;
    }
  }
  else if( !strcmp( ent->classname, "hook" ) )
  {
    gentity_t *nent;
    vec3_t v;

    nent = G_Spawn();
    if( other->takedamage && other->client )
    {
      G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
      nent->s.otherEntityNum = other->s.number;

      ent->enemy = other;

      v[0] = other->r.currentOrigin[0] + ( other->r.mins[0] + other->r.maxs[0] ) * 0.5;
      v[1] = other->r.currentOrigin[1] + ( other->r.mins[1] + other->r.maxs[1] ) * 0.5;
      v[2] = other->r.currentOrigin[2] + ( other->r.mins[2] + other->r.maxs[2] ) * 0.5;

      SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
    }
    else
    {
      VectorCopy( trace->endpos, v );
      G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
      ent->enemy = NULL;
    }

    SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

    nent->freeAfterEvent = qtrue;
    // change over to a normal entity right at the point of impact
    nent->s.eType = ET_GENERAL;
    ent->s.eType = ET_GRAPPLE;

    G_SetOrigin( ent, v );
    G_SetOrigin( nent, v );

    ent->think = G_HookThink;
    ent->nextthink = level.time + FRAMETIME;

    ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
    VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint );

    trap_LinkEntity( ent );
    trap_LinkEntity( nent );

    return;
  }

  // impact damage
  if( other->takedamage )
  {
    // FIXME: wrong damage direction?
    if( ent->damage || ent-> knockback )
    {
      vec3_t  velocity;

      BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
      if( VectorLength( velocity ) == 0 )
        velocity[ 2 ] = 1;  // stepped on a grenade

      G_Damage( other, ent, attacker, velocity, ent->s.origin,
        ent->damage, ent->knockback, 0, ent->methodOfDeath );
    }
  }

  if( returnAfterDamage )
    return;

  // is it cheaper in bandwidth to just remove this ent and create a new
  // one, rather than changing the missile into the explosion?

  if( other->takedamage && 
      ( other->s.eType == ET_PLAYER || other->s.eType == ET_BUILDABLE ) )
  {
    G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
    ent->s.otherEntityNum = other->s.number;
  }
  else if( trace->surfaceFlags & SURF_METALSTEPS )
    G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
  else
    G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );

  ent->freeAfterEvent = qtrue;

  // change over to a normal entity right at the point of impact
  ent->s.eType = ET_GENERAL;

  SnapVectorTowards( trace->endpos, ent->s.pos.trBase );  // save net bandwidth

  G_SetOrigin( ent, trace->endpos );

  // splash damage (doesn't apply to person directly hit)
  if( ent->splashDamage || ent->splashKnockback )
    G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashKnockback,
                    ent->splashRadius, other, ent->splashMethodOfDeath );

  trap_LinkEntity( ent );
}


/*
================
G_RunMissile

================
*/
void G_RunMissile( gentity_t *ent )
{
  vec3_t    origin;
  trace_t   tr;
  int       passent;
  qboolean  impact = qfalse;

  // get current position
  BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

  // ignore interactions with the missile owner
  passent = ent->r.ownerNum;

  // general trace to see if we hit anything at all
  trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs,
              origin, passent, ent->clipmask );

  if( tr.startsolid || tr.allsolid )
  {
    tr.fraction = 0.0f;
    VectorCopy( ent->r.currentOrigin, tr.endpos );
  }

  if( tr.fraction < 1.0f )
  {
    if( !ent->pointAgainstWorld || tr.contents & CONTENTS_BODY )
    {
      // We hit an entity or we don't care
      impact = qtrue;
    }
    else
    {
      trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, origin, 
                  passent, ent->clipmask );

      if( tr.fraction < 1.0f )
      {
        // Hit the world with point trace
        impact = qtrue;
      }
      else
      {
        if( tr.contents & CONTENTS_BODY )
        {
          // Hit an entity
          impact = qtrue;
        }
        else
        {
          trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, 
                      origin, passent, CONTENTS_BODY );

          if( tr.fraction < 1.0f )
            impact = qtrue;
        }
      }
    }
  }

  VectorCopy( tr.endpos, ent->r.currentOrigin );

  if( impact )
  {
    // Never explode or bounce on sky
    if( tr.surfaceFlags & SURF_NOIMPACT )
    {
      // If grapple, reset owner
      if( ent->parent && ent->parent->client && ent->parent->client->hook == ent )
      {
        ent->parent->client->hook = NULL;
      }
      G_FreeEntity( ent );
      return;
    }

    G_MissileImpact( ent, &tr );

    if( ent->s.eType != ET_MISSILE )
      return;   // exploded
  }

  ent->r.contents = CONTENTS_SOLID; //trick trap_LinkEntity into...
  trap_LinkEntity( ent );
  ent->r.contents = 0; //...encoding bbox information

  // check think function after bouncing
  G_RunThink( ent );
}


//=============================================================================

/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t	*hook;

  VectorNormalize (dir);

  hook = G_Spawn();
  hook->classname = "hook";
  hook->nextthink = level.time + 10000;
  hook->think = G_HookFree;
  hook->s.eType = ET_MISSILE;
  hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  hook->s.weapon = WP_ALEVEL0;
  hook->r.ownerNum = self->s.number;
  hook->methodOfDeath = MOD_UNKNOWN; //doesn't do damage so will never kill
  hook->clipmask = MASK_SHOT;
  hook->parent = self;
  hook->target_ent = NULL;

  hook->s.pos.trType = TR_LINEAR;
  hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
  hook->s.otherEntityNum = self->s.number; // use to match beam in client
  VectorCopy( start, hook->s.pos.trBase );
  VectorScale( dir, ALEVEL0_GRAPPLE_SPEED, hook->s.pos.trDelta );
  SnapVector( hook->s.pos.trDelta );			// save net bandwidth
  VectorCopy( start, hook->r.currentOrigin );

  self->client->hook = hook;

  return hook;
}



/*
=================
fire_flamer

=================
*/
gentity_t *fire_flamer( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;
  vec3_t    pvel;

  VectorNormalize (dir);

  bolt = G_Spawn();
  bolt->classname = "flame";
  bolt->pointAgainstWorld = qfalse;
  bolt->nextthink = level.time + ALEVEL3_FLAME_LIFETIME;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_ALEVEL3;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = ALEVEL3_FLAME_DMG;
  bolt->knockback = 0;
  bolt->splashDamage = 0;
  bolt->splashKnockback = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_ALEVEL3_FLAME;
  bolt->splashMethodOfDeath = MOD_ALEVEL3_FLAME;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[ 0 ] = bolt->r.mins[ 1 ] = bolt->r.mins[ 2 ] = -ALEVEL3_FLAME_SIZE;
  bolt->r.maxs[ 0 ] = bolt->r.maxs[ 1 ] = bolt->r.maxs[ 2 ] = ALEVEL3_FLAME_SIZE;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( self->client->ps.velocity, ALEVEL3_FLAME_LAG, pvel );
  VectorMA( pvel, ALEVEL3_FLAME_SPEED, dir, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

//=============================================================================

/*
=================
fire_blaster

=================
*/
gentity_t *fire_blaster( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize (dir);

  bolt = G_Spawn();
  bolt->classname = "blaster";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + 10000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_BLASTER;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = BLASTER_DMG;
  bolt->knockback = BLASTER_KNOCKBACK;
  bolt->splashDamage = 0;
  bolt->splashKnockback = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_BLASTER;
  bolt->splashMethodOfDeath = MOD_BLASTER;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[ 0 ] = bolt->r.mins[ 1 ] = bolt->r.mins[ 2 ] = -BLASTER_SIZE;
  bolt->r.maxs[ 0 ] = bolt->r.maxs[ 1 ] = bolt->r.maxs[ 2 ] = BLASTER_SIZE;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, BLASTER_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

//=============================================================================

/*
=================
fire_scattergun

=================
*/
gentity_t *fire_scattergun( gentity_t *self, vec3_t start, vec3_t dir, int charge )
{
  gentity_t *bolt;
  float scale = (float)charge / SCATTERGUN_BLAST_CHARGE_MAX;

  VectorNormalize( dir );

  bolt = G_Spawn( );
  bolt->classname = "scattergun";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + SCATTERGUN_BLAST_LIFETIME;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_SCATTERGUN;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = 0;
  bolt->knockback = SCATTERGUN_BLAST_KNOCKBACK * scale;
  bolt->splashDamage = 0;
  bolt->splashKnockback = SCATTERGUN_BLAST_KNOCKBACK * scale;
  bolt->splashRadius = SCATTERGUN_BLAST_RADIUS;
  bolt->methodOfDeath = MOD_SCATTERGUN_BLAST;
  bolt->splashMethodOfDeath = MOD_SCATTERGUN_BLAST;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[ 0 ] = bolt->r.mins[ 1 ] = bolt->r.mins[ 2 ] = -SCATTERGUN_BLAST_SIZE;
  bolt->r.maxs[ 0 ] = bolt->r.maxs[ 1 ] = bolt->r.maxs[ 2 ] = -bolt->r.mins[ 0 ];

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, SCATTERGUN_BLAST_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

//=============================================================================

/*
=================
fire_pulseRifle

=================
*/
gentity_t *fire_pulseRifle( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize (dir);

  bolt = G_Spawn();
  bolt->classname = "pulse";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + 10000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_PULSE_RIFLE;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = PRIFLE_DMG;
  bolt->knockback = PRIFLE_KNOCKBACK;
  bolt->splashDamage = 0;
  bolt->splashKnockback = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_PRIFLE;
  bolt->splashMethodOfDeath = MOD_PRIFLE;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[ 0 ] = bolt->r.mins[ 1 ] = bolt->r.mins[ 2 ] = -PRIFLE_SIZE;
  bolt->r.maxs[ 0 ] = bolt->r.maxs[ 1 ] = bolt->r.maxs[ 2 ] = PRIFLE_SIZE;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, PRIFLE_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

//=============================================================================

/*
=================
fire_luciferCannon

=================
*/
gentity_t *fire_luciferCannon( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;
  float charge;

  VectorNormalize( dir );

  bolt = G_Spawn( );
  bolt->classname = "lcannon";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + 10000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_LUCIFER_CANNON;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = LCANNON_DAMAGE;
  bolt->knockback = LCANNON_KNOCKBACK;
  bolt->splashDamage = LCANNON_DAMAGE;
  bolt->splashKnockback = LCANNON_KNOCKBACK;
  bolt->splashRadius = LCANNON_RADIUS;
  bolt->methodOfDeath = MOD_LCANNON;
  bolt->splashMethodOfDeath = MOD_LCANNON_SPLASH;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[ 0 ] = bolt->r.mins[ 1 ] = bolt->r.mins[ 2 ] = -LCANNON_SIZE;
  bolt->r.maxs[ 0 ] = bolt->r.maxs[ 1 ] = bolt->r.maxs[ 2 ] = -bolt->r.mins[ 0 ];
  
  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, LCANNON_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

/*
=================
launch_grenade

=================
*/
gentity_t *launch_grenade( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize( dir );

  bolt = G_Spawn( );
  bolt->classname = "grenade";
  bolt->pointAgainstWorld = qfalse;
  bolt->nextthink = level.time + 5000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_FRAG_GRENADE;
  bolt->s.eFlags = EF_BOUNCE_HALF;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = GRENADE_DAMAGE;
  bolt->knockback = GRENADE_KNOCKBACK;
  bolt->splashDamage = GRENADE_DAMAGE;
  bolt->splashKnockback = GRENADE_KNOCKBACK;
  bolt->splashRadius = GRENADE_RANGE;
  bolt->methodOfDeath = MOD_FRAG_GRENADE;
  bolt->splashMethodOfDeath = MOD_FRAG_GRENADE;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;
  bolt->r.mins[ 0 ] = bolt->r.mins[ 1 ] = bolt->r.mins[ 2 ] = -3.0f;
  bolt->r.maxs[ 0 ] = bolt->r.maxs[ 1 ] = bolt->r.maxs[ 2 ] = 3.0f;
  bolt->s.time = level.time;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, GRENADE_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth

  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

gentity_t *launch_gasGrenade( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  bolt = launch_grenade( self, start, dir );

  bolt->s.weapon = WP_GAS_GRENADE;
  bolt->methodOfDeath = MOD_GAS_GRENADE;
  bolt->splashMethodOfDeath = MOD_GAS_GRENADE;

  return bolt;
}

gentity_t *launch_sporeGrenade( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  bolt = launch_grenade( self, start, dir );

  bolt->s.weapon = WP_SPORE_GRENADE;
  bolt->methodOfDeath = MOD_SPORE_GRENADE;
  bolt->splashMethodOfDeath = MOD_SPORE_GRENADE;

  return bolt;
}

gentity_t *launch_spikeGrenade( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  bolt = launch_grenade( self, start, dir );

  bolt->s.weapon = WP_SPIKE_GRENADE;
  bolt->methodOfDeath = MOD_SPIKE_GRENADE;
  bolt->splashMethodOfDeath = MOD_SPIKE_GRENADE;

  return bolt;
}

gentity_t *launch_shockGrenade( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  bolt = launch_grenade( self, start, dir );

  bolt->s.weapon = WP_SHOCK_GRENADE;
  bolt->methodOfDeath = MOD_SHOCK_GRENADE;
  bolt->splashMethodOfDeath = MOD_SHOCK_GRENADE;

  return bolt;
}

gentity_t *launch_nerveGrenade( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  bolt = launch_grenade( self, start, dir );

  bolt->s.weapon = WP_NERVE_GRENADE;
  bolt->methodOfDeath = MOD_NERVE_GRENADE;
  bolt->splashMethodOfDeath = MOD_NERVE_GRENADE;

  return bolt;
}

gentity_t *launch_fragGrenade( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  bolt = launch_grenade( self, start, dir );

  bolt->s.weapon = WP_FRAG_GRENADE;
  bolt->methodOfDeath = MOD_FRAG_GRENADE;
  bolt->splashMethodOfDeath = MOD_FRAG_GRENADE;

  return bolt;
}

//=============================================================================



/*
================
AHive_SearchAndDestroy

Adjust the trajectory to point towards the target
================
*/
void AHive_SearchAndDestroy( gentity_t *self )
{
  vec3_t    dir;
  trace_t   tr;
  gentity_t *ent;
  int       i;
  float     d, nearest;

  if( level.time > self->timestamp )
  {
    VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
    self->s.pos.trType = TR_STATIONARY;
    self->s.pos.trTime = level.time;

    self->think = G_ExplodeMissile;
    self->nextthink = level.time + 50;
    self->parent->active = qfalse; //allow the parent to start again
    return;
  }

  nearest = DistanceSquared( self->r.currentOrigin, self->target_ent->r.currentOrigin );
  //find the closest human
  for( i = 0; i < MAX_CLIENTS; i++ )
  {
    ent = &g_entities[ i ];

    if( ent->flags & FL_NOTARGET )
      continue;

    if( ent->client &&
        ent->health > 0 &&   
        ent->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS &&
        nearest > (d = DistanceSquared( ent->r.currentOrigin, self->r.currentOrigin ) ) )
    {
      trap_Trace( &tr, self->r.currentOrigin, self->r.mins, self->r.maxs,
                  ent->r.currentOrigin, self->r.ownerNum, self->clipmask );
      if( tr.entityNum != ENTITYNUM_WORLD )
      {
        nearest = d;
        self->target_ent = ent;
      }
    }
  }
    VectorSubtract( self->target_ent->r.currentOrigin, self->r.currentOrigin, dir );
    VectorNormalize( dir );

    //change direction towards the player
    VectorScale( dir, HIVE_SPEED, self->s.pos.trDelta );
    SnapVector( self->s.pos.trDelta );      // save net bandwidth
    VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
    self->s.pos.trTime = level.time;

    self->nextthink = level.time + HIVE_DIR_CHANGE_PERIOD;
}

/*
=================
fire_hive
=================
*/
gentity_t *fire_hive( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize ( dir );

  bolt = G_Spawn( );
  bolt->classname = "hive";
  bolt->pointAgainstWorld = qfalse;
  bolt->nextthink = level.time + HIVE_DIR_CHANGE_PERIOD;
  bolt->think = AHive_SearchAndDestroy;
  bolt->s.eType = ET_MISSILE;
  bolt->s.eFlags |= EF_BOUNCE | EF_NO_BOUNCE_SOUND;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_HIVE;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = HIVE_DMG;
  bolt->knockback = 0;
  bolt->splashDamage = 0;
  bolt->splashKnockback = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_SWARM;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = self->target_ent;
  bolt->timestamp = level.time + HIVE_LIFETIME;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, HIVE_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth
  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

//=============================================================================

/*
=================
fire_lockblob
=================
*/
gentity_t *fire_lockblob( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize ( dir );

  bolt = G_Spawn( );
  bolt->classname = "lockblob";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + 15000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_LOCKBLOB_LAUNCHER;
  bolt->s.generic1 = WPM_PRIMARY; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = 0;
  bolt->knockback = 0;
  bolt->splashDamage = 0;
  bolt->splashKnockback = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_UNKNOWN; //doesn't do damage so will never kill
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_LINEAR;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, 500, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth
  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

/*
=================
fire_spit
=================
*/
gentity_t *fire_spit( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize ( dir );

  bolt = G_Spawn( );
  bolt->classname = "lockblob";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + 15000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_ALEVEL1_1;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = 0;
  bolt->knockback = 0;
  bolt->splashDamage = 0;
  bolt->splashKnockback = 0;
  bolt->splashRadius = 0;
  bolt->methodOfDeath = MOD_UNKNOWN;
  bolt->splashMethodOfDeath = MOD_UNKNOWN;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, ALEVEL1_1_SPIT_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth
  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

/*
=================
fire_paraLockBlob
=================
*/
gentity_t *fire_paraLockBlob( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize ( dir );

  bolt = G_Spawn( );
  bolt->classname = "lockblob";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + 15000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_LOCKBLOB_LAUNCHER;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = 0;
  bolt->knockback = 0;
  bolt->splashDamage = 0;
  bolt->splashKnockback = 0;
  bolt->splashRadius = 0;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, LOCKBLOB_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth
  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

/*
=================
fire_bounceBall
=================
*/
gentity_t *fire_bounceBall( gentity_t *self, vec3_t start, vec3_t dir )
{
  gentity_t *bolt;

  VectorNormalize ( dir );

  bolt = G_Spawn( );
  bolt->classname = "bounceball";
  bolt->pointAgainstWorld = qtrue;
  bolt->nextthink = level.time + 3000;
  bolt->think = G_ExplodeMissile;
  bolt->s.eType = ET_MISSILE;
  bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
  bolt->s.weapon = WP_ALEVEL5;
  bolt->s.generic1 = self->s.generic1; //weaponMode
  bolt->r.ownerNum = self->s.number;
  bolt->parent = self;
  bolt->damage = ALEVEL5_BOUNCEBALL_DMG;
  bolt->knockback = ALEVEL5_BOUNCEBALL_KNOCKBACK;
  bolt->splashDamage = ALEVEL5_BOUNCEBALL_DMG;
  bolt->splashKnockback = ALEVEL5_BOUNCEBALL_KNOCKBACK;
  bolt->splashRadius = ALEVEL5_BOUNCEBALL_RADIUS;
  bolt->methodOfDeath = MOD_ALEVEL5_BOUNCEBALL;
  bolt->splashMethodOfDeath = MOD_ALEVEL5_BOUNCEBALL;
  bolt->clipmask = MASK_SHOT;
  bolt->target_ent = NULL;

  bolt->s.pos.trType = TR_GRAVITY;
  bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;   // move a bit on the very first frame
  VectorCopy( start, bolt->s.pos.trBase );
  VectorScale( dir, ALEVEL5_BOUNCEBALL_SPEED, bolt->s.pos.trDelta );
  SnapVector( bolt->s.pos.trDelta );      // save net bandwidth
  VectorCopy( start, bolt->r.currentOrigin );

  return bolt;
}

