# Makefile for exampleapp

COMPONENT = exampleapp
CFLAGS += -IOSLibSupport:,OSLib:,C:
CFLAGS += -Otime

APP_LIBS += OSLib:OSLib32.o


# The name of the linked file defaults to COMPONENT, which is often the case
# for single-file programs. But when the linked file lives in an application
# directory, it is normally called !RunImage.
TARGET = !RunImage

# The list of source/object files defaults to TARGET, which again is often
# the case for single-file programs. But we don't want our source file to be
# called !RunImage.
OBJS = wator

# The shared makefiles don't attempt to guess the application directory name
# Usually you'll want to place it inside INSTDIR, which is passed in from
# either the MkInstall file, or (for !Builder builds) the components file.
# SEP expands to the directory separator character - we use this instead of
# a literal '.' character to help with cross-compilation.
INSTAPP = ${INSTDIR}${SEP}!Example

# You need to specify all the files that go into the application directory.
# The shared makefiles handle merging the various subdirectories of the
# Resources directory for you.
INSTALLAPPFILES = !Boot !Run !Sprites !Sprites11 !Sprites22 !RunImage

include CApp

# Dynamic dependencies:
