#ifndef __INC_METIN2_COMMON_DEFINES_H__
#define __INC_METIN2_COMMON_DEFINES_H__

enum eCommonDefines
{
	MAP_ALLOW_LIMIT = 32,					// 32 default
};


//-----SECURITY----->
#define ENABLE_PORT_SECURITY				// Blockiert db_port, p2p_port und remote adminpage exploits
// #define ENABLE_HARDWARE_ID_BANN
// #define ENABLE_CHANGE_PACKET_LOGIN			// Ändert den Login Packet




//-----CHARACTERS----->
#define ENABLE_PLAYER_PER_ACCOUNT5			// Erlaubt 5 Character

#define ENABLE_WOLFMAN_CHARACTER			// Wolfman Character
#ifdef ENABLE_WOLFMAN_CHARACTER
	#define USE_MOB_BLEEDING_AS_POISON				// Bluten wird deaktiviert und ist wie Gift
	#define USE_MOB_CLAW_AS_DAGGER					// Krallen wird deaktiviert und ist wie Dolche
	#define USE_ITEM_BLEEDING_AS_POISON				// Giftbonus kann bluten deffen (bluten wird deaktiviert)
	#define USE_ITEM_CLAW_AS_DAGGER					// Wiederstand gegen Dolche bonus kann Krallen deffen (Krallen wird deaktiviert)
	#define USE_WOLFMAN_STONES						// Wolfmenschsteine können bei Metins gedroppt werden
	#define USE_WOLFMAN_BOOKS						// Wolfmensch Skillbücher können gedropt werden
	#define USE_LYCAN_CREATE_POSITION				// if enabled, the lycan will be warped to his own village at character creation
	#define ENABLE_WOLFMAN_CHARACTER_78				// aktiviert 7&8 Skill für Wolfmenschen
	#define ENABLE_WOLFMAN_CHARACTER_ITEM_EFFECT	// 
#endif

// #define ENABLE_KEYTO_CHARACTER				// Keyto/Elfe Character
#ifdef ENABLE_KEYTO_CHARACTER
	#define ENABLE_KEYTO_CHARACTER_EFFEKT			// Skill Effekt (Red-Shield)
	#define USE_MOB_POLE_AS_BELL					// Stange wird deaktiviert und ist wie Glocke
	#define USE_ITEM_POLE_AS_BELL					// Wiederstand gegen Glocken bonus kann Stange deffen (Stange wird deaktiviert)
	#define USE_ELFE_STONES							// Elfensteine können bei Metins gedroppt werden
	#define USE_ELFE_BOOKS							// Elfen Skillbücher können gedropt werden
//	#define ENABLE_KEYTO_CHARACTER_78				// aktiviert 7&8 Skill für Elfen
//	#define ENABLE_KEYTO_CHARACTER_ITEM_EFFECT		// 
#endif

//#define ENABLE_BERSERKER_CHARACTER		// Berserk Character
#ifdef ENABLE_BERSERKER_CHARACTER
	//reserviert
#endif





//-----SYSTEMS----->
#define ENABLE_SHOW_MOB_INFO				// Aktiviert Moob,NPC,Stone LVL-Anzeige
#define ENABLE_ACCE_COSTUME_SYSTEM			// Sash mit Item-Scale
#define ENABLE_SOULBIND_SYSTEM				// Seelenbindungsystem
#define ENABLE_WEAPON_COSTUME_SYSTEM		// Waffenkostümsystem
#define ENABLE_ENCHANT_BONUS_ADD			// Costume-Bonus Add&Change
#define ENABLE_MOUNT_COSTUME_SYSTEM			// Mount Slot
#define ENABLE_OKAY_CARD					// Okay-Card-Event
// #define USE_CHANGE_ATTRIBUTE_PLUS			// Gegenstand verzaubern +
// #define USE_CHANGE_ATTRIBUTE_MINUS			// Gegenstand verzaubern -
#define ENABLE_7AND8TH_SKILLS				// 7&8 Skill-System
#define ENABLE_SHOP_EX
// #define ENABLE_ITEM_EFFECT_SYSTEM			// Item Effekt System
// #define __SKILLS_LEVEL_OVER_P__				// P1-S Skill
#define ENABLE_BOOK_COMBINATION
#define ENABLE_TITLE_SYSTEM					// Vegas Titel-System
#define __CHANGE_LOOK_SYSTEM__
#define ENABLE_VIP_SYSTEM

