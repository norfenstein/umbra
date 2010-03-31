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

/*
============
AddScore

Adds score to the client
============
*/
void AddScore( gentity_t *ent, int score )
{
  if( !ent->client )
    return;

  ent->client->ps.persistant[ PERS_SCORE ] += score;
  CalculateRanks( );
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker )
{

  if ( attacker && attacker != self )
    self->client->ps.stats[ STAT_VIEWLOCK ] = attacker - g_entities;
  else if( inflictor && inflictor != self )
    self->client->ps.stats[ STAT_VIEWLOCK ] = inflictor - g_entities;
  else
    self->client->ps.stats[ STAT_VIEWLOCK ] = self - g_entities;
}

// these are just for logging, the client prints its own messages
char *modNames[ ] =
{
  "MOD_UNKNOWN",
  "MOD_SHOTGUN",
  "MOD_BLASTER",
  "MOD_PAINSAW",
  "MOD_MACHINEGUN",
  "MOD_CHAINGUN",
  "MOD_PRIFLE",
  "MOD_MDRIVER",
  "MOD_LASGUN",
  "MOD_LCANNON",
  "MOD_LCANNON_SPLASH",
  "MOD_GRENADE",
  "MOD_WATER",
  "MOD_SLIME",
  "MOD_LAVA",
  "MOD_CRUSH",
  "MOD_TELEFRAG",
  "MOD_FALLING",
  "MOD_SUICIDE",
  "MOD_TARGET_LASER",
  "MOD_TRIGGER_HURT",

  "MOD_ABUILDER_CLAW",
  "MOD_ALEVEL0_BITE",
  "MOD_ALEVEL1_0_CLAW",
  "MOD_ALEVEL1_1_CLAW",
  "MOD_ALEVEL2_BITE",
  "MOD_ALEVEL2_KAMIKAZE",
  "MOD_ALEVEL3_CLAW",
  "MOD_ALEVEL3_FLAME",
  "MOD_ALEVEL4_CLAW",
  "MOD_ALEVEL5_BOUNCEBALL",
  "MOD_ALEVEL5_CLAW",
  "MOD_ALEVEL5_TRAMPLE",
  "MOD_STOMP",

  "MOD_POISON",
  "MOD_SWARM",

  "MOD_HSPAWN",
  "MOD_TESLAGEN",
  "MOD_MGTURRET",
  "MOD_REACTOR",

  "MOD_ASPAWN",
  "MOD_ATUBE",
  "MOD_OVERMIND",
  "MOD_DECONSTRUCT"
};

