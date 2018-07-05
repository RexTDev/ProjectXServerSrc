#include "stdafx.h"
#include "start_position.h"


char g_nation_name[4][32] =
{
	"",
	"신수국",
	"천조국",
	"진노국",
};

//	LC_TEXT("신수국")
//	LC_TEXT("천조국")
//	LC_TEXT("진노국")

long g_start_map[4] =
{
	0,	// reserved
	1,	// Rotes Reich	(신수국)
	21,	// Gelbes Reich	(천조국)
	41	// Blaues Reich	(진노국)
};

 // Stadtport
DWORD g_start_position[4][2] =
{
	{      0,      0 },	
	// reserviert
	{ 469300, 964200 },		// Rotes Reich
	{  55700, 157900 },		// Gelbes Reich
	{ 969600, 278400 }		// Blaues Reich
};

// Gildenkriegwarp
DWORD arena_return_position[4][2] =
{
	{       0,  0       },
	{   347600, 882700  }, // 자양현
	{   138600, 236600  }, // 복정현
	{   857200, 251800  }  // 박라현
};

// Charaktererstellung
#ifdef ENABLE_NEW_RETARDED_GF_START_POSITION
DWORD g_create_position[4][2] =
{
	{		0,		0 },	// reserviert

	{ 474800, 966000 },		// Rotes Reich
	{ 60000, 155700 },		// Gelbes Reich
	{ 963700, 278400 },		// Blaues Reich
};
#else
DWORD g_create_position[4][2] =
{
	{		0,		0 },	// reserviert
	{ 459800, 953900 },		// Rotes Reich
	{ 52070, 166600 },		// Gelbes Reich
	{ 957300, 255200 },		// Blaues Reich
};
#endif

DWORD g_create_position_canada[4][2] =
{
	{		0,		0 },
	{ 457100, 946900 },
	{ 45700, 166500 },
	{ 966300, 288300 },
};

