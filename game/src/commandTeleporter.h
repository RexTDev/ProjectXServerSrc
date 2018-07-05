
	int szTeleportConfig[NUMBER_MAX_MAPS + 1][4] = 
	{
	//		COORDINATES					LEVEL								INFORMATION
	//	{	X			Y			MIN		MAX		}		   MapName							MapIndex
		{	NULL,		NULL,		NULL,	NULL	},
		{	473900,		954600,		1,		170		},		// Map1 Rotes_Reich					(idxMapIndex: 1 )
		{	352300,		882700,		1,		170		},		// Map2 Rotes_Reich					(idxMapIndex: 3 )
		{	63200,		166700,		1,		170		},		// Map1 Gelbes_Reich				(idxMapIndex: 21)
		{	145700,		239800,		1,		170		},		// Map2 Gelbes_Reich				(idxMapIndex: 23)
		{	959600,		269700,		1,		170		},		// Map1 Blaues_Reich				(idxMapIndex: 41)
		{	863800,		246000,		1,		170		},		// Map2 Blaues_Reich				(idxMapIndex: 43)
		{	436300,		215700,		15,		170		},		// Eisland							(idxMapIndex: 61)
		{	333200,		746500,		15,		170		},		// Orktal (Mitte)					(idxMapIndex: 64)
		{	600900,		687500,		15,		170		},		// Flammenland						(idxMapIndex: 62)
		{	296300,		547300,		1,		170		},		// Wueste (Mitte)					(idxMapIndex: 63)
#ifndef ENABLE_ALEX_CHANGES
		{	60000,		496600,		30,		85		},		// Spinnendungeon_1					(idxMapIndex: 10)
#else
		{	665600,		435200,		30,		85		},		// Spinnendungeon_2					(idxMapIndex: 71)
#endif
		{	590500,		110500,		35,		95		}		// Hwang_Tempel (DT)				(idxMapIndex: 65)
#ifndef ENABLE_ALEX_CHANGES
		,{	284200,		809600,		45,		170		}		// Grotte_1							(idxMapIndex: 72)
#endif
	};

	char* szTableNameMaps[] =
	{
		NULL,
		"<1>  Map1 Shinsoo [Rot]				Level   MIN. %d | MAX. %d",
		"<2>  Map2 Shinsoo [Rot]				Level   MIN. %d | MAX. %d",
		"<3>  Map1 Chunjo [Gelb]				Level   MIN. %d | MAX. %d",
		"<4>  Map2 Chunjo [Gelb]				Level   MIN. %d | MAX. %d",
		"<5>  Map1 Jinno [Blau]					Level   MIN. %d | MAX. %d",
		"<6>  Map2 Jinno [Blau]					Level   MIN. %d | MAX. %d",
		"<7>  Berg Sohan (Mitte)				Level   MIN. %d | MAX. %d",
		"<8>  Orktal (Mitte)					Level   MIN. %d | MAX. %d",
		"<9>  Feuerland (Mitte)					Level   MIN. %d | MAX. %d",
		"<10> Yongbi Wüste (Mitte)				Level   MIN. %d | MAX. %d",
#ifndef ENABLE_ALEX_CHANGES
		"<11> Spinnendungeon 1					Level   MIN. %d | MAX. %d",
#else
		"<11> Spinnendungeon 2					Level   MIN. %d | MAX. %d",
#endif
		"<12> Dämonenturm						Level   MIN. %d | MAX. %d",
#ifndef ENABLE_ALEX_CHANGES
		"<13> Grotte 1							Level   MIN. %d | MAX. %d",
#endif
	};