#define FULL_YANG							// Max-Yang
// #define ENABLE_CHEQUE_SYSTEM				// WON

#define ENABLE_MAGIC_REDUCTION_SYSTEM		// Anti-Magic bonus
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
	#define USE_MAGIC_REDUCTION_STONES				// Anti-Magic Steine können bei Metins gedroppt werden
#endif

#define NEW_PET_SYSTEM						// Pet-System
#ifdef NEW_PET_SYSTEM
//	#define DISABLE_TRADE_UNSUMMON					// this disable the unsummon of pet when a excange/trade/shop/myshop/safebox windows is open,
													// MAKE SURE to have set the items with vnum 55401/55402/55403/55404 with antiflag ANTI_SAFEBOX | ANTI_PKDROP | ANTI_DROP | ANTI_SELL | ANTI_GIVE | ANTI_STACK | ANTI_MYSHOP
#endif

#define __LANGUAGE_SYSTEM__
#ifdef __LANGUAGE_SYSTEM__
	#include <sstream>

	#define SSTR( x ) static_cast< std::ostringstream & >( \
		( std::ostringstream() << std::dec << x ) ).str()
#endif

#define ENABLE_OFFLINE_SHOP					// Aktiviert Offline-Shop-System!
#ifdef ENABLE_OFFLINE_SHOP

	#define SHOP_SEARCH						// Shop Search
	#ifdef SHOP_SEARCH
		#define SHOP_SEARCH_PRICE_MIN				//Price in gui is a minimum eq. price >= item_price if is commented price in gui is a maximum
	#endif

	#ifdef ENABLE_VIP_SYSTEM
		// #define ENABLE_OFFLINESHOP_ONLY_FOR_VIP		// Offline-Shop is only useable if you're VIP
	#endif

	#define OFFLINE_SHOP					// Offline shops system
	#define GIFT_SYSTEM								// Geschenkesystem (GIFT ICON)
	#define SHOP_TIME_REFRESH 1*60					// Überprüfungszyklus der älteren Geschäfte
	#define SHOP_BLOCK_GAME99						// Verhindert shops erstellen auf Channel 99
//	#define SHOP_DISTANCE							// Zeigt Shops Distance like WoM2
	#define SHOP_AUTO_CLOSE							// Auto-close des Shops nach letztem Itemverkauf
//	#define SHOP_ONLY_ALLOWED_INDEX					// Enable limiting for other map index which is not defined in player.shop_limit
//	#define SHOP_HIDE_NAME							// Shopnamen verheimlichen -> like "Player's shop"
	#define SHOP_GM_PRIVILEGES GM_PLAYER		// Minimum GM Rechte für Shop GM Panel
#endif

#define __DUNGEON_FOR_GUILD__
#ifdef __DUNGEON_FOR_GUILD__
	#define __MELEY_LAIR_DUNGEON__
	#ifdef __MELEY_LAIR_DUNGEON__
		#define __DESTROY_INFINITE_STATUES_GM__
		#define __LASER_EFFECT_ON_75HP__
		#define __LASER_EFFECT_ON_50HP__
	#endif
#endif




//-----NEW OPTIONS REXTDEV----->
#define ENABLE_ADD_BONUS_100_PERCENT
#ifdef ENABLE_ADD_5_BONUS_WITH_NORMAL_ADDER
	#define ENABLE_ADD_5_BONUS_AT_ONCE
#endif
#define ENABLE_ADD_5_BONUS_WITH_NORMAL_ADDER



