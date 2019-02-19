ACLOCAL_AMFLAGS=-I m4 

lib_LTLIBRARIES = libmac_802154_slotted.la

libmac_802154_slotted_la_CFLAGS = $(CFLAGS) $(GLIB_FLAGS) $(GSL_FLAGS) -Wall 

libmac_802154_slotted_la_SOURCES = 802154_slotted.c beacons.c data.c mgmt.c radio.c stats.c tools.c routing_ideal.c packets.c buffer.c neigh.c param.c ctree.c

libmac_802154_slotted_la_LDFLAGS = -module -L/opt/local/lib -ligraph
