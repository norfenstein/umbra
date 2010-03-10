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


/*
 * ALIEN weapons
 *
 * _REPEAT  - time in msec until the weapon can be used again
 * _DMG     - amount of damage the weapon does
 *
 * ALIEN_WDMG_MODIFIER - overall damage modifier for coarse tuning
 *
 */

#define ALIEN_WDMG_MODIFIER         1.0f
#define ADM(d)                      ((int)((float)d*ALIEN_WDMG_MODIFIER))

#define ABUILDER_BUILD_REPEAT        500
#define ABUILDER_CLAW_DMG            ADM(20)
#define ABUILDER_CLAW_RANGE          64.0f
#define ABUILDER_CLAW_WIDTH          4.0f
#define ABUILDER_CLAW_REPEAT         1000
#define ABUILDER_CLAW_K_SCALE        1.0f

#define ALEVEL0_BITE_DMG             ADM(36)
#define ALEVEL0_BITE_RANGE           64.0f
#define ALEVEL0_BITE_WIDTH           6.0f
#define ALEVEL0_BITE_REPEAT          500
#define ALEVEL0_BITE_K_SCALE         1.0f
#define ALEVEL0_GRAPPLE_REPEAT       400
#define ALEVEL0_GRAPPLE_SPEED        800
#define ALEVEL0_GRAPPLE_PULL_SPEED   800

#define ALEVEL1_1_CLAW_DMG           ADM(32)
#define ALEVEL1_1_CLAW_RANGE         64.0f
#define ALEVEL1_1_CLAW_WIDTH         10.0f
#define ALEVEL1_1_CLAW_REPEAT        500
#define ALEVEL1_1_CLAW_K_SCALE       1.0f
#define ALEVEL1_1_POUNCE_TIME        750      // msec for full pounce
#define ALEVEL1_1_POUNCE_TIME_MIN    200      // msec before which pounce cancels  
#define ALEVEL1_1_POUNCE_SPEED_MOD   0.75f    // walking speed modifier for pounce charging
#define ALEVEL1_1_POUNCE_JUMP_MAG    700      // pounce jump power
#define ALEVEL1_1_SPIT_REPEAT        400
#define ALEVEL1_1_SPIT_AMMO          5
#define ALEVEL1_1_SPIT_REGEN         200
#define ALEVEL1_1_SPIT_REGEN_DELAY   1200
#define ALEVEL1_1_SPIT_SPEED         800.0f
#define ALEVEL1_1_SPIT_TIME          1000

#define ALEVEL3_CLAW_DMG             ADM(40)
#define ALEVEL3_CLAW_RANGE           80.0f
#define ALEVEL3_CLAW_WIDTH           14.0f
#define ALEVEL3_CLAW_REPEAT          400
#define ALEVEL3_CLAW_K_SCALE         1.0f
#define ALEVEL3_FLAME_AMMO           10
#define ALEVEL3_FLAME_REPEAT         200
#define ALEVEL3_FLAME_DMG            ADM(20)
#define ALEVEL3_FLAME_REGEN          100
#define ALEVEL3_FLAME_REGEN_DELAY    3000
#define ALEVEL3_FLAME_SIZE           15       // missile bounding box
#define ALEVEL3_FLAME_LIFETIME       700.0f
#define ALEVEL3_FLAME_SPEED          300.0f
#define ALEVEL3_FLAME_LAG            0.65f    // the amount of player velocity that is added to the fireball

#define ALEVEL4_CLAW_DMG             ADM(80)
#define ALEVEL4_CLAW_RANGE           80.0f
#define ALEVEL4_CLAW_WIDTH           12.0f
#define ALEVEL4_CLAW_REPEAT          800
#define ALEVEL4_CLAW_K_SCALE         1.0f
#define ALEVEL4_GRAB_RANGE           80.0f
#define ALEVEL4_GRAB_TIME            300