//-----OPTIONS----->
#define ENABLE_EXTEND_INVEN_SYSTEM			// Aktiviert 4 Pages / deaktiviert 2 Pages
// #define ENABLE_BIGGER_SHOPS				// Erweitert Shopgröße um das doppelte; Default: 40
#define __HIGHLIGHT_SYSTEM__				// Inventar Drop Effekt
#define ENABLE_DICE_SYSTEM					// Randomdrop in einer Gruppe bei Bossen
#define ENABLE_SELLING_DRAGONSOUL			// Erlaubt das verkaufen von Dragonsoul Items
#define ENABLE_DUELL_BLOCK_SYSTEM			// Duellanfragen Ingame Blocken
#define ENABLE_EXTENDED_EQUIP_VIEW			// EQUIPMENT VIEWER
#define ENABLE_DROP_DIALOG					// Item Drop Dialog
#define ENABLE_GM_ONLINE_LIST				// GM-Online in FL
#define ENABLE_FIX_BUFF_WO_SWORD			// VZK, AURA Effekt Bug ohne Waffe
#define ENABLE_MONARCH_AFFECT				// Monarch KING Zeichen

#define ENABLE_CHANNEL_SWITCHER				// Ingame-Channel-Switcher
#define ENABLE_BLOCK_EXP					// Ingame-Exp-Blocker
// #define ENABLE_ANNOUNCEMENT_REFINE_SUCCES	// 
#define ENABLE_ANNOUNCEMENT_LEVELUP			// Chat-Level-Anzeige bei Levelup in 10er Steps
#define ENABLE_GLOBAL_AUCTION_HOUSE			// Nicht implementiert
#define ENABLE_QUIVER_SYSTEM				// Arraw System
// #define ENABLE_GUILD_WAR_EXTENSION			// Mehrere Optionen beim Gildenkrieg
#define __SEND_TARGET_INFO__				// Mob-Drop Info @ Monsterleiste
#define ENABLE_WEARED_ITEM_SAFEBOX_BLOCK	// Blockiert das man ausgerüstete Gegenstände ins Lager geben kann

#define ENABLE_ONLINE_COMMAND				// Online Info
#ifdef ENABLE_ONLINE_COMMAND
	#define ENABLE_ONLINE_GM_COMMAND					// Wenn aktiv - nur GMs können "/online" benutzen
//	#define ENABLE_FAKE_ONLINE_COUNT					// Fake Online Spieler Anzahl
#endif

#define ENABLE_PLAYERS_SET_STATES_WITH_GM_COMMAND
#ifdef ENABLE_PLAYERS_SET_STATES_WITH_GM_COMMAND
	#define ENABLE_STATES_VALUE_CHANGE				//con+ -> ht
#endif

#define ENABLE_NEW_EFFECT					// Aktiviert neuen Clienteffekt
#ifdef ENABLE_NEW_EFFECT
	#define ENABLE_NEW_RING_EFFECT						// Ring Effekt
	#define ENABLE_NEW_WS_EFFECT						// Windschuhe Effekt
	#define ENABLE_NEW_HM_EFFECT						// Helden Medaille Effekt
	#define ENABLE_NEW_CHOC_EFFECT						// Chocolate Effekt
	#define ENABLE_EMOTION_MASK_EFFECT					// Emotions Maske Effekt

	#define ENABLE_NEW_AFFECT_POTION					// Tau-Effekte
#endif

#define ENABLE_TELEPORT_COMMAND				// /teleport Command System
#ifdef ENABLE_TELEPORT_COMMAND
	#define NUMBER_MAX_MAPS 13							// Wie viele Maps gelistet sind
	#define ENABLE_TRANSLATE_LC_GERMANY					// Sprachen: ENABLE_TRANSLATE_LC_ENGLISH, ENABLE_TRANSLATE_LC_GERMANY, ENABLE_TRANSLATE_LC_SPAIN, ENABLE_TRANSLATE_LC_POLAND
	#define ENABLE_TELEPORT_COMMAND_ONLY_AS_GAMEMASTER	// Aktiv: Nur GMs können das Teleportsystem nutzen
