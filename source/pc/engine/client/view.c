/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// view.c -- player eye positioning

#include "quakedef.h"

#include "winquake.h"
#include "glquake.h"

#include <ctype.h> // for isdigit();

cvar_t r_projection = CVARD("r_projection", "0", "0: regular perspective.\n1: stereographic (aka: pannini).\n2: fisheye.\n3: panoramic.\n4: lambert azimuthal equal-area.\n5: Equirectangular");
cvar_t ffov = CVARFD("ffov", "", 0, "Allows you to set a specific field of view for when a custom projection is specified. If empty, will use regular fov cvar, which might get messy.");
#if defined(_WIN32) && !defined(MINIMAL)
//amusing gimmick / easteregg.
#include "winquake.h"
cvar_t itburnsitburnsmakeitstop = CVARFD("itburnsitburnsmakeitstop", "0", CVAR_NOTFROMSERVER, "Ouch");
#endif

/*

The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.

*/

#ifdef SIDEVIEWS
cvar_t	vsec_enabled[SIDEVIEWS] = {CVAR("v2_enabled", "2"),		CVAR("v3_enabled", "0"),	CVAR("v4_enabled", "0"),	CVAR("v5_enabled", "0")};
cvar_t	vsec_x[SIDEVIEWS]		= {CVAR("v2_x", "0"),			CVAR("v3_x", "0.25"),		CVAR("v4_x", "0.5"),		CVAR("v5_x", "0.75")};
cvar_t	vsec_y[SIDEVIEWS]		= {CVAR("v2_y", "0"),			CVAR("v3_y", "0"),			CVAR("v4_y", "0"),			CVAR("v5_y", "0")};
cvar_t	vsec_scalex[SIDEVIEWS]	= {CVAR("v2_scalex", "0.25"),	CVAR("v3_scalex", "0.25"),	CVAR("v4_scalex", "0.25"),	CVAR("v5_scalex", "0.25")};
cvar_t	vsec_scaley[SIDEVIEWS]	= {CVAR("v2_scaley", "0.25"),	CVAR("v3_scaley", "0.25"),	CVAR("v4_scaley", "0.25"),	CVAR("v5_scaley", "0.25")};
cvar_t	vsec_yaw[SIDEVIEWS]		= {CVAR("v2_yaw", "180"),		CVAR("v3_yaw", "90"),		CVAR("v4_yaw", "270"),		CVAR("v5_yaw", "0")};
#endif

cvar_t	cl_rollspeed			= CVAR("cl_rollspeed", "200");
cvar_t	cl_rollangle			= CVAR("cl_rollangle", "2.0");
cvar_t	v_deathtilt				= CVAR("v_deathtilt", "1");

cvar_t	cl_bob					= CVAR("cl_bob","0.02");
cvar_t	cl_bobcycle				= CVAR("cl_bobcycle","0.6");
cvar_t	cl_bobup				= CVAR("cl_bobup","0.5");

cvar_t	v_kicktime				= CVAR("v_kicktime", "0.5");
cvar_t	v_kickroll				= CVAR("v_kickroll", "0.6");
cvar_t	v_kickpitch				= CVAR("v_kickpitch", "0.6");

cvar_t	v_iyaw_cycle			= CVAR("v_iyaw_cycle", "2");
cvar_t	v_iroll_cycle			= CVAR("v_iroll_cycle", "0.5");
cvar_t	v_ipitch_cycle			= CVAR("v_ipitch_cycle", "1");
cvar_t	v_iyaw_level			= CVAR("v_iyaw_level", "0.3");
cvar_t	v_iroll_level			= CVAR("v_iroll_level", "0.1");
cvar_t	v_ipitch_level			= CVAR("v_ipitch_level", "0.3");
cvar_t	v_idlescale				= CVAR("v_idlescale", "0");

cvar_t	crosshair				= CVARF("crosshair", "1", CVAR_ARCHIVE);
cvar_t	crosshaircolor			= CVARF("crosshaircolor", "255 255 255", CVAR_ARCHIVE);
cvar_t	crosshairsize			= CVARF("crosshairsize", "8", CVAR_ARCHIVE);

cvar_t	cl_crossx				= CVARF("cl_crossx", "0", CVAR_ARCHIVE);
cvar_t	cl_crossy				= CVARF("cl_crossy", "0", CVAR_ARCHIVE);
cvar_t	crosshaircorrect		= CVARF("crosshaircorrect", "0", CVAR_SEMICHEAT);
cvar_t	crosshairimage			= CVAR("crosshairimage", "");
cvar_t	crosshairalpha			= CVAR("crosshairalpha", "1");

cvar_t	gl_cshiftpercent		= CVAR("gl_cshiftpercent", "100");
cvar_t	gl_cshiftenabled		= CVARF("gl_polyblend", "1", CVAR_ARCHIVE);

cvar_t	v_bonusflash			= CVAR("v_bonusflash", "1");

cvar_t  v_contentblend			= CVARF("v_contentblend", "1", CVAR_ARCHIVE);
cvar_t	v_damagecshift			= CVAR("v_damagecshift", "1");
cvar_t	v_quadcshift			= CVAR("v_quadcshift", "1");
cvar_t	v_suitcshift			= CVAR("v_suitcshift", "1");
cvar_t	v_ringcshift			= CVAR("v_ringcshift", "1");
cvar_t	v_pentcshift			= CVAR("v_pentcshift", "1");
cvar_t	v_gunkick				= CVAR("v_gunkick", "0");
cvar_t	v_gunkick_q2			= CVAR("v_gunkick_q2", "1");

cvar_t	v_viewheight			= CVAR("v_viewheight", "0");
cvar_t	v_projectionmode		= CVAR("v_projectionmode", "0");

cvar_t	v_depthsortentities		= CVARAD("v_depthsortentities", "0", "v_reorderentitiesrandomly", "Reorder entities for transparency such that the furthest entities are drawn first, allowing nearer transparent entities to draw over the top of them.");

cvar_t	scr_autoid				= CVARD("scr_autoid", "1", "Display nametags above all players while spectating.");
cvar_t	scr_autoid_team			= CVARD("scr_autoid_team", "1", "Display nametags above team members. 0: off. 1: display with half-alpha if occluded. 2: hide when occluded.");
cvar_t	scr_autoid_health		= CVARD("scr_autoid_health", "1", "Display health as part of nametags (when known).");
cvar_t	scr_autoid_armour		= CVARD("scr_autoid_armor", "1", "Display armour as part of nametags (when known).");
cvar_t	scr_autoid_weapon		= CVARD("scr_autoid_weapon", "1", "Display the player's best weapon as part of their nametag (when known).");
cvar_t	scr_autoid_teamcolour	= CVARD("scr_autoid_teamcolour", STRINGIFY(COLOR_BLUE), "The colour for the text on the nametags of team members.");
cvar_t	scr_autoid_enemycolour	= CVARD("scr_autoid_enemycolour", STRINGIFY(COLOR_WHITE), "The colour for the text on the nametags of non-team members.");

cvar_t	chase_active			= CVAR("chase_active", "0");
cvar_t	chase_back				= CVAR("chase_back", "48");
cvar_t	chase_up				= CVAR("chase_up", "24");


extern cvar_t cl_chasecam;

player_state_t		*view_message;

/*
===============
V_CalcRoll

===============
*/
float V_CalcRoll (vec3_t angles, vec3_t velocity)
{
	vec3_t	forward, right, up;
	float	sign;
	float	side;
	float	value;

	AngleVectors (angles, forward, right, up);
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	value = cl_rollangle.value;

	if (side < cl_rollspeed.value)
		side = side * value / cl_rollspeed.value;
	else
		side = value;

	return side*sign;

}


/*
===============
V_CalcBob

===============
*/
float V_CalcBob (playerview_t *pv, qboolean queryold)
{
	float	cycle;
	float	hspeed, bob;
	vec3_t	hvel;

	if (pv->spectator)
		return 0;

	if (cl_bobcycle.value <= 0 || cl.intermissionmode != IM_NONE)
		return 0;

	if (!pv->onground || cl.paused)
	{
		pv->bobcltime = cl.time;
		return pv->bob;		// just use old value
	}

	pv->bobtime += cl.time - pv->bobcltime;
	pv->bobcltime = cl.time;
	cycle = pv->bobtime - (int)(pv->bobtime/cl_bobcycle.value)*cl_bobcycle.value;
	cycle /= cl_bobcycle.value;
	if (cycle < cl_bobup.value)
		cycle = M_PI * cycle / cl_bobup.value;
	else
		cycle = M_PI + M_PI*(cycle-cl_bobup.value)/(1.0 - cl_bobup.value);

// bob is proportional to simulated velocity in the xy plane
// (don't count Z, or jumping messes it up)
	hspeed = DotProduct(pv->simvel, pv->gravitydir);
	VectorMA(pv->simvel, hspeed, pv->gravitydir, hvel);
	hspeed = VectorLength(hvel);
	hspeed = bound(0, hspeed, 400);
	bob = hspeed * bound(0, cl_bob.value, 0.05);
	pv->bob = bob*0.3 + bob*0.7*sin(cycle);
	return pv->bob;

}


//=============================================================================


cvar_t	v_centermove = CVAR("v_centermove", "0.15");
cvar_t	v_centerspeed = CVAR("v_centerspeed","500");


void V_StartPitchDrift (playerview_t *pv)
{
#if 1
	if (pv->laststop == cl.time)
	{
		return;		// something else is keeping it from drifting
	}
#endif
	if (pv->nodrift || !pv->pitchvel)
	{
		pv->pitchvel = v_centerspeed.value;
		pv->nodrift = false;
		pv->driftmove = 0;
	}
}