#define ALEVEL5_CLAW_DMG             ADM(100)
#define ALEVEL5_CLAW_RANGE           100.0f
#define ALEVEL5_CLAW_WIDTH           14.0f
#define ALEVEL5_CLAW_HEIGHT          20.0f
#define ALEVEL5_CLAW_REPEAT          800
#define ALEVEL5_CLAW_K_SCALE         1.0f
#define ALEVEL5_BOUNCEBALL_DMG       ADM(110)
#define ALEVEL5_BOUNCEBALL_AMMO      10
#define ALEVEL5_BOUNCEBALL_REGEN     15000
#define ALEVEL5_BOUNCEBALL_REGEN_DELAY 0
#define ALEVEL5_BOUNCEBALL_REPEAT    1200
#define ALEVEL5_BOUNCEBALL_SPEED     1000.0f
#define ALEVEL5_BOUNCEBALL_RADIUS    75

#define ALEVEL5_TRAMPLE_DMG             ADM(111)
#define ALEVEL5_TRAMPLE_SPEED           2.0f
#define ALEVEL5_TRAMPLE_CHARGE_MIN      375   // minimum msec to start a charge
#define ALEVEL5_TRAMPLE_CHARGE_MAX      1000  // msec to maximum charge stored
#define ALEVEL5_TRAMPLE_CHARGE_TRIGGER  3000  // msec charge starts on its own
#define ALEVEL5_TRAMPLE_DURATION        3000  // msec trample lasts on full charge
#define ALEVEL5_TRAMPLE_STOP_PENALTY    1     // charge lost per msec when stopped
#define ALEVEL5_TRAMPLE_REPEAT          100   // msec before a trample will rehit a player

#define ALEVEL5_CRUSH_DAMAGE_PER_V      0.5f  // damage per falling velocity
#define ALEVEL5_CRUSH_DAMAGE            120   // to players only
#define ALEVEL5_CRUSH_REPEAT            500   // player damage repeat

/*
 * ALIEN classes
 *
 * _SPEED   - fraction of Q3A run speed the class can move
 * _REGEN   - health per second regained
 *
 * ALIEN_HLTH_MODIFIER - overall health modifier for coarse tuning
 *
 */

#define ALIEN_HLTH_MODIFIER         1.0f
#define AHM(h)                      ((int)((float)h*ALIEN_HLTH_MODIFIER))

#define ABUILDER_SPEED               0.9f
#define ABUILDER_HEALTH              AHM(75)
#define ABUILDER_REGEN               (0.04f * ABUILDER_HEALTH)
#define ABUILDER_COST                0

#define ALEVEL0_SPEED                1.4f
#define ALEVEL0_HEALTH               AHM(25)
#define ALEVEL0_REGEN                (0.05f * ALEVEL0_HEALTH)
#define ALEVEL0_COST                 0

#define ALEVEL1_1_SPEED              1.25f
#define ALEVEL1_1_HEALTH             AHM(80)
#define ALEVEL1_1_REGEN              (0.03f * ALEVEL1_1_HEALTH)
#define ALEVEL1_1_COST               0

#define ALEVEL3_SPEED                1.2f
#define ALEVEL3_HEALTH               AHM(175)
#define ALEVEL3_REGEN                (0.03f * ALEVEL3_HEALTH)
#define ALEVEL3_COST                 0
#define ALEVEL3_MAXSPEED             2000.0f

#define ALEVEL4_SPEED                1.1f
#define ALEVEL4_HEALTH               AHM(250)
#define ALEVEL4_REGEN                (0.03f * ALEVEL4_HEALTH)
#define ALEVEL4_COST                 0

#define ALEVEL5_SPEED                1.2f
#define ALEVEL5_HEALTH               AHM(350)
#define ALEVEL5_REGEN                (0.025f * ALEVEL5_HEALTH)
#define ALEVEL5_COST                 0

/*
 * ALIEN buildables
 *
 * _BP            - build points required for this buildable
 * _BT            - build time required for this buildable
 * _REGEN         - the amount of health per second regained
 * _SPLASHDAMGE   - the amount of damage caused by this buildable when melting
 * _SPLASHRADIUS  - the radius around which it does this damage
 *
 * CREEP_BASESIZE - the maximum distance a buildable can be from an egg/overmind
 * ALIEN_BHLTH_MODIFIER - overall health modifier for coarse tuning
 *
 */

