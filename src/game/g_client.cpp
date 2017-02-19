#include <bgame/impl.h>
#include <omnibot/et/g_etbot_interface.h>

// g_client.c -- client functions that don't happen every frame

// Ridah, new bounding box
//static vec3_t	playerMins = {-15, -15, -24};
//static vec3_t	playerMaxs = {15, 15, 32};
vec3_t	playerMins = {-18, -18, -24};
vec3_t	playerMaxs = {18, 18, 48};
// done.

/*QUAKED info_player_deathmatch (1 0 1) (-18 -18 -24) (18 18 48)
potential spawning position for deathmatch games.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
If the start position is targeting an entity, the players camera will start out facing that ent (like an info_notnull)
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;
	vec3_t	dir;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	ent->enemy = G_PickTarget( ent->target );
	if(ent->enemy)
	{
		VectorSubtract( ent->enemy->s.origin, ent->s.origin, dir );
		vectoangles( dir, ent->s.angles );
	}

}

//----(SA) added
/*QUAKED info_player_checkpoint (1 0 0) (-16 -16 -24) (16 16 32) a b c d
these are start points /after/ the level start
the letter (a b c d) designates the checkpoint that needs to be complete in order to use this start position
*/
void SP_info_player_checkpoint(gentity_t *ent) {
	ent->classname = "info_player_checkpoint";
	SP_info_player_deathmatch( ent );
}

//----(SA) end


/*QUAKED info_player_start (1 0 0) (-18 -18 -24) (18 18 48)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) AXIS ALLIED
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

extern void BotSpeedBonus(int clientNum);


/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->r.currentOrigin, playerMins, mins );
	VectorAdd( spot->r.currentOrigin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
			return qtrue;
		}

	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( const vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->r.currentOrigin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( const vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) { 
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}		
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->r.currentOrigin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
/*gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy (spot->r.currentOrigin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}*/

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodyUnlink
  
Called by BodySink
=============
*/
void BodyUnlink( gentity_t *ent ) {
	trap_UnlinkEntity( ent );
	ent->physicsObject = qfalse;
}
                
/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear 
=============
*/ 
void BodySink2( gentity_t *ent ) {
	ent->physicsObject = qfalse;
    ent->nextthink = level.time + BODY_TIME(BODY_TEAM(ent))+1500;
    ent->think = BodyUnlink;
    ent->s.pos.trType = TR_LINEAR;
    ent->s.pos.trTime = level.time;
    VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	VectorSet( ent->s.pos.trDelta, 0, 0, -8 );
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if( ent->activator ) {
		// see if parent is still disguised
		if( ent->activator->client->ps.powerups[PW_OPS_DISGUISED] ) {
			ent->nextthink = level.time + 100;
			return;
		} else {
			ent->activator = NULL;
		}
	}

	BodySink2( ent );
}


/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents, i;

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
  	contents = trap_PointContents( ent->client->ps.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	// Gordon: um, what on earth was this here for?
//	trap_UnlinkEntity (body);

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc

	if( ent->client->ps.eFlags & EF_HEADSHOT ) {
		body->s.eFlags |= EF_HEADSHOT;			// make sure the dead body draws no head (if killed that way)
	}

	body->s.eType = ET_CORPSE;
	body->classname = "corpse";
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// DHM - Clear out event system
	for( i=0; i<MAX_EVENTS; i++ )
		body->s.events[i] = 0;
	body->s.eventSequence = 0;

	// DHM - Nerve
	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) 
	{
	case BOTH_DEATH1:	
	case BOTH_DEAD1:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags & ~SVF_BOT;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);
	
	// ydnar: bodies have lower bounding box
	body->r.maxs[ 2 ] = 0;

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	// DHM - Nerve :: allow bullets to pass through bbox
	// Gordon: need something to allow the hint for covert ops
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->r.ownerNum;

	BODY_TEAM(body) =		ent->client->sess.sessionTeam;
	BODY_CLASS(body) =		ent->client->sess.playerType;
	BODY_CHARACTER(body) =	ent->client->pers.characterIndex;
	BODY_VALUE(body) =		0;
	BODY_WEAPON(body) =		ent->client->sess.playerWeapon;

    if (ent->client->sess.playerWeapon == WP_MORTAR) {
        BODY_AMMO(body) = 0;
    } else {
        BODY_AMMO(body) =       ent->client->ps.ammo[ent->client->sess.playerWeapon];
    }

	body->s.time2 =			0;

	body->activator = NULL;

	body->nextthink = level.time + BODY_TIME(ent->client->sess.sessionTeam);

	body->think = BodySink;

	body->die = body_die;

	// Jaybird - if their uniform was stolen,
	// make sure that it is still lost.
	if( G_HasJayFlag( ent, 1, JF_LOSTPANTS )) {
		BODY_TEAM(body) += 4;
	}

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);
}

//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

void SetClientViewAnglePitch( gentity_t *ent, vec_t angle ) {
	int	cmdAngle;

	cmdAngle = ANGLE2SHORT(angle);
	ent->client->ps.delta_angles[PITCH] = cmdAngle - ent->client->pers.cmd.angles[PITCH];

	ent->s.angles[ PITCH ] = 0;
	VectorCopy( ent->s.angles, ent->client->ps.viewangles);
}

/* JPW NERVE
================
limbo
================
*/
void limbo( gentity_t *ent, qboolean makeCorpse ) 
{
	int i,contents;
	//int startclient = ent->client->sess.spectatorClient;
	int startclient = ent->client->ps.clientNum;

	if(ent->r.svFlags & SVF_POW) {
		return;
	}

	if (!(ent->client->ps.pm_flags & PMF_LIMBO)) {

		if( ent->client->ps.persistant[PERS_RESPAWNS_LEFT] == 0 ) {
			if( g_maxlivesRespawnPenalty.integer ) {
				ent->client->ps.persistant[PERS_RESPAWNS_PENALTY] = g_maxlivesRespawnPenalty.integer;
			} else {
				ent->client->ps.persistant[PERS_RESPAWNS_PENALTY] = -1;
			}
		}

		// DHM - Nerve :: First save off persistant info we'll need for respawn
		for( i = 0; i < MAX_PERSISTANT; i++) {
			ent->client->saved_persistant[i] = ent->client->ps.persistant[i];
		}

		ent->client->ps.pm_flags |= PMF_LIMBO;
		ent->client->ps.pm_flags |= PMF_FOLLOW;
		
		if( makeCorpse ) {
			CopyToBodyQue (ent); // make a nice looking corpse
		} else {
			trap_UnlinkEntity (ent);
		}

		// DHM - Nerve :: reset these values
		ent->client->ps.viewlocked = 0;
		ent->client->ps.viewlocked_entNum = 0;

		ent->r.maxs[2] = 0;
		ent->r.currentOrigin[2] += 8;
		contents = trap_PointContents( ent->r.currentOrigin, -1 ); // drop stuff
		ent->s.weapon = ent->client->limboDropWeapon; // stored in player_die()
		if ( makeCorpse && !( contents & CONTENTS_NODROP ) ) {
			TossClientItems( ent );
		}

		ent->client->sess.spectatorClient = startclient;
		Cmd_FollowCycle_f(ent,1); // get fresh spectatorClient

		if (ent->client->sess.spectatorClient == startclient) {
			// No one to follow, so just stay put
			ent->client->sess.spectatorState = SPECTATOR_FREE;
		}
		else
			ent->client->sess.spectatorState = SPECTATOR_FOLLOW;

//		ClientUserinfoChanged( ent->client - level.clients );		// NERVE - SMF - don't do this
		if (ent->client->sess.sessionTeam == TEAM_AXIS) {
			ent->client->deployQueueNumber = level.redNumWaiting;
			level.redNumWaiting++;
		}
		else if (ent->client->sess.sessionTeam == TEAM_ALLIES) {
			ent->client->deployQueueNumber = level.blueNumWaiting;
			level.blueNumWaiting++;
		}

		// Jaybird - g_spectator
		if (g_spectator.integer & SPEC_PERSIST)
			return;
			
		for( i = 0; i < level.numConnectedClients; i++ ) {
			gclient_t *cl = &level.clients[level.sortedClients[i]];

			if( cl->sess.spectatorClient != (ent - g_entities) )
				continue;

			if (cl->ps.pm_flags & PMF_LIMBO) {
				Cmd_FollowCycle_f( &g_entities[level.sortedClients[i]], 1);
			} else {
				if (g_spectator.integer & SPEC_FREE) {
					StopFollowing(&g_entities[level.sortedClients[i]]);
				} else {
					Cmd_FollowCycle_f( &g_entities[level.sortedClients[i]], 1 );
				}
			}
		}
	}
}

/* JPW NERVE
================
reinforce 
================
// -- called when time expires for a team deployment cycle and there is at least one guy ready to go
*/
void reinforce(gentity_t *ent) {
	int p, team;// numDeployable=0, finished=0; // TTimo unused
	char *classname;
	gclient_t *rclient;

	if (!(ent->client->ps.pm_flags & PMF_LIMBO)) {
		G_Printf("player already deployed, skipping\n");
		return;
	}

	if(ent->client->pers.mvCount > 0) {
		G_smvRemoveInvalidClients(ent, TEAM_AXIS);
		G_smvRemoveInvalidClients(ent, TEAM_ALLIES);
	}

	// get team to deploy from passed entity
	team = ent->client->sess.sessionTeam;

	// find number active team spawnpoints
	if (team == TEAM_AXIS)
		classname = "team_CTF_redspawn";
	else if (team == TEAM_ALLIES)
		classname = "team_CTF_bluespawn";
	else
		assert(0);

	// DHM - Nerve :: restore persistant data now that we're out of Limbo
	rclient = ent->client;
	for (p=0; p<MAX_PERSISTANT; p++)
		rclient->ps.persistant[p] = rclient->saved_persistant[p];
	// dhm

	respawn(ent);
}
// jpw


/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {

	ent->client->ps.pm_flags &= ~PMF_LIMBO; // JPW NERVE turns off limbo

	// DHM - Nerve :: Decrease the number of respawns left
	if( g_gametype.integer != GT_WOLF_LMS ) {
		if( ent->client->ps.persistant[PERS_RESPAWNS_LEFT] > 0 && cvars::gameState.ivalue == GS_PLAYING ) {
			if( g_maxlives.integer > 0 ) {
				ent->client->ps.persistant[PERS_RESPAWNS_LEFT]--;
			} else {
				if( g_alliedmaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_ALLIES ) {
					ent->client->ps.persistant[PERS_RESPAWNS_LEFT]--;
				}
				if( g_axismaxlives.integer > 0 && ent->client->sess.sessionTeam == TEAM_AXIS ) {
					ent->client->ps.persistant[PERS_RESPAWNS_LEFT]--;
				}
			}
		}
	}

	G_DPrintf( "Respawning %s, %i lives left\n", ent->client->pers.netname, ent->client->ps.persistant[PERS_RESPAWNS_LEFT]);

	ClientSpawn(ent, qfalse);

	// DHM - Nerve :: Add back if we decide to have a spawn effect
	// add a teleportation effect
	//tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
	//tent->s.clientNum = ent->s.clientNum;
}

// NERVE - SMF - merge from team arena
/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount(int ignoreClientNum, int team)
{
	int i, ref, count = 0;

	for(i=0; i<level.numConnectedClients; i++) {
		if((ref = level.sortedClients[i]) == ignoreClientNum) continue;
		if(level.clients[ref].sess.sessionTeam == team) count++;
	}

	return (team_t)count;
}
// -NERVE - SMF