#endif

// #define ENABLE_BLOCKING_GM_FUNCTIONS		// Deaktiviert GM Funktionen
#ifdef ENABLE_BLOCKING_GM_FUNCTIONS
	#define GM_BLOCK_BUFF_SKILL							// GM's können keine Spieler buffen
	#define GM_BLOCK_PARTY_INVITE						// GM's & Spieler können keine Gruppe gegenseitig öffnen
	#define GM_BLOCK_SHOP_CREATION						// GM's können keinen Shop erstellen
	#define GM_BLOCK_SHOP_CREATION_SEIDENBUENDEL		// GM's können keinen Shop erstellen (Seidenbündel)
	#define GM_BLOCK_ITEM_DROP							// GM's können keine Items droppen
	#define PLAYER_BLOCK_SET_COMMAND					// /set Command blockiert
	#define PLAYER_BLOCK_A_COMMAND						// /a Command blockiert
	#define PLAYER_BLOCK_SETSKILLOTHER_COMMAND			// /setskillother Command blockiert
	#define GM_BLOCK_EXCHANGE							// GM's & Spieler können gegenseitig Items handeln
	#define GM_BLOCK_GUILD_INVITE						// GM's & Spieler können sich nicht gegenseitig in eine Gilde einladen
	#define GM_BLOCK_FRIEND_LIST						// GM's können keine Spieler in die Freundesliste hinzufügen
	#define GM_BLOCK_BUY_PRIVATE_SHOP					// GM's können keine Items von Privaten Shops kaufen
	#define GM_BLOCK_SAFEBOX							// GM's können das Lager nicht benutzen
	#define PLAYER_BLOCK_POLYMORPHED_ATTACK				// Spieler können keinen anderen Spieler in Verwandlung angreifen
#endif





//-----OTHERS----->


/*<-----DB----->*/
#define ENABLE_DEFAULT_PRIV					//	[ClientManager.cpp]			enable default priv loading from common.priv_settings
// #define ENABLE_ITEMAWARD_REFRESH			//	[ClientManager.cpp]			enable a select query every 5 seconds into player.item_award
#define ENABLE_PROTO_FROM_DB				//	[ClientManager.h]			read protos from db if "PROTO_FROM_DB = 1" is specified inside conf.txt
											//								mirror protos to db if "MIRROR2DB = 1" is specified inside conf.txt
#define ENABLE_AUTODETECT_VNUMRANGE			//	[ClientManagerBoot.cpp]		if protos are loaded from db, it will automatically detect the vnum range for ds items


/*<-----GAME----->*/
#define NEW_ICEDAMAGE_SYSTEM				//	[char.h]					add new system for nemere dungeon and so on
#define ENABLE_ANTI_CMD_FLOOD				//	[char.h]					limit player's command execution to 10 commands per second, otherwise it'll be disconnected!
#define ENABLE_OPEN_SHOP_WITH_ARMOR			//	[char.h]					if enabled, people can open a personal shop with the armor equipped
// #define ENABLE_POTIONS_LEVELUP			//	[char.cpp]					you'll get Potions if you get a levelup
#define ENABLE_GOHOME_IF_MAP_NOT_ALLOWED	//	[char.cpp]					you'll go back to your village if you're not allowed to go in that map
#define ENABLE_GM_FLAG_IF_TEST_SERVER		//	[char.cpp]					show the gm flag if it's on test server mode
#define ENABLE_GM_FLAG_FOR_LOW_WIZARD		//	[char.cpp]					GM Zeichen auch als Low-Wizard

