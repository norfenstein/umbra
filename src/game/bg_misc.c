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

// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

int  trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void trap_FS_Read( void *buffer, int len, fileHandle_t f );
void trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void trap_FS_FCloseFile( fileHandle_t f );
void trap_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin ); // fsOrigin_t
int  trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );

static const buildableAttributes_t bg_buildableList[ ] =
{
  {
    BA_A_SPAWN,            //int       buildNum;
    "eggpod",              //char      *buildName;
    "Egg",                 //char      *humanName;
    "The most basic alien structure. It allows aliens to spawn "
      "and protect the Overmind. Without any of these, the Overmind "
      "is left nearly defenseless and defeat is imminent.",
    "team_alien_spawn",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    ASPAWN_BP,             //int       buildPoints;
    ASPAWN_HEALTH,         //int       health;
    ASPAWN_REGEN,          //int       regenRate;
    ASPAWN_SPLASHDAMAGE,   //int       splashDamage;
    ASPAWN_SPLASHRADIUS,   //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    ASPAWN_BT,             //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.5f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    ASPAWN_CREEPSIZE,      //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_A_OVERMIND,         //int       buildNum;
    "overmind",            //char      *buildName;
    "Overmind",            //char      *humanName;
    "A collective consciousness that controls all the alien structures "
      "in its vicinity. It must be protected at all costs, since its "
      "death will render alien structures defenseless.",
    "team_alien_overmind", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    OVERMIND_BP,           //int       buildPoints;
    OVERMIND_HEALTH,       //int       health;
    OVERMIND_REGEN,        //int       regenRate;
    OVERMIND_SPLASHDAMAGE, //int       splashDamage;
    OVERMIND_SPLASHRADIUS, //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    OVERMIND_ATTACK_REPEAT,//int       nextthink;
    OVERMIND_BT,           //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    OVERMIND_CREEPSIZE,    //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qtrue,                 //qboolean  uniqueTest;
  },
  {
    BA_A_BARRICADE,        //int       buildNum;
    "barricade",           //char      *buildName;
    "Barricade",           //char      *humanName;
    "Used to obstruct corridors and doorways, hindering humans from "
      "threatening the spawns and Overmind. Barricades will shrink "
      "to allow aliens to pass over them, however.",
    "team_alien_barricade",//char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    BARRICADE_BP,          //int       buildPoints;
    BARRICADE_HEALTH,      //int       health;
    BARRICADE_REGEN,       //int       regenRate;
    BARRICADE_SPLASHDAMAGE,//int       splashDamage;
    BARRICADE_SPLASHRADIUS,//int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    BARRICADE_BT,          //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.707f,                //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    BARRICADE_CREEPSIZE,   //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_A_ACIDTUBE,         //int       buildNum;
    "acid_tube",           //char      *buildName;
    "Acid Tube",           //char      *humanName;
    "Ejects lethal poisonous acid at an approaching human. These "
      "are highly effective when used in conjunction with a trapper "
      "to hold the victim in place.",
    "team_alien_acid_tube",//char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    ACIDTUBE_BP,           //int       buildPoints;
    ACIDTUBE_HEALTH,       //int       health;
    ACIDTUBE_REGEN,        //int       regenRate;
    ACIDTUBE_SPLASHDAMAGE, //int       splashDamage;
    ACIDTUBE_SPLASHRADIUS, //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    200,                   //int       nextthink;
    ACIDTUBE_BT,           //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    ACIDTUBE_CREEPSIZE,    //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_A_TRAPPER,          //int       buildNum;
    "trapper",             //char      *buildName;
    "Trapper",             //char      *humanName;
    "Fires a blob of adhesive spit at any non-alien in its line of "
      "sight. This hinders their movement, making them an easy target "
      "for other defensive structures or aliens.",
    "team_alien_trapper",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    TRAPPER_BP,            //int       buildPoints;
    TRAPPER_HEALTH,        //int       health;
    TRAPPER_REGEN,         //int       regenRate;
    TRAPPER_SPLASHDAMAGE,  //int       splashDamage;
    TRAPPER_SPLASHRADIUS,  //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    TRAPPER_BT,            //int       buildTime;
    qfalse,                //qboolean  usable;
    TRAPPER_RANGE,         //int       turretRange;
    TRAPPER_REPEAT,        //int       turretFireSpeed;
    WP_LOCKBLOB_LAUNCHER,  //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    TRAPPER_CREEPSIZE,     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_A_BOOSTER,          //int       buildNum;
    "booster",             //char      *buildName;
    "Booster",             //char      *humanName;
    "Laces the attacks of any alien that touches it with a poison "
      "that will gradually deal damage to any humans exposed to it. "
      "The booster also increases the rate of health regeneration for "
      "any nearby aliens.",
    "team_alien_booster",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    BOOSTER_BP,            //int       buildPoints;
    BOOSTER_HEALTH,        //int       health;
    BOOSTER_REGEN,         //int       regenRate;
    BOOSTER_SPLASHDAMAGE,  //int       splashDamage;
    BOOSTER_SPLASHRADIUS,  //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    BOOSTER_BT,            //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.707f,                //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    BOOSTER_CREEPSIZE,     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_A_HIVE,             //int       buildNum;
    "hive",                //char      *buildName;
    "Hive",                //char      *humanName;
    "Houses millions of tiny insectoid aliens. When a human "
      "approaches this structure, the insectoids attack.",
    "team_alien_hive",     //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    HIVE_BP,               //int       buildPoints;
    HIVE_HEALTH,           //int       health;
    HIVE_REGEN,            //int       regenRate;
    HIVE_SPLASHDAMAGE,     //int       splashDamage;
    HIVE_SPLASHRADIUS,     //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    500,                   //int       nextthink;
    HIVE_BT,               //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_HIVE,               //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    HIVE_CREEPSIZE,        //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_A_HOVEL,            //int       buildNum;
    "hovel",               //char      *buildName;
    "Hovel",               //char      *humanName;
    "An armoured shell used by the Grangers to hide in, while "
      "the alien base is under attack. It may be entered or "
      "exited at any time.",
    "team_alien_hovel",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    HOVEL_BP,              //int       buildPoints;
    HOVEL_HEALTH,          //int       health;
    HOVEL_REGEN,           //int       regenRate;
    HOVEL_SPLASHDAMAGE,    //int       splashDamage;
    HOVEL_SPLASHRADIUS,    //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    150,                   //int       nextthink;
    HOVEL_BT,              //int       buildTime;
    qtrue,                 //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    HOVEL_CREEPSIZE,       //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qtrue,                 //qboolean  uniqueTest;
  },
  {
    BA_H_SPAWN,            //int       buildNum;
    "telenode",            //char      *buildName;
    "Telenode",            //char      *humanName;
    "The most basic human structure. It provides a means for humans "
      "to enter the battle arena. Without any of these the humans "
      "cannot spawn and defeat is imminent.",
    "team_human_spawn",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    HSPAWN_BP,             //int       buildPoints;
    HSPAWN_HEALTH,         //int       health;
    0,                     //int       regenRate;
    HSPAWN_SPLASHDAMAGE,   //int       splashDamage;
    HSPAWN_SPLASHRADIUS,   //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    HSPAWN_BT,             //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_H_MGTURRET,         //int       buildNum;
    "mgturret",            //char      *buildName;
    "Machinegun Turret",   //char      *humanName;
    "Automated base defense that is effective against large targets "
      "but slow to begin firing. Should always be "
      "backed up by physical support.",
    "team_human_mgturret", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    MGTURRET_BP,           //int       buildPoints;
    MGTURRET_HEALTH,       //int       health;
    0,                     //int       regenRate;
    MGTURRET_SPLASHDAMAGE, //int       splashDamage;
    MGTURRET_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    50,                    //int       nextthink;
    MGTURRET_BT,           //int       buildTime;
    qfalse,                //qboolean  usable;
    MGTURRET_RANGE,        //int       turretRange;
    MGTURRET_REPEAT,       //int       turretFireSpeed;
    WP_MGTURRET,           //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_H_TESLAGEN,         //int       buildNum;
    "tesla",               //char      *buildName;
    "Tesla Generator",     //char      *humanName;
    "A structure equipped with a strong electrical attack that fires "
      "instantly and always hits its target. It is effective against smaller "
      "aliens and for consolidating basic defense.",
    "team_human_tesla",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    TESLAGEN_BP,           //int       buildPoints;
    TESLAGEN_HEALTH,       //int       health;
    0,                     //int       regenRate;
    TESLAGEN_SPLASHDAMAGE, //int       splashDamage;
    TESLAGEN_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    150,                   //int       nextthink;
    TESLAGEN_BT,           //int       buildTime;
    qfalse,                //qboolean  usable;
    TESLAGEN_RANGE,        //int       turretRange;
    TESLAGEN_REPEAT,       //int       turretFireSpeed;
    WP_TESLAGEN,           //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_H_ARMOURY,          //int       buildNum;
    "arm",                 //char      *buildName;
    "Armoury",             //char      *humanName;
    "An essential part of the human base, providing a means "
      "to upgrade the basic human equipment. A range of upgrades "
      "and weapons are available for sale from the armoury.",
    "team_human_armoury",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    ARMOURY_BP,            //int       buildPoints;
    ARMOURY_HEALTH,        //int       health;
    0,                     //int       regenRate;
    ARMOURY_SPLASHDAMAGE,  //int       splashDamage;
    ARMOURY_SPLASHRADIUS,  //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    ARMOURY_BT,            //int       buildTime;
    qtrue,                 //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_H_DCC,              //int       buildNum;
    "dcc",                 //char      *buildName;
    "Defence Computer",    //char      *humanName;
    "A structure that enables self-repair functionality in "
      "human structures. Each Defence Computer built increases "
      "repair rate slightly.",
    "team_human_dcc",      //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    DC_BP,                 //int       buildPoints;
    DC_HEALTH,             //int       health;
    0,                     //int       regenRate;
    DC_SPLASHDAMAGE,       //int       splashDamage;
    DC_SPLASHRADIUS,       //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    DC_BT,                 //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_H_MEDISTAT,         //int       buildNum;
    "medistat",            //char      *buildName;
    "Medistation",         //char      *humanName;
    "A structure that automatically restores "
      "the health of any human that stands on it. "
      "It may only be used by one person at a time. This structure "
      "also issues medkits.",
    "team_human_medistat", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    MEDISTAT_BP,           //int       buildPoints;
    MEDISTAT_HEALTH,       //int       health;
    0,                     //int       regenRate;
    MEDISTAT_SPLASHDAMAGE, //int       splashDamage;
    MEDISTAT_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    MEDISTAT_BT,           //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  },
  {
    BA_H_REACTOR,          //int       buildNum;
    "reactor",             //char      *buildName;
    "Reactor",             //char      *humanName;
    "All structures except the telenode rely on a reactor to operate. "
      "The reactor provides power for all the human structures either "
      "directly or via repeaters. Only one reactor can be built at a time.",
    "team_human_reactor",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    REACTOR_BP,            //int       buildPoints;
    REACTOR_HEALTH,        //int       health;
    0,                     //int       regenRate;
    REACTOR_SPLASHDAMAGE,  //int       splashDamage;
    REACTOR_SPLASHRADIUS,  //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    REACTOR_ATTACK_DCC_REPEAT,//int       nextthink;
    REACTOR_BT,            //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qtrue,                 //qboolean  uniqueTest;
  },
  {
    BA_H_REPEATER,         //int       buildNum;
    "repeater",            //char      *buildName;
    "Repeater",            //char      *humanName;
    "A power distributor that transmits power from the reactor "
      "to remote locations, so that bases may be built far "
      "from the reactor.",
    "team_human_repeater", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    REPEATER_BP,           //int       buildPoints;
    REPEATER_HEALTH,       //int       health;
    0,                     //int       regenRate;
    REPEATER_SPLASHDAMAGE, //int       splashDamage;
    REPEATER_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    REPEATER_BT,           //int       buildTime;
    qfalse,                //qboolean  usable;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
  }
};

int   bg_numBuildables = sizeof( bg_buildableList ) / sizeof( bg_buildableList[ 0 ] );

static const buildableAttributes_t nullBuildable = { 0 };

/*
==============
BG_BuildableByName
==============
*/
const buildableAttributes_t *BG_BuildableByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numBuildables; i++ )
  {
    if( !Q_stricmp( bg_buildableList[ i ].name, name ) )
      return &bg_buildableList[ i ];
  }

  return &nullBuildable;
}