#define ALIEN_BHLTH_MODIFIER        1.0f
#define ABHM(h)                     ((int)((float)h*ALIEN_BHLTH_MODIFIER))

#define CREEP_BASESIZE              700
#define CREEP_TIMEOUT               1000
#define CREEP_MODIFIER              0.5f
#define CREEP_SCALEDOWN_TIME        3000

#define ASPAWN_BP                   10
#define ASPAWN_BT                   15000
#define ASPAWN_HEALTH               ABHM(250)
#define ASPAWN_REGEN                8
#define ASPAWN_SPLASHDAMAGE         50
#define ASPAWN_SPLASHRADIUS         100
#define ASPAWN_CREEPSIZE            120

#define BARRICADE_BP                8
#define BARRICADE_BT                20000
#define BARRICADE_HEALTH            ABHM(300)
#define BARRICADE_REGEN             14
#define BARRICADE_SPLASHDAMAGE      50
#define BARRICADE_SPLASHRADIUS      100
#define BARRICADE_CREEPSIZE         120
#define BARRICADE_SHRINKPROP        0.25f
#define BARRICADE_SHRINKTIMEOUT     500

#define BOOSTER_BP                  12
#define BOOSTER_BT                  15000
#define BOOSTER_HEALTH              ABHM(150)
#define BOOSTER_REGEN               8
#define BOOSTER_SPLASHDAMAGE        50
#define BOOSTER_SPLASHRADIUS        100
#define BOOSTER_CREEPSIZE           120
#define BOOSTER_REGEN_MOD           3.0f
#define BOOST_TIME                  20000
#define BOOST_WARN_TIME             15000

#define ACIDTUBE_BP                 8
#define ACIDTUBE_BT                 15000
#define ACIDTUBE_HEALTH             ABHM(125)
#define ACIDTUBE_REGEN              10
#define ACIDTUBE_SPLASHDAMAGE       50
#define ACIDTUBE_SPLASHRADIUS       100
#define ACIDTUBE_CREEPSIZE          120
#define ACIDTUBE_DAMAGE             8
#define ACIDTUBE_RANGE              300.0f
#define ACIDTUBE_REPEAT             300
#define ACIDTUBE_REPEAT_ANIM        2000

#define HIVE_BP                     12
#define HIVE_BT                     20000
#define HIVE_HEALTH                 ABHM(125)
#define HIVE_REGEN                  10
#define HIVE_SPLASHDAMAGE           30
#define HIVE_SPLASHRADIUS           200
#define HIVE_CREEPSIZE              120
#define HIVE_SENSE_RANGE            500.0f
#define HIVE_LIFETIME               3000
#define HIVE_REPEAT                 3000
#define HIVE_K_SCALE                1.0f
#define HIVE_DMG                    80
#define HIVE_SPEED                  320.0f
#define HIVE_DIR_CHANGE_PERIOD      500

#define TRAPPER_BP                  8
#define TRAPPER_BT                  12000
#define TRAPPER_HEALTH              ABHM(50)
#define TRAPPER_REGEN               6
#define TRAPPER_SPLASHDAMAGE        15
#define TRAPPER_SPLASHRADIUS        100
#define TRAPPER_CREEPSIZE           30
#define TRAPPER_RANGE               400
#define TRAPPER_REPEAT              1000
#define LOCKBLOB_SPEED              650.0f
#define LOCKBLOB_LOCKTIME           5000
#define LOCKBLOB_DOT                0.85f // max angle = acos( LOCKBLOB_DOT )
#define LOCKBLOB_K_SCALE            1.0f

#define OVERMIND_BP                 0
#define OVERMIND_BT                 30000
#define OVERMIND_HEALTH             ABHM(750)
#define OVERMIND_REGEN              6
#define OVERMIND_SPLASHDAMAGE       15
#define OVERMIND_SPLASHRADIUS       300
#define OVERMIND_CREEPSIZE          120
#define OVERMIND_ATTACK_RANGE       150.0f
#define OVERMIND_ATTACK_REPEAT      1000

