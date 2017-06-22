# $FreeBSD: src/sys/modules/vesa/Makefile,v 1.15 2009/09/09 09:50:31 delphij Exp $

KMOD=	vesa
SRCS=	opt_vga.h opt_vesa.h

//.PATH:	${.CURDIR}/../../dev/fb
.PATH:	${.CURDIR}
SRCS+=	vesa.c

//.PATH:	${.CURDIR}/../../dev/syscons
.PATH:	/usr/src/sys/dev/syscons
SRCS+=	scvesactl.c

.include <bsd.kmod.mk>