/*
==============
BG_BuildableByEntityName
==============
*/
const buildableAttributes_t *BG_BuildableByEntityName( const char *name )
{
  int i;

  for( i = 0; i < bg_numBuildables; i++ )
  {
    if( !Q_stricmp( bg_buildableList[ i ].entityName, name ) )
      return &bg_buildableList[ i ];
  }

  return &nullBuildable;
}

/*
==============
BG_Buildable
==============
*/
const buildableAttributes_t *BG_Buildable( buildable_t buildable )
{
  return ( buildable > BA_NONE && buildable < BA_NUM_BUILDABLES ) ?
    &bg_buildableList[ buildable - 1 ] : &nullBuildable;
}

static buildableConfig_t bg_buildableConfigList[ BA_NUM_BUILDABLES ];

/*
==============
BG_BuildableConfig
==============
*/
buildableConfig_t *BG_BuildableConfig( buildable_t buildable )
{
  return &bg_buildableConfigList[ buildable ];
}

/*
==============
BG_BuildableBoundingBox
==============
*/
void BG_BuildableBoundingBox( buildable_t buildable,
                              vec3_t mins, vec3_t maxs )
{
  buildableConfig_t *buildableConfig = BG_BuildableConfig( buildable );

  if( mins != NULL )
    VectorCopy( buildableConfig->mins, mins );

  if( maxs != NULL )
    VectorCopy( buildableConfig->maxs, maxs );
}

/*
======================
BG_ParseBuildableFile

Parses a configuration file describing a buildable
======================
*/
static qboolean BG_ParseBuildableFile( const char *filename, buildableConfig_t *bc )
{
  char          *text_p;
  int           i;
  int           len;
  char          *token;
  char          text[ 20000 ];
  fileHandle_t  f;
  float         scale;
  int           defined = 0;
  enum
  {
      MODEL         = 1 << 0,
      MODELSCALE    = 1 << 1,
      MINS          = 1 << 2,
      MAXS          = 1 << 3,
      ZOFFSET       = 1 << 4
  };


  // load the file
  len = trap_FS_FOpenFile( filename, &f, FS_READ );
  if( len < 0 )
  {
    Com_Printf( S_COLOR_RED "ERROR: Buildable file %s doesn't exist\n", filename );
    return qfalse;
  }

  if( len == 0 || len >= sizeof( text ) - 1 )
  {
    trap_FS_FCloseFile( f );
    Com_Printf( S_COLOR_RED "ERROR: Buildable file %s is %s\n", filename,
      len == 0 ? "empty" : "too long" );
    return qfalse;
  }

  trap_FS_Read( text, len, f );
  text[ len ] = 0;
  trap_FS_FCloseFile( f );

  // parse the text
  text_p = text;

  // read optional parameters
  while( 1 )
  {
    token = COM_Parse( &text_p );

    if( !token )
      break;

    if( !Q_stricmp( token, "" ) )
      break;

    if( !Q_stricmp( token, "model" ) )
    {
      int index = 0;

      token = COM_Parse( &text_p );
      if( !token )
        break;

      index = atoi( token );

      if( index < 0 )
        index = 0;
      else if( index > 3 )
        index = 3;

      token = COM_Parse( &text_p );
      if( !token )
        break;

      Q_strncpyz( bc->models[ index ], token, sizeof( bc->models[ 0 ] ) );

      defined |= MODEL;
      continue;
    }
    else if( !Q_stricmp( token, "modelScale" ) )
    {
      token = COM_Parse( &text_p );
      if( !token )
        break;

      scale = atof( token );

      if( scale < 0.0f )
        scale = 0.0f;

      bc->modelScale = scale;

      defined |= MODELSCALE;
      continue;
    }
    else if( !Q_stricmp( token, "mins" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        bc->mins[ i ] = atof( token );
      }

      defined |= MINS;
      continue;
    }
    else if( !Q_stricmp( token, "maxs" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        bc->maxs[ i ] = atof( token );
      }

      defined |= MAXS;
      continue;
    }
    else if( !Q_stricmp( token, "zOffset" ) )
    {
      float offset;

      token = COM_Parse( &text_p );
      if( !token )
        break;

      offset = atof( token );

      bc->zOffset = offset;

      defined |= ZOFFSET;
      continue;
    }


    Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
    return qfalse;
  }

  if(      !( defined & MODEL      ) )  token = "model";
  else if( !( defined & MODELSCALE ) )  token = "modelScale";
  else if( !( defined & MINS       ) )  token = "mins";
  else if( !( defined & MAXS       ) )  token = "maxs";
  else if( !( defined & ZOFFSET    ) )  token = "zOffset";
  else                                  token = "";

  if( strlen( token ) > 0 )
  {
      Com_Printf( S_COLOR_RED "ERROR: %s not defined in %s\n",
                  token, filename );
      return qfalse;
  }

  return qtrue;
}