#ifdef ENABLE_TRANSLATE_LC_GERMANY
	const char* szTableTranslate[] =
	{
/* szTableTranslate[0]  */	"Du bist im Zuschauermodus, du kannst das nicht tun.",
/* szTableTranslate[1]  */	"Du bist tot, du kannst das nicht tun.",
/* szTableTranslate[2]  */	"Du bist betäubt, du kannst das nicht tun.",
/* szTableTranslate[3]  */	"Du bist ein Shop, du kannst das nicht tun.",
/* szTableTranslate[4]  */	"Du bist in einem Dungeon, du kannst das nicht tun.",
/* szTableTranslate[5]  */	"Du hast ein Fenster geöffnet, bitte schließe es, dannach kannst du teleportieren.",
/* szTableTranslate[6]  */	"Du kannst keine kleinere Zahl wie 1 eingeben.",
/* szTableTranslate[7]  */	"Du kannst keine höhere Zahl eingeben, wie [%d].",
/* szTableTranslate[8]  */	"Du kannst dich nicht teleportieren, weil du zu schwach bist, du musst mind. Level [%d] sein.",
/* szTableTranslate[9]  */	"Du kannst dich nicht teleportieren, weil du zu stark bist, du darfst nicht höher als Level [%d] sein.",
/* szTableTranslate[10] */	"Du hast die Nummer: [%d] ausgewählt.",
/* szTableTranslate[11] */	"VERWENDUNG : /teleport <nummer>",
/* szTableTranslate[12] */	"Liste mit Maps, aktiv für Teleport mit der Nummer:",
/* szTableTranslate[13] */	"Bitte warte 10 Sekunden."
	};
#endif

#ifdef ENABLE_TRANSLATE_LC_ENGLISH
	const char* szTableTranslate[] =
	{
		"You are in observation mode, you can not do that.",
		"You're dead, you can not do that.",
		"You're dizzy, you can not do that.",
		"You're a store, you can not do that.",
		"You're in a dungeon, you can not do that.",
		"Have a malicious window open, please close it then you can do this.",
		"You can not enter a smaller number like 1.",
		"You can not enter a higher number as [%d].",
		"You you cannot teleport in this map because you're too weak, you need level min: [%d].",
		"You you cannot teleport in this map because you're too big, you need level max: [%d].",
		"You chose number: [%d].",
		"USAGE : /teleport <number>",
		"List with map's active for teleport with number:",
		"Please wait 10 seconds."
	};
#endif

#ifdef ENABLE_TRANSLATE_LC_POLAND
	const char* szTableTranslate[] =
	{
		"Jeste? w trybie obserwacji, nie mo?na tego zrobi?.",
		"Jeste? martwy, nie mo?na tego zrobi?.",
		"Jeste? w głowie, ?e nie mog? tego zrobi?.",
		"Jeste? sklep, nie mo?na tego zrobi?.",
		"Jeste? w lochu, nie mo?na tego zrobi?.",
		"Maj? szkodliwy okno otwarte, zamknij go, a nast?pnie mo?na to zrobi?.",
		"Nie mo?na wprowadzi? mniejsz? liczb? jak 1.",
		"Nie mo?na wprowadzi? wi?ksz? liczb? jako [%d].",
		"Ty nie mo?na teleportowa? si? w tej mapie, bo jeste? zbyt słaby, trzeba poziomu MIN: [%d].",
		"Ty nie mo?na teleportowa? si? w tej mapie, bo jeste? zbyt du?e, trzeba Maksymalny poziom: [%d].",
		"Wybrano numer: [%d].",
		"STOSOWANIE : /teleport <numer>",
		"List z aktywno?ci? mapy do teleportu z numerem:",
		""
	};
#endif

#ifdef ENABLE_TRANSLATE_LC_SPAIN
	const char* szTableTranslate[] =
	{
		"Usted esta en modo de observacion, no se puede hacer eso.",
		"Estas muerto, no se puede hacer eso.",
		"Estas mareada, no se puede hacer eso.",
		"Eres una tienda, no se puede hacer eso.",
		"Estas en un calabozo, no se puede hacer eso.",
		"Tener una ventana abierta maliciosos, por favor cerrarla entonces usted puede hacer esto.",
		"No se puede introducir un numero mas pequeno como 1.",
		"No se puede introducir un numero mas alto como [%d].",
		"Usted no puede teletransportarse en este mapa porque esta demasiado debil, que necesita el nivel min: [%d].",
		"Usted no puede teletransportarse en este mapa porque estas demasiado grande, se necesita el nivel maximo: [%d].",
		"Elegiste numero: [%d].",
		"USO : /teleport <numero>",
		"Una lista con actividad mapa de teletransporte con el numero:",
		""
	};
#endif