#define HOVEL_BP                    0
#define HOVEL_BT                    15000
#define HOVEL_HEALTH                ABHM(375)
#define HOVEL_REGEN                 20
#define HOVEL_SPLASHDAMAGE          20
#define HOVEL_SPLASHRADIUS          200
#define HOVEL_CREEPSIZE             120

/*
 * ALIEN misc
 *
 * ALIENSENSE_RANGE - the distance alien sense is useful for
 *
 */

#define ALIENSENSE_RANGE            1000.0f
#define REGEN_BOOST_RANGE           200.0f

#define ALIEN_POISON_TIME           10000
#define ALIEN_POISON_DMG            5
#define ALIEN_POISON_DIVIDER        (1.0f/1.32f) //about 1.0/(time`th root of damage)

#define ALIEN_SPAWN_REPEAT_TIME     10000

#define ALIEN_REGEN_DAMAGE_TIME     2000 //msec since damage that regen starts again
#define ALIEN_REGEN_NOCREEP_MOD     (1.0f/3.0f) //regen off creep

#define FLIGHT_DISABLE_TIME         1000 //time to disable flight when player damaged
#define FLIGHT_DISABLE_CHANCE       0.7f

/*
 * HUMAN weapons
 *
 * _REPEAT  - time between firings
 * _RELOAD  - time needed to reload
 *
 * HUMAN_WDMG_MODIFIER - overall damage modifier for coarse tuning
 *
 */

#define HUMAN_WDMG_MODIFIER         1.0f
#define HDM(d)                      ((int)((float)d*HUMAN_WDMG_MODIFIER))

#define BLASTER_REPEAT              600
#define BLASTER_K_SCALE             1.0f
#define BLASTER_SPREAD              200
#define BLASTER_SPEED               1400
#define BLASTER_DMG                 HDM(10)
#define BLASTER_SIZE                5

#define RIFLE_CLIPSIZE              30
#define RIFLE_MAXCLIPS              6
#define RIFLE_REPEAT                90
#define RIFLE_K_SCALE               1.0f
#define RIFLE_RELOAD                2000
#define RIFLE_SPREAD                200
#define RIFLE_DMG                   HDM(5)

#define PAINSAW_REPEAT              75
#define PAINSAW_K_SCALE             1.0f
#define PAINSAW_DAMAGE              HDM(11)
#define PAINSAW_RANGE               64.0f
#define PAINSAW_WIDTH               0.0f
#define PAINSAW_HEIGHT              8.0f

#define GRENADE_REPEAT              0
#define GRENADE_K_SCALE             1.0f
#define GRENADE_DAMAGE              HDM(310)
#define GRENADE_RANGE               192.0f
#define GRENADE_SPEED               400.0f

#define SHOTGUN_SHELLS              8
#define SHOTGUN_PELLETS             14 //used to sync server and client side
#define SHOTGUN_MAXCLIPS            3
#define SHOTGUN_REPEAT              1000
#define SHOTGUN_K_SCALE             1.0f
#define SHOTGUN_RELOAD              2000
#define SHOTGUN_SPREAD              900
#define SHOTGUN_DMG                 HDM(4)
#define SHOTGUN_RANGE               (8192 * 12)

#define LASGUN_AMMO                 60
#define LASGUN_REGEN                50
#define LASGUN_REGEN_DELAY          1000
#define LASGUN_REPEAT               200
#define LASGUN_K_SCALE              1.0f
#define LASGUN_RELOAD               2000
#define LASGUN_DAMAGE               HDM(9)

#define MDRIVER_CLIPSIZE            5
#define MDRIVER_MAXCLIPS            4
#define MDRIVER_DMG                 HDM(40)
#define MDRIVER_REPEAT              1000
#define MDRIVER_K_SCALE             1.0f
#define MDRIVER_RELOAD              2000