/*
===============
BG_InitBuildableConfigs
===============
*/
void BG_InitBuildableConfigs( void )
{
  int               i;
  buildableConfig_t *bc;

  for( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
  {
    bc = BG_BuildableConfig( i );
    Com_Memset( bc, 0, sizeof( buildableConfig_t ) );

    BG_ParseBuildableFile( va( "configs/buildables/%s.cfg",
                               BG_Buildable( i )->name ), bc );
  }
}

////////////////////////////////////////////////////////////////////////////////

static const classAttributes_t bg_classList[ ] =
{
  {
    PCL_NONE,                                       //int     classnum;
    "spectator",                                    //char    *className;
    "",
    0,                                              //int     health;
    0.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    0,                                              //int     abilities;
    { WP_NONE, WP_NONE, WP_NONE },                  //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    90,                                             //int     fov;
    0.000f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    0,                                              //int     steptime;
    600,                                            //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    270.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    0,                                              //int     cost;
  },
  {
    PCL_ALIEN_BUILDER,                              //int     classnum;
    "abuilder",                                     //char    *classname;
    "Similar to the base Granger, except that in addition to "
      "being able to build structures it has a spit attack "
      "that slows victims and the ability to crawl on walls.",
    ABUILDER_HEALTH,                                //int     health;
    0.2f,                                           //float   fallDamage;
    ABUILDER_REGEN,                                 //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_FOVWARPS|SCA_WALLCLIMBER|SCA_ALIENSENSE,    //int     abilities;
    { WP_ABUILD, WP_NONE, WP_NONE },                //weapon_t  weapons[ 3 ];
    105.0f,                                         //float   buildDist;
    110,                                            //int     fov;
    0.001f,                                         //float   bob;
    2.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    ABUILDER_SPEED,                                 //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    270.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    ABUILDER_COST,                                  //int     cost;
  },
  {
    PCL_ALIEN_LEVEL0,                               //int     classnum;
    "alevel0",                                      //char    *classname;
    "Has a lethal reflexive bite and the ability to crawl on "
      "walls and ceilings.",
    ALEVEL0_HEALTH,                                 //int     health;
    0.0f,                                           //float   fallDamage;
    ALEVEL0_REGEN,                                  //float   regenRate;
    SCA_WALLCLIMBER|SCA_FOVWARPS|SCA_ALIENSENSE,    //int     abilities;
    { WP_ALEVEL0, WP_NONE, WP_NONE },               //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    140,                                            //int     fov;
    0.0f,                                           //float   bob;
    2.5f,                                           //float   bobCycle;
    25,                                             //int     steptime;
    ALEVEL0_SPEED,                                  //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    400.0f,                                         //float   stopSpeed;
    250.0f,                                         //float   jumpMagnitude;
    2.0f,                                           //float   knockbackScale;
    ALEVEL0_COST,                                   //int     cost;
  },
  {
    PCL_ALIEN_LEVEL1_0,                             //int     classnum;
    "alevel10",                                     //char    *classname;
    "In addition to the basic Basilisk abilities, the Advanced "
      "Basilisk sprays a poisonous gas which disorients any "
      "nearby humans. Has a strong healing aura that "
      "that accelerates the healing rate of nearby aliens.",
    ALEVEL1_1_HEALTH,                               //int     health;
    0.0f,                                           //float   fallDamage;
    ALEVEL1_1_REGEN,                                //float   regenRate;
    SCA_FOVWARPS|SCA_ALIENSENSE,                    //int     abilities;
    { WP_ALEVEL1_0, WP_NONE, WP_NONE },             //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    120,                                            //int     fov;
    0.001f,                                         //float   bob;
    1.8f,                                           //float   bobCycle;
    60,                                             //int     steptime;
    ALEVEL1_1_SPEED,                                //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    300.0f,                                         //float   stopSpeed;
    310.0f,                                         //float   jumpMagnitude;
    1.1f,                                           //float   knockbackScale;
    ALEVEL1_1_COST,                                 //int     cost;
  },
  {
    PCL_ALIEN_LEVEL1_1,                             //int     classnum;
    "alevel11",                                     //char    *classname;
    "In addition to the basic Basilisk abilities, the Advanced "
      "Basilisk sprays a poisonous gas which disorients any "
      "nearby humans. Has a strong healing aura that "
      "that accelerates the healing rate of nearby aliens.",
    ALEVEL1_1_HEALTH,                               //int     health;
    0.0f,                                           //float   fallDamage;
    ALEVEL1_1_REGEN,                                //float   regenRate;
    SCA_FOVWARPS|SCA_ALIENSENSE,                    //int     abilities;
    { WP_ALEVEL1_1, WP_NONE, WP_NONE },             //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    120,                                            //int     fov;
    0.001f,                                         //float   bob;
    1.8f,                                           //float   bobCycle;
    60,                                             //int     steptime;
    ALEVEL1_1_SPEED,                                //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    300.0f,                                         //float   stopSpeed;
    310.0f,                                         //float   jumpMagnitude;
    1.1f,                                           //float   knockbackScale;
    ALEVEL1_1_COST,                                 //int     cost;
  },
  {
    PCL_ALIEN_LEVEL2,                               //int     classnum;
    "alevel2",                                      //char    *classname;
    "Has a lethal reflexive bite and the ability to crawl on "
      "walls and ceilings.",
    ALEVEL0_HEALTH,                                 //int     health;
    0.0f,                                           //float   fallDamage;
    ALEVEL0_REGEN,                                  //float   regenRate;
    SCA_WALLCLIMBER|SCA_FOVWARPS|SCA_ALIENSENSE,    //int     abilities;
    { WP_ALEVEL2, WP_NONE, WP_NONE },               //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    140,                                            //int     fov;
    0.0f,                                           //float   bob;
    2.5f,                                           //float   bobCycle;
    25,                                             //int     steptime;
    ALEVEL0_SPEED,                                  //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    400.0f,                                         //float   stopSpeed;
    250.0f,                                         //float   jumpMagnitude;
    2.0f,                                           //float   knockbackScale;
    ALEVEL0_COST,                                   //int     cost;
  },
  {
    PCL_ALIEN_LEVEL3,                               //int     classnum;
    "alevel3",                                      //char    *classname;
    "The Advanced Marauder has all the abilities of the basic Marauder "
      "with the addition of an area effect electric shock attack.",
    ALEVEL3_HEALTH,                                 //int     health;
    0.0f,                                           //float   fallDamage;
    ALEVEL3_REGEN,                                  //float   regenRate;
    SCA_WALLJUMPER|SCA_FOVWARPS|SCA_ALIENSENSE,     //int     abilities;
    { WP_ALEVEL3, WP_NONE, WP_NONE },               //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    90,                                             //int     fov;
    0.001f,                                         //float   bob;
    1.5f,                                           //float   bobCycle;
    80,                                             //int     steptime;
    ALEVEL3_SPEED,                                  //float   speed;
    10.0f,                                          //float   acceleration;
    2.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    380.0f,                                         //float   jumpMagnitude;
    0.7f,                                           //float   knockbackScale;
    ALEVEL3_COST,                                   //int     cost;
  },
  {
    PCL_ALIEN_LEVEL4,                               //int     classnum;
    "alevel4",                                      //char    *classname;
    "In addition to the basic Dragoon abilities, the Advanced "
      "Dragoon has 3 barbs which may be used to attack humans "
      "from a distance.",
    ALEVEL4_HEALTH,                                 //int     health;
    0.0f,                                           //float   fallDamage;
    ALEVEL4_REGEN,                                  //float   regenRate;
    SCA_WALLCLIMBER|SCA_FOVWARPS|SCA_ALIENSENSE,    //int     abilities;
    { WP_ALEVEL4, WP_NONE, WP_NONE },               //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    110,                                            //int     fov;
    0.0005f,                                        //float   bob;
    1.3f,                                           //float   bobCycle;
    90,                                             //int     steptime;
    ALEVEL4_SPEED,                                  //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    200.0f,                                         //float   stopSpeed;
    270.0f,                                         //float   jumpMagnitude;
    0.4f,                                           //float   knockbackScale;
    ALEVEL4_COST,                                   //int     cost;
  },
  {
    PCL_ALIEN_LEVEL5,                               //int     classnum;
    "alevel5",                                      //char    *classname;
    "A large alien with a strong melee attack, this class can "
      "also charge at enemy humans and structures, inflicting "
      "great damage. Any humans or their structures caught under "
      "a falling Tyrant will be crushed by its weight.",
    ALEVEL5_HEALTH,                                 //int     health;
    0.0f,                                           //float   fallDamage;
    ALEVEL5_REGEN,                                  //float   regenRate;
    SCA_FOVWARPS|SCA_ALIENSENSE,                    //int     abilities;
    { WP_ALEVEL5, WP_NONE, WP_NONE },               //weapon_t  weapons[ 3 ];
    0.0f,                                           //float   buildDist;
    90,                                             //int     fov;
    0.001f,                                         //float   bob;
    1.1f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    ALEVEL5_SPEED,                                  //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    170.0f,                                         //float   jumpMagnitude;
    0.1f,                                           //float   knockbackScale;
    ALEVEL5_COST,                                   //int     cost;
  },
  {
    PCL_HUMAN_BUILDER,                              //int     classnum;
    "hbuilder",                                     //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS,          //int     abilities;
    { WP_HBUILD, WP_BLASTER, WP_NONE },             //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    0,                                              //int     cost;
  },
  {
    PCL_HUMAN_LEVEL0,                               //int     classnum;
    "hlevel0",                                      //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS,          //int     abilities;
    { WP_MACHINEGUN, WP_BLASTER, WP_NONE },         //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    0,                                              //int     cost;
  },
  {
    PCL_HUMAN_LEVEL1_0,                             //int     classnum;
    "hlevel10",                                     //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS|SCA_SCANNER,//int     abilities;
    { WP_SHOTGUN, WP_BLASTER, WP_NONE },            //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    1,                                              //int     cost;
  },
  {
    PCL_HUMAN_LEVEL1_1,                             //int     classnum;
    "hlevel11",                                     //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS,          //int     abilities;
    { WP_SHOTGUN, WP_PAIN_SAW, WP_NONE },           //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    1,                                              //int     cost;
  },
  {
    PCL_HUMAN_LEVEL2,                               //int     classnum;
    "hlevel2",                                      //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS,          //int     abilities;
    { WP_LUCIFER_CANNON, WP_BLASTER, WP_NONE },     //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    2,                                              //int     cost;
  },
  {
    PCL_HUMAN_LEVEL3,                               //int     classnum;
    "hlevel3",                                      //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS,          //int     abilities;
    { WP_CHAINGUN, WP_MASS_DRIVER, WP_NONE },       //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    3,                                              //int     cost;
  },
  {
    PCL_HUMAN_LEVEL4,                               //int     classnum;
    "hlevel4",                                      //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS,          //int     abilities;
    { WP_LAS_GUN, WP_NONE, WP_NONE },               //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    4,                                              //int     cost;
  },
  {
    PCL_HUMAN_LEVEL5,                               //int     classnum;
    "hlevel5",                                      //char    *classname;
    "",
    100,                                            //int     health;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS,          //int     abilities;
    { WP_PULSE_RIFLE, WP_NONE, WP_NONE },           //weapon_t  weapons[ 3 ];
    110.0f,                                         //float   buildDist;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    5,                                              //int     cost;
  }
};

int   bg_numClasses = sizeof( bg_classList ) / sizeof( bg_classList[ 0 ] );

static const classAttributes_t nullClass = { 0 };

/*
==============
BG_ClassByName
==============
*/
const classAttributes_t *BG_ClassByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numClasses; i++ )
  {
    if( !Q_stricmp( bg_classList[ i ].name, name ) )
      return &bg_classList[ i ];
  }

  return &nullClass;
}

/*
==============
BG_Class
==============
*/
const classAttributes_t *BG_Class( class_t class )
{
  return ( class >= PCL_NONE && class < PCL_NUM_CLASSES ) ?
    &bg_classList[ class ] : &nullClass;
}

static classConfig_t bg_classConfigList[ PCL_NUM_CLASSES ];

/*
==============
BG_ClassConfig
==============
*/
classConfig_t *BG_ClassConfig( class_t class )
{
  return &bg_classConfigList[ class ];
}

/*
==============
BG_ClassBoundingBox
==============
*/
void BG_ClassBoundingBox( class_t class,
                          vec3_t mins, vec3_t maxs,
                          vec3_t cmaxs, vec3_t dmins, vec3_t dmaxs )
{
  classConfig_t *classConfig = BG_ClassConfig( class );

  if( mins != NULL )
    VectorCopy( classConfig->mins, mins );

  if( maxs != NULL )
    VectorCopy( classConfig->maxs, maxs );

  if( cmaxs != NULL )
    VectorCopy( classConfig->crouchMaxs, cmaxs );

  if( dmins != NULL )
    VectorCopy( classConfig->deadMins, dmins );

  if( dmaxs != NULL )
    VectorCopy( classConfig->deadMaxs, dmaxs );
}

/*
==============
BG_ClassHasAbility
==============
*/
qboolean BG_ClassHasAbility( class_t class, int ability )
{
  int abilities = BG_Class( class )->abilities;

  return abilities & ability;
}

