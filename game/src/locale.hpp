#ifndef __INC_METIN2_GAME_LOCALE_H__
#define __INC_METIN2_GAME_LOCALE_H__
#include "../../common/CommonDefines.h"

extern "C"
{
#ifdef __LANGUAGE_SYSTEM__
	void locale_init(const char *filename, BYTE lang = 0); 
#else
	void locale_init(const char *filename);
#endif

#ifdef __LANGUAGE_SYSTEM__
	const char *locale_findDefault(const char *string);
	const char *locale_findLang1(const char *string);
	const char *locale_findLang2(const char *string);
	const char *locale_findLang3(const char *string);
	const char *locale_findLang4(const char *string);
#endif
	const char *locale_find(const char *string);

	extern int g_iUseLocale;

#define LC_TEXT(str) locale_find(str)
};

#endif