#define CHAINGUN_BULLETS            300
#define CHAINGUN_REPEAT             80
#define CHAINGUN_K_SCALE            1.0f
#define CHAINGUN_SPREAD             900
#define CHAINGUN_DMG                HDM(6)

#define PRIFLE_CLIPS                40
#define PRIFLE_MAXCLIPS             5
#define PRIFLE_REPEAT               100
#define PRIFLE_K_SCALE              1.0f
#define PRIFLE_RELOAD               2000
#define PRIFLE_DMG                  HDM(9)
#define PRIFLE_SPEED                1200
#define PRIFLE_SIZE                 5

#define LCANNON_AMMO                80
#define LCANNON_K_SCALE             1.0f
#define LCANNON_REPEAT              500
#define LCANNON_RELOAD              0
#define LCANNON_DAMAGE              HDM(265)
#define LCANNON_RADIUS              150      // primary splash damage radius
#define LCANNON_SIZE                5        // missile bounding box radius
#define LCANNON_SECONDARY_DAMAGE    HDM(30)
#define LCANNON_SECONDARY_RADIUS    75       // secondary splash damage radius
#define LCANNON_SECONDARY_SPEED     1400
#define LCANNON_SECONDARY_RELOAD    2000
#define LCANNON_SECONDARY_REPEAT    1000
#define LCANNON_SPEED               700
#define LCANNON_CHARGE_TIME_MAX     3000
#define LCANNON_CHARGE_TIME_MIN     100
#define LCANNON_CHARGE_TIME_WARN    2000
#define LCANNON_CHARGE_AMMO         10       // ammo cost of a full charge shot

#define HBUILD_REPEAT               1000
#define HBUILD_HEALRATE             18

/*
 * HUMAN upgrades
 */

#define SCANNER_RANGE               1000.0f

#define JETPACK_ASCEND_SPEED        540.0f //up movement speed
#define JETPACK_SKI_AMMO_TIME       250 //msec between ammo deduction for skiing
#define JETPACK_HOVER_AMMO_TIME     100 //msec between ammo deduction for hovering
#define JETPACK_ASCEND_AMMO_TIME    20 //msec between ammo deduction for rising

#define MEDKIT_POISON_IMMUNITY_TIME 0
#define MEDKIT_STARTUP_TIME         4000
#define MEDKIT_STARTUP_SPEED        5

/*
 * HUMAN buildables
 *
 * _BP            - build points required for this buildable
 * _BT            - build time required for this buildable
 * _SPLASHDAMGE   - the amount of damage caused by this buildable when it blows up
 * _SPLASHRADIUS  - the radius around which it does this damage
 *
 * REACTOR_BASESIZE - the maximum distance a buildable can be from an reactor
 * REPEATER_BASESIZE - the maximum distance a buildable can be from a repeater
 * HUMAN_BHLTH_MODIFIER - overall health modifier for coarse tuning
 *
 */

#define HUMAN_BHLTH_MODIFIER        1.0f
#define HBHM(h)                     ((int)((float)h*HUMAN_BHLTH_MODIFIER))

#define REACTOR_BASESIZE            1000
#define REPEATER_BASESIZE           500
#define HUMAN_DETONATION_DELAY      5000

#define HSPAWN_BP                   10
#define HSPAWN_BT                   10000
#define HSPAWN_HEALTH               HBHM(310)
#define HSPAWN_SPLASHDAMAGE         50
#define HSPAWN_SPLASHRADIUS         100

#define MEDISTAT_BP                 8
#define MEDISTAT_BT                 10000
#define MEDISTAT_HEALTH             HBHM(190)
#define MEDISTAT_SPLASHDAMAGE       50
#define MEDISTAT_SPLASHRADIUS       100