/*
================
PickTeam

================
*/
team_t PickTeam(int ignoreClientNum)
{
	int counts[TEAM_NUM_TEAMS] = { 0, 0, 0 };

	counts[TEAM_ALLIES] = TeamCount(ignoreClientNum, TEAM_ALLIES);
	counts[TEAM_AXIS] = TeamCount(ignoreClientNum, TEAM_AXIS);

	if(counts[TEAM_ALLIES] > counts[TEAM_AXIS]) return(TEAM_AXIS);
	if(counts[TEAM_AXIS] > counts[TEAM_ALLIES]) return(TEAM_ALLIES);

	// equal team count, so join the team with the lowest score
	return(((level.teamScores[TEAM_ALLIES] > level.teamScores[TEAM_AXIS]) ? TEAM_AXIS : TEAM_ALLIES));
}

/*
===========
AddExtraSpawnAmmo
===========
*/
static void AddExtraSpawnAmmo( gclient_t *client, weapon_t weaponNum)
{
	switch( weaponNum ) {
		//case WP_KNIFE:
		case WP_LUGER:
		case WP_COLT:
		case WP_STEN:
		case WP_SILENCER:
		case WP_CARBINE:
		case WP_KAR98:
		case WP_SILENCED_COLT:
			if( client->sess.skill[SK_LIGHT_WEAPONS] >= 1 )
				client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
			break;
		case WP_MP40:
		case WP_THOMPSON:
		case WP_M97:
			if( (client->sess.skill[SK_FIRST_AID] >= 1 && client->sess.playerType == PC_MEDIC) || client->sess.skill[SK_LIGHT_WEAPONS] >= 1 ) {
				client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
			}
			break;
		case WP_M7:
		case WP_GPG40:
			if( client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1 )
				client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += 4;
			break;
		case WP_GRENADE_PINEAPPLE:
		case WP_GRENADE_LAUNCHER:
			switch (client->sess.playerType) {
				case PC_ENGINEER:
					if (client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 5 && cvars::bg_sk5_eng.ivalue & SK5_ENG_GRENADES)
						client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 6;
					else if (client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 1)
						client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 4;
					break;

				case PC_SOLDIER:
					if (client->sess.skill[SK_HEAVY_WEAPONS] >= 5 && cvars::bg_sk5_soldier.ivalue & SK5_SOL_GRENADES)
						client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 4;
					break;

				case PC_MEDIC:
					if (client->sess.skill[SK_FIRST_AID] >= 5 && cvars::bg_sk5_medic.ivalue & SK5_MED_GRENADES)
						client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 3;
					else if (client->sess.skill[SK_FIRST_AID] >= 1)
						client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 1;
					break;

				case PC_FIELDOPS:
					if (client->sess.skill[SK_SIGNALS] >= 5 && cvars::bg_sk5_fdops.ivalue & SK5_FDO_GRENADES)
						client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 1;
					break;

				case PC_COVERTOPS:
					if (client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 5 && cvars::bg_sk5_cvops.ivalue & SK5_CVO_GRENADES)
						client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 2;
					break;

			}
			break;
		/*case WP_MOBILE_MG42:
		case WP_PANZERFAUST:
		case WP_FLAMETHROWER:
			if( client->sess.skill[SK_HEAVY_WEAPONS] >= 1 )
				client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
			break;
		case WP_MORTAR:
		case WP_MORTAR_SET:
			if( client->sess.skill[SK_HEAVY_WEAPONS] >= 1 )
				client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += 2;
			break;*/
		case WP_MEDIC_SYRINGE:
		case WP_MEDIC_ADRENALINE:
		case WP_ADRENALINE_SHARE:
		case WP_POISON_SYRINGE:
			if( client->sess.skill[SK_FIRST_AID] >= 2 )
 				client->ps.ammoclip[BG_FindAmmoForWeapon(weaponNum)] += 2;
			break;
		case WP_GARAND:
		case WP_K43:
		case WP_FG42:
			if( client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1 || client->sess.skill[SK_LIGHT_WEAPONS] >= 1 )
				client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
			break;
		case WP_GARAND_SCOPE:
		case WP_K43_SCOPE:
		case WP_FG42SCOPE:
			if( client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 1 )
				client->ps.ammo[BG_FindAmmoForWeapon(weaponNum)] += GetAmmoTableData(weaponNum)->maxclip;
			break;
		default:
			break;
	}
}

qboolean AddWeaponToPlayer( gclient_t *client, weapon_t weapon, int ammo, int ammoclip, qboolean setcurrent ) {
	COM_BitSet( client->ps.weapons, weapon );
	client->ps.ammoclip[BG_FindClipForWeapon(weapon)] = ammoclip;
	client->ps.ammo[BG_FindAmmoForWeapon(weapon)] = ammo;
	if( setcurrent )
		client->ps.weapon = weapon;

	// skill handling
	AddExtraSpawnAmmo( client, weapon );

	Bot_Event_AddWeapon(client->ps.clientNum, Bot_WeaponGameToBot(weapon));

	return qtrue;
}

void BotSetPOW(int entityNum, qboolean isPOW);

/*
 G_AddClassSpecificTools  //tjw
 This stuff was stripped out of SetWolfSpawnWeapons()  so it can
 be used for class stealing.  It needed cleaning up badly anyway.
 It still does.
 */
void G_AddClassSpecificTools(gclient_t *client) 
{
	qboolean add_binocs = qfalse;
	if(client->sess.skill[SK_BATTLE_SENSE] >= 1)
		add_binocs = qtrue;

	switch(client->sess.playerType) {
		case PC_ENGINEER:
			AddWeaponToPlayer( client, WP_DYNAMITE, 0, 1, qfalse );
			AddWeaponToPlayer( client, WP_PLIERS, 0, 1, qfalse );
			AddWeaponToPlayer( client, WP_LANDMINE, GetAmmoTableData(WP_LANDMINE)->defaultStartingAmmo, GetAmmoTableData(WP_LANDMINE)->defaultStartingClip, qfalse );

            if ((cvars::bg_weapons.ivalue & SBW_MOLOTOV) && client->sess.skill[SK_LIGHT_WEAPONS] >= 2)
			    AddWeaponToPlayer(client, WP_MOLOTOV, GetAmmoTableData(WP_MOLOTOV)->defaultStartingAmmo, GetAmmoTableData(WP_MOLOTOV)->defaultStartingClip, qfalse);

			if (client->sess.skill[SK_EXPLOSIVES_AND_CONSTRUCTION] >= 5) {
				if (cvars::bg_sk5_eng.ivalue & SK5_ENG_LM_BBETTY)
					AddWeaponToPlayer(client, WP_LANDMINE_BBETTY, GetAmmoTableData(WP_LANDMINE_BBETTY)->defaultStartingAmmo, GetAmmoTableData(WP_LANDMINE_BBETTY)->defaultStartingClip, qfalse );
				if (cvars::bg_sk5_eng.ivalue & SK5_ENG_LM_PGAS)
					AddWeaponToPlayer(client, WP_LANDMINE_PGAS, GetAmmoTableData(WP_LANDMINE_PGAS)->defaultStartingAmmo, GetAmmoTableData(WP_LANDMINE_PGAS)->defaultStartingClip, qfalse );
			}
			break;
		case PC_COVERTOPS:
			add_binocs = qtrue;

			// SniperWar hijacking
			if (cvars::bg_sniperWar.ivalue)
				break;

			AddWeaponToPlayer(client, WP_SMOKE_BOMB, GetAmmoTableData(WP_SMOKE_BOMB)->defaultStartingAmmo, GetAmmoTableData(WP_SMOKE_BOMB)->defaultStartingClip, qfalse);
			// See if we already have a satchel charge placed 
			// this needs to be here for class stealing
			if( G_FindSatchel( &g_entities[client->ps.clientNum] ) ) {
				AddWeaponToPlayer( client, WP_SATCHEL, 0, 0, qfalse );
				AddWeaponToPlayer( client, WP_SATCHEL_DET, 0, 1, qfalse );
			} else {
				AddWeaponToPlayer( client, WP_SATCHEL, 0, 1, qfalse );
				AddWeaponToPlayer( client, WP_SATCHEL_DET, 0, 0, qfalse );
			}

			if (client->sess.skill[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] >= 5) {
                if (cvars::bg_sk5_cvops.ivalue & SK5_CVO_POISON) {
				    AddWeaponToPlayer(client, WP_POISON_GAS, GetAmmoTableData(WP_POISON_GAS)->defaultStartingAmmo, GetAmmoTableData(WP_POISON_GAS)->defaultStartingClip, qfalse);
                }
			}
			break;
		case PC_FIELDOPS:
			add_binocs = qtrue;
			if ( client->sess.skill[SK_SIGNALS] < 1 && (cvars::bg_weapons.ivalue & SBW_FOPS))
				add_binocs = qfalse;

			AddWeaponToPlayer(client, WP_AMMO, 0, 1, qfalse);
			AddWeaponToPlayer( client, WP_SMOKE_MARKER, GetAmmoTableData(WP_SMOKE_MARKER)->defaultStartingAmmo, GetAmmoTableData(WP_SMOKE_MARKER)->defaultStartingClip, qfalse);

            if ((cvars::bg_weapons.ivalue & SBW_MOLOTOV) && client->sess.skill[SK_LIGHT_WEAPONS] >= 2)
			    AddWeaponToPlayer(client, WP_MOLOTOV, GetAmmoTableData(WP_MOLOTOV)->defaultStartingAmmo, GetAmmoTableData(WP_MOLOTOV)->defaultStartingClip, qfalse);
			break;
		case PC_MEDIC:
			AddWeaponToPlayer(client, WP_MEDIC_SYRINGE, GetAmmoTableData(WP_MEDIC_SYRINGE)->defaultStartingAmmo, GetAmmoTableData(WP_MEDIC_SYRINGE)->defaultStartingClip, qfalse);
			AddWeaponToPlayer(client, WP_MEDKIT, GetAmmoTableData(WP_MEDKIT)->defaultStartingAmmo, GetAmmoTableData(WP_MEDKIT)->defaultStartingClip, qfalse);
			break;
		case PC_SOLDIER:
			if (client->sess.skill[SK_HEAVY_WEAPONS] >= 5) {
                if (cvars::bg_sk5_soldier.ivalue & SK5_SOL_POISON) {
				    AddWeaponToPlayer(client, WP_POISON_GAS, GetAmmoTableData(WP_POISON_GAS)->defaultStartingAmmo, GetAmmoTableData(WP_POISON_GAS)->defaultStartingClip, qfalse);
                }
			}

            if ((cvars::bg_weapons.ivalue & SBW_MOLOTOV) && client->sess.skill[SK_LIGHT_WEAPONS] >= 2)
			    AddWeaponToPlayer(client, WP_MOLOTOV, GetAmmoTableData(WP_MOLOTOV)->defaultStartingAmmo, GetAmmoTableData(WP_MOLOTOV)->defaultStartingClip, qfalse);
			break;
	}

    // Binoculars
	if (add_binocs) {
		if(AddWeaponToPlayer(client, WP_BINOCULARS, 1, 0, qfalse)) {
			client->ps.stats[STAT_KEYS] |= ( 1 << INV_BINOCS );
		}
	}

	// Adrenaline
    if (!(cvars::bg_weapons.ivalue & SBW_NOADRENALINE) &&
        client->sess.skill[SK_FIRST_AID] >= 4)
    {
        if (client->sess.playerType == PC_MEDIC || (cvars::bg_skills.ivalue & SBS_MEDI)) {
            AddWeaponToPlayer(
                client,
                WP_MEDIC_ADRENALINE,
                GetAmmoTableData( WP_MEDIC_ADRENALINE )->defaultStartingAmmo,
                GetAmmoTableData( WP_MEDIC_ADRENALINE )->defaultStartingClip,
                qfalse );
        }

        if (client->sess.playerType == PC_MEDIC && g_medics.integer & MEDIC_SHAREADRENALINE) {
            AddWeaponToPlayer(
                client,
                WP_ADRENALINE_SHARE,
                GetAmmoTableData( WP_ADRENALINE_SHARE )->defaultStartingAmmo,
                GetAmmoTableData( WP_ADRENALINE_SHARE )->defaultStartingClip,
                qfalse );
		}
    }

    // Poison Syringes
    if (cvars::bg_poisonSyringes.ivalue) {
        AddWeaponToPlayer(
            client,
            WP_POISON_SYRINGE,
            GetAmmoTableData( WP_POISON_SYRINGE )->defaultStartingAmmo,
            GetAmmoTableData( WP_POISON_SYRINGE )->defaultStartingClip,
            qfalse );
    }
}