void V_StopPitchDrift (playerview_t *pv)
{
	pv->laststop = cl.time;
	pv->nodrift = true;
	pv->pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards cl.idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.

Drifting is enabled when the center view key is hit, mlook is released and
lookspring is non 0, or when
===============
*/
void V_DriftPitch (playerview_t *pv)
{
	float		delta, move;

	if (!pv->onground || cls.demoplayback )
	{
		pv->driftmove = 0;
		pv->pitchvel = 0;
		return;
	}

// don't count small mouse motion
	if (pv->nodrift)
	{
		if (Length(pv->simvel) < 200)
			pv->driftmove = 0;
		else
			pv->driftmove += host_frametime;

		if ( pv->driftmove > v_centermove.value)
		{
			V_StartPitchDrift (pv);
		}
		return;
	}

	delta = 0 - pv->viewangles[PITCH];

	if (!delta)
	{
		pv->pitchvel = 0;
		return;
	}

	move = host_frametime * pv->pitchvel;
	pv->pitchvel += host_frametime * v_centerspeed.value;

//Con_Printf ("move: %f (%f)\n", move, host_frametime);

	if (delta > 0)
	{
		if (move > delta)
		{
			pv->pitchvel = 0;
			move = delta;
		}
		pv->viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			pv->pitchvel = 0;
			move = -delta;
		}
		pv->viewangles[PITCH] -= move;
	}
}





/*
==============================================================================

						PALETTE FLASHES

==============================================================================
*/

static void QDECL V_Gamma_Callback(struct cvar_s *var, char *oldvalue);

cshift_t	cshift_empty = { {130,80,50}, 0 };
cshift_t	cshift_water = { {130,80,50}, 128 };
cshift_t	cshift_slime = { {0,25,5}, 150 };
cshift_t	cshift_lava = { {255,80,0}, 150 };

cshift_t	cshift_server = { {130,80,50}, 0 };

cvar_t		v_gamma = CVARFCD("gamma", "1.0", CVAR_ARCHIVE|CVAR_RENDERERCALLBACK, V_Gamma_Callback, "Controls how bright the screen is. Setting this to anything but 1 without hardware gamma requires glsl support and can noticably harm your framerate.");
cvar_t		v_contrast = CVARFCD("contrast", "1.0", CVAR_ARCHIVE, V_Gamma_Callback, "Scales colour values linearly to make your screen easier to see. Setting this to anything but 1 without hardware gamma will reduce your framerates a little.");
cvar_t		v_brightness = CVARFCD("brightness", "0.0", CVAR_ARCHIVE, V_Gamma_Callback, "Brightness is how much 'white' to add to each and every pixel on the screen.");

qbyte		gammatable[256];	// palette is sent through this


qboolean		gammaworks;
float		hw_blend[4];		// rgba 0.0 - 1.0
/*
void BuildGammaTable (float g)
{
	int		i, inf;

	if (g == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
		return;
	}

	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * pow ( (i+0.5)/255.5 , g ) + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		gammatable[i] = inf;
	}
}*/
void BuildGammaTable (float g, float c, float b)
{
	int i, inf;

//	g = bound (0.1, g, 3);
//	c = bound (1, c, 3);

	if (g == 1 && c == 1)
	{
		for (i = 0; i < 256; i++)
			gammatable[i] = i;
		return;
	}

	for (i = 0; i < 256; i++)
	{
		//the 0.5s are for rounding.
		inf = 255 * (pow ((i + 0.5) / 255.5 * c, g) + b) + 0.5;
		if (inf < 0)
			inf = 0;
		else if (inf > 255)
			inf = 255;
		gammatable[i] = inf;
	}
}

/*
=================
V_CheckGamma
=================
*/
static void QDECL V_Gamma_Callback(struct cvar_s *var, char *oldvalue)
{
	BuildGammaTable (v_gamma.value, v_contrast.value, v_brightness.value);
	V_UpdatePalette (true);
}

#if defined(_WIN32) && !defined(_XBOX)
void W32_BlowChunk(vec3_t pos, float radius)
{
	vec3_t center;
	if (Matrix4x4_CM_Project(pos, center, r_refdef.viewangles, r_refdef.vieworg, r_refdef.fov_x, r_refdef.fov_y))
	{
		int mid_x = center[0]*r_refdef.vrect.width+r_refdef.vrect.x;
		int mid_y = (1-center[1])*r_refdef.vrect.height+r_refdef.vrect.y;

		HRGN tmp = CreateRectRgn(0,0,0,0);
		HRGN newrgn = CreateRectRgn(0,0,0,0);
		HRGN oldrgn = CreateRectRgn(0,0,0,0);
		HRGN hole = CreateEllipticRgn(mid_x-radius, mid_y-radius, mid_x+radius, mid_y+radius);
		if (GetWindowRgn(mainwindow, oldrgn) <= NULLREGION)
		{
			RECT rect;
			DeleteObject(oldrgn);
			GetWindowRect(mainwindow, &rect);
			oldrgn = CreateRectRgn(0,0,rect.right-rect.left,rect.bottom-rect.top);
		}
		CombineRgn(tmp, oldrgn, hole, RGN_XOR);
		CombineRgn(newrgn, oldrgn, tmp, RGN_AND);
		DeleteObject(oldrgn);
		DeleteObject(hole);
		DeleteObject(tmp);
		SetWindowRgn(mainwindow, newrgn, TRUE);
	}
}
#endif


/*
===============
V_ParseDamage
===============
*/
void V_ParseDamage (playerview_t *pv)
{
	int		armor, blood;
	vec3_t	from;
	int		i;
	vec3_t	forward, right, up;
	float	side;
	float	count;

	armor = MSG_ReadByte ();
	blood = MSG_ReadByte ();
	for (i=0 ; i<3 ; i++)
		from[i] = MSG_ReadCoord ();

	pv->faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

#if defined(_WIN32) && !defined(MINIMAL) && !defined(_XBOX)
	if (itburnsitburnsmakeitstop.value > 0)
		W32_BlowChunk(from, (armor+blood) * itburnsitburnsmakeitstop.value);
#endif

#ifdef CSQC_DAT
	if (CSQC_Parse_Damage(armor, blood, from))
		return;
#endif

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

#ifdef ANDROID
	//later versions of android might support strength values, but the simple standard interface is duration only.
	Sys_Vibrate(count);
#endif

	if (v_damagecshift.value >= 0)
		count *= v_damagecshift.value;

	pv->cshifts[CSHIFT_DAMAGE].percent += 3*count;
	if (pv->cshifts[CSHIFT_DAMAGE].percent < 0)
		pv->cshifts[CSHIFT_DAMAGE].percent = 0;
	if (pv->cshifts[CSHIFT_DAMAGE].percent > 150)
		pv->cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)
	{
		pv->cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		pv->cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		pv->cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		pv->cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		pv->cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		pv->cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		pv->cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		pv->cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		pv->cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}

//
// calculate view angle kicks
//
	VectorSubtract (from, pv->simorg, from);
	VectorNormalize (from);

	AngleVectors (pv->simangles, forward, right, up);

	side = DotProduct (from, right);
	pv->v_dmg_roll = count*side*v_kickroll.value;

	side = DotProduct (from, forward);
	pv->v_dmg_pitch = count*side*v_kickpitch.value;

	pv->v_dmg_time = v_kicktime.value;
}