#define ENABLE_NEWEXP_CALCULATION			//	[char_battle.cpp]			recalculate exp rate so you won't get random negative exp/marriage points
#define ENABLE_EFFECT_PENETRATE				//	[char_battle.cpp]			enable penetrate effect when performing a penetration
// #define ENABLE_FIREWORK_STUN				//	[char_item.cpp]				enable stun affect when using firework items
#define ENABLE_ADDSTONE_FAILURE				//	[char_item.cpp]				enable add stone failure
#define ENABLE_EFFECT_EXTRAPOT				//	[char_item.cpp]				enable extrapot effects when using green/purple potions
#define ENABLE_BOOKS_STACKFIX				//	[char_item.cpp]				enable stackable books instead of remove all the pile
#define ENABLE_STACK_BELT_INV				//	[char_item.cpp]				enable stackable potitions automatically in belt inventory
#define ENABLE_SWITCH_ALL					//	[char_item.cpp]				all armors/weapons are switchable with green switcher
// #define ENABLE_IMMUNE_PERC				//	[char_resist.cpp]			enable 90% of success instead of 100% regarding immunes (antistun/slow/fall)
#define ENABLE_FORCE2MASTERSKILL			//	[char_skill.cpp]			skill always pass to m1 when b17 instead of b(number(17-20))
// #define ENABLE_MOUNTSKILL_CHECK			//	[char_skill.cpp]			check whether horse mount vnum should be checked when skilling
// #define ENABLE_NULLIFYAFFECT_LIMIT		//	[char_skill.cpp]			sura skill 66 won't nullify players with level < or > of yours by 9
// #define ENABLE_MASTER_SKILLBOOK_NO_STEPS	//	[char_skill.cpp]			if enabled, you will only need a book to increase a master skill, and not as many as the level-20
#define ENABLE_OX_SKILL_BLOCK				//	[char_skill.cpp]			block at OX-Event all skills

#define ENABLE_CMD_PLAYER					//	[config.cpp]				enable PLAYER grade inside CMD
#define ENABLE_EXPTABLE_FROMDB				//	[config.cpp]				read the exp table from the db
#define ENABLE_AUTODETECT_INTERNAL_IP		//	[config.cpp]				autodetect internal ip if the public one is missing
// #define ENABLE_GENERAL_CMD				//	[config.cpp]				if enabled, it reads a general CMD from "locale/%s/conf/GENERAL_CMD", "locale/%s/conf/GENERAL_CMD_CHANNEL_%d", and/or "locale/%s/conf/GENERAL_CMD_CHANNEL_%d_HOSTNAME_%s"
// #define ENABLE_GENERAL_CONFIG			//	[config.cpp]				if enabled, it reads a general CONFIG from "locale/%s/conf/GENERAL_CONFIG", "locale/%s/conf/GENERAL_CONFIG_CHANNEL_%d", and/or "locale/%s/conf/GENERAL_CONFIG_CHANNEL_%d_HOSTNAME_%s"
											//								in the GENERAL_CONFIG, all the options are valid except: HOSTNAME, CHANNEL, PLAYER_SQL, COMMON_SQL, LOG_SQL, PORT, P2P_PORT, MAP_ALLOW, AUTH_SERVER, TEEN_ADDR, TEEN_PORT

// #define ENABLE_BLOCK_CMD_SHORTCUT		//	[cmd.cpp]					if enabled, people won't be able to shorten commands
#define ENABLE_STATPLUS_NOLIMIT				//	[cmd_gm.cpp]				disable only 90 points for con+/int+/str+/dex+ commands
#define ENABLE_SET_STATE_WITH_TARGET		//	[cmd_gm.cpp]				enable set_state target as 3rd arg
#define ENABLE_CMD_IPURGE_EX				//	[cmd_gm.cpp]				/ipurge 2nd arg can remove items from a specific window (inv/equip/ds/belt/all)

// #define ENABLE_SPAMDB_REFRESH			//	[db.cpp]					enable a select query every 10 minutes into common.spam_db
#define ENABLE_FISHINGROD_RENEWAL			//	[fishing.cpp]				if the upgrading of the fishing rod will fail, it won't turn back of 1 grade, but just lose 10% mastering points.
#define ENABLE_NEWGUILDMAKE					//	[guild.h]					enable pc.make_guild0 and disable CInputMain::AnswerMakeGuild

