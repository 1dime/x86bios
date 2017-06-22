
#.PATH:  ${.CURDIR} /usr/src/sys/dev/syscons
.PATH:  ${.CURDIR}

KMOD=	x86bios
SRCS=   x86bios.c
SRCS+=	x86bios_alloc.c

WERROR=

.include<bsd.kmod.mk>