static bool G_IsPrimaryWeapon(int classnum, team_t teamnum, weapon_t weapnum) {

	if (classnum != PC_COVERTOPS) {
		switch (weapnum) {
		case WP_MP40:
			return (teamnum == TEAM_AXIS);
		case WP_THOMPSON:
			return (teamnum == TEAM_ALLIES);
		default:
			break;
		}
	}

    switch (classnum) {
    case PC_SOLDIER:
        switch (weapnum) {
        case WP_PANZERFAUST:
        case WP_MORTAR:
        case WP_MOBILE_MG42:
        case WP_FLAMETHROWER:
        case WP_M97:
            return true;
        default:
            return false;
        }
    case PC_MEDIC:
        switch (weapnum) {
        case WP_M97:
            return true;
        default:
            return false;
        }
    case PC_FIELDOPS:
        switch (weapnum) {
        case WP_M97:
            return true;
        default:
            return false;
        }
    case PC_COVERTOPS:
        switch (weapnum) {
        case WP_STEN:
        case WP_FG42:
            return true;
        case WP_GARAND:
            return (teamnum == TEAM_ALLIES);
        case WP_K43:
            return (teamnum == TEAM_AXIS);
        default:
            return false;
        }
    case PC_ENGINEER:
        switch (weapnum) {
        case WP_M97:
            return true;
        case WP_CARBINE:
            return (teamnum == TEAM_ALLIES);
        case WP_KAR98:
            return (teamnum == TEAM_AXIS);
        default:
            return false;
        }
    }

    // Something went really bad if they got here :)
    return false;
}

static bool G_IsSecondaryWeapon(gclient_t* client, weapon_t weapnum) {
    int classnum = client->sess.playerType;
    team_t teamnum = client->sess.sessionTeam;

    // Soldier is somewhat special because it offers more than pistols
    if (classnum == PC_SOLDIER) {
        if (weapnum == WP_M97)
            return (client->sess.skill[SK_HEAVY_WEAPONS] >= 4);
        if (weapnum == WP_MP40)
            return (teamnum == TEAM_AXIS && client->sess.skill[SK_HEAVY_WEAPONS] >= 4);
        if (weapnum == WP_THOMPSON)
            return (teamnum == TEAM_ALLIES && client->sess.skill[SK_HEAVY_WEAPONS] >= 4);
    }

    if (classnum == PC_COVERTOPS) {
        // Allied weapons
        if (weapnum == WP_SILENCED_COLT) {
            return (teamnum == TEAM_ALLIES);
        }
        else if (weapnum == WP_AKIMBO_SILENCEDCOLT) {
            return (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && teamnum == TEAM_ALLIES);
        }

        // Axis weapons
        if (weapnum == WP_SILENCER) {
            return (teamnum == TEAM_AXIS);
        }
        else if (weapnum == WP_AKIMBO_SILENCEDLUGER) {
            return (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && teamnum == TEAM_AXIS);
        }
    } else {
        // Allied weapons
        if (weapnum == WP_COLT) {
            return (teamnum == TEAM_ALLIES);
        }
        else if (weapnum == WP_AKIMBO_COLT) {
            return (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && teamnum == TEAM_ALLIES);
        }

        // Axis weapons
        if (weapnum == WP_LUGER) {
            return (teamnum == TEAM_AXIS);
        }
        else if (weapnum == WP_AKIMBO_LUGER) {
            return (client->sess.skill[SK_LIGHT_WEAPONS] >= 4 && teamnum == TEAM_AXIS);
        }
    }

    return false;
}

qboolean _SetSoldierSpawnWeapons(gclient_t *client) 
{
    // Should be on a team
    if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
        return qfalse;

    // Get Primary weapon
    weapon_t w = (weapon_t)client->sess.latchPlayerWeapon;

    // Make sure the class can use it
    if (!G_IsPrimaryWeapon(client->sess.playerType, client->sess.sessionTeam, w)) {
        w = (client->sess.sessionTeam == TEAM_AXIS) ? WP_MP40 : WP_THOMPSON;
    }

	// Jaybird - override weapon select in panzerwar
	if( cvars::bg_panzerWar.ivalue ) {
		w = WP_PANZERFAUST;
	}

    // Add the primary weapon
	AddWeaponToPlayer(client, w, GetAmmoTableData(w)->defaultStartingAmmo, GetAmmoTableData(w)->defaultStartingClip, qtrue);

	// snax - Add Engitools if enabled
	if( cvars::bg_omniEngi.ivalue ) {
		AddWeaponToPlayer( client, WP_DYNAMITE, 0, 1, qfalse );
		AddWeaponToPlayer( client, WP_PLIERS, 0, 1, qfalse );
	}

    // Handle secondaries
	if( w == WP_MOBILE_MG42 ) {
		AddWeaponToPlayer(client, WP_MOBILE_MG42_SET, GetAmmoTableData(WP_MOBILE_MG42_SET)->defaultStartingAmmo, GetAmmoTableData(WP_MOBILE_MG42_SET)->defaultStartingClip, qfalse);
	}
	else if( w == WP_MORTAR ) {
		AddWeaponToPlayer(client, WP_MORTAR_SET, GetAmmoTableData(WP_MORTAR_SET)->defaultStartingAmmo, GetAmmoTableData(WP_MORTAR_SET)->defaultStartingClip, qfalse);
	}

    // Get secondary weapon if not panzerwar
    if (!cvars::bg_panzerWar.ivalue) {
        weapon_t w2 = (weapon_t)client->sess.latchPlayerWeapon2;

        // Make sure they can use it
        if (!G_IsSecondaryWeapon(client, w2)) {
            w2 = (client->sess.sessionTeam == TEAM_AXIS) ? WP_LUGER : WP_COLT;
        }

        // Add ammo for akimbos
	    if(w2 == WP_AKIMBO_LUGER || w2 == WP_AKIMBO_COLT) {
		    client->ps.ammoclip[BG_FindClipForWeapon((weapon_t)BG_AkimboSidearm(w2))] = GetAmmoTableData(w2)->defaultStartingClip;
	    }

        // Add secondary weapon
	    AddWeaponToPlayer(client, w2, GetAmmoTableData(w2)->defaultStartingAmmo, GetAmmoTableData(w2)->defaultStartingClip, qfalse);
    }

    // Grenades
	if( client->sess.sessionTeam == TEAM_AXIS ) {
		// Panzerwar get 100 grenades
		if( cvars::bg_panzerWar.ivalue )
            AddWeaponToPlayer( client, WP_GRENADE_LAUNCHER, 0, 100, qfalse );
		else
            AddWeaponToPlayer( client, WP_GRENADE_LAUNCHER, 0, 4, qfalse );
 	}
	else if( client->sess.sessionTeam == TEAM_ALLIES ) {
		// Panzerwar get 100 grenades
		if( cvars::bg_panzerWar.ivalue )
			AddWeaponToPlayer( client, WP_GRENADE_PINEAPPLE, 0, 100, qfalse );
		else
			AddWeaponToPlayer( client, WP_GRENADE_PINEAPPLE, 0, 4, qfalse );
	}

	return qtrue;
}

qboolean _SetMedicSpawnWeapons(gclient_t *client) 
{
    // Should be on a team
    if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
        return qfalse;

    // Get Primary weapon
    weapon_t w = (weapon_t)client->sess.latchPlayerWeapon;

    // Make sure the class can use it
    if (!G_IsPrimaryWeapon(client->sess.playerType, client->sess.sessionTeam, w)) {
        w = (client->sess.sessionTeam == TEAM_AXIS) ? WP_MP40 : WP_THOMPSON;
    }

    // Add the primary weapon
	AddWeaponToPlayer(client, w, 0, GetAmmoTableData(w)->defaultStartingClip, qtrue);

    // Give another clip for M97
	if (w == WP_M97)
		client->ps.ammo[BG_FindClipForWeapon(WP_M97)] += GetAmmoTableData(w)->maxclip;
    // Get secondary weapon
    weapon_t w2 = (weapon_t)client->sess.latchPlayerWeapon2;

    // Make sure they can use it
    if (!G_IsSecondaryWeapon(client, w2)) {
        w2 = (client->sess.sessionTeam == TEAM_AXIS) ? WP_LUGER : WP_COLT;
    }

    // Add ammo for akimbos
	if(w2 == WP_AKIMBO_LUGER || w2 == WP_AKIMBO_COLT) {
		client->ps.ammoclip[BG_FindClipForWeapon((weapon_t)BG_AkimboSidearm(w2))] = GetAmmoTableData(w2)->defaultStartingClip;
	}

    // Add secondary weapon
	AddWeaponToPlayer(client, w2, GetAmmoTableData(w2)->defaultStartingAmmo, GetAmmoTableData(w2)->defaultStartingClip, qfalse);

    // Grenades
	if(client->sess.sessionTeam == TEAM_AXIS) {
		AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 1, qfalse);
	}
	else if(client->sess.sessionTeam == TEAM_ALLIES) {
		AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 1, qfalse);
	}

	return qtrue;
}

qboolean _SetEngineerSpawnWeapons(gclient_t *client) 
{
    // Should be on a team
    if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
        return qfalse;

    // Get Primary weapon
    weapon_t w = (weapon_t)client->sess.latchPlayerWeapon;

    // Make sure the class can use it
    if (!G_IsPrimaryWeapon(client->sess.playerType, client->sess.sessionTeam, w)) {
        w = (client->sess.sessionTeam == TEAM_AXIS) ? WP_MP40 : WP_THOMPSON;
    }

    // Add the primary weapon
	AddWeaponToPlayer(client, w, GetAmmoTableData(w)->defaultStartingAmmo, GetAmmoTableData(w)->defaultStartingClip, qtrue);

    // Add secondaries
	if(w == WP_KAR98) {
		AddWeaponToPlayer(client, WP_GPG40, GetAmmoTableData(WP_GPG40)->defaultStartingAmmo, GetAmmoTableData(WP_GPG40)->defaultStartingClip, qfalse);
	}
	else if(w == WP_CARBINE) {
		AddWeaponToPlayer(client, WP_M7, GetAmmoTableData(WP_M7)->defaultStartingAmmo, GetAmmoTableData(WP_M7)->defaultStartingClip, qfalse);
	}

    // Get secondary weapon
    weapon_t w2 = (weapon_t)client->sess.latchPlayerWeapon2;

    // Make sure they can use it
    if (!G_IsSecondaryWeapon(client, w2)) {
        w2 = (client->sess.sessionTeam == TEAM_AXIS) ? WP_LUGER : WP_COLT;
    }

    // Add ammo for akimbos
	if(w2 == WP_AKIMBO_LUGER || w2 == WP_AKIMBO_COLT) {
		client->ps.ammoclip[BG_FindClipForWeapon((weapon_t)BG_AkimboSidearm(w2))] = GetAmmoTableData(w2)->defaultStartingClip;
	}

    // Add secondary weapon
	AddWeaponToPlayer(client, w2, GetAmmoTableData(w2)->defaultStartingAmmo, GetAmmoTableData(w2)->defaultStartingClip, qfalse);

    // Grenades
	if(client->sess.sessionTeam == TEAM_AXIS) {
		AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 4, qfalse);
	}
	else if(client->sess.sessionTeam == TEAM_ALLIES) {
		AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 4, qfalse);
	}

	return qtrue;
}

