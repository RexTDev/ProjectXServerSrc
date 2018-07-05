#include "stdafx.h"
#include "locale_service.h"

#ifdef __LANGUAGE_SYSTEM__
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "desc_client.h"
#include "log.h"
#include "utils.h"
#endif

typedef std::map< std::string, std::string > LocaleStringMapType;

#ifndef __LANGUAGE_SYSTEM__
LocaleStringMapType localeString;
#endif

int g_iUseLocale = 0;

#ifdef __LANGUAGE_SYSTEM__
LocaleStringMapType localeStringDefault;
LocaleStringMapType localeStringLang1;
LocaleStringMapType localeStringLang2;
LocaleStringMapType localeStringLang3;
LocaleStringMapType localeStringLang4;

void locale_add(const char **strings)
{
	LocaleStringMapType::const_iterator iter = localeStringDefault.find( strings[0] );

	if( iter == localeStringDefault.end() )
	{
		localeStringDefault.insert( std::make_pair( strings[0], strings[1] ) );
	}
}

void locale_addLang1(const char **strings)
{
	LocaleStringMapType::const_iterator iter = localeStringLang1.find( strings[0] );

	if( iter == localeStringLang1.end() )
	{
		localeStringLang1.insert( std::make_pair( strings[0], strings[1] ) );
	}
}

void locale_addLang2(const char **strings)
{
	LocaleStringMapType::const_iterator iter = localeStringLang2.find( strings[0] );

	if( iter == localeStringLang2.end() )
	{
		localeStringLang2.insert( std::make_pair( strings[0], strings[1] ) );
	}
}

void locale_addLang3(const char **strings)
{
	LocaleStringMapType::const_iterator iter = localeStringLang3.find( strings[0] );

	if( iter == localeStringLang3.end() )
	{
		localeStringLang3.insert( std::make_pair( strings[0], strings[1] ) );
	}
}

void locale_addLang4(const char **strings)
{
	LocaleStringMapType::const_iterator iter = localeStringLang4.find( strings[0] );

	if( iter == localeStringLang4.end() )
	{
		localeStringLang4.insert( std::make_pair( strings[0], strings[1] ) );
	}
}

const char * locale_find(const char *string)
{
	const char* s_string;
	LPCHARACTER ch = quest::CQuestManager::instance().GetCurrentCharacterPtr();
	if(ch && ch->GetLanguage() >= 1 && ch->GetLanguage() <= 4)
	{
		switch(ch->GetLanguage())
		{
			case 0:
				s_string = locale_findDefault(string);
				break;
			case 1:
				s_string = locale_findLang1(string);
				break;
			case 2:
				s_string = locale_findLang2(string);
				break;
			case 3:
				s_string = locale_findLang3(string);
				break;
			case 4:
				s_string = locale_findLang4(string);
				break;
			default:
				s_string = locale_findDefault(string);
				break;
		}
	}
	else
	{
		s_string = locale_findDefault(string);
	}
	return s_string;
}

const char * locale_findDefault(const char *string)
{
	if (0 == g_iUseLocale || LC_IsKorea() || LC_IsWE_Korea())
	{
		return (string);
	}

	LocaleStringMapType::const_iterator iter = localeStringDefault.find( string );

	if( iter == localeStringDefault.end() )
	{
		static char s_line[1024] = "@0949";
		strlcpy(s_line + 5, string, sizeof(s_line) - 5);

		sys_err("LOCALE_ERROR: \"%s\";", string);
		return s_line;
	}

	return iter->second.c_str();
}

const char * locale_findLang1(const char *string)
{
	if (0 == g_iUseLocale || LC_IsKorea() || LC_IsWE_Korea())
	{
		return (string);
	}

	LocaleStringMapType::const_iterator iter = localeStringLang1.find( string );

	if( iter == localeStringLang1.end() )
	{
		static char s_line[1024] = "@0949";
		strlcpy(s_line + 5, string, sizeof(s_line) - 5);

		sys_err("LOCALE_ERROR: \"%s\";", string);
		return s_line;
	}

	return iter->second.c_str();
}

const char * locale_findLang2(const char *string)
{
	if (0 == g_iUseLocale || LC_IsKorea() || LC_IsWE_Korea())
	{
		return (string);
	}

	LocaleStringMapType::const_iterator iter = localeStringLang2.find( string );

	if( iter == localeStringLang2.end() )
	{
		static char s_line[1024] = "@0949";
		strlcpy(s_line + 5, string, sizeof(s_line) - 5);

		sys_err("LOCALE_ERROR: \"%s\";", string);
		return s_line;
	}

	return iter->second.c_str();
}

const char * locale_findLang3(const char *string)
{
	if (0 == g_iUseLocale || LC_IsKorea() || LC_IsWE_Korea())
	{
		return (string);
	}

	LocaleStringMapType::const_iterator iter = localeStringLang3.find( string );

	if( iter == localeStringLang3.end() )
	{
		static char s_line[1024] = "@0949";
		strlcpy(s_line + 5, string, sizeof(s_line) - 5);

		sys_err("LOCALE_ERROR: \"%s\";", string);
		return s_line;
	}

	return iter->second.c_str();
}