/*
======================
BG_ParseClassFile

Parses a configuration file describing a class
======================
*/
static qboolean BG_ParseClassFile( const char *filename, classConfig_t *cc )
{
  char          *text_p;
  int           i;
  int           len;
  char          *token;
  char          text[ 20000 ];
  fileHandle_t  f;
  float         scale = 0.0f;
  int           defined = 0;
  enum
  {
      MODEL           = 1 << 0,
      SKIN            = 1 << 1,
      HUD             = 1 << 2,
      MODELSCALE      = 1 << 3,
      SHADOWSCALE     = 1 << 4,
      MINS            = 1 << 5,
      MAXS            = 1 << 6,
      DEADMINS        = 1 << 7,
      DEADMAXS        = 1 << 8,
      CROUCHMAXS      = 1 << 9,
      VIEWHEIGHT      = 1 << 10,
      CVIEWHEIGHT     = 1 << 11,
      ZOFFSET         = 1 << 12,
      NAME            = 1 << 13,
      SHOULDEROFFSETS = 1 << 14
  };

  // load the file
  len = trap_FS_FOpenFile( filename, &f, FS_READ );
  if( len < 0 )
    return qfalse;

  if( len == 0 || len >= sizeof( text ) - 1 )
  {
    trap_FS_FCloseFile( f );
    Com_Printf( S_COLOR_RED "ERROR: Class file %s is %s\n", filename,
      len == 0 ? "empty" : "too long" );
    return qfalse;
  }

  trap_FS_Read( text, len, f );
  text[ len ] = 0;
  trap_FS_FCloseFile( f );

  // parse the text
  text_p = text;

  // read optional parameters
  while( 1 )
  {
    token = COM_Parse( &text_p );

    if( !token )
      break;

    if( !Q_stricmp( token, "" ) )
      break;

    if( !Q_stricmp( token, "model" ) )
    {
      token = COM_Parse( &text_p );
      if( !token )
        break;

      Q_strncpyz( cc->modelName, token, sizeof( cc->modelName ) );

      defined |= MODEL;
      continue;
    }
    else if( !Q_stricmp( token, "skin" ) )
    {
      token = COM_Parse( &text_p );
      if( !token )
        break;

      Q_strncpyz( cc->skinName, token, sizeof( cc->skinName ) );

      defined |= SKIN;
      continue;
    }
    else if( !Q_stricmp( token, "hud" ) )
    {
      token = COM_Parse( &text_p );
      if( !token )
        break;

      Q_strncpyz( cc->hudName, token, sizeof( cc->hudName ) );

      defined |= HUD;
      continue;
    }
    else if( !Q_stricmp( token, "modelScale" ) )
    {
      token = COM_Parse( &text_p );
      if( !token )
        break;

      scale = atof( token );

      if( scale < 0.0f )
        scale = 0.0f;

      cc->modelScale = scale;

      defined |= MODELSCALE;
      continue;
    }
    else if( !Q_stricmp( token, "shadowScale" ) )
    {
      token = COM_Parse( &text_p );
      if( !token )
        break;

      scale = atof( token );

      if( scale < 0.0f )
        scale = 0.0f;

      cc->shadowScale = scale;

      defined |= SHADOWSCALE;
      continue;
    }
    else if( !Q_stricmp( token, "mins" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        cc->mins[ i ] = atof( token );
      }

      defined |= MINS;
      continue;
    }
    else if( !Q_stricmp( token, "maxs" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        cc->maxs[ i ] = atof( token );
      }

      defined |= MAXS;
      continue;
    }
    else if( !Q_stricmp( token, "deadMins" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        cc->deadMins[ i ] = atof( token );
      }

      defined |= DEADMINS;
      continue;
    }
    else if( !Q_stricmp( token, "deadMaxs" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        cc->deadMaxs[ i ] = atof( token );
      }

      defined |= DEADMAXS;
      continue;
    }
    else if( !Q_stricmp( token, "crouchMaxs" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        cc->crouchMaxs[ i ] = atof( token );
      }

      defined |= CROUCHMAXS;
      continue;
    }
    else if( !Q_stricmp( token, "viewheight" ) )
    {
      token = COM_Parse( &text_p );
      cc->viewheight = atoi( token );
      defined |= VIEWHEIGHT;
      continue;
    }
    else if( !Q_stricmp( token, "crouchViewheight" ) )
    {
      token = COM_Parse( &text_p );
      cc->crouchViewheight = atoi( token );
      defined |= CVIEWHEIGHT;
      continue;
    }
    else if( !Q_stricmp( token, "zOffset" ) )
    {
      float offset;

      token = COM_Parse( &text_p );
      if( !token )
        break;

      offset = atof( token );

      cc->zOffset = offset;

      defined |= ZOFFSET;
      continue;
    }
    else if( !Q_stricmp( token, "name" ) )
    {
      token = COM_Parse( &text_p );
      if( !token )
        break;

      Q_strncpyz( cc->humanName, token, sizeof( cc->humanName ) );

      defined |= NAME;
      continue;
    }
    else if( !Q_stricmp( token, "shoulderOffsets" ) )
    {
      for( i = 0; i <= 2; i++ )
      {
        token = COM_Parse( &text_p );
        if( !token )
          break;

        cc->shoulderOffsets[ i ] = atof( token );
      }

      defined |= SHOULDEROFFSETS;
      continue;
    }

    Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
    return qfalse;
  }

  if(      !( defined & MODEL           ) ) token = "model";
  else if( !( defined & SKIN            ) ) token = "skin";
  else if( !( defined & HUD             ) ) token = "hud";
  else if( !( defined & MODELSCALE      ) ) token = "modelScale";
  else if( !( defined & SHADOWSCALE     ) ) token = "shadowScale";
  else if( !( defined & MINS            ) ) token = "mins";
  else if( !( defined & MAXS            ) ) token = "maxs";
  else if( !( defined & DEADMINS        ) ) token = "deadMins";
  else if( !( defined & DEADMAXS        ) ) token = "deadMaxs";
  else if( !( defined & CROUCHMAXS      ) ) token = "crouchMaxs";
  else if( !( defined & VIEWHEIGHT      ) ) token = "viewheight";
  else if( !( defined & CVIEWHEIGHT     ) ) token = "crouchViewheight";
  else if( !( defined & ZOFFSET         ) ) token = "zOffset";
  else if( !( defined & NAME            ) ) token = "name";
  else if( !( defined & SHOULDEROFFSETS ) ) token = "shoulderOffsets";
  else                                      token = "";

  if( strlen( token ) > 0 )
  {
      Com_Printf( S_COLOR_RED "ERROR: %s not defined in %s\n",
                  token, filename );
      return qfalse;
  }

  return qtrue;
}

/*
===============
BG_InitClassConfigs
===============
*/
void BG_InitClassConfigs( void )
{
  int           i;
  classConfig_t *cc;

  for( i = PCL_NONE; i < PCL_NUM_CLASSES; i++ )
  {
    cc = BG_ClassConfig( i );

    BG_ParseClassFile( va( "configs/classes/%s.cfg",
                           BG_Class( i )->name ), cc );
  }
}

////////////////////////////////////////////////////////////////////////////////

static const weaponAttributes_t bg_weapons[ ] =
{
  {
    WP_ALEVEL0,           //int       weaponNum;
    0,                    //int       price;
    "alevel0",            //char      *weaponName;
    "Bite",               //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    ALEVEL0_BITE_REPEAT,  //int       repeatRate1;
    ALEVEL0_GRAPPLE_REPEAT,//int      repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    ALEVEL0_BITE_K_SCALE, //float     knockbackScale;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL1_0,         //int       weaponNum;
    0,                    //int       price;
    "alevel10",           //char      *weaponName;
    "Claws",              //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    ALEVEL1_1_CLAW_REPEAT,//int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    ALEVEL1_1_CLAW_K_SCALE,//float    knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL1_1,         //int       weaponNum;
    0,                    //int       price;
    "alevel11",           //char      *weaponName;
    "Claws",              //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    ALEVEL1_1_CLAW_REPEAT,//int       repeatRate1;
    0,                    //int       repeatRate2;
    ALEVEL1_1_SPIT_REPEAT,//int       repeatRate3;
    0,                    //int       reloadTime;
    ALEVEL1_1_CLAW_K_SCALE,//float    knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qtrue,                //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL2,           //int       weaponNum;
    0,                    //int       price;
    "alevel2",            //char      *weaponName;
    "Bite",               //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    ALEVEL0_BITE_REPEAT,  //int       repeatRate1;
    ALEVEL0_GRAPPLE_REPEAT,//int      repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    ALEVEL0_BITE_K_SCALE, //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL3,           //int       weaponNum;
    0,                    //int       price;
    "alevel3",            //char      *weaponName;
    "Zap",                //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    ALEVEL3_CLAW_REPEAT,  //int       repeatRate1;
    ALEVEL3_FLAME_REPEAT, //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    ALEVEL3_CLAW_K_SCALE, //float     knockbackScale;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL4,           //int       weaponNum;
    0,                    //int       price;
    "alevel4",            //char      *weaponName;
    "Pounce",             //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    ALEVEL4_CLAW_REPEAT,  //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    ALEVEL4_CLAW_K_SCALE, //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL5,           //int       weaponNum;
    0,                    //int       price;
    "alevel5",            //char      *weaponName;
    "Charge",             //char      *humanName;
    3,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    ALEVEL5_BOUNCEBALL_REGEN,//int    ammoRegen;
    ALEVEL5_CLAW_REPEAT,  //int       repeatRate1;
    0,                    //int       repeatRate2;
    ALEVEL5_BOUNCEBALL_REPEAT,//int   repeatRate3;
    0,                    //int       reloadTime;
    ALEVEL5_CLAW_K_SCALE, //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qtrue,                //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_BLASTER,           //int       weaponNum;
    0,                    //int       price;
    "blaster",            //char      *weaponName;
    "Blaster",            //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    BLASTER_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    BLASTER_K_SCALE,      //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_MACHINEGUN,        //int       weaponNum;
    RIFLE_PRICE,          //int       price;
    "rifle",              //char      *weaponName;
    "Rifle",              //char      *humanName;
    RIFLE_CLIPSIZE,       //int       maxAmmo;
    RIFLE_MAXCLIPS,       //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    RIFLE_REPEAT,         //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    RIFLE_RELOAD,         //int       reloadTime;
    RIFLE_K_SCALE,        //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_PAIN_SAW,          //int       weaponNum;
    PAINSAW_PRICE,        //int       price;
    "psaw",               //char      *weaponName;
    "Pain Saw",           //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    PAINSAW_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    PAINSAW_K_SCALE,      //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_SHOTGUN,           //int       weaponNum;
    SHOTGUN_PRICE,        //int       price;
    "shotgun",            //char      *weaponName;
    "Shotgun",            //char      *humanName;
    SHOTGUN_SHELLS,       //int       maxAmmo;
    SHOTGUN_MAXCLIPS,     //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    SHOTGUN_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    SHOTGUN_RELOAD,       //int       reloadTime;
    SHOTGUN_K_SCALE,      //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LAS_GUN,           //int       weaponNum;
    LASGUN_PRICE,         //int       price;
    "lgun",               //char      *weaponName;
    "Las Gun",            //char      *humanName;
    LASGUN_AMMO,          //int       maxAmmo;
    0,                    //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    LASGUN_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    LASGUN_RELOAD,        //int       reloadTime;
    LASGUN_K_SCALE,       //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_MASS_DRIVER,       //int       weaponNum;
    MDRIVER_PRICE,        //int       price;
    "mdriver",            //char      *weaponName;
    "Mass Driver",        //char      *humanName;
    MDRIVER_CLIPSIZE,     //int       maxAmmo;
    MDRIVER_MAXCLIPS,     //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    MDRIVER_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    MDRIVER_RELOAD,       //int       reloadTime;
    MDRIVER_K_SCALE,      //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qtrue,                //qboolean  canZoom;
    20.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_CHAINGUN,          //int       weaponNum;
    CHAINGUN_PRICE,       //int       price;
    "chaingun",           //char      *weaponName;
    "Chaingun",           //char      *humanName;
    CHAINGUN_BULLETS,     //int       maxAmmo;
    0,                    //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    CHAINGUN_REPEAT,      //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    CHAINGUN_K_SCALE,     //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_PULSE_RIFLE,       //int       weaponNum;
    PRIFLE_PRICE,         //int       price;
    "prifle",             //char      *weaponName;
    "Pulse Rifle",        //char      *humanName;
    PRIFLE_CLIPS,         //int       maxAmmo;
    PRIFLE_MAXCLIPS,      //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    PRIFLE_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    PRIFLE_RELOAD,        //int       reloadTime;
    PRIFLE_K_SCALE,       //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LUCIFER_CANNON,    //int       weaponNum;
    LCANNON_PRICE,        //int       price;
    "lcannon",            //char      *weaponName;
    "Lucifer Cannon",     //char      *humanName;
    LCANNON_AMMO,         //int       maxAmmo;
    0,                    //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    LCANNON_REPEAT,       //int       repeatRate1;
    LCANNON_SECONDARY_REPEAT, //int   repeatRate2;
    0,                    //int       repeatRate3;
    LCANNON_RELOAD,       //int       reloadTime;
    LCANNON_K_SCALE,      //float     knockbackScale;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_GRENADE,           //int       weaponNum;
    GRENADE_PRICE,        //int       price;
    "grenade",            //char      *weaponName;
    "Grenade",            //char      *humanName;
    1,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qfalse,               //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    GRENADE_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    GRENADE_K_SCALE,      //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LOCKBLOB_LAUNCHER, //int       weaponNum;
    0,                    //int       price;
    "lockblob",           //char      *weaponName;
    "Lock Blob",          //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    500,                  //int       repeatRate1;
    500,                  //int       repeatRate2;
    500,                  //int       repeatRate3;
    0,                    //int       reloadTime;
    LOCKBLOB_K_SCALE,     //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_HIVE,              //int       weaponNum;
    0,                    //int       price;
    "hive",               //char      *weaponName;
    "Hive",               //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    500,                  //int       repeatRate1;
    500,                  //int       repeatRate2;
    500,                  //int       repeatRate3;
    0,                    //int       reloadTime;
    HIVE_K_SCALE,         //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_TESLAGEN,          //int       weaponNum;
    0,                    //int       price;
    "teslagen",           //char      *weaponName;
    "Tesla Generator",    //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    500,                  //int       repeatRate1;
    500,                  //int       repeatRate2;
    500,                  //int       repeatRate3;
    0,                    //int       reloadTime;
    TESLAGEN_K_SCALE,     //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_MGTURRET,          //int       weaponNum;
    0,                    //int       price;
    "mgturret",           //char      *weaponName;
    "Machinegun Turret",  //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    0,                    //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    MGTURRET_K_SCALE,     //float     knockbackScale;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_ABUILD,            //int       weaponNum;
    0,                    //int       price;
    "abuildupg",          //char      *weaponName;
    "Alien build weapon2",//char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    ABUILDER_BUILD_REPEAT,//int       repeatRate1;
    ABUILDER_CLAW_REPEAT, //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    ABUILDER_CLAW_K_SCALE,//float     knockbackScale;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_HBUILD,            //int       weaponNum;
    HBUILD_PRICE,         //int       price;
    "ckit",               //char      *weaponName;
    "Construction Kit",   //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    qtrue,                //int       infiniteAmmo;
    0,                    //int       ammoRegen;
    HBUILD_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       reloadTime;
    0.0f,                 //float     knockbackScale;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  longRanged;
    TEAM_HUMANS           //team_t    team;
  }
};