/*
==================
V_cshift_f
==================
*/
void V_cshift_f (void)
{
	int r, g, b, p;
	playerview_t *pv = &cl.playerview[CL_TargettedSplit(true)];

	r = g = b = p = 0;

	if (Cmd_Argc() >= 5)
	{
		r = atoi(Cmd_Argv(1));
		g = atoi(Cmd_Argv(2));
		b = atoi(Cmd_Argv(3));
		p = atoi(Cmd_Argv(4));
	}

	if (Cmd_FromGamecode())
	{
		if (Cmd_Argc() >= 5)
		{
			qboolean term = false;
			int i;
			char *c = Cmd_Argv(4);

			// malice jumbles commands into a v_cshift so this attempts to fix
			while (isdigit(*c) || *c == '.')
				c++;

			if (*c)
			{
				Cbuf_AddText(c, RESTRICT_SERVER);
				term = true;
			}
			for (i = 5; i < Cmd_Argc(); i++)
			{
				Cbuf_AddText(" ", RESTRICT_SERVER);
				Cbuf_AddText(Cmd_Argv(i), RESTRICT_SERVER);
				term = true;
			}
			if (term)
				Cbuf_AddText("\n", RESTRICT_SERVER);
		}
		else if (Cmd_Argc() > 1)
			Con_DPrintf("broken v_cshift from gamecode\n");

		// ensure we always clear out or set for nehahra
		pv->cshifts[CSHIFT_SERVER].destcolor[0] = r;
		pv->cshifts[CSHIFT_SERVER].destcolor[1] = g;
		pv->cshifts[CSHIFT_SERVER].destcolor[2] = b;
		pv->cshifts[CSHIFT_SERVER].percent = p;
		return;
	}

	if (Cmd_Argc() != 5 && Cmd_Argc() != 1)
	{
		Con_Printf("v_cshift: v_cshift <r> <g> <b> <alpha>\n");
		return;
	}

	cshift_empty.destcolor[0] = r;
	cshift_empty.destcolor[1] = g;
	cshift_empty.destcolor[2] = b;
	cshift_empty.percent = p;
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void V_BonusFlash_f (void)
{
	playerview_t *pv = &cl.playerview[CL_TargettedSplit(true)];
	float frac;
	if (!gl_cshiftenabled.ival)
		frac = 0;
	else if (Cmd_FromGamecode())
		frac = v_bonusflash.value;
	else
		frac = 1;

	{
		//still adheres to gl_cshiftpercent even when forced.
		float minfrac = atof(Cmd_Argv(5));
		if (frac < minfrac)
			frac = minfrac;
	}

	frac *= gl_cshiftpercent.value / 100.0;

	if (frac)
	{
		if (Cmd_Argc() > 1)
		{	//this is how I understand DP expects them.
			pv->cshifts[CSHIFT_BONUS].destcolor[0] = atof(Cmd_Argv(1))*255;
			pv->cshifts[CSHIFT_BONUS].destcolor[1] = atof(Cmd_Argv(2))*255;
			pv->cshifts[CSHIFT_BONUS].destcolor[2] = atof(Cmd_Argv(3))*255;
			pv->cshifts[CSHIFT_BONUS].percent = atof(Cmd_Argv(4))*255*frac;
		}
		else
		{
			pv->cshifts[CSHIFT_BONUS].destcolor[0] = 215;
			pv->cshifts[CSHIFT_BONUS].destcolor[1] = 186;
			pv->cshifts[CSHIFT_BONUS].destcolor[2] = 69;
			pv->cshifts[CSHIFT_BONUS].percent = 50*frac;
		}
	}
}
void V_DarkFlash_f (void)
{
	playerview_t *pv = &cl.playerview[CL_TargettedSplit(true)];
	float frac;
	if (!gl_cshiftenabled.ival)
		frac = 0;
	else
		frac = 1;
	frac *= gl_cshiftpercent.value / 100.0;

	pv->cshifts[CSHIFT_BONUS].destcolor[0] = 0;
	pv->cshifts[CSHIFT_BONUS].destcolor[1] = 0;
	pv->cshifts[CSHIFT_BONUS].destcolor[2] = 0;
	pv->cshifts[CSHIFT_BONUS].percent = 255*frac;
}
void V_WhiteFlash_f (void)
{
	playerview_t *pv = &cl.playerview[CL_TargettedSplit(true)];
	float frac;
	if (!gl_cshiftenabled.ival)
		frac = 0;
	else
		frac = 1;
	frac *= gl_cshiftpercent.value / 100.0;

	pv->cshifts[CSHIFT_BONUS].destcolor[0] = 255;
	pv->cshifts[CSHIFT_BONUS].destcolor[1] = 255;
	pv->cshifts[CSHIFT_BONUS].destcolor[2] = 255;
	pv->cshifts[CSHIFT_BONUS].percent = 255*frac;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift

FIXME: Uses Q1 contents
=============
*/
void V_SetContentsColor (int contents)
{
	int i;
	playerview_t *pv = r_refdef.playerview;

	if (contents & FTECONTENTS_LAVA)
		pv->cshifts[CSHIFT_CONTENTS] = cshift_lava;
	else if (contents & (FTECONTENTS_SLIME | FTECONTENTS_SOLID))
		pv->cshifts[CSHIFT_CONTENTS] = cshift_slime;
	else if (contents & FTECONTENTS_WATER)
		pv->cshifts[CSHIFT_CONTENTS] = cshift_water;
	else
		pv->cshifts[CSHIFT_CONTENTS] = cshift_empty;

	pv->cshifts[CSHIFT_CONTENTS].percent *= v_contentblend.value;

	if (pv->cshifts[CSHIFT_CONTENTS].percent)
	{	//bound contents so it can't go negative
		if (pv->cshifts[CSHIFT_CONTENTS].percent < 0)
			pv->cshifts[CSHIFT_CONTENTS].percent = 0;

		for (i = 0; i < 3; i++)
			if (pv->cshifts[CSHIFT_CONTENTS].destcolor[0] < 0)
				pv->cshifts[CSHIFT_CONTENTS].destcolor[0] = 0;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift (playerview_t *pv)
{
#ifdef QUAKESTATS
	int im = pv->stats[STAT_ITEMS];

	if (im & IT_QUAD)
	{
		pv->cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		pv->cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		pv->cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		pv->cshifts[CSHIFT_POWERUP].percent = 30*v_quadcshift.value;
	}
	else if (im & IT_SUIT)
	{
		pv->cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		pv->cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		pv->cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		pv->cshifts[CSHIFT_POWERUP].percent = 20*v_suitcshift.value;
	}
	else if (im & IT_INVISIBILITY)
	{
		pv->cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		pv->cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		pv->cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		pv->cshifts[CSHIFT_POWERUP].percent = 100*v_ringcshift.value;
	}
	else if (im & IT_INVULNERABILITY)
	{
		pv->cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		pv->cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		pv->cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		pv->cshifts[CSHIFT_POWERUP].percent = 30*v_pentcshift.value;
	}
	else
		pv->cshifts[CSHIFT_POWERUP].percent = 0;

	if (pv->cshifts[CSHIFT_POWERUP].percent<0)
		pv->cshifts[CSHIFT_POWERUP].percent=0;
#endif
}


/*
=============
V_CalcBlend
=============
*/
void V_CalcBlend (float *hw_blend)
{
	extern qboolean r2d_canhwgamma;
	float	a2;
	int		j, seat;
	float *blend;
	playerview_t *pv;

	memset(hw_blend, 0, sizeof(float)*4);
	for (seat = 0; seat < cl.splitclients; seat++)
	{
		pv = &cl.playerview[seat];
		memset(pv->screentint, 0, sizeof(pv->screentint));

		//don't apply it to the server, we'll blend the two later if the user has no hardware gamma (if they do have it, we use just the server specified value) This way we avoid winnt users having a cheat with flashbangs and stuff.
		for (j=0 ; j<NUM_CSHIFTS ; j++)
		{
			if (j != CSHIFT_SERVER && j != CSHIFT_BONUS)
			{
				if (!gl_cshiftpercent.value || !gl_cshiftenabled.ival)
					continue;

				a2 = ((pv->cshifts[j].percent * gl_cshiftpercent.value) / 100.0) / 255.0;
			}
			else
			{
				a2 = pv->cshifts[j].percent / 255.0;	//don't allow modification of this one.
			}

			if (!a2)
				continue;

			if (j == CSHIFT_SERVER)
			{
				/*server blend always goes into sw, ALWAYS*/
				blend = pv->screentint;
			}
			else
			{
				/*flashing things should not change hardware gamma ramps - windows is too slow*/
				/*splitscreen should only use hw gamma ramps if they're all equal, and they're probably not*/
				/*hw blends may also not be supported or may be disabled*/
				if (j == CSHIFT_BONUS || j == CSHIFT_DAMAGE || gl_nohwblend.ival || !r2d_canhwgamma || cl.splitclients > 1)
					blend = pv->screentint;
				else	//powerup or contents?
					blend = hw_blend;
			}

			blend[3] = blend[3] + a2*(1-blend[3]);
			a2 = a2/blend[3];
			blend[0] = blend[0]*(1-a2) + pv->cshifts[j].destcolor[0]*a2/255.0;
			blend[1] = blend[1]*(1-a2) + pv->cshifts[j].destcolor[1]*a2/255.0;
			blend[2] = blend[2]*(1-a2) + pv->cshifts[j].destcolor[2]*a2/255.0;
		}

		if (pv->screentint[3] > 1)
			pv->screentint[3] = 1;
		if (pv->screentint[3] < 0)
			pv->screentint[3] = 0;
	}
	for (; seat < MAX_SPLITS; seat++)
	{
		pv = &cl.playerview[seat];
		memset(pv->screentint, 0, sizeof(pv->screentint));
	}
	if (hw_blend[3] > 1)
		hw_blend[3] = 1;
	if (hw_blend[3] < 0)
		hw_blend[3] = 0;
}

/*
=============
V_UpdatePalette
=============
*/
void V_UpdatePalette (qboolean force)
{
	extern	qboolean r2d_canhwgamma;
	int		i;
	float	newhw_blend[4];
	int		ir, ig, ib;
	float	ftime;
	qboolean applied;
	static double oldtime;
	RSpeedMark();

	ftime = cl.time - oldtime;
	oldtime = cl.time;
	if (ftime < 0)
		ftime = 0;

	for (i = 0; i < MAX_SPLITS; i++)
	{
		playerview_t *pv = &cl.playerview[i];
		V_CalcPowerupCshift(pv);
	// drop the damage value
		pv->cshifts[CSHIFT_DAMAGE].percent -= ftime*150;
		if (pv->cshifts[CSHIFT_DAMAGE].percent <= 0)
			pv->cshifts[CSHIFT_DAMAGE].percent = 0;

	// drop the bonus value
		pv->cshifts[CSHIFT_BONUS].percent -= ftime*100;
		if (pv->cshifts[CSHIFT_BONUS].percent <= 0)
			pv->cshifts[CSHIFT_BONUS].percent = 0;
	}

	V_CalcBlend(newhw_blend);

	if (hw_blend[0] != newhw_blend[0] || hw_blend[1] != newhw_blend[1] || hw_blend[2] != newhw_blend[2] || hw_blend[3] != newhw_blend[3] || force)
	{
		float r,g,b,a;
		static unsigned short		allramps[3*2048];
		unsigned int rampsize = min(vid.gammarampsize, countof(allramps)/3);
		unsigned short *ramps[3] = {&allramps[0],&allramps[rampsize],&allramps[rampsize*2]};
		Vector4Copy(newhw_blend, hw_blend);

		a = hw_blend[3];
		r = 255*hw_blend[0]*a;
		g = 255*hw_blend[1]*a;
		b = 255*hw_blend[2]*a;

		a = 1-a;
		a *= 256.0/rampsize;
		for (i=0 ; i < rampsize; i++)
		{
			ir = i*a + r;
			ig = i*a + g;
			ib = i*a + b;
			if (ir > 255)
				ir = 255;
			if (ig > 255)
				ig = 255;
			if (ib > 255)
				ib = 255;

			//FIXME: shit precision
			ramps[0][i] = gammatable[ir]<<8;
			ramps[1][i] = gammatable[ig]<<8;
			ramps[2][i] = gammatable[ib]<<8;
		}

		if (qrenderer)
		{
			applied = rf->VID_ApplyGammaRamps (rampsize, allramps);
			if (!applied && r2d_canhwgamma)
				rf->VID_ApplyGammaRamps (0, NULL);
			r2d_canhwgamma = applied;
		}
	}

	RSpeedEnd(RSPEED_PALETTEFLASHES);
}

/*
=============
V_UpdatePalette
=============
*/

void V_ClearCShifts (void)
{
	int i, seat;

	for (seat = 0; seat < MAX_SPLITS; seat++)
		for (i = 0; i < NUM_CSHIFTS; i++)
			cl.playerview[seat].cshifts[i].percent = 0;
}

/*
==============================================================================

						VIEW RENDERING

==============================================================================
*/

float angledelta (float a)
{
	a = anglemod(a);
	if (a > 180)
		a -= 360;
	return a;
}

/*
==================
CalcGunAngle
==================
*/
void V_CalcGunPositionAngle (playerview_t *pv, float bob)
{
	float	yaw, pitch, move;
	static float oldyaw = 0;
	static float oldpitch = 0;
	vec3_t vw_angles;
	int i;

	yaw = r_refdef.viewangles[YAW];
	pitch = -r_refdef.viewangles[PITCH];

	yaw = angledelta(yaw - r_refdef.viewangles[YAW]) * 0.4;
	if (yaw > 10)
		yaw = 10;
	if (yaw < -10)
		yaw = -10;
	pitch = angledelta(-pitch - r_refdef.viewangles[PITCH]) * 0.4;
	if (pitch > 10)
		pitch = 10;
	if (pitch < -10)
		pitch = -10;
	move = host_frametime*20;
	if (yaw > oldyaw)
	{
		if (oldyaw + move < yaw)
			yaw = oldyaw + move;
	}
	else
	{
		if (oldyaw - move > yaw)
			yaw = oldyaw - move;
	}

	if (pitch > oldpitch)
	{
		if (oldpitch + move < pitch)
			pitch = oldpitch + move;
	}
	else
	{
		if (oldpitch - move > pitch)
			pitch = oldpitch - move;
	}

	oldyaw = yaw;
	oldpitch = pitch;

	vw_angles[YAW] = r_refdef.viewangles[YAW] + yaw;
	vw_angles[PITCH] = r_refdef.viewangles[PITCH] + pitch;

	vw_angles[YAW] = r_refdef.viewangles[YAW];
	vw_angles[PITCH] = r_refdef.viewangles[PITCH];
	vw_angles[ROLL] = r_refdef.viewangles[ROLL];

	AngleVectors(vw_angles, pv->vw_axis[0], pv->vw_axis[1], pv->vw_axis[2]);
	VectorInverse(pv->vw_axis[1]);



	VectorCopy (r_refdef.vieworg, pv->vw_origin);
	for (i=0 ; i<3 ; i++)
	{
		pv->vw_origin[i] += pv->vw_axis[0][i]*bob*0.4;
//		pv->vw_origin[i] += pv->vw_axis[1][i]*sin(cl.time*5.5342452354235)*0.1;
//		pv->vw_origin[i] += pv->vw_axis[2][i]*bob*0.8;
	}

// fudge position around to keep amount of weapon visible
// roughly equal with different FOV
//FIXME: should use y fov, not viewsize.
	if (r_refdef.drawsbar)	//no sbar = no viewsize cvar.
	{
		if (scr_viewsize.value == 110)
			pv->vw_origin[2] += 1;
		else if (scr_viewsize.value == 100)
			pv->vw_origin[2] += 2;
		else if (scr_viewsize.value == 90)
			pv->vw_origin[2] += 1;
		else if (scr_viewsize.value == 80)
			pv->vw_origin[2] += 0.5;
	}
}

/*
==============
V_BoundOffsets
==============
*/
void V_BoundOffsets (int pnum)
{
// absolutely bound refresh reletive to entity clipping hull
// so the view can never be inside a solid wall

	if (r_refdef.vieworg[0] < cl.playerview[pnum].simorg[0] - 14)
		r_refdef.vieworg[0] = cl.playerview[pnum].simorg[0] - 14;
	else if (r_refdef.vieworg[0] > cl.playerview[pnum].simorg[0] + 14)
		r_refdef.vieworg[0] = cl.playerview[pnum].simorg[0] + 14;
	if (r_refdef.vieworg[1] < cl.playerview[pnum].simorg[1] - 14)
		r_refdef.vieworg[1] = cl.playerview[pnum].simorg[1] - 14;
	else if (r_refdef.vieworg[1] > cl.playerview[pnum].simorg[1] + 14)
		r_refdef.vieworg[1] = cl.playerview[pnum].simorg[1] + 14;
	if (r_refdef.vieworg[2] < cl.playerview[pnum].simorg[2] - 22)
		r_refdef.vieworg[2] = cl.playerview[pnum].simorg[2] - 22;
	else if (r_refdef.vieworg[2] > cl.playerview[pnum].simorg[2] + 30)
		r_refdef.vieworg[2] = cl.playerview[pnum].simorg[2] + 30;
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle (playerview_t *pv)
{
	//defaults: for use if idlescale is locked and the var isn't.
	float yaw_cycle		= 2;
	float roll_cycle	= 0.5;
	float pitch_cycle	= 1;
	float yaw_level		= 0.3;
	float roll_level	= 0.1;
	float pitch_level	= 0.3;

	if (v_iyaw_cycle.flags & CVAR_SERVEROVERRIDE || !(v_idlescale.flags & CVAR_SERVEROVERRIDE))
		yaw_cycle = v_iyaw_cycle.value;
	if (v_iroll_cycle.flags & CVAR_SERVEROVERRIDE || !(v_idlescale.flags & CVAR_SERVEROVERRIDE))
		roll_cycle = v_iroll_cycle.value;
	if (v_ipitch_cycle.flags & CVAR_SERVEROVERRIDE || !(v_idlescale.flags & CVAR_SERVEROVERRIDE))
		pitch_cycle = v_ipitch_cycle.value;

	if (v_iyaw_level.flags & CVAR_SERVEROVERRIDE || !(v_idlescale.flags & CVAR_SERVEROVERRIDE))
		yaw_level = v_iyaw_level.value;
	if (v_iroll_level.flags & CVAR_SERVEROVERRIDE || !(v_idlescale.flags & CVAR_SERVEROVERRIDE))
		roll_level = v_iroll_level.value;
	if (v_ipitch_level.flags & CVAR_SERVEROVERRIDE || !(v_idlescale.flags & CVAR_SERVEROVERRIDE))
		pitch_level = v_ipitch_level.value;

	r_refdef.viewangles[ROLL] += v_idlescale.value * sin(cl.time*roll_cycle) * roll_level;
	r_refdef.viewangles[PITCH] += v_idlescale.value * sin(cl.time*pitch_cycle) * pitch_level;
	r_refdef.viewangles[YAW] += v_idlescale.value * sin(cl.time*yaw_cycle) * yaw_level;

//	pv->viewent.angles[ROLL] -= v_idlescale.value * sin(cl.time*roll_cycle) * roll_level;
//	pv->viewent.angles[PITCH] -= v_idlescale.value * sin(cl.time*pitch_cycle) * pitch_level;
//	pv->viewent.angles[YAW] -= v_idlescale.value * sin(cl.time*yaw_cycle) * yaw_level;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll (playerview_t *pv)
{
	float		side;
	float	adjspeed;

	side = V_CalcRoll (pv->simangles, pv->simvel);

	adjspeed = fabs(cl_rollangle.value);
	if (adjspeed<1)
		adjspeed=1;
	if (adjspeed>45)
		adjspeed = 45;
	adjspeed*=20;
	if (side > pv->rollangle)
	{
		pv->rollangle += host_frametime * adjspeed;
		if (pv->rollangle > side)
			pv->rollangle = side;
	}
	else if (side < pv->rollangle)
	{
		pv->rollangle -= host_frametime * adjspeed;
		if (pv->rollangle < side)
			pv->rollangle = side;
	}
	r_refdef.viewangles[ROLL] += pv->rollangle;

	if (pv->v_dmg_time > 0)
	{
		r_refdef.viewangles[ROLL] += pv->v_dmg_time/v_kicktime.value*pv->v_dmg_roll;
		r_refdef.viewangles[PITCH] += pv->v_dmg_time/v_kicktime.value*pv->v_dmg_pitch;
		pv->v_dmg_time -= host_frametime;
	}

}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef (playerview_t *pv)
{
	float		old;

	VectorCopy (pv->simorg, r_refdef.vieworg);
	VectorCopy (pv->simangles, r_refdef.viewangles);

	if (cl.intermissionmode == IM_H2FINALE)
		VectorMA(r_refdef.vieworg, -pv->viewheight, pv->gravitydir, r_refdef.vieworg);

// always idle in intermission
	old = v_idlescale.value;
	v_idlescale.value = 1;
	V_AddIdle (pv);
	v_idlescale.value = old;
}

float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x <= 0 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	x = fov_x/360*M_PI;
	x = tan(x);
	x = width/x;

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}
void V_ApplyAFov(playerview_t *pv)
{
	//explicit fov overrides aproximate fov.
	//aproximate fov is our regular fov value. explicit is settable by gamecode for weird aspect ratios
	if (!r_refdef.fov_x || !r_refdef.fov_y)
	{
		extern cvar_t r_stereo_separation;
		float ws;

		float afov = r_refdef.afov;
		if (!afov)	//make sure its sensible.
			afov = scr_fov.value;
#ifdef QUAKESTATS
		if (pv && pv->stats[STAT_VIEWZOOM])
			afov *= pv->stats[STAT_VIEWZOOM]/255.0f;
#endif
		afov = bound(0.001, afov, 170);

		ws = 1;
#ifdef FTE_TARGET_WEB
		if (r_refdef.stereomethod == STEREO_WEBVR)
			ws = 0.5;
#endif
		if (r_refdef.stereomethod == STEREO_CROSSEYED && r_stereo_separation.value)
			ws = 0.5;

		//attempt to retain a classic fov
		if (ws*r_refdef.vrect.width < (r_refdef.vrect.height*640)/432)
		{
			r_refdef.fov_y = CalcFov(afov, (ws*r_refdef.vrect.width*r_refdef.pxrect.width)/vid.fbvwidth, (r_refdef.vrect.height*r_refdef.pxrect.height)/vid.fbvheight);
			r_refdef.fov_x = afov;//CalcFov(r_refdef.fov_y, 432, 640);
		}
		else
		{
			r_refdef.fov_y = CalcFov(afov, 640, 432);
			r_refdef.fov_x = CalcFov(r_refdef.fov_y, r_refdef.vrect.height, r_refdef.vrect.width*ws);
		}
	}
	if (r_refdef.useperspective)
	{
		if (r_refdef.mindist < 1)
			r_refdef.mindist = 1;
		if (r_refdef.maxdist && r_refdef.maxdist < 100)
			r_refdef.maxdist = 0;	//small values should just use infinite.
	}
}
/*
=================
v_ApplyRefdef

called to apply any dirty refdef bits and recalculates pending data.
=================
*/
void V_ApplyRefdef (void)
{
#ifdef QUAKEHUD
	float           size;
	int             h;
	qboolean		full = false;
#endif

// force the status bar to redraw
	Sbar_Changed ();

//========================================

	if (r_refdef.playerview->gamerectknown != cls.framecount)
	{
		r_refdef.playerview->gamerectknown = cls.framecount;
		r_refdef.playerview->gamerect = r_refdef.grect;
	}

#ifndef QUAKEHUD
	r_refdef.vrect = r_refdef.grect;
#else


// intermission is always full screen
	if (cl.intermissionmode != IM_NONE || !r_refdef.drawsbar)
		size = 120;
	else
		size = scr_viewsize.value;

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)	//q2 never has a hud.
		sb_lines = 0;
	else
#endif
	     if (size >= 120)
		sb_lines = 0;           // no status bar at all
	else if (size >= 110)
		sb_lines = 24;          // no inventory
	else
		sb_lines = 24+16+8;

	if (scr_viewsize.value >= 100.0 || scr_chatmode)
	{
		full = true;
		size = 100.0;
	}
	else
		size = scr_viewsize.value;

	if (cl.intermissionmode != IM_NONE || !r_refdef.drawsbar)
	{
		full = true;
		size = 100.0;
		sb_lines = 0;
	}
	size /= 100.0;

	if (cl_sbar.value!=1 && full)
		h = r_refdef.grect.height;
	else
		h = r_refdef.grect.height - sb_lines;
	if (h < 0)
		h = 0;

	r_refdef.vrect.width = r_refdef.grect.width * size;
	if (r_refdef.vrect.width < 96)
		r_refdef.vrect.width = 96;      // min for icons

	r_refdef.vrect.height = r_refdef.grect.height * size;
	if (cl_sbar.value==1 || !full)
	{
  		if (r_refdef.vrect.height > r_refdef.grect.height - sb_lines)
  			r_refdef.vrect.height = r_refdef.grect.height - sb_lines;
	}
	else if (r_refdef.vrect.height > r_refdef.grect.height)
		r_refdef.vrect.height = r_refdef.grect.height;
	if (r_refdef.vrect.height < 0)
		r_refdef.vrect.height = 0;

	r_refdef.vrect.x = (r_refdef.grect.width - r_refdef.vrect.width)/2;
	if (full)
		r_refdef.vrect.y = 0;
	else
		r_refdef.vrect.y = (h - r_refdef.vrect.height)/2;

	if (scr_chatmode)
	{
		if (scr_chatmode != 2)
			r_refdef.vrect.height = r_refdef.vrect.y=r_refdef.grect.height/2;
		r_refdef.vrect.width = r_refdef.vrect.x=r_refdef.grect.width/2;
		if (r_refdef.vrect.width<320 || r_refdef.vrect.height<200)	//disable hud if too small
			sb_lines=0;
	}
	r_refdef.vrect.x += r_refdef.grect.x;
	r_refdef.vrect.y += r_refdef.grect.y;
#endif

	if (r_refdef.dirty & RDFD_FOV)
		V_ApplyAFov(r_refdef.playerview);

	r_refdef.dirty = 0;

	if (chase_active.ival && cls.allow_cheats)
		V_EditExternalModels(0, NULL, 0);
	if (v_depthsortentities.ival)
		V_DepthSortEntities(r_refdef.vieworg);
}

//if the view entities differ, removes all externalmodel flags except for adding it to the new entity, and removes weaponmodels.
//returns the number of view entities that were stripped out
int V_EditExternalModels(int newviewentity, entity_t *viewentities, int maxviewenties)
{
	int i;
	int viewents = 0;
	for (i = 0; i < cl_numvisedicts; )
	{
		if (cl_visedicts[i].keynum == newviewentity && newviewentity)
			cl_visedicts[i].flags |= RF_EXTERNALMODEL;
		else
			cl_visedicts[i].flags &= ~RF_EXTERNALMODEL;

		if (cl_visedicts[i].flags & RF_WEAPONMODEL)
		{
			if (viewents < maxviewenties)
				viewentities[viewents++] = cl_visedicts[i];
			memmove(&cl_visedicts[i], &cl_visedicts[i+1], sizeof(*cl_visedicts) * (cl_numvisedicts-(i+1)));
			cl_numvisedicts--;
		}
		else
			i++;
	}
	return viewents;
}

//this is for transparency effects, so we actually want the furthest first
//this is contrary for overdraw performance, but transparency doesn't work like that
//we use angles[0] to hold distance. the renderer shouldn't be using it anyway.
static int QDECL V_DepthSortTwoEntities(const void *p1,const void *p2)
{
	const entity_t *a = p1;
	const entity_t *b = p2;

	if (a->angles[0] < b->angles[0])
		return -1;
	if (a->angles[0] > b->angles[0])
		return 1;
	return 0;
}
void V_DepthSortEntities(float *vieworg)
{
	int i;
	vec3_t disp;
	for (i = 0; i < cl_numvisedicts; i++)
	{
		if (cl_visedicts[i].flags & RF_WEAPONMODEL)
		{	//weapon models have their own extra matrix thing going on. don't mess up because of it.
			cl_visedicts[i].angles[0] = 0;
			continue;
		}
		if (cl_visedicts[i].rtype == RT_MODEL && cl_visedicts[i].model && cl_visedicts[i].model->type == mod_brush)
		{
			VectorAdd(cl_visedicts[i].model->maxs, cl_visedicts[i].model->mins, disp);
			VectorMA(cl_visedicts[i].origin, 0.5, disp, disp);
			VectorSubtract(disp, vieworg, disp);
		}
		else
		{
			VectorSubtract(cl_visedicts[i].origin, vieworg, disp);
		}
		cl_visedicts[i].angles[0] = DotProduct(disp,disp);	//don't bother with sqrts
	}
	qsort(cl_visedicts, cl_numvisedicts, sizeof(cl_visedicts[0]), V_DepthSortTwoEntities);
}

/*
clears the refdef to defaults.
*/
void V_ClearRefdef(playerview_t *pv)
{
	r_refdef.playerview = pv;
	r_refdef.dirty = ~0;

	r_refdef.grect.x = 0;
	r_refdef.grect.y = 0;
	r_refdef.grect.width = vid.fbvwidth;//vid.width;
	r_refdef.grect.height = vid.fbvheight;//vid.height;

	r_refdef.afov = scr_fov.value;	//will have a better value applied if fov is bad. this allows setting.
	r_refdef.fov_x = 0;
	r_refdef.fov_y = 0;

	r_refdef.drawsbar = cl.intermissionmode == IM_NONE;
	r_refdef.flags = 0;

	r_refdef.areabitsknown = false;

//	memset(r_refdef.postprocshader, 0, sizeof(r_refdef.postprocshader));
//	memset(r_refdef.postprocsize, 0, sizeof(r_refdef.postprocsize));
//	r_refdef.postproccube = 0;
}

/*
==================
V_CalcRefdef

==================
*/
void V_CalcRefdef (playerview_t *pv)
{
	float		bob;
	float		viewheight;

	r_refdef.playerview = pv;

	memset(&r_refdef.globalfog, 0, sizeof(r_refdef.globalfog));

	r_refdef.time = cl.servertime;
#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		VectorCopy (pv->simorg, r_refdef.vieworg);
		VectorCopy (pv->simangles, r_refdef.viewangles);
		return;
	}
#endif

	if (v_viewheight.value < -7)
		bob=-7;
	else if (v_viewheight.value > 4)
		bob=4;
	else if (v_viewheight.value)
		bob=v_viewheight.value;
	else
		bob = V_CalcBob (pv, false);

// refresh position from simulated origin
	VectorCopy (pv->simorg, r_refdef.vieworg);

	r_refdef.useperspective = true;
	r_refdef.mindist = bound(0.1, gl_mindist.value, 4);
	r_refdef.maxdist = gl_maxdist.value;

// never let it sit exactly on a node line, because a water plane can
// dissapear when viewed with the eye exactly on it.
// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
	r_refdef.vieworg[0] += 1.0/16;
	r_refdef.vieworg[1] += 1.0/16;
	r_refdef.vieworg[2] += 1.0/16;

	VectorCopy (pv->simangles, r_refdef.viewangles);

	V_CalcViewRoll (pv);
	V_AddIdle (pv);

	viewheight = pv->viewheight;
	if (viewheight == DEFAULT_VIEWHEIGHT && cls.protocol == CP_QUAKEWORLD && !(cls.z_ext & Z_EXT_VIEWHEIGHT))
	{
		if (view_message && view_message->flags & PF_GIB)
			viewheight = 8;	// gib view height
		else if (view_message && view_message->flags & PF_DEAD)
			viewheight = 16;	// corpse view height
	}

#ifdef QUAKESTATS
	if (pv->stats[STAT_HEALTH] < 0 && (!pv->spectator || pv->cam_state == CAM_EYECAM) && v_deathtilt.value)		// PF_GIB will also set PF_DEAD
	{
		if (!pv->spectator || cl_chasecam.ival)
			r_refdef.viewangles[ROLL] = 80*v_deathtilt.value;	// dead view angle
	}
	else
#endif
	{
		// v_viewheight only affects the view if the player is alive
		viewheight += bob;
	}
	viewheight += pv->crouch;

	VectorMA(r_refdef.vieworg, -viewheight, pv->gravitydir, r_refdef.vieworg);

// set up gun position
	V_CalcGunPositionAngle (pv, bob);

// set up the refresh position
	if (v_gunkick.value)
	{
		r_refdef.viewangles[PITCH] += pv->punchangle_cl*v_gunkick.value;
		VectorMA(r_refdef.viewangles, v_gunkick.value, pv->punchangle_sv, r_refdef.viewangles);
		VectorMA(r_refdef.vieworg, v_gunkick.value, pv->punchorigin, r_refdef.vieworg);
	}


	if (chase_active.ival && cls.allow_cheats)	//cheat restriction might be lifted some time when any wallhacks are solved.
	{
		vec3_t axis[3];
		vec3_t camorg, camdir;
		trace_t tr;
		float len;
		//r_refdef.viewangles[0] += chase_pitch.value;
		//r_refdef.viewangles[1] += chase_yaw.value;
		//r_refdef.viewangles[2] += chase_roll.value;
		if (chase_active.ival >= 2)
			r_refdef.viewangles[0] = 90;
		if (chase_active.ival >= 3)
			r_refdef.viewangles[1] = 0;
		AngleVectors(r_refdef.viewangles, axis[0], axis[1], axis[2]);
		VectorScale(axis[0], -chase_back.value, camdir);
		if (pv->pmovetype == PM_6DOF)
			VectorMA(camdir, chase_up.value, axis[2], camdir);
		else
			VectorMA(camdir, -chase_up.value, pv->gravitydir, camdir);
		len = VectorLength(camdir);
		VectorMA(r_refdef.vieworg, (len+128)/len, camdir, camorg);	//push it 128qu further
		if (cl.worldmodel && cl.worldmodel->funcs.NativeTrace)
		{
			cl.worldmodel->funcs.NativeTrace(cl.worldmodel, 0, 0, NULL, r_refdef.vieworg, camorg, vec3_origin, vec3_origin, true, MASK_WORLDSOLID, &tr);
			if (!tr.startsolid)
			{
				if (tr.fraction < 1)
				{
					//we found a plane, bisect it weirdly to push 4qu infront
					float d1,d2, frac;
					VectorMA(r_refdef.vieworg, 1, camdir, camorg);
					d1 = DotProduct(r_refdef.vieworg, tr.plane.normal) - (tr.plane.dist+4);
					d2 = DotProduct(camorg, tr.plane.normal) - (tr.plane.dist+4);
					frac = d1 / (d1-d2);
					frac = bound(0, frac, 1);
					VectorMA(r_refdef.vieworg, frac, camdir, r_refdef.vieworg);
				}
				else
					VectorMA(r_refdef.vieworg, 1, camdir, r_refdef.vieworg);
			}
		}
	}
}

/*
=============
DropPunchAngle
=============
*/
void DropPunchAngle (playerview_t *pv)
{
	if (pv->punchangle_cl < 0)
	{
		pv->punchangle_cl += 10*host_frametime;
		if (pv->punchangle_cl > 0)
			pv->punchangle_cl = 0;
	}
	else
	{
		pv->punchangle_cl -= 10*host_frametime;
		if (pv->punchangle_cl < 0)
			pv->punchangle_cl = 0;
	}
}

/*
==================
V_RenderView

The player's clipping box goes from (-16 -16 -24) to (16 16 32) from
the entity origin, so any view position inside that will be valid
==================
*/
extern vrect_t scr_vrect;

qboolean r_secondaryview;
#ifdef SIDEVIEWS

#ifdef PEXT_VIEW2
entity_t *CL_EntityNum(int num)
{
	int i;
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		if (cl_visedicts[i].keynum == num)
			return &cl_visedicts[i];
	}
	return NULL;
}
#endif
#endif

float CalcFov (float fov_x, float width, float height);
static void SCR_VRectForPlayer(vrect_t *vrect, int pnum, unsigned maxseats)
{
#if MAX_SPLITS > 4
#pragma warning "Please change this function to cope with the new MAX_SPLITS value"
#endif
	switch(maxseats)
	{
	case 1:
		vrect->width = vid.fbvwidth;
		vrect->height = vid.fbvheight;
		vrect->x = 0;
		vrect->y = 0;

		if (scr_chatmode == 2)
		{
			vrect->height/=2;
			vrect->y += vrect->height;
		}
		break;

	case 2:	//horizontal bands
	case 3:
#ifdef GLQUAKE
		if (qrenderer == QR_OPENGL && vid.rotpixelwidth > vid.rotpixelheight * 2
			&& r_projection.ival == PROJ_PANORAMA /*panoramic view always stacks player views*/
			)
		{	//over twice as wide as high, assume dual moniter, horizontal.
			vrect->width = vid.fbvwidth/cl.splitclients;
			vrect->height = vid.fbvheight;
			vrect->x = 0 + vrect->width*pnum;
			vrect->y = 0;
		}
		else
#endif
		{
			//stack them vertically
			vrect->width = vid.fbvwidth;
			vrect->height = vid.fbvheight/cl.splitclients;
			vrect->x = 0;
			vrect->y = 0 + vrect->height*pnum;
		}

		break;

	case 4:	//4 squares
		vrect->width = vid.fbvwidth/2;
		vrect->height = vid.fbvheight/2;
		vrect->x = (pnum&1) * vrect->width;
		vrect->y = (pnum&2)/2 * vrect->height;
		break;

	default:
		Sys_Error("cl.splitclients is invalid.");
	}
}

void Draw_ExpandedString(float x, float y, conchar_t *str);

static void SCR_DrawAutoID(vec3_t org, player_info_t *pl, qboolean isteam)
{
	conchar_t buffer[256];
	int len;
	vec3_t center;
	vec3_t tagcenter;
	float alpha;
	qboolean obscured;
	int x, y;
	int r;
	float barwidth;
	qboolean haveinfo;
	unsigned int textflags;
	int h;
	char *pname;

	int health;
	int armour;
	unsigned int items;
	static vec4_t healthcolours[] =
	{
		{0.7, 0.45, 0.45, 1},
		{0.3, 0, 0, 1},
		{1, 0, 0, 1},
		{1, 0.4, 0, 1},
		{1, 1, 1, 1}
	};
#ifdef QUAKESTATS
	static vec4_t armourcolours[] =
	{
		{25, 170, 0, 0.2},
		{25, 170, 0, 1},
		{225, 220, 0, 0.2},
		{225, 220, 0, 1},
		{255, 0, 0, 0.2},
		{255, 0, 0, 1}
	};

	extern cvar_t tp_name_sg,tp_name_ssg,tp_name_ng,tp_name_sng,tp_name_gl,tp_name_rl,tp_name_lg;
	static cvar_t *wbitnames[] =
	{
		&tp_name_sg,
		&tp_name_ssg,
		&tp_name_ng,
		&tp_name_sng,
		&tp_name_gl,
		&tp_name_rl,
		&tp_name_lg
	};
#endif

	VectorCopy(org, tagcenter);
	tagcenter[2] += 32;
	if (!Matrix4x4_CM_Project(tagcenter, center, r_refdef.viewangles, r_refdef.vieworg, r_refdef.fov_x, r_refdef.fov_y))
		return;	//behind the camera
	if (center[0] < 0 || center[0] > 1 || center[1] < 0 || center[1] > 1)
		return;	//off the side of the screen

	obscured = !TP_IsPlayerVisible(org);
	
	if (obscured && !isteam)
		return;	//only teammembers are drawn when obscured
	if (isteam)
		textflags = scr_autoid_teamcolour.ival << CON_FGSHIFT;
	else
		textflags = scr_autoid_enemycolour.ival << CON_FGSHIFT;
	if (obscured)
	{
		if (scr_autoid_team.ival == 2)
			return;
		textflags |= CON_HALFALPHA;
		alpha = 0.25;
	}
	else
		alpha = 1;

	x = center[0]*r_refdef.vrect.width+r_refdef.vrect.x;
	y = (1-center[1])*r_refdef.vrect.height+r_refdef.vrect.y;

#ifdef QUAKESTATS
	if (cls.demoplayback == DPB_MVD || cls.demoplayback == DPB_EZTV)
	{
		health = pl->statsf[STAT_HEALTH];
		armour = pl->statsf[STAT_ARMOR];
		items = pl->stats[STAT_ITEMS];
		pname = pl->name;
		haveinfo = true;
	}
	else
#endif
	{
		health = pl->tinfo.health;
		armour = pl->tinfo.armour;
		items = pl->tinfo.items;
		pname = ((*pl->tinfo.nick)?pl->tinfo.nick:(cl.teamfortress?"-":pl->name));
		haveinfo = pl->tinfo.time > cl.time;
	}

	if (strcmp(pname, "-"))	//a tinfo nick of - hides names. this can be used for TF to hide names for spies.
	{
		y -= 8;
		len = COM_ParseFunString(textflags, pname, buffer, sizeof(buffer), false) - buffer;
		Draw_ExpandedString(x - len*4, y, buffer);
	}

	if (!haveinfo)
		return;	//we don't trust the info that we have, so no ids.

	h = 0;

	barwidth = 32;

	//display health bar
	if (scr_autoid_health.ival)
	{
		float frac = health / 100.0;
		if (frac < 0)
			frac = 0;
		r = frac;
		frac -= r;
		if (r > countof(healthcolours)-2)
		{
			r = countof(healthcolours)-2;
			frac = 1;
		}
		h += 8;
		y -= 8;
		R2D_ImageColours(healthcolours[r][0], healthcolours[r][1], healthcolours[r][2], healthcolours[r][3]*alpha);
		R2D_FillBlock(x - barwidth*0.5 + barwidth * frac, y, barwidth * (1-frac), 8);
		r++;
		R2D_ImageColours(healthcolours[r][0], healthcolours[r][1], healthcolours[r][2], healthcolours[r][3]*alpha);
		R2D_FillBlock(x - barwidth*0.5, y, barwidth * frac, 8);
	}

	if (health <= 0)	//armour+weapons are not relevant when dead
		return;

#ifdef QUAKESTATS
	if (scr_autoid_armour.ival)
	{
		//display armour bar above that
		if (items & IT_ARMOR3)
			r = 4, health = 200;
		else if (items & IT_ARMOR2)
			r = 2, health = 150;
		else if (items & IT_ARMOR1)
			r = 0, health = 100;
		else r = -1;
		if (r >= 0)
		{
			h += 8;
			y -= 8;
			armour = bound(0, armour, health);
			barwidth = 32;
			R2D_ImageColours(armourcolours[r][0], armourcolours[r][1], armourcolours[r][2], armourcolours[r][3]*alpha);
			R2D_FillBlock(x - barwidth*0.5 + barwidth * armour/(float)health, y, barwidth * (health-armour)/(float)health, 8);
			r++;
			R2D_ImageColours(armourcolours[r][0], armourcolours[r][1], armourcolours[r][2], armourcolours[r][3]*alpha);
			R2D_FillBlock(x - barwidth*0.5, y, barwidth * armour/(float)health, 8);
		}
	}

	if (scr_autoid_weapon.ival)
	{
		if (h < 8)
			h = 8;
		y += (h-8)/2;

		for (r = countof(wbitnames)-1; r>=0; r--)
			if (items & (1<<r))
				break;
		R2D_ImageColours(1, 1, 1, 1);
		if (r >= 0)
		{
			len = COM_ParseFunString(textflags, wbitnames[r]->string, buffer, sizeof(buffer), false) - buffer;
			if (textflags & CON_HALFALPHA)
			{
				for (r = 0; r < len; r++)
					if (!(buffer[r] & CON_RICHFORECOLOUR))
						buffer[r] |= CON_HALFALPHA;
			}
			if (len && (buffer[0] & CON_CHARMASK) == '{' && (buffer[len-1] & CON_CHARMASK) == '}')
			{	//these are often surrounded by {} to make them white in chat messages, and recoloured.
				buffer[len-1] = 0;
				Draw_ExpandedString(x + barwidth*0.5 + 4, y, buffer+1);
			}
			else
				Draw_ExpandedString(x + barwidth*0.5 + 4, y, buffer);
		}
	}
#else
	(void)items;
	(void)armour;
#endif
}

#include "pr_common.h"
extern vec3_t nametagorg[MAX_CLIENTS];
extern qboolean nametagseen[MAX_CLIENTS];
void R_DrawNameTags(void)
{
	int i;
	lerpents_t *le;
	qboolean isteam;
	char *ourteam;
	int ourcolour;

	extern cvar_t r_showfields, r_projection;

	if (r_projection.ival)	//we don't actually know how to transform the points unless the projection is coded in advance. and it isn't.
		return;
	if (cls.protocol == CP_QUAKE2)
		return;	//FIXME: q2 has its own ent logic, which messes stuff up here.

	if (r_showfields.ival && cls.allow_cheats)
	{
		world_t *w = NULL;
		wedict_t *e;
		vec3_t org;
		vec3_t screenspace;
		vec3_t diff;
		if ((r_showfields.ival & 3) == 1)
		{
#ifndef CLIENTONLY
			w = &sv.world;
#endif
		}
		else if ((r_showfields.ival & 3) == 2)
		{
#ifdef CSQC_DAT
			extern world_t csqc_world;
			w = &csqc_world;
#endif
		}
		else if ((r_showfields.ival & 3) == 3)
		{
			inframe_t *frame;
			packet_entities_t *pak;
			entity_state_t *state;
			model_t *mod;

			frame = &cl.inframes[cl.parsecount & UPDATE_MASK];
			pak = &frame->packet_entities;

			for (i=0 ; i<pak->num_entities ; i++)
			{
				state = &pak->entities[i];

				mod = cl.model_precache[state->modelindex];
				if (mod && mod->loadstate == MLS_LOADED)
					VectorInterpolate(mod->mins, 0.5, mod->maxs, org);
				else
					VectorClear(org);
				VectorAdd(org, state->origin, org);
				if (Matrix4x4_CM_Project(org, screenspace, r_refdef.viewangles, r_refdef.vieworg, r_refdef.fov_x, r_refdef.fov_y))
				{
					char *entstr;
					int x, y;

					entstr = va("%i", state->number);
					if (entstr)
					{
						vec2_t scale = {8,8};
						x = screenspace[0]*r_refdef.vrect.width+r_refdef.vrect.x;
						y = (1-screenspace[1])*r_refdef.vrect.height+r_refdef.vrect.y;
						R_DrawTextField(x, y, vid.width - x, vid.height - y, entstr, CON_WHITEMASK, CPRINT_TALIGN|CPRINT_LALIGN, font_default, scale);
					}

				}
			}
		}
		if (w && w->progs)
		{
			int best = 0;
			float bestscore = 0, score = 0;
			for (i = 1; i < w->num_edicts; i++)
			{
				e = WEDICT_NUM(w->progs, i);
				if (ED_ISFREE(e))
					continue;
				VectorInterpolate(e->v->mins, 0.5, e->v->maxs, org);
				VectorAdd(org, e->v->origin, org);
				VectorSubtract(org, r_refdef.vieworg, diff);
				if (DotProduct(diff, diff) < 16*16)
					continue;	//ignore stuff too close(like the player themselves)
				VectorNormalize(diff);
				score = DotProduct(diff, vpn);// r_refdef.viewaxis[0]);
				if (score > bestscore)
				{
					vec3_t imp;
					if (CL_TraceLine(r_refdef.vieworg, org, imp, NULL, NULL)>=1)
					{
						best = i;
						bestscore = score;
					}
				}
			}
			if (best)
			{
				e = WEDICT_NUM(w->progs, best);
				VectorInterpolate(e->v->mins, 0.5, e->v->maxs, org);
				VectorAdd(org, e->v->origin, org);
				if (Matrix4x4_CM_Project(org, screenspace, r_refdef.viewangles, r_refdef.vieworg, r_refdef.fov_x, r_refdef.fov_y))
				{
					char asciibuffer[8192];
					char *entstr;
					size_t buflen;
					int x, y;

					sprintf(asciibuffer, "entity %i ", e->entnum);
					buflen = strlen(asciibuffer);
					entstr = w->progs->saveent(w->progs, asciibuffer, &buflen, sizeof(asciibuffer), (edict_t*)e);	//will save just one entities vars
					if (entstr)
					{
						vec2_t scale = {8,8};
						x = screenspace[0]*r_refdef.vrect.width+r_refdef.vrect.x;
						y = (1-screenspace[1])*r_refdef.vrect.height+r_refdef.vrect.y;
						R_DrawTextField(x, y, vid.width - x, vid.height - y, entstr, CON_WHITEMASK, CPRINT_TALIGN|CPRINT_LALIGN, font_default, scale);
					}

				}
			}
		}
	}

	if (((!r_refdef.playerview->spectator && !cls.demoplayback) || !scr_autoid.ival) && (!cl.teamplay || !scr_autoid_team.ival))
		return;
	if (cls.state != ca_active || !cl.validsequence || cl.intermissionmode != IM_NONE)
		return;

	if (r_refdef.playerview->cam_state != CAM_FREECAM && r_refdef.playerview->cam_spec_track >= 0)
	{
		ourteam = cl.players[r_refdef.playerview->cam_spec_track].team;
		ourcolour = cl.players[r_refdef.playerview->cam_spec_track].rbottomcolor;
	}
	else
	{
		ourteam = cl.players[r_refdef.playerview->playernum].team;
		ourcolour = cl.players[r_refdef.playerview->playernum].rbottomcolor;
	}

	for (i = 0; i < cl.allocated_client_slots; i++)
	{
		if (!*cl.players[i].name)
			continue;	//slot is empty.

		if (!nametagseen[i])
		{
			if (i+1 < cl.maxlerpents && cl.lerpentssequence && cl.lerpents[i+1].sequence == cl.lerpentssequence)
			{
				le = &cl.lerpents[i+1];
				VectorCopy(le->origin, nametagorg[i]);
			}
			else if (cl.lerpplayers[i].sequence == cl.lerpentssequence)
			{
				le = &cl.lerpplayers[i];
				VectorCopy(le->origin, nametagorg[i]);
			}
			else if (cl.players[i].tinfo.time > cl.time)
				VectorCopy(cl.players[i].tinfo.org, nametagorg[i]);
			else
				continue;
		}

		//while cl.lerpplayers exists, it tends to not be configured properly.
		if (i == r_refdef.playerview->playernum)
			continue;	// Don't draw tag for the local player
		if (cl.players[i].spectator)
			continue;
		if (i == Cam_TrackNum(r_refdef.playerview))	//no tag for the player that you're tracking, either.
			continue;

		if (!cl.teamplay || !scr_autoid_team.ival)
			isteam = false;
		else if ((cl.teamfortress && !r_refdef.playerview->spectator) || cls.protocol == CP_NETQUAKE)	//teamfortress should go by their colours instead, because spies. primarily this is to allow enemy spies to appear through walls as well as your own team (note that the qc will also need tinfo stuff for tf, to avoid issues with just checking player names).
			isteam = cl.players[i].rbottomcolor == ourcolour;
		else
			isteam = !strcmp(cl.players[i].team, ourteam);

		if (!isteam)
			if ((!r_refdef.playerview->spectator && !cls.demoplayback) || !scr_autoid.ival)
				continue;	//only show our team when playing, too cheaty otherwise.

		SCR_DrawAutoID(nametagorg[i], &cl.players[i], isteam);
	}
}

void R2D_PolyBlend (void);
void V_RenderPlayerViews(playerview_t *pv)
{
	int oldnuments;
	int oldstris;
#ifdef SIDEVIEWS
	int viewnum;
#endif
//	cl.simangles[plnum][ROLL] = 0;	// FIXME @@@

	DropPunchAngle (pv);

	Cam_SelfTrack(pv);

	oldnuments = cl_numvisedicts;
	oldstris = cl_numstris;
	CL_LinkViewModel ();

	if (cl.intermissionmode != IM_NONE)
	{	// intermission / finale rendering
		V_CalcIntermissionRefdef (pv);
	}
	else
	{
		V_DriftPitch (pv);
		V_CalcRefdef (pv);
	}
	V_ApplyRefdef();

	R_RenderView ();
	R2D_PolyBlend ();
	R_DrawNameTags();

	if(cl.intermissionmode == IM_NONE)
		R2D_DrawCrosshair();

	cl_numvisedicts = oldnuments;
	cl_numstris = oldstris;

	if (scr_chatmode == 2)
	{
		vec3_t dir;

		r_refdef.vrect.y -= r_refdef.vrect.height;
		r_secondaryview = 2;

		VectorSubtract(r_refdef.vieworg, pv->cam_desired_position, dir);
		VectorAngles(dir, NULL, r_refdef.viewangles, false);


		VectorCopy(pv->cam_desired_position, r_refdef.vieworg);
		R_RenderView ();
	}
	r_secondaryview = true;


#ifdef SIDEVIEWS
/*	//adjust main view height to strip off the rearviews at the top
	if (vsecwidth >= 1)
	{
		r_refdef.vrect.y -= vsecheight;
		r_refdef.vrect.height += vsecheight;
	}
*/
	for (viewnum = 0; viewnum < SIDEVIEWS; viewnum++)
	if (vsec_scalex[viewnum].value>0&&vsec_scaley[viewnum].value>0
		&& ((vsec_enabled[viewnum].value && vsec_enabled[viewnum].value != 2) 	//rearview if v2_enabled = 1 and not 2
		|| (vsec_enabled[viewnum].value && pv->stats[STAT_VIEW2]&&viewnum==0)))			//v2 enabled if v2_enabled is non-zero
	{
		vrect_t oldrect;
		vec3_t oldangles;
		vec3_t oldposition;
//		int oldviewent;
		struct entity_s *e;
		float ofx;
		float ofy;

		if (vsec_x[viewnum].value < 0)
			vsec_x[viewnum].value = 0;
		if (vsec_y[viewnum].value < 0)
			vsec_y[viewnum].value = 0;

		if (vsec_scalex[viewnum].value+vsec_x[viewnum].value > 1)
			continue;
		if (vsec_scaley[viewnum].value+vsec_y[viewnum].value > 1)
			continue;

		oldrect = r_refdef.vrect;
		memcpy(oldangles, r_refdef.viewangles, sizeof(vec3_t));
		memcpy(oldposition, r_refdef.vieworg, sizeof(vec3_t));
		ofx = r_refdef.fov_x;
		ofy = r_refdef.fov_y;

		r_refdef.vrect.x += r_refdef.vrect.width*vsec_x[viewnum].value;
		r_refdef.vrect.y += r_refdef.vrect.height*vsec_y[viewnum].value;
		r_refdef.vrect.width *= vsec_scalex[viewnum].value;
		r_refdef.vrect.height *= vsec_scaley[viewnum].value;

		r_refdef.fov_x = 0;
		r_refdef.fov_y = 0;
		V_ApplyAFov(NULL);
#ifdef PEXT_VIEW2
			//secondary view entity.
		e=NULL;
		if (viewnum==0&&pv->stats[STAT_VIEW2])
		{
			e = CL_EntityNum (pv->stats[STAT_VIEW2]);
		}
		if (e)
		{
			memcpy(r_refdef.viewangles, e->angles, sizeof(vec3_t));
			memcpy(r_refdef.vieworg, e->origin, sizeof(vec3_t));
//			cl.viewentity = cl.viewentity2;

			r_refdef.vieworg[0]=r_refdef.vieworg[0];//*s+(1-s)*e->lerporigin[0];
			r_refdef.vieworg[1]=r_refdef.vieworg[1];//*s+(1-s)*e->lerporigin[1];
			r_refdef.vieworg[2]=r_refdef.vieworg[2];//*s+(1-s)*e->lerporigin[2];

			r_refdef.viewangles[0]=e->angles[0];//*s+(1-s)*e->msg_angles[1][0];
			r_refdef.viewangles[1]=e->angles[1];//*s+(1-s)*e->msg_angles[1][1];
			r_refdef.viewangles[2]=e->angles[2];//*s+(1-s)*e->msg_angles[1][2];

			if (e->keynum >= 1 && e->keynum <= cl.allocated_client_slots)
			{
				r_refdef.viewangles[PITCH] *= 3;
				r_refdef.vieworg[2] += pv->statsf[STAT_VIEWHEIGHT];
			}


			V_EditExternalModels(e->keynum, NULL, 0);

			R_RenderView ();
//				r_framecount = old_framecount;
		}
		else
#endif
		{
			//rotate the view, keeping pitch and roll.
			r_refdef.viewangles[YAW] += vsec_yaw[viewnum].value;
			r_refdef.viewangles[ROLL] += sin(vsec_yaw[viewnum].value / 180 * 3.14) * r_refdef.viewangles[PITCH];
			r_refdef.viewangles[PITCH] *= -cos((vsec_yaw[viewnum].value / 180 * 3.14)+3.14);
			if (vsec_enabled[viewnum].value!=2)
			{
				V_EditExternalModels(pv->viewentity, NULL, 0);
				R_RenderView ();
			}
		}

		r_refdef.vrect = oldrect;
		memcpy(r_refdef.viewangles, oldangles, sizeof(vec3_t));
		memcpy(r_refdef.vieworg, oldposition, sizeof(vec3_t));
		r_refdef.fov_x = ofx;
		r_refdef.fov_y = ofy;
	}
#endif
	r_refdef.externalview = false;
}

void V_RenderView (void)
{
	int viewnum;
	int maxviews = cl.splitclients;

	Surf_LessenStains();

	if (cls.state != ca_active)
		return;

	if (cl.intermissionmode != IM_NONE)
		maxviews = 1;

	R_PushDlights ();

	r_secondaryview = 0;
	for (viewnum = 0; viewnum < maxviews; viewnum++)
	{
		V_ClearRefdef(&cl.playerview[viewnum]);
		if (viewnum)
		{
			//should be enough to just hack a few things.
			V_EditExternalModels(cl.playerview[viewnum].viewentity, NULL, 0);
		}
		else
		{
			if (r_worldentity.model && r_worldentity.model->loadstate == MLS_LOADED)
			{
				RSpeedMark();

				CL_AllowIndependantSendCmd(false);

				CL_SetSolidEntities ();
				CL_TransitionEntities();
				CL_PredictMove ();

				// build a refresh entity list
				CL_EmitEntities ();

				CL_AllowIndependantSendCmd(true);

				RSpeedEnd(RSPEED_LINKENTITIES);
			}
		}
		if (R2D_Flush)
			R2D_Flush();
		SCR_VRectForPlayer(&r_refdef.grect, viewnum, maxviews);
		V_RenderPlayerViews(r_refdef.playerview);

#ifdef PLUGINS
		Plug_SBar (r_refdef.playerview);
#else
		if (Sbar_ShouldDraw(r_refdef.playerview))
		{
			SCR_TileClear (sb_lines);
			Sbar_Draw (r_refdef.playerview);
			Sbar_DrawScoreboard (r_refdef.playerview);
		}
		else
			SCR_TileClear (0);
#endif
	}
	r_refdef.playerview = NULL;
}

//============================================================================

/*
=============
V_Init
=============
*/
void V_Init (void)
{
#define VIEWVARS "View variables"
#ifdef SIDEVIEWS
	int i;
#endif
	Cmd_AddCommand ("v_cshift", V_cshift_f);
	Cmd_AddCommand ("bf", V_BonusFlash_f);
	Cmd_AddCommand ("df", V_DarkFlash_f);
	Cmd_AddCommand ("wf", V_WhiteFlash_f);
//	Cmd_AddCommand ("centerview", V_StartPitchDrift);

	Cvar_Register (&v_centermove, VIEWVARS);
	Cvar_Register (&v_centerspeed, VIEWVARS);

	Cvar_Register (&v_idlescale, VIEWVARS);
	Cvar_Register (&v_iyaw_cycle, VIEWVARS);
	Cvar_Register (&v_iroll_cycle, VIEWVARS);
	Cvar_Register (&v_ipitch_cycle, VIEWVARS);
	Cvar_Register (&v_iyaw_level, VIEWVARS);
	Cvar_Register (&v_iroll_level, VIEWVARS);
	Cvar_Register (&v_ipitch_level, VIEWVARS);

	Cvar_Register (&v_contentblend, VIEWVARS);
	Cvar_Register (&v_damagecshift, VIEWVARS);
	Cvar_Register (&v_quadcshift, VIEWVARS);
	Cvar_Register (&v_suitcshift, VIEWVARS);
	Cvar_Register (&v_ringcshift, VIEWVARS);
	Cvar_Register (&v_pentcshift, VIEWVARS);
	Cvar_Register (&v_gunkick, VIEWVARS);
	Cvar_Register (&v_gunkick_q2, VIEWVARS);

	Cvar_Register (&v_bonusflash, VIEWVARS);

	Cvar_Register (&v_viewheight, VIEWVARS);
	Cvar_Register (&v_depthsortentities, VIEWVARS);

	Cvar_Register (&crosshaircolor, VIEWVARS);
	Cvar_Register (&crosshair, VIEWVARS);
	Cvar_Register (&crosshairsize, VIEWVARS);
	Cvar_Register (&crosshaircorrect, VIEWVARS);
	Cvar_Register (&crosshairimage, VIEWVARS);
	Cvar_Register (&crosshairalpha, VIEWVARS);
	Cvar_Register (&cl_crossx, VIEWVARS);
	Cvar_Register (&cl_crossy, VIEWVARS);
	Cvar_Register (&gl_cshiftpercent, VIEWVARS);
	Cvar_Register (&gl_cshiftenabled, VIEWVARS);

	Cvar_Register (&cl_rollspeed, VIEWVARS);
	Cvar_Register (&cl_rollangle, VIEWVARS);
	Cvar_Register (&cl_bob, VIEWVARS);
	Cvar_Register (&cl_bobcycle, VIEWVARS);
	Cvar_Register (&cl_bobup, VIEWVARS);

	Cvar_Register (&v_kicktime, VIEWVARS);
	Cvar_Register (&v_kickroll, VIEWVARS);
	Cvar_Register (&v_kickpitch, VIEWVARS);

	Cvar_Register (&v_deathtilt, VIEWVARS);

	Cvar_Register (&scr_autoid, VIEWVARS);
	Cvar_Register (&scr_autoid_team, VIEWVARS);
	Cvar_Register (&scr_autoid_health, VIEWVARS);
	Cvar_Register (&scr_autoid_armour, VIEWVARS);
	Cvar_Register (&scr_autoid_weapon, VIEWVARS);
	Cvar_Register (&scr_autoid_teamcolour, VIEWVARS);
	Cvar_Register (&scr_autoid_enemycolour, VIEWVARS);

#ifdef SIDEVIEWS
#define SECONDARYVIEWVARS "Secondary view vars"
	for (i = 0; i < SIDEVIEWS; i++)
	{
		Cvar_Register (&vsec_enabled[i], SECONDARYVIEWVARS);
		Cvar_Register (&vsec_x[i], SECONDARYVIEWVARS);
		Cvar_Register (&vsec_y[i], SECONDARYVIEWVARS);
		Cvar_Register (&vsec_scalex[i], SECONDARYVIEWVARS);
		Cvar_Register (&vsec_scaley[i], SECONDARYVIEWVARS);
		Cvar_Register (&vsec_yaw[i], SECONDARYVIEWVARS);
	}
#endif

	Cvar_Register (&ffov, VIEWVARS);
	Cvar_Register (&r_projection, VIEWVARS);

	BuildGammaTable (1.0, 1.0, 0.0);	// no gamma yet
	Cvar_Register (&v_gamma, VIEWVARS);
	Cvar_Register (&v_contrast, VIEWVARS);
	Cvar_Register (&v_brightness, VIEWVARS);

	Cvar_Register (&chase_active, VIEWVARS);
	Cvar_Register (&chase_back, VIEWVARS);
	Cvar_Register (&chase_up, VIEWVARS);

#if defined(_WIN32) && !defined(MINIMAL)
	Cvar_Register (&itburnsitburnsmakeitstop, VIEWVARS);
#endif
}