const char * locale_findLang4(const char *string)
{
	if (0 == g_iUseLocale || LC_IsKorea() || LC_IsWE_Korea())
	{
		return (string);
	}

	LocaleStringMapType::const_iterator iter = localeStringLang4.find( string );

	if( iter == localeStringLang4.end() )
	{
		static char s_line[1024] = "@0949";
		strlcpy(s_line + 5, string, sizeof(s_line) - 5);

		sys_err("LOCALE_ERROR: \"%s\";", string);
		return s_line;
	}

	return iter->second.c_str();
}

#else
void locale_add(const char **strings)
{
	LocaleStringMapType::const_iterator iter = localeString.find( strings[0] );

	if( iter == localeString.end() )
	{
		localeString.insert( std::make_pair( strings[0], strings[1] ) );
	}
}

const char * locale_find(const char *string)
{
	if (0 == g_iUseLocale || LC_IsKorea() || LC_IsWE_Korea())
	{
		return (string);
	}

	LocaleStringMapType::const_iterator iter = localeString.find( string );

	if( iter == localeString.end() )
	{
		static char s_line[1024] = "@0949";
		strlcpy(s_line + 5, string, sizeof(s_line) - 5);

		sys_err("LOCALE_ERROR: \"%s\";", string);
		return s_line;
	}

	return iter->second.c_str();
}
#endif

const char *quote_find_end(const char *string)
{
	const char  *tmp = string;
	int         quote = 0;

	while (*tmp)
	{
		if (quote && *tmp == '\\' && *(tmp + 1))
		{
			// \ 다음 문자가 " 면 스킵한다.
			switch (*(tmp + 1))
			{
				case '"':
					tmp += 2;
					continue;
			}
		}
		else if (*tmp == '"')
		{
			quote = !quote;
		}
		else if (!quote && *tmp == ';')
			return (tmp);

		tmp++;
	}

	return (NULL);
}

char *locale_convert(const char *src, int len)
{
	const char	*tmp;
	int		i, j;
	char	*buf, *dest;
	int		start = 0;
	char	last_char = 0;

	if (!len)
		return NULL;

	buf = M2_NEW char[len + 1];

	for (j = i = 0, tmp = src, dest = buf; i < len; i++, tmp++)
	{
		if (*tmp == '"')
		{
			if (last_char != '\\')
				start = !start;
			else
				goto ENCODE;
		}
		else if (*tmp == ';')
		{
			if (last_char != '\\' && !start)
				break;
			else
				goto ENCODE;
		}
		else if (start)
		{
ENCODE:
			if (*tmp == '\\' && *(tmp + 1) == 'n')
			{
				*(dest++) = '\n';
				tmp++;
				last_char = '\n';
			}
			else
			{
				*(dest++) = *tmp;
				last_char = *tmp;
			}

			j++;
		}
	}

	if (!j)
	{
		M2_DELETE_ARRAY(buf);
		return NULL;
	}

	*dest = '\0';
	return (buf);
}

#define NUM_LOCALES 2
#ifdef __LANGUAGE_SYSTEM__
void locale_init(const char *filename, BYTE lang)
#else
void locale_init(const char *filename)
#endif
{
	FILE        *fp = fopen(filename, "rb");
	char        *buf;

	if (!fp) return;

	fseek(fp, 0L, SEEK_END);
	int i = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	i++;

	buf = M2_NEW char[i];

	memset(buf, 0, i);

	fread(buf, i - 1, sizeof(char), fp);

	fclose(fp);

	const char * tmp;
	const char * end;

	char *	strings[NUM_LOCALES];

	if (!buf)
	{
		sys_err("locale_read: no file %s", filename);
		exit(1);
	}

	tmp = buf;

	do
	{
		for (i = 0; i < NUM_LOCALES; i++)
			strings[i] = NULL;

		if (*tmp == '"')
		{
			for (i = 0; i < NUM_LOCALES; i++)
			{
				if (!(end = quote_find_end(tmp)))
					break;

				strings[i] = locale_convert(tmp, end - tmp);
				tmp = ++end;

				while (*tmp == '\n' || *tmp == '\r' || *tmp == ' ') tmp++;

				if (i + 1 == NUM_LOCALES)
					break;

				if (*tmp != '"')
				{
					sys_err("locale_init: invalid format filename %s", filename);
					break;
				}
			}

			if (strings[0] == NULL || strings[1] == NULL)
				break;
#ifdef __LANGUAGE_SYSTEM__
			switch(lang)
			{
				case 0:
					locale_add((const char**)strings);
					break;
				case 1:
					locale_addLang1((const char**)strings);
					break;
				case 2:
					locale_addLang2((const char**)strings);
					break;
				case 3:
					locale_addLang3((const char**)strings);
					break;
				case 4:
					locale_addLang4((const char**)strings);
					break;
					
			}
#else
			locale_add((const char**)strings);
#endif
			for (i = 0; i < NUM_LOCALES; i++)
				if (strings[i])
					M2_DELETE_ARRAY(strings[i]);
		}
		else
		{
			tmp = strchr(tmp, '\n');

			if (tmp)
				tmp++;
		}
	}
	while (tmp && *tmp);

	M2_DELETE_ARRAY(buf);
}