#define ENABLE_ACCOUNT_W_SPECIALCHARS		//	[input_auth.cpp]			enable special characters in account names (account.account.login)
#define ENABLE_GOHOME_IF_MAP_NOT_EXIST		//	[input_db.cpp]				you'll go back to your village if the map doesn't exist
#define ENABLE_CHAT_COLOR_SYSTEM			//	[input_main.cpp]			enable chat colors based on IsGm or GetEmpire (+colored empire name)
#define ENABLE_CHAT_SPAMLIMIT				//	[input_main.cpp]			limit chat spam to 4 messages for 5 seconds, if you spam it for 10 times, you'll be disconnected!
#define ENABLE_WHISPER_CHAT_SPAMLIMIT		//	[input_main.cpp]			limit whisper chat to 10 messages per 5 seconds, otherwise you'll be disconnected!
#define ENABLE_CHAT_LOGGING					//	[input_main.cpp]			enable chat logging (which saves all the gm chats)
#define ENABLE_CHECK_GHOSTMODE				//	[input_main.cpp]			enable check that blocks the movements if the character is dead
#define ENABLE_IMMUNE_FIX					//	[item.cpp]					fix immune bug where you need to equip shield at last (or refresh compute e.g. un/riding horse)

// #define ENABLE_LIMIT_TIME				//	[limit_time.h]				enable game timestamp expiration
#define ENABLE_PICKAXE_RENEWAL				//	[mining.cpp]				if the upgrading of the pickaxe will fail, it won't turn back of 1 grade, but just lose 10% mastering points

#define ENABLE_QUEST_DIE_EVENT				//	[quest.h]					add quest event "die"
#define ENABLE_TRANSLATE_LUA				//	[questlua.cpp]				enable translate.lua loading
// #define ENABLE_LOCALECHECK_CHANGENAME	//	[questlua_pc.cpp]			enable check that unable change name on Europe Locales
// #define ENABLE_PC_OPENSHOP				//	[questlua_pc.cpp]			enable pc.open_shop0(idshop) but buy/sell not work yet
#define ENABLE_PARTYKILL					//	[questmanager.cpp]			re-enable PartyKill
#define QUEST_FIX							//	[questmanager.cpp]			xXxXx

// #define ENABLE_SHOP_BLACKLIST			//	[shop.cpp]					enable ignore 70024 (Blessing Marble) and 70035 (Magic Copper Ore)
#define ENABLE_NEW_RETARDED_GF_START_POSITION	//	[start_position.cpp]	New Gameforge Startposition

#define ENABLE_D_NJGUILD					//	[general]					enable d.new_jump_all_guild+cpp relative functions (untested)
//#define ENABLE_QUEST_CATEGORY				//	[general]					enable quest category+new packet types (unimplemented)
#define ENABLE_FULL_NOTICE					//	[general]					enable new big notice features
#define ENABLE_NEWSTUFF						//	[general]					enable new stuff (new lua funcs, new CONFIG options, ecc)




//-----KUNDENCHANGES----->
// #define ENABLE_ALEX_CHANGES					// IP-Bind, Teleportanpassungen
// #define ENABLE_BAUM_CHANGES


/*
@#GENERAL MACROS
#define __OBSOLETE__						//useless and pointless code removed
#define __UNIMPLEMENTED__					//drafts of new things to be implemented
*/
#endif