#define MGTURRET_BP                 8
#define MGTURRET_BT                 10000
#define MGTURRET_HEALTH             HBHM(190)
#define MGTURRET_SPLASHDAMAGE       100
#define MGTURRET_SPLASHRADIUS       100
#define MGTURRET_ANGULARSPEED       12
#define MGTURRET_ACCURACY_TO_FIRE   0
#define MGTURRET_VERTICALCAP        30  // +/- maximum pitch
#define MGTURRET_REPEAT             150
#define MGTURRET_K_SCALE            1.0f
#define MGTURRET_RANGE              400.0f
#define MGTURRET_SPREAD             200
#define MGTURRET_DMG                HDM(8)
#define MGTURRET_SPINUP_TIME        750 // time between target sighted and fire

#define TESLAGEN_BP                 10
#define TESLAGEN_BT                 15000
#define TESLAGEN_HEALTH             HBHM(220)
#define TESLAGEN_SPLASHDAMAGE       50
#define TESLAGEN_SPLASHRADIUS       100
#define TESLAGEN_REPEAT             250
#define TESLAGEN_K_SCALE            4.0f
#define TESLAGEN_RANGE              150
#define TESLAGEN_DMG                HDM(10)

#define DC_BP                       8
#define DC_BT                       10000
#define DC_HEALTH                   HBHM(190)
#define DC_SPLASHDAMAGE             50
#define DC_SPLASHRADIUS             100
#define DC_ATTACK_PERIOD            10000 // how often to spam "under attack"
#define DC_HEALRATE                 3
#define DC_RANGE                    1000

#define ARMOURY_BP                  10
#define ARMOURY_BT                  10000
#define ARMOURY_HEALTH              HBHM(420)
#define ARMOURY_SPLASHDAMAGE        50
#define ARMOURY_SPLASHRADIUS        100

#define REACTOR_BP                  0
#define REACTOR_BT                  20000
#define REACTOR_HEALTH              HBHM(930)
#define REACTOR_SPLASHDAMAGE        200
#define REACTOR_SPLASHRADIUS        300
#define REACTOR_ATTACK_RANGE        100.0f
#define REACTOR_ATTACK_REPEAT       1000
#define REACTOR_ATTACK_DAMAGE       40
#define REACTOR_ATTACK_DCC_REPEAT   1000
#define REACTOR_ATTACK_DCC_RANGE    150.0f
#define REACTOR_ATTACK_DCC_DAMAGE   40

#define REPEATER_BP                 4
#define REPEATER_BT                 10000
#define REPEATER_HEALTH             HBHM(250)
#define REPEATER_SPLASHDAMAGE       50
#define REPEATER_SPLASHRADIUS       100

/*
 * HUMAN misc
 */

#define HUMAN_DODGE_SIDE_MODIFIER   2.5f
#define HUMAN_DODGE_SLOWED_MODIFIER 0.9f
#define HUMAN_DODGE_UP_MODIFIER     0.5f
#define HUMAN_DODGE_TIMEOUT         500
#define HUMAN_LAND_FRICTION         3.0f

#define HUMAN_SPAWN_REPEAT_TIME     10000
#define HUMAN_REGEN_DAMAGE_TIME     2000 //msec since damage before dcc repairs

#define HUMAN_BUILDABLE_INACTIVE_TIME 90000

/*
 * Misc
 */

#define MAX_CREDITS                 1000
#define MIN_CREDITS                 -1000
#define CREDITS_PER_FRAG            100

#define DEFAULT_FREEKILL_PERIOD     "120" //seconds
#define FREEKILL_VALUE              1

#define BUILDER_SCOREINC            0 // builders receive this many points every 10 seconds

#define MIN_FALL_DISTANCE           30.0f  //the fall distance at which fall damage kicks in
#define MAX_FALL_DISTANCE           120.0f //the fall distance at which maximum damage is dealt
#define AVG_FALL_DISTANCE           ((MIN_FALL_DISTANCE+MAX_FALL_DISTANCE)/2.0f)

#define DEFAULT_ALIEN_BUILDPOINTS   "100"
#define DEFAULT_ALIEN_QUEUE_TIME    "8000"
#define DEFAULT_HUMAN_BUILDPOINTS   "100"
#define DEFAULT_HUMAN_QUEUE_TIME    "8000"
                                         
                                         
#define MAXIMUM_BUILD_TIME          20000 // used for pie timer

