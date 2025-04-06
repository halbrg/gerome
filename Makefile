PROG=   gerome
SRCS=   gerome.c ui.c geom.c
MAN=
LDADD=  -lncurses -lutil -lgeom

.include <bsd.prog.mk>
