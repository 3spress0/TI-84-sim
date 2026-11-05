# ------------------------------
# MetroCE — TI-84 Plus CE city simulator
# ------------------------------

NAME = METROCE
DESCRIPTION = "MetroCE City Simulator"
COMPRESSED = YES
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

include $(shell cedev-config --makefile)