/*
#@general
@warme001: be aware about PLAYER_MAX_LEVEL_CONST (common/length.h) and gPlayerMaxLevel (game/config.h)
@warme002: be aware about ITEM_MAX_COUNT (common/item_length.h) and g_bItemCountLimit (game/config.h)
@warme003: do_click_safebox can be used by PLAYER in every map!
@warme004: `when vnum.kill begin` and `when kill begin` are both triggered
@warme005: different locale stuff
@warme006: not implemented stuff from another locale
@warme007: on db/src/ClientManager.cpp; commented locale set from common.locale due to its uselessness and bugginess (./close && ./start)
			it processes a NULL mysql connection (dat ymir threading) if there was a bit of overload before starting the process up again
@warme008: on char_item.cpp; now 27996 (poison bottle) can inflict poison
@warme009: on char_resist.cpp; if bleeding is used as poison, the bleeding enchantment is poison enchantment/50 (so mobs can bleed players)
@warme010: on char_state.cpp; test_server is used as "BOOL g_test_server"
@warme011: on dungeon.cpp; you should never use d.join instead of d.new_jump_party since it causes random crashes due to a wrong implementation of the party hash check
@warme012: trivial errors are now considered as simple logs


#@common
@fixme301: on tables.h; TPlayerTable hp/mp from short to int (hp/mp >32767 should be fixed)


#@db/src
@fixme201: on ProtoReader.cpp; changed 'SAMLL' into 'SMALL'
@fixme202: on ClientManagerGuild.cpp; fixed the guild remove member time issue if the player was offline
			(withdraw_time -> new_withdraw_time)
@fixme203: on ClientManagerPlayer.cpp; dandling pointer for "command"
@fixme204: on Cache.cpp; myshop_pricelist primary key duplication error if there are many items of the same vnum in the personal shop


#@game/src
@fixme101: on log.cpp; fixed '%s' for invalid_server_log
@fixme102: on cmd_general.cpp; inside ACMD(do_war) fixed the unsigned bug
@fixme103: on config, input_login, input_main.cpp; fixed clientcheckversion (version > date) to (version != date) and delay from 10 to 0
@fixme104: on char.cpp, questlua_pc.cpp; fixed get status point after lv90 changing 90 with gPlayerMaxLevel
@fixme105: on cmd.cpp; disabled every korean command
@fixme106: on input_main.cpp; if a full-speeded player is on a mount (es. lion), he'll be brought back due to the distance range
@fixme107: on char_battle.cpp; if character (player|mob) has negative hp on dead, sura&co will absorb hp/mp losing 'em themselves
@fixme108: on char.cpp; if you change a mount but the previous is not 0, all the entities (npcs&co) in the player client
			(not others) are vanished until another refresh (not exists mounts still bug you after second mount call)
@fixme109: on questmanager.cpp; if you kill a player (war m), `when kill begin` will be triggered twice
@fixme110: on char_affect.cpp; if you attack when semi-transparent (revived or ninja skill or white flag) you'll still be transparent
@fixme111: on test.cpp; ConvertAttribute2 has x and y inverted (before y->x after x->y)
@fixme112: on char_item.cpp; you can change bonuses in equipped items too (until re-login)
			bonus values will not be refreshed by ChangePoint and unequipping it will remove back only the new bonuses set on
			e.g. you had a 2500hp bonus shoes, you changed it to 50mp when equipped and you'll unequipped
			what will it happen? instead of remove 2500hp, you won't receive 50mp and you also lose 50mp when unequipped
@fixme113: on char_item.cpp; same thing of #112
			you can remove stones from equipped items w/o losing bonuses
			e.g. have an item with antiwar+4 equipped:
			1) remove all the stones 2) unequip it 3) re-add stone 4) re-equip it 5) repeat it thrice
			result? an item with no stones but you'll have 75% of antiwar
@fixme114: on char_item.cpp; gathering of #111, #112 and few others.
@fixme115: on char_item.cpp; you can retrieve all the item on the ground if you're in a party and the owner is not in yours.
@fixme116: on char_skill.cpp; normal horse mount skills cannot inflict damage
@fixme117: on char_item.cpp; you can't swap equipment from inventory if full, and also prevent unmotivated belt swap if its inventory is not empty
@fixme118: on char.cpp; when ComputePoints is called:
			you'll gain as many hp/mp as many you have in your equipment bonuses
			affect hp/mp will be lost when login or updating
@fixme119: on input_main.cpp; you can put items from safebox/mall to belt inventory w/o checking the type (items with size>1 are not placeable anyway)
@fixme120: on input_login.cpp; few packet IDs not checked
@fixme121: on char_item.cpp; the refine scroll item value 1 from the magic stone was generating useless syserrs
@fixme122: on arena.cpp; few other potions were not checked on arena map
@fixme123: on char_item.cpp; USE_CHANGE_ATTRIBUTE2 (24) sub type check bug (the condition could never be true)
@fixme124: on char_item.cpp; no check on 6-7 add/change items about costume stuff
@fixme125: on char.cpp; dungeon regen pointing to a dangling pointer (not required -> removed)
@fixme126: on marriage.cpp; fix lovepoints overflow
@fixme127: on cube.cpp; /cube r_info exploit fix; it can cause a crash due to an unchecked cube npc masters vnums
			e.g. 1) you open the Baek-Go cube's console 2) click on an npc/kill a mob without close the cube console
			3) digit /cube r_info 4) crash core
@fixme128: on char.cpp; mining hack fix; you can mine a vein anywhere in the map because there's no check on the character
			which means, you can stay at 0x0y and mining a vein in 666x999y and get the stuff beside him or in the pc's inventory
@fixme129: on PetSystem.cpp; the azrael pets (53005->34004 normal/53006->34009 gold) don't give the buff if not in dungeon at summon up and remove them anyway when unsummoned
@fixme130: on messenger_manager.cpp; and cmd_general.cpp if you do /messenger_auth n XXX, the player with the name XXX will receive a "refused friend invite" print from you
			which means, if you flood this packet, the "victim" will be disconnected or at maximum could get lag
@fixme131: on char.cpp; fix annoying sync packets sendable even on unfightable pc/npc entities
			e.g. wallhack against players' shops inside the village's squares (where the NOPK attr is set) to move them out and kill them
@fixme132: on shop.cpp; if two people buy the same item at the same time from a pc's shop, the slower one will receive a wrong return packet (crash client)
@fixme133: on input_main.cpp; banword check and hyper text feature were processing the final chat string instead of the raw one
@fixme134: on questlua_pc.cpp; the pc.mount_bonus was addable even if the mount wasn't spawn (only /unmount pc.unmount can remove it)
@fixme135: on char.cpp; if the Sync is made before a move packet and the sectree differs of few x/y coordinates, the sectree will be changed without update (crash character) (troublesome -> removed)
@fixme136: on char.cpp; there are no checks about the zero division exception: e.g. if you set a mob's max hp to 0 in the mob proto, you'll get random crashes.
@fixme137: on char_battle.cpp; when a player dies, the HP could have a negative value. Now it's 0 like the official.
@fixme138: on db.cpp, input_auth.cpp; the account's password was shown in the mysql history queries as clear text at every login attempt (mysql full granted user required -> now hashed)
@fixme139: on shop.h; CShop class destructor wasn't virtual. If a derived class like CShopEx was deleted, a memory leak would have been generated.
@fixme140: on input_main.cpp; the belt could be put into the safebox even though the belt inventory isn't empty.
@fixme141: on char_item.cpp; the items in the belt inventory could be used even if their slot were not available
@fixme142: on messenger_manager.cpp; sql injection fix about net.SendMessengerRemovePacket
@fixme143: on guild_manager.cpp; sql injection fix about net.SendAnswerMakeGuildPacket
@fixme144: on sectree_manager.cpp; if map/index doesn't end with a newline, the game will crash
@fixme145: on input_main.cpp; guild_add_member can add any vid as guild's member even if it's a mob or an npc
@fixme147: on char_item.cpp; ramadan candy item can be used even if the relative affect is still up
@fixme148: on item_manager_read_tables.cpp; type quest, special, attr not handled in ConvSpecialDropItemFile
@fixme149: on char.cpp; refine material skip exploit if items are swapped
@fixme150: on exchange.cpp; char_item.cpp; prevent item module swapping if the quest is suspended
*/
