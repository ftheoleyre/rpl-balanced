ACLOCAL_AMFLAGS = -I m4
lib_LTLIBRARIES = libroutage_rpl.la libmac_802154_slotted.la libpropagation_shadowing_802154_mine.la libradio_generic_mine.la libmobility_static_disk_mine.la libapplication_cbr_mine.la

libroutage_rpl_la_CFLAGS = $(CFLAGS) $(GLIB_FLAGS) -Wall -g
libroutage_rpl_la_SOURCES = git_rpl/wsnet_rpl/rplWSNet.c \
		git_rpl/rpl/rpl-dag.c \
		git_rpl/rpl/rpl-multipath.c \
		git_rpl/rpl/rpl-icmp6.c  \
		git_rpl/rpl/rpl-of0.c  \
		git_rpl/rpl/rpl-of-elt_multipath.c \
		git_rpl/rpl/rpl-timers.c  \
		git_rpl/rpl/stats.c 

libroutage_rpl_la_LDFLAGS = -module 


libmac_802154_slotted_la_CFLAGS = $(CFLAGS) $(GLIB_FLAGS) $(GSL_FLAGS) -Wall 
libmac_802154_slotted_la_SOURCES = git_rpl/mac_beacon/mac/802154_slotted.c \
		git_rpl/mac_beacon/mac/beacons.c \
		git_rpl/mac_beacon/mac/data.c \
		git_rpl/mac_beacon/mac/mgmt.c \
		git_rpl/mac_beacon/mac/radio.c \
		git_rpl/mac_beacon/mac/stats.c \
		git_rpl/mac_beacon/mac/tools.c \
		git_rpl/mac_beacon/mac/routing_ideal.c \
		git_rpl/mac_beacon/mac/packets.c \
		git_rpl/mac_beacon/mac/buffer.c \
		git_rpl/mac_beacon/mac/neigh.c \
		git_rpl/mac_beacon/mac/param.c \
		git_rpl/mac_beacon/mac/ctree.c
		
libmac_802154_slotted_la_LDFLAGS = -module -L/usr/local/include/igraph -ligraph


libpropagation_shadowing_802154_mine_la_CFLAGS = $(CFLAGS) $(GLIB_FLAGS) $(GSL_FLAGS) -Wall
libpropagation_shadowing_802154_mine_la_SOURCES = git_rpl/propagation/shadowing_802154.c
libpropagation_shadowing_802154_mine_la_LDFLAGS = -module

libradio_generic_mine_la_CFLAGS = $(CFLAGS) $(GLIB_FLAGS) $(GSL_FLAGS) -Wall
libradio_generic_mine_la_SOURCES = git_rpl/radio/radio_generic.c
libradio_generic_mine_la_LDFLAGS = -module


libmobility_static_disk_mine_la_CFLAGS = $(CFLAGS) $(GLIB_FLAGS) $(GSL_FLAGS) -Wall
libmobility_static_disk_mine_la_SOURCES = git_rpl/mobility/random_static_disk_centered_sink.c
libmobility_static_disk_mine_la_LDFLAGS = -module



libapplication_cbr_mine_la_CFLAGS = $(CFLAGS) $(GLIB_FLAGS) $(GSL_FLAGS) -Wall -g
libapplication_cbr_mine_la_SOURCES = git_rpl/application/cbr_mine.c
libapplication_cbr_mine_la_LDFLAGS = -module