int   bg_numWeapons = sizeof( bg_weapons ) / sizeof( bg_weapons[ 0 ] );

static const weaponAttributes_t nullWeapon = { 0 };

/*
==============
BG_WeaponByName
==============
*/
const weaponAttributes_t *BG_WeaponByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numWeapons; i++ )
  {
    if( !Q_stricmp( bg_weapons[ i ].name, name ) )
    {
      return &bg_weapons[ i ];
    }
  }

  return &nullWeapon;
}

/*
==============
BG_Weapon
==============
*/
const weaponAttributes_t *BG_Weapon( weapon_t weapon )
{
  return ( weapon > WP_NONE && weapon < WP_NUM_WEAPONS ) ?
    &bg_weapons[ weapon - 1 ] : &nullWeapon;
}

////////////////////////////////////////////////////////////////////////////////

static const upgradeAttributes_t bg_upgrades[ ] =
{
  {
    UP_MEDKIT,              //int   upgradeNum;
    "medkit",               //char  *upgradeName;
    "Medkit",               //char  *humanName;
    "icons/iconu_atoxin",
    TEAM_HUMANS             //team_t  team;
  },
  {
    UP_JETPACK,             //int   upgradeNum;
    "jetpack",              //char  *upgradeName;
    "Jet Pack",             //char  *humanName;
    "icons/iconu_jetpack",
    TEAM_HUMANS             //team_t  team;
  },
  {
    UP_GRENADE,             //int   upgradeNum;
    "gren",                 //char  *upgradeName;
    "Grenade",              //char  *humanName;
    0,
    TEAM_HUMANS             //team_t  team;
  }
};

int   bg_numUpgrades = sizeof( bg_upgrades ) / sizeof( bg_upgrades[ 0 ] );

static const upgradeAttributes_t nullUpgrade = { 0 };

/*
==============
BG_UpgradeByName
==============
*/
const upgradeAttributes_t *BG_UpgradeByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numUpgrades; i++ )
  {
    if( !Q_stricmp( bg_upgrades[ i ].name, name ) )
    {
      return &bg_upgrades[ i ];
    }
  }

  return &nullUpgrade;
}

/*
==============
BG_Upgrade
==============
*/
const upgradeAttributes_t *BG_Upgrade( upgrade_t upgrade )
{
  return ( upgrade > UP_NONE && upgrade < UP_NUM_UPGRADES ) ?
    &bg_upgrades[ upgrade - 1 ] : &nullUpgrade;
}

////////////////////////////////////////////////////////////////////////////////

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result )
{
  float   deltaTime;
  float   phase;

  switch( tr->trType )
  {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
      VectorCopy( tr->trBase, result );
      break;

    case TR_LINEAR:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      break;

    case TR_SINE:
      deltaTime = ( atTime - tr->trTime ) / (float)tr->trDuration;
      phase = sin( deltaTime * M_PI * 2 );
      VectorMA( tr->trBase, phase, tr->trDelta, result );
      break;

    case TR_LINEAR_STOP:
      if( atTime > tr->trTime + tr->trDuration )
        atTime = tr->trTime + tr->trDuration;

      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      if( deltaTime < 0 )
        deltaTime = 0;

      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      break;

    case TR_GRAVITY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      result[ 2 ] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;   // FIXME: local gravity...
      break;

    case TR_BUOYANCY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      result[ 2 ] += 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;   // FIXME: local gravity...
      break;

    default:
      Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
      break;
  }
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result )
{
  float deltaTime;
  float phase;

  switch( tr->trType )
  {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
      VectorClear( result );
      break;

    case TR_LINEAR:
      VectorCopy( tr->trDelta, result );
      break;

    case TR_SINE:
      deltaTime = ( atTime - tr->trTime ) / (float)tr->trDuration;
      phase = cos( deltaTime * M_PI * 2 );  // derivative of sin = cos
      phase *= 0.5;
      VectorScale( tr->trDelta, phase, result );
      break;

    case TR_LINEAR_STOP:
      if( atTime > tr->trTime + tr->trDuration )
      {
        VectorClear( result );
        return;
      }
      VectorCopy( tr->trDelta, result );
      break;

    case TR_GRAVITY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorCopy( tr->trDelta, result );
      result[ 2 ] -= DEFAULT_GRAVITY * deltaTime;   // FIXME: local gravity...
      break;

    case TR_BUOYANCY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorCopy( tr->trDelta, result );
      result[ 2 ] += DEFAULT_GRAVITY * deltaTime;   // FIXME: local gravity...
      break;

    default:
      Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
      break;
  }
}

char *eventnames[ ] =
{
  "EV_NONE",

  "EV_FOOTSTEP",
  "EV_FOOTSTEP_METAL",
  "EV_FOOTSTEP_SQUELCH",
  "EV_FOOTSPLASH",
  "EV_FOOTWADE",
  "EV_SWIM",

  "EV_STEP_4",
  "EV_STEP_8",
  "EV_STEP_12",
  "EV_STEP_16",

  "EV_STEPDN_4",
  "EV_STEPDN_8",
  "EV_STEPDN_12",
  "EV_STEPDN_16",

  "EV_FALL_SHORT",
  "EV_FALL_MEDIUM",
  "EV_FALL_FAR",
  "EV_FALLING",

  "EV_JUMP",
  "EV_WATER_TOUCH", // foot touches
  "EV_WATER_LEAVE", // foot leaves
  "EV_WATER_UNDER", // head touches
  "EV_WATER_CLEAR", // head leaves

  "EV_NOAMMO",
  "EV_CHANGE_WEAPON",
  "EV_FIRE_WEAPON",
  "EV_FIRE_WEAPON2",
  "EV_FIRE_WEAPON3",

  "EV_PLAYER_RESPAWN", // for fovwarp effects
  "EV_PLAYER_TELEPORT_IN",
  "EV_PLAYER_TELEPORT_OUT",

  "EV_GRENADE_BOUNCE",    // eventParm will be the soundindex

  "EV_GENERAL_SOUND",
  "EV_GLOBAL_SOUND",    // no attenuation

  "EV_BULLET_HIT_FLESH",
  "EV_BULLET_HIT_WALL",

  "EV_SHOTGUN",
  "EV_MASS_DRIVER",

  "EV_MISSILE_HIT",
  "EV_MISSILE_MISS",
  "EV_MISSILE_MISS_METAL",
  "EV_TESLATRAIL",
  "EV_BULLET",        // otherEntity is the shooter

  "EV_ALEV4_GRAB",
  "EV_ALEV5_TRAMPLE_PREPARE",
  "EV_ALEV5_TRAMPLE_START",

  "EV_PAIN",
  "EV_DEATH1",
  "EV_DEATH2",
  "EV_DEATH3",
  "EV_OBITUARY",

  "EV_GIB_PLAYER",      // gib a previously living player

  "EV_BUILD_CONSTRUCT",
  "EV_BUILD_DESTROY",
  "EV_BUILD_DELAY",     // can't build yet
  "EV_BUILD_REPAIR",    // repairing buildable
  "EV_BUILD_REPAIRED",  // buildable has full health
  "EV_HUMAN_BUILDABLE_EXPLOSION",
  "EV_ALIEN_BUILDABLE_EXPLOSION",
  "EV_ALIEN_ACIDTUBE",

  "EV_MEDKIT_USED",

  "EV_DEBUG_LINE",
  "EV_STOPLOOPINGSOUND",
  "EV_TAUNT",

  "EV_OVERMIND_ATTACK", // overmind under attack
  "EV_OVERMIND_DYING",  // overmind close to death
  "EV_OVERMIND_SPAWNS", // overmind needs spawns

  "EV_DCC_ATTACK",      // dcc under attack

  "EV_MGTURRET_SPINUP", // trigger a sound

  "EV_RPTUSE_SOUND"     // trigger a sound
};

