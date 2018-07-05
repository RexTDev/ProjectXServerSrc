CC=gcc
CXX=g++

PLATFORM = $(shell file /bin/ls | cut -d' ' -f3 | cut -d'-' -f1)
BSD_VERSION = $(shell uname -v 2>&1 | cut -d' ' -f2 | cut -d'.' -f1)
SVR_VERSION = $(shell cat __REVISION__)

# default: libthecore libpoly libgame liblua libsql libserverkey game db
default: liblua libsql libgame libpoly libthecore game db
	@echo "--------------------------------------"
	@echo "Build Done"
	@echo "--------------------------------------"

liblua: .
	$(MAKE) -C $@/.lua50 clean
	$(MAKE) -C $@/.lua50

libsql: .
	@touch $@/Depend
	$(MAKE) -C $@ dep
	$(MAKE) -C $@ clean
	$(MAKE) -C $@

libgame: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	$(MAKE) -C $@/src

libpoly: .
	@touch $@/Depend
	$(MAKE) -C $@ dep
	$(MAKE) -C $@ clean
	$(MAKE) -C $@

libthecore: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	$(MAKE) -C $@/src

libserverkey: .
	@touch $@/Depend
	$(MAKE) -C $@ dep
	$(MAKE) -C $@ clean
	$(MAKE) -C $@

game: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	# $(MAKE) -C $@/src limit_time
	$(MAKE) -C $@/src
	$(MAKE) -C $@/src symlink

db: .
	@touch $@/src/Depend
	$(MAKE) -C $@/src dep
	$(MAKE) -C $@/src clean
	$(MAKE) -C $@/src
	$(MAKE) -C $@/src symlink

ver:
	@$(CC) -v
ver2:
	@$(CC) -v
	$(MAKE) -C game/src ver

strip:
	$(MAKE) -C game/src strip
	$(MAKE) -C db/src strip

all:
	@echo "--------------------------------------"
	@echo "Update Revision"
	@echo "--------------------------------------"
	@expr $(SVR_VERSION) + 1 > __REVISION__
	@cat  __REVISION__

	@echo "--------------------------------------"
	@echo "Full Build Start"
	@echo "--------------------------------------"

	$(MAKE) -C liblua/.lua50 clean
	$(MAKE) -C liblua/.lua50

	@touch libsql/Depend
	$(MAKE) -C libsql dep
	$(MAKE) -C libsql clean
	$(MAKE) -C libsql

	@touch libgame/src/Depend
	$(MAKE) -C libgame/src dep
	$(MAKE) -C libgame/src clean
	$(MAKE) -C libgame/src

	@touch libpoly/Depend
	$(MAKE) -C libpoly dep
	$(MAKE) -C libpoly clean
	$(MAKE) -C libpoly

	@touch libthecore/src/Depend
	$(MAKE) -C libthecore/src dep
	$(MAKE) -C libthecore/src clean
	$(MAKE) -C libthecore/src

	@touch libserverkey/Depend
	$(MAKE) -C libserverkey dep
	$(MAKE) -C libserverkey clean
	$(MAKE) -C libserverkey

	@touch game/src/Depend
	$(MAKE) -C game/src dep
	$(MAKE) -C game/src clean
	# $(MAKE) -C game/src limit_time
	$(MAKE) -C game/src
	# $(MAKE) -C game/src symlink
	$(MAKE) -C game/src strip

	@touch db/src/Depend
	$(MAKE) -C db/src dep
	$(MAKE) -C db/src clean
	$(MAKE) -C db/src
	# $(MAKE) -C db/src symlink
	$(MAKE) -C db/src strip
	@echo "--------------------------------------"
	@echo "Full Build End"
	@echo "--------------------------------------"
