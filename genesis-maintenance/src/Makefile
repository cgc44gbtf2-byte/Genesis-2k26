CC       ?= cc
CPPFLAGS += -DMERC -Dunix -DMEMWATCH
CFLAGS   ?= -O2
CFLAGS   += -std=gnu89 -Wall -Wno-builtin-declaration-mismatch
LDLIBS   ?= -lcrypt

OBJECTS = act_comm.o act_info.o act_move.o act_obj.o act_wiz.o bit.o board.o \
clan.o clearclass.o comm.o network.o pset_currency.o const.o core.o db.o disc.o dragon.o \
	drow.o elemental.o fight.o flag.o handler.o highland.o interp.o \
	kav_info.o kav_wiz.o mage.o magic.o memwatch.o monk.o more.o ninja.o \
	olc.o random.o rename.o rot_wiz.o save.o signals.o sorcerer.o \
special.o stance.o string.o trap.o update.o werewolf.o wizutil.o reconstructed.o \
wwauspice.o wwbreed.o wwtribe.o parents.o hybrid.o phoenix.o cyborg_core.o reference_state.o crafted_state.o lineage_state.o crafted.o newclasses.o

.PHONY: all clean

all: merc

merc: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c merc.h board.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJECTS): cyborg_core.h reference_state.h crafted_state.h lineage_state.h parents.h
parents.o: parents.h parent_powers.def hybrid_roster.def lineage_registry.def lineage_powers.def crafted_powers.def cyborg_components.def
crafted.o: crafted_powers.def lineages.inc lineage_powers.def
hybrid.o: parents.h
newclasses.o: reference_abilities.inc reference_legacy.inc parents.h
comm.o network.o: network.h
sorcerer.o: sorcerer_starter.inc
cyborg_core.o: cyborg_core.h cyborg_components.def

clean:
	rm -f $(OBJECTS) merc