/*
===============
BG_EventName
===============
*/
const char *BG_EventName( int num )
{
  if( num < 0 || num >= sizeof( eventnames ) / sizeof( char * ) )
    return "UNKNOWN";

  return eventnames[ num ];
}

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void  trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps )
{
#ifdef _DEBUG
  {
    char buf[ 256 ];
    trap_Cvar_VariableStringBuffer( "showevents", buf, sizeof( buf ) );

    if( atof( buf ) != 0 )
    {
#ifdef GAME
      Com_Printf( " game event svt %5d -> %5d: num = %20s parm %d\n",
                  ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence,
                  BG_EventName( newEvent ), eventParm );
#else
      Com_Printf( "Cgame event svt %5d -> %5d: num = %20s parm %d\n",
                  ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence,
                  BG_EventName( newEvent ), eventParm );
#endif
    }
  }
#endif
  ps->events[ ps->eventSequence & ( MAX_PS_EVENTS - 1 ) ] = newEvent;
  ps->eventParms[ ps->eventSequence & ( MAX_PS_EVENTS - 1 ) ] = eventParm;
  ps->eventSequence++;
}


/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap )
{
  int     i;

  if( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE )
    s->eType = ET_INVISIBLE;
  else if( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
    s->eType = ET_INVISIBLE;
  else
    s->eType = ET_PLAYER;

  s->number = ps->clientNum;

  s->pos.trType = TR_INTERPOLATE;
  VectorCopy( ps->origin, s->pos.trBase );

  if( snap )
    SnapVector( s->pos.trBase );

  //set the trDelta for flag direction
  VectorCopy( ps->velocity, s->pos.trDelta );

  s->apos.trType = TR_INTERPOLATE;
  VectorCopy( ps->viewangles, s->apos.trBase );

  if( snap )
    SnapVector( s->apos.trBase );

  s->time2 = ps->movementDir;
  s->legsAnim = ps->legsAnim;
  s->torsoAnim = ps->torsoAnim;
  s->weaponAnim = ps->weaponAnim;
  s->clientNum = ps->clientNum;   // ET_PLAYER looks here instead of at number
                    // so corpses can also reference the proper config
  s->eFlags = ps->eFlags;
  if( ps->stats[STAT_HEALTH] <= 0 )
    s->eFlags |= EF_DEAD;
  else
    s->eFlags &= ~EF_DEAD;

  if( ps->stats[ STAT_STATE ] & SS_BLOBLOCKED )
    s->eFlags |= EF_BLOBLOCKED;
  else
    s->eFlags &= ~EF_BLOBLOCKED;

  if( ps->externalEvent )
  {
    s->event = ps->externalEvent;
    s->eventParm = ps->externalEventParm;
  }
  else if( ps->entityEventSequence < ps->eventSequence )
  {
    int   seq;

    if( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS )
      ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;

    seq = ps->entityEventSequence & ( MAX_PS_EVENTS - 1 );
    s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
    s->eventParm = ps->eventParms[ seq ];
    ps->entityEventSequence++;
  }

  s->weapon = ps->weapon;
  s->groundEntityNum = ps->groundEntityNum;

  //store items held and active items in modelindex and modelindex2
  s->modelindex = 0;
  s->modelindex2 = 0;
  for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
  {
    if( BG_InventoryContainsUpgrade( i, ps->stats ) )
    {
      s->modelindex |= 1 << i;

      if( BG_UpgradeIsActive( i, ps->stats ) )
        s->modelindex2 |= 1 << i;
    }
  }

  // use misc field to store team/class info:
  s->misc = ps->stats[ STAT_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

  // have to get the surfNormal through somehow...
  VectorCopy( ps->grapplePoint, s->angles2 );

  s->loopSound = ps->loopSound;
  s->generic1 = ps->generic1;

  if( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
    s->generic1 = WPM_PRIMARY;

  s->otherEntityNum = ps->otherEntityNum;  
}


/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap )
{
  int     i;

  if( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE )
    s->eType = ET_INVISIBLE;
  else if( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
    s->eType = ET_INVISIBLE;
  else
    s->eType = ET_PLAYER;

  s->number = ps->clientNum;

  s->pos.trType = TR_LINEAR_STOP;
  VectorCopy( ps->origin, s->pos.trBase );

  if( snap )
    SnapVector( s->pos.trBase );

  // set the trDelta for flag direction and linear prediction
  VectorCopy( ps->velocity, s->pos.trDelta );
  // set the time for linear prediction
  s->pos.trTime = time;
  // set maximum extra polation time
  s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

  s->apos.trType = TR_INTERPOLATE;
  VectorCopy( ps->viewangles, s->apos.trBase );
  if( snap )
    SnapVector( s->apos.trBase );

  s->time2 = ps->movementDir;
  s->legsAnim = ps->legsAnim;
  s->torsoAnim = ps->torsoAnim;
  s->weaponAnim = ps->weaponAnim;
  s->clientNum = ps->clientNum;   // ET_PLAYER looks here instead of at number
                    // so corpses can also reference the proper config
  s->eFlags = ps->eFlags;

  if( ps->stats[STAT_HEALTH] <= 0 )
    s->eFlags |= EF_DEAD;
  else
    s->eFlags &= ~EF_DEAD;

  if( ps->stats[ STAT_STATE ] & SS_BLOBLOCKED )
    s->eFlags |= EF_BLOBLOCKED;
  else
    s->eFlags &= ~EF_BLOBLOCKED;

  if( ps->externalEvent )
  {
    s->event = ps->externalEvent;
    s->eventParm = ps->externalEventParm;
  }
  else if( ps->entityEventSequence < ps->eventSequence )
  {
    int   seq;

    if( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS )
      ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;

    seq = ps->entityEventSequence & ( MAX_PS_EVENTS - 1 );
    s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
    s->eventParm = ps->eventParms[ seq ];
    ps->entityEventSequence++;
  }

  s->weapon = ps->weapon;
  s->groundEntityNum = ps->groundEntityNum;

  //store items held and active items in modelindex and modelindex2
  s->modelindex = 0;
  s->modelindex2 = 0;

  for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
  {
    if( BG_InventoryContainsUpgrade( i, ps->stats ) )
    {
      s->modelindex |= 1 << i;

      if( BG_UpgradeIsActive( i, ps->stats ) )
        s->modelindex2 |= 1 << i;
    }
  }

  // use misc field to store team/class info:
  s->misc = ps->stats[ STAT_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

  // have to get the surfNormal through somehow...
  VectorCopy( ps->grapplePoint, s->angles2 );

  s->loopSound = ps->loopSound;
  s->generic1 = ps->generic1;

  if( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
    s->generic1 = WPM_PRIMARY;

  s->otherEntityNum = ps->otherEntityNum;
}

/*
========================
BG_WeaponIsFull

Check if a weapon has full ammo
========================
*/
qboolean BG_WeaponIsFull( weapon_t weapon, int stats[ ], int ammo, int clips )
{
  return
    BG_Weapon( weapon )->maxAmmo == ammo &&
    BG_Weapon( weapon )->maxClips == clips;
}

/*
========================
BG_InventoryContainsWeapon

Does the player hold a weapon?
========================
*/
qboolean BG_InventoryContainsWeapon( int weapon, int stats[ ] )
{
  if ( weapon == BG_Class( stats[ STAT_CLASS ] )->weapons[ 0 ] )
    return qtrue;

  if ( weapon == BG_Class( stats[ STAT_CLASS ] )->weapons[ 1 ] )
    return qtrue;

  if ( weapon == BG_Class( stats[ STAT_CLASS ] )->weapons[ 2 ] )
    return qtrue;

  return qfalse;
}

/*
========================
BG_WeaponSlotFromWeapon

Which STAT_WEAPON is a weapon for a given class?
========================
*/
int BG_WeaponSlotFromWeapon( int class, int weapon )
{
  if ( weapon == BG_Class( class )->weapons[ 0 ] )
    return STAT_WEAPON1;

  if ( weapon == BG_Class( class )->weapons[ 1 ] )
    return STAT_WEAPON2;

  if ( weapon == BG_Class( class )->weapons[ 2 ] )
    return STAT_WEAPON3;

  return -1;
}

/*
========================
BG_AddClassItems

Give items for a class
========================
*/
void BG_AddClassItems( playerState_t *ps )
{
  int      maxAmmo, maxClips;
  weapon_t weapon;
  class_t  class = ps->stats[ STAT_CLASS ];

  switch( class )
  {
    case PCL_ALIEN_BUILDER:
    case PCL_ALIEN_LEVEL0:
    case PCL_ALIEN_LEVEL1_0:
    case PCL_ALIEN_LEVEL1_1:
    case PCL_ALIEN_LEVEL2:
    case PCL_ALIEN_LEVEL3:
    case PCL_ALIEN_LEVEL4:
    case PCL_ALIEN_LEVEL5:
      break;

    case PCL_HUMAN_BUILDER:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      break;

    case PCL_HUMAN_LEVEL0:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      break;

    case PCL_HUMAN_LEVEL1_0:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      BG_AddUpgradeToInventory( UP_GRENADE, ps->stats );
      break;

    case PCL_HUMAN_LEVEL1_1:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      BG_AddUpgradeToInventory( UP_GRENADE, ps->stats );
      break;

    case PCL_HUMAN_LEVEL2:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      break;

    case PCL_HUMAN_LEVEL3:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      BG_AddUpgradeToInventory( UP_GRENADE, ps->stats );
      break;

    case PCL_HUMAN_LEVEL4:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      BG_AddUpgradeToInventory( UP_GRENADE, ps->stats );
      BG_AddUpgradeToInventory( UP_JETPACK, ps->stats );
      break;

    case PCL_HUMAN_LEVEL5:
      BG_AddUpgradeToInventory( UP_MEDKIT, ps->stats );
      break;

    default:
      break;
  }

  weapon = BG_Class( class )->weapons[ 2 ];
  maxAmmo = BG_Weapon( weapon )->maxAmmo;
  maxClips = BG_Weapon( weapon )->maxClips;
  ps->stats[ STAT_WEAPON3 ] = ( maxAmmo << 4 ) + maxClips;

  weapon = BG_Class( class )->weapons[ 1 ];
  maxAmmo = BG_Weapon( weapon )->maxAmmo;
  maxClips = BG_Weapon( weapon )->maxClips;
  ps->stats[ STAT_WEAPON2 ] = ( maxAmmo << 4 ) + maxClips;

  weapon = BG_Class( class )->weapons[ 0 ];
  maxAmmo = BG_Weapon( weapon )->maxAmmo;
  maxClips = BG_Weapon( weapon )->maxClips;
  ps->stats[ STAT_WEAPON1 ] = ( maxAmmo << 4 ) + maxClips;

  ps->weapon = weapon;
  ps->ammo = maxAmmo;
  ps->clips = maxClips;

}

/*
========================
BG_AddUpgradeToInventory

Give the player an upgrade
========================
*/
void BG_AddUpgradeToInventory( int item, int stats[ ] )
{
  stats[ STAT_ITEMS ] |= ( 1 << item );
}

/*
========================
BG_RemoveUpgradeFromInventory

Take an upgrade from the player
========================
*/
void BG_RemoveUpgradeFromInventory( int item, int stats[ ] )
{
  stats[ STAT_ITEMS ] &= ~( 1 << item );
}

/*
========================
BG_InventoryContainsUpgrade

Does the player hold an upgrade?
========================
*/
qboolean BG_InventoryContainsUpgrade( int item, int stats[ ] )
{
  return( stats[ STAT_ITEMS ] & ( 1 << item ) );
}

/*
========================
BG_ActivateUpgrade

Activates an upgrade
========================
*/
void BG_ActivateUpgrade( int item, int stats[ ] )
{
  stats[ STAT_ACTIVEITEMS ] |= ( 1 << item );
}

/*
========================
BG_DeactivateUpgrade

Deactivates an upgrade
========================
*/
void BG_DeactivateUpgrade( int item, int stats[ ] )
{
  stats[ STAT_ACTIVEITEMS ] &= ~( 1 << item );
}

/*
========================
BG_UpgradeIsActive

Is this upgrade active?
========================
*/
qboolean BG_UpgradeIsActive( int item, int stats[ ] )
{
  return( stats[ STAT_ACTIVEITEMS ] & ( 1 << item ) );
}

/*
===============
BG_RotateAxis

Shared axis rotation function
===============
*/
qboolean BG_RotateAxis( vec3_t surfNormal, vec3_t inAxis[ 3 ],
                        vec3_t outAxis[ 3 ], qboolean inverse, qboolean ceiling )
{
  vec3_t  refNormal = { 0.0f, 0.0f, 1.0f };
  vec3_t  ceilingNormal = { 0.0f, 0.0f, -1.0f };
  vec3_t  localNormal, xNormal;
  float   rotAngle;

  //the grapplePoint being a surfNormal rotation Normal hack... see above :)
  if( ceiling )
  {
    VectorCopy( ceilingNormal, localNormal );
    VectorCopy( surfNormal, xNormal );
  }
  else
  {
    //cross the reference normal and the surface normal to get the rotation axis
    VectorCopy( surfNormal, localNormal );
    CrossProduct( localNormal, refNormal, xNormal );
    VectorNormalize( xNormal );
  }

  //can't rotate with no rotation vector
  if( VectorLength( xNormal ) != 0.0f )
  {
    rotAngle = RAD2DEG( acos( DotProduct( localNormal, refNormal ) ) );

    if( inverse )
      rotAngle = -rotAngle;

    AngleNormalize180( rotAngle );

    //hmmm could get away with only one rotation and some clever stuff later... but i'm lazy
    RotatePointAroundVector( outAxis[ 0 ], xNormal, inAxis[ 0 ], -rotAngle );
    RotatePointAroundVector( outAxis[ 1 ], xNormal, inAxis[ 1 ], -rotAngle );
    RotatePointAroundVector( outAxis[ 2 ], xNormal, inAxis[ 2 ], -rotAngle );
  }
  else
    return qfalse;

  return qtrue;
}

/*
===============
BG_GetClientNormal

Get the normal for the surface the client is walking on
===============
*/
void BG_GetClientNormal( const playerState_t *ps, vec3_t normal )
{
  if( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
  {
    if( ps->eFlags & EF_WALLCLIMBCEILING )
      VectorSet( normal, 0.0f, 0.0f, -1.0f );
    else
      VectorCopy( ps->grapplePoint, normal );
  }
  else
    VectorSet( normal, 0.0f, 0.0f, 1.0f );
}

/*
===============
BG_PositionBuildableRelativeToPlayer

Find a place to build a buildable
===============
*/
void BG_PositionBuildableRelativeToPlayer( const playerState_t *ps,
                                           const vec3_t mins, const vec3_t maxs,
                                           void (*trace)( trace_t *, const vec3_t, const vec3_t,
                                                          const vec3_t, const vec3_t, int, int ),
                                           vec3_t outOrigin, vec3_t outAngles, trace_t *tr )
{
  vec3_t  forward, entityOrigin, targetOrigin;
  vec3_t  angles, playerOrigin, playerNormal;
  float   buildDist;

  BG_GetClientNormal( ps, playerNormal );

  VectorCopy( ps->viewangles, angles );
  VectorCopy( ps->origin, playerOrigin );
  buildDist = BG_Class( ps->stats[ STAT_CLASS ] )->buildDist;

  AngleVectors( angles, forward, NULL, NULL );
  ProjectPointOnPlane( forward, forward, playerNormal );
  VectorNormalize( forward );

  VectorMA( playerOrigin, buildDist, forward, entityOrigin );

  VectorCopy( entityOrigin, targetOrigin );

  //so buildings can be placed facing slopes
  VectorMA( entityOrigin, 32, playerNormal, entityOrigin );

  //so buildings drop to floor
  VectorMA( targetOrigin, -128, playerNormal, targetOrigin );

  // The mask is MASK_DEADSOLID on purpose to avoid collisions with other entities
  (*trace)( tr, entityOrigin, mins, maxs, targetOrigin, ps->clientNum, MASK_DEADSOLID );
  VectorCopy( tr->endpos, entityOrigin );
  VectorMA( entityOrigin, 0.1f, playerNormal, outOrigin );
  vectoangles( forward, outAngles );
}

/*
=================
BG_PlayerCanChangeWeapon
=================
*/
qboolean BG_PlayerCanChangeWeapon( playerState_t *ps )
{
  // Do not allow Lucifer Cannon "canceling" via weapon switch
  if( ps->weapon == WP_LUCIFER_CANNON &&
      ps->stats[ STAT_MISC ] > LCANNON_CHARGE_TIME_MIN )
    return qfalse;

  return ps->weaponTime <= 0 || ps->weaponstate != WEAPON_FIRING;
}

/*
=================
BG_GetPlayerWeapon

Returns the players current weapon or the weapon they are switching to.
Only needs to be used for human weapons.
=================
*/
weapon_t BG_GetPlayerWeapon( playerState_t *ps )
{
  if( ps->persistant[ PERS_NEWWEAPON ] )
    return ps->persistant[ PERS_NEWWEAPON ];

  return ps->weapon;
}

/*
=================
BG_GetPlayerWeaponAndAmmo

Returns the weapon_t, ammo, and clips for the given weapon for a player
=================
*/
void BG_GetAmmoForWeapon( playerState_t *ps, weapon_t weapon, int *ammo, int *clips )
{
  int weaponSlot;

  *ammo = *clips = 0;

  if( weapon == ps->weapon )
  {
	  *ammo = ps->ammo;
	  *clips = ps->clips;
  }
  else
  {
    weaponSlot = BG_WeaponSlotFromWeapon( ps->stats[ STAT_CLASS ], weapon );
    if( weaponSlot > 0 )
    {
      *ammo = ps->stats[ weaponSlot ] >> 4;
      *clips = ps->stats[ weaponSlot ] & 15;
    }
  }
}

/*
===============
atof_neg

atof with an allowance for negative values
===============
*/
float atof_neg( char *token, qboolean allowNegative )
{
  float value;

  value = atof( token );

  if( !allowNegative && value < 0.0f )
    value = 1.0f;

  return value;
}

/*
===============
atoi_neg

atoi with an allowance for negative values
===============
*/
int atoi_neg( char *token, qboolean allowNegative )
{
  int value;

  value = atoi( token );

  if( !allowNegative && value < 0 )
    value = 1;

  return value;
}

#define MAX_NUM_PACKED_ENTITY_NUMS 10

/*
===============
BG_PackEntityNumbers

Pack entity numbers into an entityState_t
===============
*/
void BG_PackEntityNumbers( entityState_t *es, const int *entityNums, int count )
{
  int i;

  if( count > MAX_NUM_PACKED_ENTITY_NUMS )
  {
    count = MAX_NUM_PACKED_ENTITY_NUMS;
    Com_Printf( S_COLOR_YELLOW "WARNING: A maximum of %d entity numbers can be "
      "packed, but BG_PackEntityNumbers was passed %d entities",
      MAX_NUM_PACKED_ENTITY_NUMS, count );
  }

  es->misc = es->time = es->time2 = es->constantLight = 0;

  for( i = 0; i < MAX_NUM_PACKED_ENTITY_NUMS; i++ )
  {
    int entityNum;

    if( i < count )
      entityNum = entityNums[ i ];
    else
      entityNum = ENTITYNUM_NONE;

    if( entityNum & ~GENTITYNUM_MASK )
    {
      Com_Error( ERR_FATAL, "BG_PackEntityNumbers passed an entity number (%d) which "
        "exceeds %d bits", entityNum, GENTITYNUM_BITS );
    }

    switch( i )
    {
      case 0: es->misc |= entityNum;                                       break;
      case 1: es->time |= entityNum;                                       break;
      case 2: es->time |= entityNum << GENTITYNUM_BITS;                    break;
      case 3: es->time |= entityNum << (GENTITYNUM_BITS * 2);              break;
      case 4: es->time2 |= entityNum;                                      break;
      case 5: es->time2 |= entityNum << GENTITYNUM_BITS;                   break;
      case 6: es->time2 |= entityNum << (GENTITYNUM_BITS * 2);             break;
      case 7: es->constantLight |= entityNum;                              break;
      case 8: es->constantLight |= entityNum << GENTITYNUM_BITS;           break;
      case 9: es->constantLight |= entityNum << (GENTITYNUM_BITS * 2);     break;
      default: Com_Error( ERR_FATAL, "Entity index %d not handled", i );   break;
    }
  }
}

/*
===============
BG_UnpackEntityNumbers

Unpack entity numbers from an entityState_t
===============
*/
int BG_UnpackEntityNumbers( entityState_t *es, int *entityNums, int count )
{
  int i;

  if( count > MAX_NUM_PACKED_ENTITY_NUMS )
    count = MAX_NUM_PACKED_ENTITY_NUMS;

  for( i = 0; i < count; i++ )
  {
    int *entityNum = &entityNums[ i ];

    switch( i )
    {
      case 0: *entityNum = es->misc;                                      break;
      case 1: *entityNum = es->time;                                      break;
      case 2: *entityNum = (es->time >> GENTITYNUM_BITS);                 break;
      case 3: *entityNum = (es->time >> (GENTITYNUM_BITS * 2));           break;
      case 4: *entityNum = es->time2;                                     break;
      case 5: *entityNum = (es->time2 >> GENTITYNUM_BITS);                break;
      case 6: *entityNum = (es->time2 >> (GENTITYNUM_BITS * 2));          break;
      case 7: *entityNum = es->constantLight;                             break;
      case 8: *entityNum = (es->constantLight >> GENTITYNUM_BITS);        break;
      case 9: *entityNum = (es->constantLight >> (GENTITYNUM_BITS * 2));  break;
      default: Com_Error( ERR_FATAL, "Entity index %d not handled", i );  break;
    }

    *entityNum &= GENTITYNUM_MASK;

    if( *entityNum == ENTITYNUM_NONE )
      break;
  }

  return i;
}

/*
===============
BG_ParseCSVEquipmentList
===============
*/
void BG_ParseCSVEquipmentList( const char *string, weapon_t *weapons, int weaponsSize,
    upgrade_t *upgrades, int upgradesSize )
{
  char      buffer[ MAX_STRING_CHARS ];
  int       i = 0, j = 0;
  char      *p, *q;
  qboolean  EOS = qfalse;

  Q_strncpyz( buffer, string, MAX_STRING_CHARS );

  p = q = buffer;

  while( *p != '\0' )
  {
    //skip to first , or EOS
    while( *p != ',' && *p != '\0' )
      p++;

    if( *p == '\0' )
      EOS = qtrue;

    *p = '\0';

    //strip leading whitespace
    while( *q == ' ' )
      q++;

    if( weaponsSize )
      weapons[ i ] = BG_WeaponByName( q )->number;

    if( upgradesSize )
      upgrades[ j ] = BG_UpgradeByName( q )->number;

    if( weaponsSize && weapons[ i ] == WP_NONE &&
        upgradesSize && upgrades[ j ] == UP_NONE )
      Com_Printf( S_COLOR_YELLOW "WARNING: unknown equipment %s\n", q );
    else if( weaponsSize && weapons[ i ] != WP_NONE )
      i++;
    else if( upgradesSize && upgrades[ j ] != UP_NONE )
      j++;

    if( !EOS )
    {
      p++;
      q = p;
    }
    else
      break;

    if( i == ( weaponsSize - 1 ) || j == ( upgradesSize - 1 ) )
      break;
  }

  if( weaponsSize )
    weapons[ i ] = WP_NONE;

  if( upgradesSize )
    upgrades[ j ] = UP_NONE;
}

/*
===============
BG_ParseCSVClassList
===============
*/
void BG_ParseCSVClassList( const char *string, class_t *classes, int classesSize )
{
  char      buffer[ MAX_STRING_CHARS ];
  int       i = 0;
  char      *p, *q;
  qboolean  EOS = qfalse;

  Q_strncpyz( buffer, string, MAX_STRING_CHARS );

  p = q = buffer;

  while( *p != '\0' )
  {
    //skip to first , or EOS
    while( *p != ',' && *p != '\0' )
      p++;

    if( *p == '\0' )
      EOS = qtrue;

    *p = '\0';

    //strip leading whitespace
    while( *q == ' ' )
      q++;

    classes[ i ] = BG_ClassByName( q )->number;

    if( classes[ i ] == PCL_NONE )
      Com_Printf( S_COLOR_YELLOW "WARNING: unknown class %s\n", q );
    else
      i++;

    if( !EOS )
    {
      p++;
      q = p;
    }
    else
      break;
  }

  classes[ i ] = PCL_NONE;
}

/*
===============
BG_ParseCSVBuildableList
===============
*/
void BG_ParseCSVBuildableList( const char *string, buildable_t *buildables, int buildablesSize )
{
  char      buffer[ MAX_STRING_CHARS ];
  int       i = 0;
  char      *p, *q;
  qboolean  EOS = qfalse;

  Q_strncpyz( buffer, string, MAX_STRING_CHARS );

  p = q = buffer;

  while( *p != '\0' )
  {
    //skip to first , or EOS
    while( *p != ',' && *p != '\0' )
      p++;

    if( *p == '\0' )
      EOS = qtrue;

    *p = '\0';

    //strip leading whitespace
    while( *q == ' ' )
      q++;

    buildables[ i ] = BG_BuildableByName( q )->number;

    if( buildables[ i ] == BA_NONE )
      Com_Printf( S_COLOR_YELLOW "WARNING: unknown buildable %s\n", q );
    else
      i++;

    if( !EOS )
    {
      p++;
      q = p;
    }
    else
      break;
  }

  buildables[ i ] = BA_NONE;
}

typedef struct gameElements_s
{
  buildable_t       buildables[ BA_NUM_BUILDABLES ];
  class_t           classes[ PCL_NUM_CLASSES ];
  weapon_t          weapons[ WP_NUM_WEAPONS ];
  upgrade_t         upgrades[ UP_NUM_UPGRADES ];
} gameElements_t;

static gameElements_t bg_disabledGameElements;

/*
============
BG_InitAllowedGameElements
============
*/
void BG_InitAllowedGameElements( void )
{
  char cvar[ MAX_CVAR_VALUE_STRING ];

  trap_Cvar_VariableStringBuffer( "g_disabledEquipment",
      cvar, MAX_CVAR_VALUE_STRING );

  BG_ParseCSVEquipmentList( cvar,
      bg_disabledGameElements.weapons, WP_NUM_WEAPONS,
      bg_disabledGameElements.upgrades, UP_NUM_UPGRADES );

  trap_Cvar_VariableStringBuffer( "g_disabledClasses",
      cvar, MAX_CVAR_VALUE_STRING );

  BG_ParseCSVClassList( cvar,
      bg_disabledGameElements.classes, PCL_NUM_CLASSES );

  trap_Cvar_VariableStringBuffer( "g_disabledBuildables",
      cvar, MAX_CVAR_VALUE_STRING );

  BG_ParseCSVBuildableList( cvar,
      bg_disabledGameElements.buildables, BA_NUM_BUILDABLES );
}

/*
============
BG_WeaponIsAllowed
============
*/
qboolean BG_WeaponIsAllowed( weapon_t weapon )
{
  int i;

  for( i = 0; i < WP_NUM_WEAPONS &&
      bg_disabledGameElements.weapons[ i ] != WP_NONE; i++ )
  {
    if( bg_disabledGameElements.weapons[ i ] == weapon )
      return qfalse;
  }

  return qtrue;
}

/*
============
BG_UpgradeIsAllowed
============
*/
qboolean BG_UpgradeIsAllowed( upgrade_t upgrade )
{
  int i;

  for( i = 0; i < UP_NUM_UPGRADES &&
      bg_disabledGameElements.upgrades[ i ] != UP_NONE; i++ )
  {
    if( bg_disabledGameElements.upgrades[ i ] == upgrade )
      return qfalse;
  }

  return qtrue;
}

/*
============
BG_ClassIsAllowed
============
*/
qboolean BG_ClassIsAllowed( class_t class )
{
  int i;

  for( i = 0; i < PCL_NUM_CLASSES &&
      bg_disabledGameElements.classes[ i ] != PCL_NONE; i++ )
  {
    if( bg_disabledGameElements.classes[ i ] == class )
      return qfalse;
  }

  return qtrue;
}

/*
============
BG_BuildableIsAllowed
============
*/
qboolean BG_BuildableIsAllowed( buildable_t buildable )
{
  int i;

  for( i = 0; i < BA_NUM_BUILDABLES &&
      bg_disabledGameElements.buildables[ i ] != BA_NONE; i++ )
  {
    if( bg_disabledGameElements.buildables[ i ] == buildable )
      return qfalse;
  }

  return qtrue;
}

/*
============
BG_LoadEmoticons
============
*/
int BG_LoadEmoticons( char names[ ][ MAX_EMOTICON_NAME_LEN ], int widths[ ] )
{
  int numFiles;
  char fileList[ MAX_EMOTICONS * ( MAX_EMOTICON_NAME_LEN + 9 ) ] = {""};
  int i;
  char *filePtr;
  int fileLen;
  char emoticon[ MAX_EMOTICON_NAME_LEN + 9 ] = {""};
  int loaded = 0;
  int count;
  int width = 0;

  numFiles = trap_FS_GetFileList( "emoticons", "x1.tga", fileList,
    sizeof( fileList ) );

  if( numFiles < 1 )
    return 0;

  filePtr = fileList;
  fileLen = 0;
  count = 0;
  for( i = 0; i < numFiles; i++, filePtr += fileLen + 1 )
  {
    if( count >= MAX_EMOTICONS )
    {
      count++;
      continue;
    }

    fileLen = strlen( filePtr );
    if( fileLen > MAX_EMOTICON_NAME_LEN + 8 )
    {
      Com_Printf( S_COLOR_YELLOW "WARNING: MAX_EMOTICON_NAME_LEN is %d. "
        "skipping \"%s\", filename too long", MAX_EMOTICON_NAME_LEN, filePtr );
      continue;
    }
    if( fileLen < 9 )
    {
      Com_Printf( S_COLOR_YELLOW "WARNING: skipping \"%s\", filename too short",
        filePtr );
      continue;
    }
    if( !trap_FS_FOpenFile( va( "emoticons/%s", filePtr ), NULL, FS_READ ) )
    {
      Com_Printf( S_COLOR_YELLOW "WARNING: BG_LoadEmoticons(): detected "
        " an unreadable .tga file name \"emoticons/%s\" in emoticon detection",
        filePtr );
      break;
    }

    Q_strncpyz( emoticon, filePtr, sizeof( emoticon ) );

    if( emoticon[ fileLen - 8 ] != '_' )
    {
      Com_Printf( S_COLOR_YELLOW "WARNING: skipping \"%s\", filename invalid",
        filePtr );
      continue;
    }

    width = emoticon[ fileLen - 7 ] - '0';

    if( width < 1 || width > 9 )
    {
      Com_Printf( S_COLOR_YELLOW "WARNING: skipping \"%s\", invalid width",
        filePtr );
      continue;
    }

    emoticon[ fileLen - 8 ] = '\0';

    Q_strncpyz( names[ count ], emoticon, sizeof( names[ count ] ) );
    if( widths )
      widths[ count ] = width;
    count++;
    loaded = count;
  }

  Com_Printf( "Loaded %d of %d emoticons (MAX_EMOTICONS is %d)\n",
    loaded, count, MAX_EMOTICONS );
  return loaded;
}

/*
============
BG_TeamName
============
*/
char *BG_TeamName( team_t team )
{
  if( team == TEAM_NONE )
    return "spectator";
  if( team == TEAM_ALIENS )
    return "alien";
  if( team == TEAM_HUMANS )
    return "human";
  return "<team>";
}

int cmdcmp( const void *a, const void *b )
{
  return Q_stricmp( (const char *)a, ((dummyCmd_t *)b)->name );
}