qboolean _SetFieldOpSpawnWeapons(gclient_t *client) 
{
    // Should be on a team
    if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
        return qfalse;

    // Get Primary weapon
    weapon_t w = (weapon_t)client->sess.latchPlayerWeapon;

    // Make sure the class can use it
    if (!G_IsPrimaryWeapon(client->sess.playerType, client->sess.sessionTeam, w)) {
        w = (client->sess.sessionTeam == TEAM_AXIS) ? WP_MP40 : WP_THOMPSON;
    }

    // Add the primary weapon
	AddWeaponToPlayer(client, w, GetAmmoTableData(w)->defaultStartingAmmo, GetAmmoTableData(w)->defaultStartingClip, qtrue);

    // Get secondary weapon
    weapon_t w2 = (weapon_t)client->sess.latchPlayerWeapon2;

    // Make sure they can use it
    if (!G_IsSecondaryWeapon(client, w2)) {
        w2 = (client->sess.sessionTeam == TEAM_AXIS) ? WP_LUGER : WP_COLT;
    }

    // Add ammo for akimbos
	if(w2 == WP_AKIMBO_LUGER || w2 == WP_AKIMBO_COLT) {
		client->ps.ammoclip[BG_FindClipForWeapon((weapon_t)BG_AkimboSidearm(w2))] = GetAmmoTableData(w2)->defaultStartingClip;
	}

    // Add secondary weapon
	AddWeaponToPlayer(client, w2, GetAmmoTableData(w2)->defaultStartingAmmo, GetAmmoTableData(w2)->defaultStartingClip, qfalse);

    // Grenades
	if(client->sess.sessionTeam == TEAM_AXIS) {
		AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 1, qfalse);
	}
	else if(client->sess.sessionTeam == TEAM_ALLIES) {
		AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 1, qfalse);
	}

	return qtrue;
}

qboolean _SetCovertSpawnWeapons(gclient_t *client) 
{
    // Should be on a team
    if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES)
        return qfalse;

    // Get Primary weapon
    weapon_t w = (weapon_t)client->sess.latchPlayerWeapon;

    // Make sure the class can use it
    if (!G_IsPrimaryWeapon(client->sess.playerType, client->sess.sessionTeam, w)) {
        w = WP_STEN;
    }

    // Add the primary weapon
    if (cvars::bg_sniperWar.ivalue) {
        if (client->sess.sessionTeam == TEAM_AXIS) {
            w = WP_K43;
        } else {
            w = WP_GARAND;
        }
        AddWeaponToPlayer(client, w, 400, GetAmmoTableData(w)->defaultStartingClip, qtrue);
    } else {
		AddWeaponToPlayer(client, w, GetAmmoTableData(w)->defaultStartingAmmo, GetAmmoTableData(w)->defaultStartingClip, qtrue);
    }
	
    // Add secondaries
	if (w == WP_K43) {
        if (cvars::bg_sniperWar.ivalue) {
            AddWeaponToPlayer(client, WP_K43_SCOPE, 400, GetAmmoTableData(WP_K43_SCOPE)->defaultStartingClip, qfalse);
        } else {
            AddWeaponToPlayer(client, WP_K43_SCOPE, GetAmmoTableData(WP_K43_SCOPE)->defaultStartingAmmo, GetAmmoTableData(WP_K43_SCOPE)->defaultStartingClip, qfalse);
        }
    }
	else if(w == WP_GARAND) {
        if (cvars::bg_sniperWar.ivalue) {
            AddWeaponToPlayer(client, WP_GARAND_SCOPE, 400, GetAmmoTableData(WP_GARAND_SCOPE)->defaultStartingClip, qfalse);
        } else {
            AddWeaponToPlayer(client, WP_GARAND_SCOPE, GetAmmoTableData(WP_GARAND_SCOPE)->defaultStartingAmmo, GetAmmoTableData(WP_GARAND_SCOPE)->defaultStartingClip, qfalse);
        }
	}
	else if(w == WP_FG42) {
        AddWeaponToPlayer(client, WP_FG42SCOPE, GetAmmoTableData(WP_FG42SCOPE)->defaultStartingAmmo, GetAmmoTableData(WP_FG42SCOPE)->defaultStartingClip, qfalse);
    }

	// For sniperwar, we're done
	if (cvars::bg_sniperWar.ivalue)
		return qtrue;

    // Get secondary weapon
    weapon_t w2 = (weapon_t)client->sess.latchPlayerWeapon2;
    weapon_t w3 = WP_NONE;

        // Make sure they can use it
    if (!G_IsSecondaryWeapon(client, w2)) {
        w2 = (client->sess.sessionTeam == TEAM_AXIS) ? WP_SILENCER : WP_SILENCED_COLT;
        w3 = (client->sess.sessionTeam == TEAM_AXIS) ? WP_LUGER : WP_COLT;
    }


    // Add ammo for akimbos
	if(BG_IsAkimboWeapon(w2)) {
		client->ps.ammoclip[BG_FindClipForWeapon((weapon_t)BG_AkimboSidearm(w2))] = GetAmmoTableData(w2)->defaultStartingClip;
    } else {
        w3 = (client->sess.sessionTeam == TEAM_AXIS) ? WP_LUGER : WP_COLT;
    }

    // Add secondary weapon
	AddWeaponToPlayer(client, w2, GetAmmoTableData(w2)->defaultStartingAmmo, GetAmmoTableData(w2)->defaultStartingClip, qfalse);
    if (w3 != WP_NONE)
	    AddWeaponToPlayer(client, w3, GetAmmoTableData(w2)->defaultStartingAmmo, GetAmmoTableData(w2)->defaultStartingClip, qfalse);

    // Covert Ops have a silence sidearm
	client->pmext.silencedSideArm = 1;

    // Grenades
    if(client->sess.sessionTeam== TEAM_AXIS) {
		AddWeaponToPlayer(client, WP_GRENADE_LAUNCHER, 0, 2, qfalse);
	}
	else if(client->sess.sessionTeam == TEAM_ALLIES) {
		AddWeaponToPlayer(client, WP_GRENADE_PINEAPPLE, 0, 2, qfalse);
	}

    return qtrue;
}

/*
===========
SetWolfSpawnWeapons
===========
*/
void SetWolfSpawnWeapons( gclient_t *client ) 
{
	int		pc = client->sess.playerType;
	//qboolean	isPOW = (g_entities[client->ps.clientNum].r.svFlags & SVF_POW) ? qtrue : qfalse;

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) 
		return;

	Bot_Event_ResetWeapons(client->ps.clientNum);

	// Reset special weapon time
	// Modified to support g_slashKill
	if (client->pers.slashKillTime && g_slashKill.integer == KILL_NOBAR)
		client->ps.classWeaponTime = level.time;
	else if (client->pers.slashKillTime && g_slashKill.integer == KILL_HALFBAR) {
		switch(client->sess.playerType) {
			case PC_MEDIC:
                client->ps.classWeaponTime = level.time - (level.medicChargeTime[client->sess.sessionTeam-1] / 2);
				break;
			case PC_ENGINEER:
                client->ps.classWeaponTime = level.time - (level.engineerChargeTime[client->sess.sessionTeam-1] / 2);
				break;
			case PC_FIELDOPS:
                client->ps.classWeaponTime = level.time - (level.lieutenantChargeTime[client->sess.sessionTeam-1] / 2);
				break;
			case PC_COVERTOPS:
                client->ps.classWeaponTime = level.time - (level.covertopsChargeTime[client->sess.sessionTeam-1] / 2);
				break;
			default:
				client->ps.classWeaponTime = level.time - (level.soldierChargeTime[client->sess.sessionTeam-1] / 2);
		}
	}
	else if (client->pers.slashKillTime && g_slashKill.integer == KILL_SAMEBAR)
		client->ps.classWeaponTime = client->pers.slashKillTime;
	else
		client->ps.classWeaponTime = -999999;
		
	// Reset slashKillTime
	client->pers.slashKillTime = 0;

	// Communicate it to cgame
	client->ps.stats[STAT_PLAYER_CLASS] = pc;

	// Abuse teamNum to store player class as well (can't see stats 
	// for all clients in cgame)
	client->ps.teamNum = pc;

	// JPW NERVE -- zero out all ammo counts
	memset(client->ps.ammo, 0, MAX_WEAPONS * sizeof(int));

	// All players start with a knife (not OR-ing so that it clears 
	// previous weapons)
	client->ps.weapons[0] = 0;
	client->ps.weapons[1] = 0;

	// Jaybird - throwing knives
	AddWeaponToPlayer( client, WP_KNIFE, GetAmmoTableData(WP_KNIFE)->defaultStartingAmmo, GetAmmoTableData(WP_KNIFE)->defaultStartingClip, qtrue );

	client->ps.weaponstate = WEAPON_READY;

	if( g_knifeonly.integer ) {
		switch(pc) {
		case PC_MEDIC:
			AddWeaponToPlayer( client, WP_MEDIC_SYRINGE, 0, 20, qfalse );
			if( !(cvars::bg_weapons.ivalue & SBW_NOADRENALINE) && client->sess.skill[SK_FIRST_AID] >= 4 ) {
				AddWeaponToPlayer( client, WP_MEDIC_ADRENALINE, 0, 10, qfalse );
 			}
			break;
		case PC_ENGINEER:
			AddWeaponToPlayer( client, WP_DYNAMITE, 0, 1, qfalse );
			AddWeaponToPlayer( client, WP_PLIERS, 0, 1, qfalse );
			break;
 		}
		return;
	}
	switch( pc ) {
		case PC_SOLDIER:
			_SetSoldierSpawnWeapons( client );
			break;
		case PC_MEDIC:
			_SetMedicSpawnWeapons( client );
			break;
		case PC_ENGINEER:
			_SetEngineerSpawnWeapons( client );
			break;
		case PC_FIELDOPS:
			_SetFieldOpSpawnWeapons( client );
			break;
		case PC_COVERTOPS:
			_SetCovertSpawnWeapons( client );
			break;
 	}

	G_AddClassSpecificTools( client );
 }

 int G_CountTeamMedics( team_t team, qboolean alivecheck ) {
	int numMedics = 0;
	int i, j;

	for( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if( level.clients[j].sess.sessionTeam != team ) {
			continue;
		}

		if( level.clients[j].sess.playerType != PC_MEDIC ) {
			continue;
		}

		if( alivecheck ) {
			if( g_entities[j].health <= 0 ) {
				continue;
			}

			if( level.clients[j].ps.pm_type == PM_DEAD || level.clients[j].ps.pm_flags & PMF_LIMBO ) {
				continue;
			}
		}

		numMedics++;
	}

	return numMedics;
}