/*
==================
G_RewardAttackers

Function to distribute rewards to entities that killed this one.
Returns the total damage dealt.
==================
*/
void G_RewardAttackers( gentity_t *self )
{
  float score;
  int team, i, maxHealth = 0;
  qboolean isSpawn = qfalse;

  if( self->client )
  {
    // players are worth 1 + cost score
    score = ( 1 + BG_Class( self->client->ps.stats[ STAT_CLASS ] )->cost );
    team = self->client->pers.teamSelection;
    maxHealth = self->client->ps.stats[ STAT_MAX_HEALTH ];
  }
  else if( self->s.eType == ET_BUILDABLE )
  {
    const buildableAttributes_t *buildable = BG_Buildable( self->s.modelindex );

    // buildables are worth buildPoints score
    score = buildable->buildPoints;
    isSpawn = buildable->number == BA_A_SPAWN || buildable->number == BA_H_SPAWN;

    // only give partial credits for a buildable not yet completed
    if( !self->spawned )
    {
      score *= (float)( level.time - self->buildTime ) /
          buildable->buildTime;
    }

    team = self->buildableTeam;
    maxHealth = buildable->health;
  }
  else
  {
    return;
  }

  // Distribute credits and score, and empty the damage accounts array
  for( i = 0; i < MAX_CLIENTS; i++ )
  {
    gentity_t *player = g_entities + i;
    float frags = 0;
    float fraction = (float)self->damageAccounts[ i ] / maxHealth;
    
    if( !player->client || !self->damageAccounts[ i ] )
      continue;

    if( self->s.eType != ET_BUILDABLE )
    {
      if( fraction > 0 )
      {
        // frags earned are equal to: 1 - killer's cost * 0.2 + target's cost * 0.2
        frags = 1.0 - ( 1 + (float)BG_Class( player->client->ps.stats[ STAT_CLASS ] )->cost ) * 0.2 +
                      ( 1 + (float)BG_Class( self->client->ps.stats[ STAT_CLASS ] )->cost ) * 0.2;
      }
      else
      {
        // teamkills deduct the full cost of the target
        frags = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->cost;
      }
    }
    else if( isSpawn )
    {
      // spawns are always worth 1; all other buildables are worth 0
      frags = 1;
    }

    if( player->client->ps.stats[ STAT_TEAM ] == team &&
        self != player &&
        fraction > 0 )
    {
      // never reward for team kills
      fraction = 0;
    }

    if( g_debugDamage.integer )
      G_Printf( "%i: client:%i fraction: %f score:%f credits: %f\n",
        level.time,
        i,
        fraction,
        rint( score * fraction ),
        rint( frags * fraction * CREDITS_PER_FRAG ) );

    AddScore( player, rint( score * fraction ) );

    G_AddCreditToClient( player->client, frags * rint( fraction * CREDITS_PER_FRAG ), qtrue );

    self->damageAccounts[ i ] = 0;
  }
}

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
{
  gentity_t *ent;
  int       anim;
  int       killer;
  int       i;
  char      *killerName, *obit;
  float     totalDamage = 0.0f;

  if( self->client->ps.pm_type == PM_DEAD )
    return;

  if( level.intermissiontime )
    return;

  self->client->ps.pm_type = PM_DEAD;
  self->suicideTime = 0;

  if( self->client && self->client->hook )
  {
    G_HookFree( self->client->hook );
  }

  if( attacker )
  {
    killer = attacker->s.number;

    if( attacker->client )
      killerName = attacker->client->pers.netname;
    else
      killerName = "<world>";
  }
  else
  {
    killer = ENTITYNUM_WORLD;
    killerName = "<world>";
  }

  if( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[0] ) )
    // fall back on the number
    obit = va( "%d", meansOfDeath );
  else
    obit = modNames[ meansOfDeath ];

  G_LogPrintf( "Die: %d %d %s: %s" S_COLOR_WHITE " killed %s\n",
    killer,
    self - g_entities,
    obit,
    killerName,
    self->client->pers.netname );

  // deactivate all upgrades
  for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
    BG_DeactivateUpgrade( i, self->client->ps.stats );

  // broadcast the death event to everyone
  ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
  ent->s.eventParm = meansOfDeath;
  ent->s.otherEntityNum = self->s.number;
  ent->s.otherEntityNum2 = killer;
  ent->r.svFlags = SVF_BROADCAST; // send to everyone

  self->enemy = attacker;
  self->client->ps.persistant[ PERS_KILLED ]++;

  if( attacker && attacker->client )
  {
    attacker->client->lastkilled_client = self->s.number;

    if( OnSameTeam( self, attacker ) && attacker == self )
    {
      // unless kamikaze, suicide always subtracts 1 frag and 1 score
      if( meansOfDeath != MOD_ALEVEL2_KAMIKAZE )
      {
        G_AddCreditToClient( attacker->client, -CREDITS_PER_FRAG, qtrue );
        AddScore( attacker, -1 );
      }

      // forgive teammates for friendly fire
      for( i = 0; i < MAX_CLIENTS; i++ )
      {
        if( self->damageAccounts[ i ] < 0 )
          self->damageAccounts[ i ] = 0;
      }
    }
  }

  // give credits for killing this player
  G_RewardAttackers( self );

  ScoreboardMessage( self );    // show scores

  // send updated scores to any clients that are following this one,
  // or they would get stale scoreboards
  for( i = 0 ; i < level.maxclients ; i++ )
  {
    gclient_t *client;

    client = &level.clients[ i ];
    if( client->pers.connected != CON_CONNECTED )
      continue;

    if( client->sess.spectatorState == SPECTATOR_NOT )
      continue;

    if( client->sess.spectatorClient == self->s.number )
      ScoreboardMessage( g_entities + i );
  }

  VectorCopy( self->s.origin, self->client->pers.lastDeathLocation );

  self->takedamage = qfalse; // can still be gibbed

  self->s.weapon = WP_NONE;
  self->r.contents = CONTENTS_CORPSE;

  self->s.angles[ PITCH ] = 0;
  self->s.angles[ ROLL ] = 0;
  self->s.angles[ YAW ] = self->s.apos.trBase[ YAW ];
  LookAtKiller( self, inflictor, attacker );

  VectorCopy( self->s.angles, self->client->ps.viewangles );

  self->s.loopSound = 0;

  self->r.maxs[ 2 ] = -8;

  // don't allow respawn until the death anim is done
  // g_forcerespawn may force spawning at some later time
  self->client->respawnTime = level.time + 1700;

  // clear misc
  memset( self->client->ps.misc, 0, sizeof( self->client->ps.misc ) );

  {
    // normal death
    static int i;

    if( !( self->client->ps.persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    {
      switch( i )
      {
        case 0:
          anim = BOTH_DEATH1;
          break;
        case 1:
          anim = BOTH_DEATH2;
          break;
        case 2:
        default:
          anim = BOTH_DEATH3;
          break;
      }
    }
    else
    {
      switch( i )
      {
        case 0:
          anim = NSPA_DEATH1;
          break;
        case 1:
          anim = NSPA_DEATH2;
          break;
        case 2:
        default:
          anim = NSPA_DEATH3;
          break;
      }
    }

    self->client->ps.legsAnim =
      ( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

    if( !( self->client->ps.persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    {
      self->client->ps.torsoAnim =
        ( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
    }

    // use own entityid if killed by non-client to prevent uint8_t overflow
    G_AddEvent( self, EV_DEATH1 + i,
      ( killer < MAX_CLIENTS ) ? killer : self - g_entities );

    // globally cycle through the different death animations
    i = ( i + 1 ) % 3;
  }

  trap_LinkEntity( self );
}

/*
============
T_Damage

targ    entity that is being damaged
inflictor entity that is causing the damage
attacker  entity that caused the inflictor to damage targ
  example: targ=monster, inflictor=rocket, attacker=player

dir     direction of the attack for knockback
point   point at which the damage is being inflicted, used for headshots
damage    amount of damage being inflicted
knockback force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags    these flags are used to control how T_Damage works
  DAMAGE_RADIUS     damage was indirect (from a nearby explosion)
  DAMAGE_NO_ARMOR     armor does not protect from this damage
  DAMAGE_NO_PROTECTION  kills godmode, armor, everything
============
*/

// team is the team that is immune to this damage
void G_SelectiveDamage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
         vec3_t dir, vec3_t point, int damage, int knockback, int dflags, int mod, int team )
{
  if( targ->client && ( team != targ->client->ps.stats[ STAT_TEAM ] ) )
    G_Damage( targ, inflictor, attacker, dir, point, damage, knockback, dflags, mod );
}

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
         vec3_t dir, vec3_t point, int damage, int knockback, int dflags, int mod )
{
  gclient_t *client;
  int     take;
  int     save;
  int     asave = 0;

  // Can't deal damage sometimes
  if( !targ->takedamage || targ->health <= 0 || level.intermissionQueued )
    return;

  if( !inflictor )
    inflictor = &g_entities[ ENTITYNUM_WORLD ];

  if( !attacker )
    attacker = &g_entities[ ENTITYNUM_WORLD ];

  // shootable doors / buttons don't actually have any health
  if( targ->s.eType == ET_MOVER )
  {
    if( targ->use && ( targ->moverState == MOVER_POS1 ||
                       targ->moverState == ROTATOR_POS1 ) )
      targ->use( targ, inflictor, attacker );

    return;
  }

  client = targ->client;
  if( client && client->noclip )
    return;

  if( !dir )
    knockback = 0;
  else
    VectorNormalize( dir );

  if( targ->client )
  {
    knockback = (int)( (float)knockback *
      BG_Class( targ->client->ps.stats[ STAT_CLASS ] )->knockbackScale );
  }

  // Too much knockback from falling really far makes you "bounce" and 
  //  looks silly. However, none at all also looks bad. Cap it.
  if( mod == MOD_FALLING && knockback > 50 ) 
    knockback = 50;

  if( knockback > 200 )
    knockback = 200;

  if( targ->flags & FL_NO_KNOCKBACK )
    knockback = 0;

  // figure momentum add, even if the damage won't be taken
  if( knockback && targ->client )
  {
    vec3_t  kvel;
    float   mass;

    mass = 200;

    VectorScale( dir, g_knockback.value * (float)knockback / mass, kvel );
    VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );

    // set the timer so that the other client can't cancel
    // out the movement immediately
    if( !targ->client->ps.pm_time )
    {
      int   t;

      t = knockback * 2;
      if( t < 50 )
        t = 50;

      if( t > 200 )
        t = 200;

      targ->client->ps.pm_time = t;
      targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
    }
  }

  // don't do friendly fire on movement attacks
  if( ( mod == MOD_ALEVEL5_TRAMPLE || mod == MOD_STOMP ) &&
      targ->s.eType == ET_BUILDABLE && targ->buildableTeam == attacker->client->pers.teamSelection )
  {
    return;
  }

  // check for completely getting out of the damage
  if( !( dflags & DAMAGE_NO_PROTECTION ) )
  {

    // if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
    // if the attacker was on the same team
    if( targ != attacker && OnSameTeam( targ, attacker ) )
    {
      // don't do friendly fire on movement attacks
      if( mod == MOD_ALEVEL5_TRAMPLE || mod == MOD_STOMP )
        return;

      // if dretchpunt is enabled and this is a dretch, do dretchpunt instead of damage
      if( g_dretchPunt.integer &&
          ( targ->client->ps.stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL0 ||
            targ->client->ps.stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL2 ) )
      {
        vec3_t dir, push;

        VectorSubtract( targ->r.currentOrigin, attacker->r.currentOrigin, dir );
        VectorNormalizeFast( dir );
        VectorScale( dir, ( damage * 10.0f ), push );
        push[2] = 64.0f;
        VectorAdd( targ->client->ps.velocity, push, targ->client->ps.velocity );
        return;
      }

      // check if friendly fire has been disabled
      if( !g_friendlyFire.integer )
      {
        return;
      }
    }

    if( targ->s.eType == ET_BUILDABLE && attacker->client &&
        mod != MOD_DECONSTRUCT )
    {
      if( targ->buildableTeam == attacker->client->pers.teamSelection &&
        !g_friendlyBuildableFire.integer && mod != MOD_DECONSTRUCT &&
        mod != MOD_SUICIDE )
      {
        return;
      }

      // base is under attack warning if DCC'd
      if( targ->buildableTeam == TEAM_HUMANS && G_FindDCC( targ ) &&
          level.time > level.humanBaseAttackTimer &&
          mod != MOD_SUICIDE )
      {
        level.humanBaseAttackTimer = level.time + DC_ATTACK_PERIOD;
        G_BroadcastEvent( EV_DCC_ATTACK, 0 );
      }
    }

    // check for godmode
    if ( targ->flags & FL_GODMODE )
      return;
  }

  // add to the attacker's hit counter
  if( attacker->client && targ != attacker && targ->health > 0
      && targ->s.eType != ET_MISSILE
      && targ->s.eType != ET_GENERAL )
  {
    if( OnSameTeam( targ, attacker ) )
      attacker->client->ps.persistant[ PERS_HITS ]--;
    else
      attacker->client->ps.persistant[ PERS_HITS ]++;
  }

  take = damage;
  save = 0;

  if( take < 0 )
    take = 0;

  // add to the damage inflicted on a player this frame
  // the total will be turned into screen blends and view angle kicks
  // at the end of the frame
  if( client )
  {
    if( attacker )
      client->ps.persistant[ PERS_ATTACKER ] = attacker->s.number;
    else
      client->ps.persistant[ PERS_ATTACKER ] = ENTITYNUM_WORLD;

    client->damage_armor += asave;
    client->damage_blood += take;
    client->damage_knockback += knockback;

    if( dir )
    {
      VectorCopy ( dir, client->damage_from );
      client->damage_fromWorld = qfalse;
    }
    else
    {
      VectorCopy ( targ->r.currentOrigin, client->damage_from );
      client->damage_fromWorld = qtrue;
    }

    // set the last client who damaged the target
    targ->client->lasthurt_client = attacker->s.number;
    targ->client->lasthurt_mod = mod;

    //if boosted poison every attack
    if( attacker->client && attacker->client->ps.stats[ STAT_STATE ] & SS_BOOSTED )
    {
      if( targ->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS && mod != MOD_POISON &&
          targ->client->poisonImmunityTime < level.time )
      {
        targ->client->ps.stats[ STAT_STATE ] |= SS_POISONED;
        targ->client->lastPoisonTime = level.time;
        targ->client->lastPoisonClient = attacker;
      }
    }
  }

  // do the damage
  if( take )
  {
    targ->health -= take;

    if( targ->client )
      targ->client->ps.stats[ STAT_HEALTH ] = targ->health;

    targ->lastDamageTime = level.time;
    targ->nextRegenTime = level.time + ALIEN_REGEN_DAMAGE_TIME;

    // reset lcannon charge on damage
    if( targ->client->ps.weapon == WP_LUCIFER_CANNON )
      targ->client->ps.stats[ STAT_MISC ] = 0;

    // add to the attackers "account" on the target
    if( attacker->client && attacker != targ )
    {
      int debit = take;

      // don't record overflow damage
      if( targ->health < 0 )
        debit += targ->health;

      if( OnSameTeam( targ, attacker ) ||
          ( targ->s.eType == ET_BUILDABLE && targ->buildableTeam == attacker->client->pers.teamSelection ) )
      {
        // team damage is subtracted, and also clears any previous positive account damage
        if( targ->damageAccounts[ attacker->client->ps.clientNum ] > 0 )
          targ->damageAccounts[ attacker->client->ps.clientNum ] = 0;

        targ->damageAccounts[ attacker->client->ps.clientNum ] -= debit;
      }
      else
      {
        targ->damageAccounts[ attacker->client->ps.clientNum ] += debit;
      }
    }

    if( targ->health <= 0 )
    {
      if( client )
        targ->flags |= FL_NO_KNOCKBACK;

      if( targ->health < -999 )
        targ->health = -999;

      targ->enemy = attacker;
      targ->die( targ, inflictor, attacker, take, mod );
      return;
    }
    else if( targ->pain )
      targ->pain( targ, attacker, take );
  }

  if( g_debugDamage.integer )
  {
    G_Printf( "%i: client:%i health:%i damage:%i armor:%i account:%i\n", level.time, targ->s.number,
      targ->health, take, asave, targ->damageAccounts[ attacker->client->ps.clientNum ] );
  }
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage( gentity_t *targ, vec3_t origin )
{
  vec3_t  dest;
  trace_t tr;
  vec3_t  midpoint;

  // use the midpoint of the bounds instead of the origin, because
  // bmodels may have their origin is 0,0,0
  VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
  VectorScale( midpoint, 0.5, midpoint );

  VectorCopy( midpoint, dest );
  trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
  if( tr.fraction == 1.0  || tr.entityNum == targ->s.number )
    return qtrue;

  // this should probably check in the plane of projection,
  // rather than in world coordinate, and also include Z
  VectorCopy( midpoint, dest );
  dest[ 0 ] += 15.0;
  dest[ 1 ] += 15.0;
  trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
  if( tr.fraction == 1.0 )
    return qtrue;

  VectorCopy( midpoint, dest );
  dest[ 0 ] += 15.0;
  dest[ 1 ] -= 15.0;
  trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
  if( tr.fraction == 1.0 )
    return qtrue;

  VectorCopy( midpoint, dest );
  dest[ 0 ] -= 15.0;
  dest[ 1 ] += 15.0;
  trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
  if( tr.fraction == 1.0 )
    return qtrue;

  VectorCopy( midpoint, dest );
  dest[ 0 ] -= 15.0;
  dest[ 1 ] -= 15.0;
  trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
  if( tr.fraction == 1.0 )
    return qtrue;

  return qfalse;
}

/*
============
G_SelectiveRadiusDamage
============
*/
qboolean G_SelectiveRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float knockback,
                                  float radius, gentity_t *ignore, int mod, int team )
{
  float     scale, dist;
  gentity_t *ent;
  int       entityList[ MAX_GENTITIES ];
  int       numListedEntities;
  vec3_t    mins, maxs;
  vec3_t    v;
  vec3_t    dir;
  int       i, e;
  qboolean  hitClient = qfalse;

  if( radius < 1 )
    radius = 1;

  for( i = 0; i < 3; i++ )
  {
    mins[ i ] = origin[ i ] - radius;
    maxs[ i ] = origin[ i ] + radius;
  }

  numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

  for( e = 0; e < numListedEntities; e++ )
  {
    ent = &g_entities[ entityList[ e ] ];

    if( ent == ignore )
      continue;

    if( !ent->takedamage )
      continue;

    // find the distance from the edge of the bounding box
    for( i = 0 ; i < 3 ; i++ )
    {
      if( origin[ i ] < ent->r.absmin[ i ] )
        v[ i ] = ent->r.absmin[ i ] - origin[ i ];
      else if( origin[ i ] > ent->r.absmax[ i ] )
        v[ i ] = origin[ i ] - ent->r.absmax[ i ];
      else
        v[ i ] = 0;
    }

    dist = VectorLength( v );
    if( dist >= radius )
      continue;

    scale = ( 1.0 - dist / radius );

    if( CanDamage( ent, origin ) && ent->client &&
        ent->client->ps.stats[ STAT_TEAM ] != team )
    {
      VectorSubtract( ent->r.currentOrigin, origin, dir );
      // push the center of mass higher than the origin so players
      // get knocked into the air more
      dir[ 2 ] += 24;
      hitClient = qtrue;
      G_Damage( ent, NULL, attacker, dir, origin,
          (int)( damage * scale ), (int)( knockback * scale ), DAMAGE_RADIUS, mod );
    }
  }

  return hitClient;
}


/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float knockback,
                         float radius, gentity_t *ignore, int mod )
{
  float     scale, dist;
  gentity_t *ent;
  int       entityList[ MAX_GENTITIES ];
  int       numListedEntities;
  vec3_t    mins, maxs;
  vec3_t    v;
  vec3_t    dir;
  int       i, e;
  qboolean  hitClient = qfalse;

  if( radius < 1 )
    radius = 1;

  for( i = 0; i < 3; i++ )
  {
    mins[ i ] = origin[ i ] - radius;
    maxs[ i ] = origin[ i ] + radius;
  }

  numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

  for( e = 0; e < numListedEntities; e++ )
  {
    ent = &g_entities[ entityList[ e ] ];

    if( ent == ignore )
      continue;

    if( !ent->takedamage )
      continue;

    // find the distance from the edge of the bounding box
    for( i = 0; i < 3; i++ )
    {
      if( origin[ i ] < ent->r.absmin[ i ] )
        v[ i ] = ent->r.absmin[ i ] - origin[ i ];
      else if( origin[ i ] > ent->r.absmax[ i ] )
        v[ i ] = origin[ i ] - ent->r.absmax[ i ];
      else
        v[ i ] = 0;
    }

    dist = VectorLength( v );
    if( dist >= radius )
      continue;

    scale = ( 1.0 - dist / radius );

    if( CanDamage( ent, origin ) )
    {
      VectorSubtract( ent->r.currentOrigin, origin, dir );
      // push the center of mass higher than the origin so players
      // get knocked into the air more
      dir[ 2 ] += 24;
      hitClient = qtrue;
      G_Damage( ent, NULL, attacker, dir, origin,
          (int)( damage * scale ), (int)( knockback * scale ), DAMAGE_RADIUS, mod );
    }
  }

  return hitClient;
}

/*
================
G_LogDestruction

Log deconstruct/destroy events
================
*/
void G_LogDestruction( gentity_t *self, gentity_t *actor, int mod )
{
  if( !actor )
    return;

  // don't log when marked structures are removed
  if( mod == MOD_DECONSTRUCT && !actor->client )
    return;

  if( actor->client && actor->client->pers.teamSelection ==
    BG_Buildable( self->s.modelindex )->team )
  {
    G_TeamCommand( actor->client->ps.stats[ STAT_TEAM ],
      va( "print \"%s ^3%s^7 by %s\n\"",
        BG_Buildable( self->s.modelindex )->humanName,
        mod == MOD_DECONSTRUCT ? "DECONSTRUCTED" : "DESTROYED",
        actor->client->pers.netname ) );
  }

  G_LogPrintf( S_COLOR_YELLOW "Deconstruct: %d %d %s %s: %s %s by %s\n",
    actor - g_entities,
    self - g_entities,
    BG_Buildable( self->s.modelindex )->name,
    modNames[ mod ],
    BG_Buildable( self->s.modelindex )->humanName,
    mod == MOD_DECONSTRUCT ? "deconstructed" : "destroyed",
    actor->client ? actor->client->pers.netname : "<world>" );
}