//
// AddMedicTeamBonus
//
void AddMedicTeamBonus( gclient_t *client ) {
	int numMedics = G_CountTeamMedics( client->sess.sessionTeam, qfalse );

	// compute health mod
	client->pers.maxHealth = 100 + 10 * numMedics;

	if( client->pers.maxHealth > 125 ) {
		client->pers.maxHealth = 125;
	}

	if( client->sess.skill[SK_BATTLE_SENSE] >= 3 ) {
		client->pers.maxHealth += 15;
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
}

/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize )
{
	int		len, colorlessLen;
	char	ch;
	char	*p;
	int		spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while( 1 ) {
		ch = *in++;
		if( !ch ) {
			break;
		}

		// don't allow leading spaces
		if( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if( !*in ) {
				break;
			}

			// don't allow black in a name, period
/*			if( ColorIndex(*in) == 0 ) {
				in++;
				continue;
			}
*/
			// make sure room in dest for both chars
			if( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if( ch == ' ' ) {
			spaces++;
			if( spaces > 3 ) {
				continue;
			}
		}
		else {
			spaces = 0;
		}

		if( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "UnnamedPlayer", outSize );
	}
}

void G_StartPlayerAppropriateSound(gentity_t *ent, char *soundType) {
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
    gentity_t *ent;
    char    *s;
    char    oldname[MAX_STRING_CHARS];
    char    userinfo[MAX_INFO_STRING];
    gclient_t   *client;
    int     i;
    char    skillStr[16] = "";
    char    medalStr[16] = "";
    int     characterIndex;
    string  mac;

    // unindex user because values may change
    User& user = *connectedUsers[clientNum];
    userDB.unindex( user );

    ent = g_entities + clientNum;
    client = ent->client;

    client->ps.clientNum = clientNum;

    client->medals = 0;
    for( i = 0; i < SK_NUM_SKILLS; i++ ) {
        client->medals += client->sess.medals[ i ];
    }

    trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

    // If their info is invalid, drop the client
    if (!Info_Validate(userinfo)) {
        trap_DropClient(clientNum, "Your client is providing invalid user information.", 0);
    }

#ifndef DEBUG_STATS
    if( g_developer.integer || *g_log.string || g_dedicated.integer ) 
#endif
    {
        G_LogPrintf("Userinfo: %s\n", userinfo);
    }

    // check for local client
    s = Info_ValueForKey( userinfo, "ip" );
    if ( *s && !strcmp( s, "localhost" ) ) {
        client->pers.localClient = qtrue;
        level.fLocalHost = qtrue;
        client->sess.referee = RL_REFEREE;
    }

    // Jaybird - check for MAC
    mac = Info_ValueForKey(userinfo, "cl_mac");
    str::toLower( mac );

	// Don't put up with bullshit
	if (!user.mac.empty() && mac.empty()) {
		ClientDisconnect(clientNum);
	}

    if (!mac.empty() && user.mac != mac) {
        user.mac = mac;

        // Since new information has arrived, check MAC (and GUID just in case it did happen to change).
        User* subject;
        string detail;
        switch (userDB.checkBan( user.guid, "", user.mac, subject, detail )) {
            default:
            case UserDB::BAN_NONE:
                break;

            case UserDB::BAN_LIFTED:
                G_LogPrintf( "BAN lifted: %s (client %d)\n", detail.c_str(), clientNum );
                if (g_logOptions.integer & LOGOPTS_BAN)
                    AP( va( "cpm \"BAN lifted: %s (client %d)\"", detail.c_str(), clientNum ));
                break;

            case UserDB::BAN_ACTIVE:
                {
                    G_LogPrintf( "BAN enforced: %s (client %d)\n", detail.c_str(), clientNum );
                    if (g_logOptions.integer & LOGOPTS_BAN)
                        AP( va( "cpm \"BAN enforced: %s (client %d)\"", detail.c_str(), clientNum ));

                    const string remain = subject->banExpiry
                       ? str::toStringSecondsRemaining( subject->banExpiry )
                       : "PERMANENT";

                    using namespace text;
                    Buffer msg;

                    msg << '\n' << "player: " << xvalue( subject->namex )
                        << '\n'
                        << '\n' << "remaining:"
                        << '\n' << xvalue( remain );

                    if (!subject->banReason.empty()) {
                        msg << '\n'
                            << '\n' << "reason:"
                            << '\n' << xvalue( subject->banReason );
                    }

                    SEngine::dropClient( clientNum, msg, "You are banned from this server." );
                }
                break;
        }
    }

	s = Info_ValueForKey(userinfo, "pmove_fixed");
	if (!atoi(s)) {
		client->pers.pmoveFixed = qfalse;
	}
	else {
		client->pers.pmoveFixed = (qboolean)atoi(s);
	}

//unlagged - client options
    // see if the player has opted out
    s = Info_ValueForKey( userinfo, "cg_delag" );
    if ( !atoi( s ) ) {
        client->pers.antilag = 0;
    } else {
        client->pers.antilag = atoi( s );
    }
//unlagged - client options

    // OSP - extra client info settings
    //       FIXME: move other userinfo flag settings in here
    if(ent->r.svFlags & SVF_BOT) {
        client->pers.autoActivate = PICKUP_TOUCH;
        client->pers.bAutoReloadAux = qtrue;
        client->pmext.bAutoReload = qtrue;
        client->pers.predictItemPickup = qfalse;
    } else {
        s = Info_ValueForKey(userinfo, "cg_uinfo");
        sscanf(s, "%i %i %i",
            &client->pers.clientFlags,
            &client->pers.clientTimeNudge,
            &client->pers.clientMaxPackets);

        client->pers.autoActivate = (client->pers.clientFlags & CGF_AUTOACTIVATE) ? PICKUP_TOUCH : PICKUP_ACTIVATE;
        client->pers.predictItemPickup = (qboolean)((client->pers.clientFlags & CGF_PREDICTITEMS) != 0);

        if(client->pers.clientFlags & CGF_AUTORELOAD) {
            client->pers.bAutoReloadAux = qtrue;
            client->pmext.bAutoReload = qtrue;
        } else {
            client->pers.bAutoReloadAux = qfalse;
            client->pmext.bAutoReload = qfalse;
        }
    }

    // Set up jaymiscflags
    s = Info_ValueForKey(userinfo, "cg_jaymiscflags");
    if (s) {
        // Get values
        int flags = atoi(s);
        client->pers.pmblock = (flags & JAYFLAGS_PMBLOCK) ? 1 : 0;
    }
    else {
        // Set defaults
        client->pers.pmblock = 0;
    }

    // set name
    Q_strncpyz( oldname, client->pers.netname, sizeof( oldname ) );
    s = Info_ValueForKey (userinfo, "name");
    ClientCleanName( s, client->pers.netname, sizeof(client->pers.netname) );

    // Jaybird - also updated UserDB
    {
        user.namex = client->pers.netname;
        user.name = user.namex;
        str::etDecolorize( user.name );
    }

    if ( client->pers.connected == CON_CONNECTED ) {
        if ( strcmp( oldname, client->pers.netname ) ) {
            g_clientObjects[clientNum].notifyNameChanged();
            trap_SendServerCommand( -1, va("print \"[lof]%s" S_COLOR_WHITE " [lon]renamed to[lof] %s\n\"", oldname, 
                client->pers.netname) );
        }
    }

    for( i = 0; i < SK_NUM_SKILLS; i++ ) {
        Q_strcat( skillStr, sizeof(skillStr), va("%i",client->sess.skill[i]) );
        Q_strcat( medalStr, sizeof(medalStr), va("%i",client->sess.medals[i]) );
        // FIXME: Gordon: wont this break if medals > 9 arnout? JK: Medal count is tied to skill count :() Gordon: er, it's based on >> skill per map, so for a huuuuuuge campaign it could break...
    }

    client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

    // check for custom character
    s = Info_ValueForKey( userinfo, "ch" );
    if( *s ) {
        characterIndex = atoi(s);
    } else {
        characterIndex = -1;
    }

    // To communicate it to cgame
    client->ps.stats[ STAT_PLAYER_CLASS ] = client->sess.playerType;
    // Gordon: Not needed any more as it's in clientinfo?

    // send over a subset of the userinfo keys so other clients can
    // print scoreboards, display models, and play custom sounds
    s = va( "n\\%s\\t\\%i\\c\\%i\\r\\%i\\m\\%s\\s\\%s\\dn\\%s\\dr\\%i\\w\\%i\\lw\\%i\\sw\\%i\\mu\\%i\\ref\\%i\\sc\\%i",
        client->pers.netname, 
        client->sess.sessionTeam, 
        client->sess.playerType, 
        client->sess.rank, 
        medalStr,
        skillStr,
        client->disguiseNetname,
        client->disguiseRank,
        client->sess.playerWeapon,
        client->sess.latchPlayerWeapon,
        client->sess.latchPlayerWeapon2,
        connectedUsers[clientNum]->muted ? 1 : 0,
        client->sess.referee,
        client->sess.shoutcaster
    );

    trap_GetConfigstring( CS_PLAYERS + clientNum, oldname, sizeof( oldname ) );
    trap_SetConfigstring( CS_PLAYERS + clientNum, s );

    if (Q_stricmp( oldname, s )) {
        G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
        G_DPrintf( "ClientUserinfoChanged: %i :: %s\n", clientNum, s );
    }

    // index user now that values have been updated
    userDB.index( user );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return false if the client should be allowed, otherwise return true and
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
bool
ClientConnect( string& outmsg, int clientNum, qboolean firstTime, qboolean isBot ) {
    outmsg.clear();

	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	char		userinfo2[MAX_INFO_STRING];
	gentity_t	*ent;
	int			i;
	int			clientNum2;

	string		guid;
	string		value;

	bool		sv_pb_enabled;
	bool		cl_pb_enabled;

	ent = &g_entities[ clientNum ];

	// Gordon: porting q3f flag bug fix
	// If a player reconnects quickly after a disconnect, the client disconnect
	// may never be called, thus flag can get lost in the ether
	if( ent->inuse ) {
		ostringstream msg;
		msg	<< "Reconnection happened too soon, cleaning data: client " << clientNum << "\n";
		G_LogPrintf( msg.str().c_str() );
        ClientDisconnect( clientNum );
	}

	// Set alarm to check client version.
	//
	// Important to give time for cgame to load, otherwise we experience
	// a problem where drop message is not displayed on client properly.
	// Also sometimes it takes a while for Userinfo to be populated by
	// cgame as opposed to what ui offers.
	if( !isBot && !( ent->r.svFlags & SVF_BOT ))
		ent->clientCheckAlarm = trap_Milliseconds() + 60000;

	// Get Userinfo (typically not complete)
	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	if (!Info_Validate(userinfo)) {
		outmsg = "Your client is providing invalidate user information.";
		return true;
	}

	// IP flood fill check
	{
		char* ip = Info_ValueForKey(userinfo, "ip");
		if (!G_IPFloodCheck(clientNum, ip)) {
			stringstream msg;
			msg << "Only 3 connections per IP are allowed on this server.";
			outmsg = msg.str();
			return true;
		}
	}

	// Get PB status
	sv_pb_enabled = trap_Cvar_VariableIntegerValue( "sv_punkbuster" ) > 0 ? true : false;
	cl_pb_enabled = atoi(Info_ValueForKey(userinfo, "cl_punkbuster")) > 0 ? true : false;

	// Get GUID
	guid = Info_ValueForKey(userinfo, "cl_guid");

	// Check GUID
    bool fakeguid = false;
	if (guid.length() != 32) {
		if (sv_pb_enabled || cl_pb_enabled) {
			// If PB is enabled anywhere, must have a valid GUID
			outmsg = "You have an invalid GUID.  This might be a temporary problem, and you should try reconnecting.";
 			return true;
		} else {
			// Generate a fake local GUID
			stringstream newguid;
			newguid << "CLIENT" << setw(2) << setfill('0') << clientNum << right << setw(24) << "";
			guid = newguid.str().c_str();
            fakeguid = true;
		}
	}

    // Bots are also fake
    if (isBot || (ent->r.svFlags & SVF_BOT)) {
        fakeguid = true;
    }

    // Get user object
    string err;
    connectedUsers[clientNum] = &userDB.fetchByKey( guid, err, true );
    User& user = *connectedUsers[clientNum];

    // Track fake GUIDs
    // Also, be sure to clear previous used records
    if (fakeguid) {
        user.reset();
        user.fakeguid = true;
    }

    // Real player checks.
	if ( !isBot && !( ent->r.svFlags & SVF_BOT )) {
        // Enforce legacy-IP-bans.
        value = Info_ValueForKey( userinfo, "ip" );
        if (G_FilterIPBanPacket( value.c_str() )) {
            G_LogPrintf( "BAN enforced: LEGACY IP %s (client %d)\n", value.c_str(), clientNum );
            if (g_logOptions.integer & LOGOPTS_BAN)
                AP( va( "cpm \"BAN enforced: LEGACY IP %s (client %d)\"", value.c_str(), clientNum ));

            using namespace text;
            Buffer msg;

            msg << '\n' << "IP: " << xvalue( value )
                << '\n'
                << '\n' << "reason:"
                << '\n' << xvalue( "Your IP is filter-banned." );

            str::toDropMessage( outmsg, true, msg, "Your are banned from this server." );
            return true;
        }

        // Enforce user.db bans.
        {
            string mac = Info_ValueForKey( userinfo, "cl_mac" );
            str::toLower( mac );

            string ip = Info_ValueForKey( userinfo, "ip" );
            G_StripIPPort( ip );

            User* subject;
            string detail;
            switch (userDB.checkBan( guid, ip, mac, subject, detail )) {
                default:
                case UserDB::BAN_NONE:
                    break;

                case UserDB::BAN_LIFTED:
                    G_LogPrintf( "BAN lifted: %s (client %d)\n", detail.c_str(), clientNum );
		            if (g_logOptions.integer & LOGOPTS_BAN)
                        AP( va( "cpm \"BAN lifted: %s (client %d)\"", detail.c_str(), clientNum ));
                    break;

                case UserDB::BAN_ACTIVE:
                    {
                        G_LogPrintf( "BAN enforced: %s (client %d)\n", detail.c_str(), clientNum );
                        if (g_logOptions.integer & LOGOPTS_BAN)
                            AP( va( "cpm \"BAN enforced: %s (client %d)\"", detail.c_str(), clientNum ));

                        const string remain = subject->banExpiry
                           ? str::toStringSecondsRemaining( subject->banExpiry )
                           : "PERMANENT";

                        using namespace text;
                        Buffer msg;

                        msg << '\n' << "player: " << xvalue( subject->namex )
                            << '\n'
                            << '\n' << "remaining:"
                            << '\n' << xvalue( remain );

                        if (!subject->banReason.empty()) {
                            msg << '\n'
                                << '\n' << "reason:"
                                << '\n' << xvalue( subject->banReason );
                        }

                        str::toDropMessage( outmsg, true, msg, "You are banned from this server." );
                        return true;
                    }
                    break;
            }
        }

		// Jaybird - see if the server is admins only
		if( cvars::bg_misc.ivalue & MISC_ADMINSONLY ) {
			if (cmd::levelForEntity( ent ) == 0) {
				outmsg = "Only administrators can connect to this server at the moment.\n";
				return true;
			}
		}

		// Check for server password
		if ( *g_password.string && strcmp(Info_ValueForKey ( userinfo, "ip" ), "localhost") ) {
			value = Info_ValueForKey (userinfo, "password");
			if ( Q_stricmp( g_password.string, "none" ) && strcmp( g_password.string, value.c_str()) != 0) {
				if( !sv_privatepassword.string[ 0 ] || strcmp( sv_privatepassword.string, value.c_str() ) ) {
					outmsg = "Invalid password.";
					return true;
				}
			}
		}
	}

	// Max lives check
	if( g_gametype.integer != GT_WOLF_LMS ) {
		if( g_enforcemaxlives.integer && (g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0) ) {

			// Check by GUID
			if (guid.length() && G_FilterMaxLivesPacket( guid.c_str() )) {
				outmsg = "Max Lives Enforcement Temp Ban. You will be able to reconnect when the next round starts. This ban is enforced to ensure you don't reconnect to get additional lives.";
				return true;
			}

			// Check by IP
			value = Info_ValueForKey ( userinfo, "ip" );
			if (G_FilterMaxLivesIPPacket( value.c_str() )) {
				outmsg = "Max Lives Enforcement Temp Ban. You will be able to reconnect when the next round starts. This ban is enforced to ensure you don't reconnect to get additional lives.";
				return true;
			}
		}
	}

	// Jaybird - Prevent duplicate GUID players
	if ( !isBot && !( ent->r.svFlags & SVF_BOT )) {
		if( !user.fakeguid && firstTime && !trap_Cvar_VariableIntegerValue( "sv_wwwDlDisconnected" )) {
			for( i = 0; i < level.numConnectedClients; i++ ) {
				clientNum2 = level.sortedClients[i];

				// Jaybird - don't check ourselves
				if( clientNum == clientNum2 )
					continue;

				trap_GetUserinfo( clientNum2, userinfo2, sizeof( userinfo2 ));
				value = Info_ValueForKey( userinfo2, "cl_guid" );

				// Do not compare if no guid here
				if( !value.length() )
					continue;

				// Drop client if using duplicate GUID.
				if( !Q_stricmp( guid.c_str(), value.c_str() )) { 
	            	ostringstream msg;
					msg	<< "Duplicate GUID already in use by client " << clientNum2
						<< ", disconnecting: client " << clientNum << "\n";
					G_LogPrintf( msg.str().c_str() );
					outmsg = "Duplicate GUID detected.";
					return true;
				}
			}
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	Client& clientObject = g_clientObjects[clientNum];
	clientObject.reset();

	client->pers.connected = CON_CONNECTING;
	client->pers.connectTime = level.time;			// DHM - Nerve

	if( firstTime )
		client->pers.initialSpawn = qtrue;				// DHM - Nerve

    // unindex user before updating values
    userDB.unindex( user );

	// IP Address
    user.ip = Info_ValueForKey( userinfo, "ip" );
    G_StripIPPort( user.ip );

    // MAC
    user.mac = Info_ValueForKey(userinfo, "cl_mac");
    str::toLower( user.mac );

    // index user after updating values
    userDB.index( user );

	// Read or initialize the session data
	if( firstTime ) {
		G_InitSessionData( client, userinfo );
		client->pers.enterTime = level.time;
		client->ps.persistant[PERS_SCORE] = 0;
	} else {
		G_ReadSessionData( client );
	}

	// Jaybird - by this point they should have already hit
	// the minimums (if they have them), unless it was changed in their absence.
	G_ApplyCustomRanks(client);

	if( g_gametype.integer == GT_WOLF_CAMPAIGN ) {
		if( g_campaigns[level.currentCampaign].current == 0 || level.newCampaign ) {
			client->pers.enterTime = level.time;
		}
	} else {
		client->pers.enterTime = level.time;
	}

	if( isBot ) {
		ent->s.number = clientNum;

		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
	} else if( firstTime ) {
		// force into spectator
		client->sess.sessionTeam = TEAM_SPECTATOR;
		client->sess.spectatorState = SPECTATOR_FREE;
		client->sess.spectatorClient = 0;

		// unlink the entity - just in case they were already connected
		trap_UnlinkEntity( ent );
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	G_UpdateCharacter( client );
	Bot_Event_ClientConnected(clientNum, isBot);
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	//		TAT 12/10/2002 - Don't display connected messages in single player
	if ( firstTime )
	{
		trap_SendServerCommand( -1, va("cpm \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname) );
	}

	// Jaybird
	// Reset binocCount
	ent->client->pers.binocCount = 0;
	// Reset slashKillTime
	ent->client->pers.slashKillTime = 0;
	// Reset lose spree
	ent->client->pers.losespreekills = 0;


	// AndyStutz - resetting session kills/deaths between map changes.  
	// Moved this here instead of ClientBegin as we only
	// want to reset between map loads, NOT when players change teams
	// as excellent suggestion from Sn4cky. :D
	client->sess.kills = 0;
	client->sess.deaths = 0;

	// Let's reset our panzer reload time to default between matches
	client->sess.panzerreloadtimems = 2000;
	// End AndyStutz


	// count current clients and rank for scoreboard
	CalculateRanks();

	// Jaybird - announce admin level entry.
	clientObject.notifyConnecting( firstTime );

	return false;
}

//
// Scaling for late-joiners of maxlives based on current game time
//
int G_ComputeMaxLives(gclient_t *cl, int maxRespawns)
{
	float scaled = (float)(maxRespawns - 1) * (1.0f - ((float)(level.time - level.startTime) / (g_timelimit.value * 60000.0f)));
	int val = (int)scaled;

	// rain - #102 - don't scale of the timelimit is 0
	if (g_timelimit.value == 0.0) {
		return maxRespawns - 1;
	}

	val += ((scaled - (float)val) < 0.5f) ? 0 : 1;
	return(val);
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum )
{
	gentity_t	*ent;
	gclient_t	*client;
	int			flags;
	int			spawn_count, lives_left;		// DHM - Nerve

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}

	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	// DHM - Nerve :: Also save PERS_SPAWN_COUNT, so that CG_Respawn happens
	spawn_count = client->ps.persistant[PERS_SPAWN_COUNT];
	//bani - proper fix for #328
	if( client->ps.persistant[PERS_RESPAWNS_LEFT] > 0 ) {
		lives_left = client->ps.persistant[PERS_RESPAWNS_LEFT] - 1;
	} else {
		lives_left = client->ps.persistant[PERS_RESPAWNS_LEFT];
	}
	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;
	client->ps.persistant[PERS_SPAWN_COUNT] = spawn_count;
	client->ps.persistant[PERS_RESPAWNS_LEFT] = lives_left;
	
	client->pers.complaintClient = -1;
	client->pers.complaintEndTime = -1;

	//Omni-bot
	client->sess.botSuicide = qfalse;
	client->sess.botPush = (ent->r.svFlags & SVF_BOT) ? qtrue : qfalse;

	// Jaybird - shrubbot shortcuts
    Q_strncpyz(client->pers.lastammo, "nobody", sizeof(client->pers.lastammo));
	Q_strncpyz(client->pers.lastkilled, "nobody", sizeof(client->pers.lastkilled));
	Q_strncpyz(client->pers.lasthealth, "nobody", sizeof(client->pers.lasthealth));
	Q_strncpyz(client->pers.lastkill, "nobody", sizeof(client->pers.lastkill));
	Q_strncpyz(client->pers.lastrevive, "nobody", sizeof(client->pers.lastrevive));

	// locate ent at a spawn point
	ClientSpawn( ent, qfalse );

	// Xian -- Changed below for team independant maxlives
	if( g_gametype.integer != GT_WOLF_LMS ) {
		if( ( client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES ) ) {
		
			if( !client->maxlivescalced ) {
				if(g_maxlives.integer > 0) {
					client->ps.persistant[PERS_RESPAWNS_LEFT] = G_ComputeMaxLives(client, g_maxlives.integer);
				} else {
					client->ps.persistant[PERS_RESPAWNS_LEFT] = -1;
				}

				if( g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0 ) {
					if(client->sess.sessionTeam == TEAM_AXIS) {
						client->ps.persistant[PERS_RESPAWNS_LEFT] = G_ComputeMaxLives(client, g_axismaxlives.integer);	
					} else if(client->sess.sessionTeam == TEAM_ALLIES) {
						client->ps.persistant[PERS_RESPAWNS_LEFT] = G_ComputeMaxLives(client, g_alliedmaxlives.integer);
					} else {
						client->ps.persistant[PERS_RESPAWNS_LEFT] = -1;
					}
 				}

				client->maxlivescalced = qtrue;
			} else {
				if( g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0 ) {
					if( client->sess.sessionTeam == TEAM_AXIS ) {
						if( client->ps.persistant[ PERS_RESPAWNS_LEFT ] > g_axismaxlives.integer ) {
							client->ps.persistant[ PERS_RESPAWNS_LEFT ] = g_axismaxlives.integer;
						}
					} else if( client->sess.sessionTeam == TEAM_ALLIES ) {
						if( client->ps.persistant[ PERS_RESPAWNS_LEFT ] > g_alliedmaxlives.integer ) {
							client->ps.persistant[ PERS_RESPAWNS_LEFT ] = g_alliedmaxlives.integer;
						}
					}
 				}
			}
		}
	}	


	// DHM - Nerve :: Start players in limbo mode if they change teams during the match
	if(client->sess.sessionTeam != TEAM_SPECTATOR && (level.time - level.startTime > FRAMETIME * GAME_INIT_FRAMES) ) {
/*	  if( (client->sess.sessionTeam != TEAM_SPECTATOR && (level.time - client->pers.connectTime) > 60000) ||
		( cvars::gameState.ivalue == GS_PLAYING && ( client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES ) && 
		 g_gametype.integer == GT_WOLF_LMS && ( level.numTeamClients[0] > 0 || level.numTeamClients[1] > 0 ) ) ) {*/
		ent->health = 0;
		ent->r.contents = CONTENTS_CORPSE;

		client->ps.pm_type = PM_DEAD;
		client->ps.stats[STAT_HEALTH] = 0;

		if( g_gametype.integer != GT_WOLF_LMS ) {
			if( g_maxlives.integer > 0 ) {
				client->ps.persistant[PERS_RESPAWNS_LEFT]++;
			}
		}

		limbo(ent, qfalse);
	}

	if(client->sess.sessionTeam != TEAM_SPECTATOR) {
		trap_SendServerCommand( -1, va("print \"[lof]%s" S_COLOR_WHITE " [lon]entered the game\n\"", client->pers.netname) );
	}

	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// Xian - Check for maxlives enforcement
	if( g_gametype.integer != GT_WOLF_LMS ) {
		if ( g_enforcemaxlives.integer == 1 && (g_maxlives.integer > 0 || g_axismaxlives.integer > 0 || g_alliedmaxlives.integer > 0)) {
			char *value;
			char userinfo[MAX_INFO_STRING];
			trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
			value = Info_ValueForKey ( userinfo, "cl_guid" );
			G_LogPrintf( "EnforceMaxLives-GUID: %s\n", value );
			AddMaxLivesGUID( value );

			value = Info_ValueForKey (userinfo, "ip");
			G_LogPrintf( "EnforceMaxLives-IP: %s\n", value );
			AddMaxLivesBan( value );
		}
	}
	// End Xian

	// count current clients and rank for scoreboard
	CalculateRanks();

	// No surface determined yet.
	ent->surfaceFlags = 0;

	// OSP
	G_smvUpdateClientCSList(ent);
	// OSP

	g_clientObjects[clientNum].notifyBegin();
}

gentity_t *SelectSpawnPointFromList( char *list, vec3_t spawn_origin, vec3_t spawn_angles )
{
	char *pStr, *token;
	gentity_t	*spawnPoint=NULL, *trav;
	#define	MAX_SPAWNPOINTFROMLIST_POINTS	16
	int	valid[MAX_SPAWNPOINTFROMLIST_POINTS];
	int numValid;

	memset( valid, 0, sizeof(valid) );
	numValid = 0;

	pStr = list;
	while((token = COM_Parse( &pStr )) != NULL && token[0]) {
		trav = g_entities + level.maxclients;
		while((trav = G_FindByTargetname(trav, token)) != NULL) {
			if (!spawnPoint) spawnPoint = trav;
			if (!SpotWouldTelefrag( trav )) {
				valid[numValid++] = trav->s.number;
				if (numValid >= MAX_SPAWNPOINTFROMLIST_POINTS) {
					break;
				}
			}
		}
	}

	if (numValid)
	{
		spawnPoint = &g_entities[valid[rand()%numValid]];

		// Set the origin of where the bot will spawn
		VectorCopy (spawnPoint->r.currentOrigin, spawn_origin);
		spawn_origin[2] += 9;

		// Set the angle we'll spawn in to
		VectorCopy (spawnPoint->s.angles, spawn_angles);
	}

	return spawnPoint;
}


// TAT 1/14/2003 - init the bot's movement autonomy pos to it's current position
void BotInitMovementAutonomyPos(gentity_t *bot);

#if 0 // rain - not used
static char *G_CheckVersion( gentity_t *ent )
{
	// Prevent nasty version mismatches (or people sticking in Q3Aimbot cgames)

	char userinfo[MAX_INFO_STRING];
	char *s;

	trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );
	s = Info_ValueForKey( userinfo, "cg_etVersion" );
	if( !s || strcmp( s, GAME_VERSION_DATED ) )
		return( s );
	return( NULL );
}
#endif

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn( gentity_t *ent, qboolean revived )
{
	vec3_t		spawn_origin, spawn_angles;
	int			i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int			persistant[MAX_PERSISTANT];
	gentity_t	*spawnPoint;
	int			flags;
	int			savedPing;
	int			savedTeam;
	int			savedSlotNumber;

	const int index = ent - g_entities;
	gclient_t* const client = ent->client;

	G_UpdateSpawnCounts();

	// AndyStutz - reset fastpanzer kill spree counter
	ent->client->pers.fastpanzerkillspreekills = 0;

	// Jaybird - reset kill spree counter
	ent->client->pers.killspreekills = 0;

	client->pers.lastSpawnTime = level.time;
	client->pers.lastBattleSenseBonusTime = level.timeCurrent;
	client->pers.lastHQMineReportTime = level.timeCurrent;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if( revived ) {
		spawnPoint = ent;
		VectorCopy( ent->r.currentOrigin, spawn_origin );
		spawn_origin[2] += 9;	// spawns seem to be sunk into ground?
		VectorCopy( ent->s.angles, spawn_angles );
	} else {
		// Arnout: let's just be sure it does the right thing at all times. (well maybe not the right thing, but at least not the bad thing!)
		//if( client->sess.sessionTeam == TEAM_SPECTATOR || client->sess.sessionTeam == TEAM_FREE ) {
		if( client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES ) {
			spawnPoint = SelectSpectatorSpawnPoint( spawn_origin, spawn_angles );
		} else {
			// RF, if we have requested a specific spawn point, use it (fixme: what if this will place us inside another character?)
/*			spawnPoint = NULL;
			trap_GetUserinfo( ent->s.number, userinfo, sizeof(userinfo) );
			if( (str = Info_ValueForKey( userinfo, "spawnPoint" )) != NULL && str[0] ) {
				spawnPoint = SelectSpawnPointFromList( str, spawn_origin, spawn_angles );
				if (!spawnPoint) {
					G_Printf( "WARNING: unable to find spawn point \"%s\" for bot \"%s\"\n", str, ent->aiName );
				}
			}
			//
			if( !spawnPoint ) {*/
				spawnPoint = SelectCTFSpawnPoint( client->sess.sessionTeam, client->pers.teamState.state, spawn_origin, spawn_angles, client->sess.spawnObjectiveIndex );
//			}
		}
	}

	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags = ent->client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;
	flags |= (client->ps.eFlags & EF_VOTED);
	// clear everything but the persistant data

	ent->s.eFlags &= ~EF_MOUNTEDTANK;

	saved			= client->pers;
	savedSess		= client->sess;
	savedPing		= client->ps.ping;
	savedTeam		= client->ps.teamNum;
	// START	xkan, 8/27/2002
	savedSlotNumber	= client->botSlotNumber;
	// END		xkan, 8/27/2002

	for( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}

    Client& clientObject = g_clientObjects[index];
	clientObject.reset();

	client->maxlivescalced = client->maxlivescalced;

	client->pers			= saved;
	client->sess			= savedSess;
	client->ps.ping			= savedPing;
	client->ps.teamNum		= savedTeam;
	// START	xkan, 8/27/2002
	client->botSlotNumber	= savedSlotNumber;
	// END		xkan, 8/27/2002

	for( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	if( revived ) {
		client->ps.persistant[PERS_REVIVE_COUNT]++;
	}
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	client->ps.persistant[PERS_HWEAPON_USE] = 0;

	client->airOutTime = level.time + HOLDBREATHTIME;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;
	// MrE: use capsules for AI and player
	//client->ps.eFlags |= EF_CAPSULE;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;

	ent->clipmask = MASK_PLAYERSOLID;

	// DHM - Nerve :: Init to -1 on first spawn;
	if ( !revived )
		ent->props_frame_state = -1;

	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;
	
	VectorCopy( playerMins, ent->r.mins );
	VectorCopy( playerMaxs, ent->r.maxs );

	// Ridah, setup the bounding boxes and viewheights for prediction
	VectorCopy( ent->r.mins, client->ps.mins );
	VectorCopy( ent->r.maxs, client->ps.maxs );
	
	client->ps.crouchViewHeight = CROUCH_VIEWHEIGHT;
	client->ps.standViewHeight = DEFAULT_VIEWHEIGHT;
	client->ps.deadViewHeight = DEAD_VIEWHEIGHT;
	
	client->ps.crouchMaxZ = client->ps.maxs[2] - (client->ps.standViewHeight - client->ps.crouchViewHeight);

	client->ps.runSpeedScale = 0.8;
	client->ps.sprintSpeedScale = 1.1;
	client->ps.crouchSpeedScale = 0.25;
	client->ps.weaponstate = WEAPON_READY;

	// Rafael
	client->pmext.sprintTime = int( SPRINTTIME );
	client->ps.sprintExertTime = 0;

	client->ps.friction = 1.0;
	// done.

	// TTimo
	// retrieve from the persistant storage (we use this in pmoveExt_t beause we need it in bg_*)
	client->pmext.bAutoReload = client->pers.bAutoReloadAux;
	// done

	client->ps.clientNum = index;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );	// NERVE - SMF - moved this up here

	// DHM - Nerve :: Add appropriate weapons
	if ( !revived ) {
		qboolean update = qfalse;

        // Check if class is full
        if (G_IsClassDisabled( ent, ent->client->sess.latchPlayerType, true))
            client->sess.latchPlayerType = PC_SOLDIER;

		// Jaybird - set player class for the wars
		if( cvars::bg_sniperWar.ivalue ) {
			if( cvars::bg_panzerWar.ivalue )
				trap_Cvar_Set("g_panzerWar","0");
			client->sess.playerType = PC_COVERTOPS;
		}		
		else if( cvars::bg_panzerWar.ivalue ) {
			if( cvars::bg_sniperWar.ivalue )
				trap_Cvar_Set("g_sniperWar","0");
			client->sess.playerType = PC_SOLDIER;
		}
		else {
			if( client->sess.playerType != client->sess.latchPlayerType )
				update = qtrue;
			client->sess.playerType = client->sess.latchPlayerType;
		}

		if( G_IsWeaponDisabled( ent, (weapon_t)client->sess.latchPlayerWeapon, qtrue ) ) {
            bg_playerclass_t* classInfo = BG_PlayerClassForPlayerState( &ent->client->ps );
			client->sess.latchPlayerWeapon = classInfo->classWeapons[0];
			update = qtrue;
		}

		if( client->sess.playerWeapon != client->sess.latchPlayerWeapon ) {
			client->sess.playerWeapon = client->sess.latchPlayerWeapon;
			update = qtrue;
		}

		if( G_IsWeaponDisabled( ent, (weapon_t)client->sess.playerWeapon, qtrue ) ) {
            bg_playerclass_t* classInfo = BG_PlayerClassForPlayerState( &ent->client->ps );
			client->sess.playerWeapon = classInfo->classWeapons[0];
			update = qtrue;
		}

		client->sess.playerWeapon2 = client->sess.latchPlayerWeapon2;

		if( update ) {
			ClientUserinfoChanged( index );
		}
	}

	// TTimo keep it isolated from spectator to be safe still
	if( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// Xian - Moved the invul. stuff out of SetWolfSpawnWeapons and put it here for clarity
		if ( g_fastres.integer == 1 && revived )
			client->ps.powerups[PW_INVULNERABLE] = level.time + 1000; 
		// Jaybird - g_spawnInvul
		else if(revived) 
 			client->ps.powerups[PW_INVULNERABLE] = level.time + 3000; 
		else
			client->ps.powerups[PW_INVULNERABLE] = level.time + (g_spawnInvul.integer * 1000);
	}
	// End Xian

	G_UpdateCharacter( client );

	// important to do this before weapons are added to the bot
	//if (ent->r.svFlags & SVF_BOT)
	//{
	//	// Send the respawn event.
	//	if(!revived)
	//		Bot_Event_Spawn(client->ps.clientNum);
	//}

	SetWolfSpawnWeapons( client ); 
	
	// START	Mad Doctor I changes, 8/17/2002

	// JPW NERVE -- increases stats[STAT_MAX_HEALTH] based on # of medics in game
	AddMedicTeamBonus( client );

	// END		Mad Doctor I changes, 8/17/2002

	if( !revived ) {
		client->pers.cmd.weapon = ent->client->ps.weapon;
	}
// dhm - end

	// JPW NERVE ***NOTE*** the following line is order-dependent and must *FOLLOW* SetWolfSpawnWeapons() in multiplayer
	// AddMedicTeamBonus() now adds medic team bonus and stores in ps.stats[STAT_MAX_HEALTH].

	if( client->sess.skill[SK_BATTLE_SENSE] >= 3 )
		// We get some extra max health, but don't spawn with that much
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] - 15;
	else
		ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	if( !revived ) {
		SetClientViewAngle( ent, spawn_angles );
	} else {
		//bani - #245 - we try to orient them in the freelook direction when revived
		vec3_t	newangle;

		newangle[YAW] = SHORT2ANGLE( ent->client->pers.cmd.angles[YAW] + ent->client->ps.delta_angles[YAW] );
		newangle[PITCH] = 0;
		newangle[ROLL] = 0;

		SetClientViewAngle( ent, newangle );
	}

	if( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		//G_KillBox( ent );
		trap_LinkEntity (ent);
	}

	client->respawnTime = level.timeCurrent;
	if (client->sess.sessionTeam == TEAM_SPECTATOR)
		client->inactivityTime = level.time + g_spectatorInactivity.integer * 1000;
	else
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;
	client->latched_wbuttons = 0;	//----(SA)	added

	// xkan, 1/13/2003 - reset death time
	client->deathTime = 0;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		if ( !revived )
			G_UseTargets( spawnPoint, ent );
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	clientObject.think();

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, level.time, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// set idle animation on weapon
	ent->client->ps.weapAnim = ( ( ent->client->ps.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | PM_IdleAnimForWeapon( ent->client->ps.weapon );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, level.time, qtrue );

	// RF, start the scripting system
	if (!revived && client->sess.sessionTeam != TEAM_SPECTATOR) {
		// RF, call entity scripting event
		G_Script_ScriptEvent( ent, "playerstart", "" );
	}

	// Jaybird - reset shakeTime
	client->pers.shakeTime = level.time;

	// Jaybird - clear abused effect1Time >;]
	ent->s.effect1Time = 0;
	ent->s.effect2Time = 0;
	ent->s.effect3Time = 0;

	// Jaybird - clear PERS_JAYFLAGS
	ent->client->ps.persistant[PERS_JAYFLAGS1] = 0;
	ent->client->ps.persistant[PERS_JAYFLAGS2] = 0;

	// Jaybird - clear stealTime
	ent->stealTime = 0;
	ent->stealProgress = 0;

	// Jaybird - Reset (init) poison events
	G_ResetPoisonEvents(ent);
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {

	gentity_t	*ent;
	gentity_t	*flag=NULL;
	gitem_t		*item=NULL;
	vec3_t		launchvel;
	int			i;

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}


	// If we're currently in a killing spree and the player disconnecting is
	// the killing spree player, then we want to stop the killing spree
	// so players get reset back to their teams.
	if (g_fastpanzerkillspreeon && g_fastpanzerkillspreeclientnum == ent->client->ps.clientNum)
	{
		//string buffer = va("!fastpanzerkillspree stop %d\n", ent->client->ps.clientNum);
		//trap_SendConsoleCommand( EXEC_APPEND, buffer.c_str() );
		// Call new direct method
		G_StopFastPanzerKillSpree();
	}


	//////////////////////////////////////////////////////////////////////////
	Bot_Event_ClientDisConnected(clientNum);
	//////////////////////////////////////////////////////////////////////////

	// Update client User record
    connectedUsers[clientNum]->timestamp = time( NULL );
    g_clientObjects[clientNum].xpBackup();

    connectedUsers[clientNum] = &User::BAD;

	G_RemoveClientFromFireteams( clientNum, qtrue, qfalse );
	G_RemoveFromAllIgnoreLists( clientNum );
	G_LeaveTank( ent, qfalse );

	// stop any following clients
	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		flag = g_entities + level.sortedClients[i];
		if ( flag->client->sess.sessionTeam == TEAM_SPECTATOR
			&& flag->client->sess.spectatorState == SPECTATOR_FOLLOW
			&& flag->client->sess.spectatorClient == clientNum ) {
			StopFollowing( flag );
		}
		if ( flag->client->ps.pm_flags & PMF_LIMBO
			&& flag->client->sess.spectatorClient == clientNum ) {
			Cmd_FollowCycle_f( flag, 1 );
		}
	}

	// NERVE - SMF - remove complaint client
	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		if ( flag->client->pers.complaintEndTime > level.time && flag->client->pers.complaintClient == clientNum ) {
			flag->client->pers.complaintClient = -1;
			flag->client->pers.complaintEndTime = -1;

			CPx( level.sortedClients[i], "complaint -2" );
			break;
		}
	}

	if( g_landminetimeout.integer ) {
		G_ExplodeMines(ent);
	}
	G_FadeItems(ent, MOD_SATCHEL);

	// remove ourself from teamlists
	{
		mapEntityData_t	*mEnt;
		mapEntityData_Team_t *teamList;

		for( i = 0; i < 2; i++ ) {
			teamList = &mapEntityData[i];

			if((mEnt = G_FindMapEntityData(&mapEntityData[0], ent-g_entities)) != NULL) {
				G_FreeMapEntityData( teamList, mEnt );
			}

			mEnt = G_FindMapEntityDataSingleClient( teamList, NULL, ent->s.number, -1 );
			
			while( mEnt ) {
				mapEntityData_t	*mEntFree = mEnt;

				mEnt = G_FindMapEntityDataSingleClient( teamList, mEnt, ent->s.number, -1 );

				G_FreeMapEntityData( teamList, mEntFree );
			}
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED 
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR
		&& !(ent->client->ps.pm_flags & PMF_LIMBO) ) {

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );

		// New code for tossing flags
			if (ent->client->ps.powerups[PW_REDFLAG]) {
				item = BG_FindItem("Red Flag");
				if (!item)
					item = BG_FindItem("Objective");

				ent->client->ps.powerups[PW_REDFLAG] = 0;
			}
			if (ent->client->ps.powerups[PW_BLUEFLAG]) {
				item = BG_FindItem("Blue Flag");
				if (!item)
					item = BG_FindItem("Objective");

				ent->client->ps.powerups[PW_BLUEFLAG] = 0;
			}

			if( item ) {
				// OSP - fix for suicide drop exploit through walls/gates
				launchvel[0] = 0;//crandom()*20;
				launchvel[1] = 0;//crandom()*20;
				launchvel[2] = 0;//10+random()*10;

				flag = LaunchItem(item,ent->r.currentOrigin,launchvel,ent-g_entities);
				flag->s.modelindex2 = ent->s.otherEntityNum2;// JPW NERVE FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
				flag->message = ent->message;	// DHM - Nerve :: also restore item name
				// Clear out player's temp copies
				ent->s.otherEntityNum2 = 0;
				ent->message = NULL;
			}

		// OSP - Log stats too
		G_LogPrintf("WeaponStats: %s\n", G_createStats(ent, true));
	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

    // Jaybird - moved this here as the rest of this
    // function relies on client structure being sane.
    g_clientObjects[clientNum].reset();

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	i = ent->client->sess.sessionTeam;
	ent->client->sess.sessionTeam = TEAM_FREE;
	ent->active = qfalse;
	ent->r.svFlags &= ~SVF_BOT;
	trap_SetConfigstring( CS_PLAYERS + clientNum, "");


	CalculateRanks();

	// OSP
	G_verifyMatchState(i);
	G_smvAllRemoveSingleClient(ent - g_entities);
	// OSP
}

// In just the GAME DLL, we want to store the groundtrace surface stuff,
// so we don't have to keep tracing.
void ClientStoreSurfaceFlags
( 
	int clientNum, 
	int surfaceFlags
)
{
	// Store the surface flags
	g_entities[clientNum].surfaceFlags = surfaceFlags;

}

bool G_IPFloodCheck(int clientNum, string left)
{
	int count = 0;
	int max = 3;
	char userinfo[MAX_INFO_STRING];

	for (int i = 0; i < level.numConnectedClients; i++) {
		if (i == clientNum) {
			continue;
		}

		trap_GetUserinfo(i, userinfo, sizeof(userinfo ));
		string right = Info_ValueForKey(userinfo, "ip");

		if (right == "localhost") {
			continue;
		}

		if (left == right) {
			if (++count > max) {
				return false;
			}
		}
	}

	return true;
}

