
/*
 *	???
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/lock.h>
#include <sys/mutex.h>

#include <vm/vm.h>
#include <vm/vm_extern.h>
#include <vm/vm_kern.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>

#include <machine/cpufunc.h>

#include <contrib/x86emu/x86emu.h>
#include <contrib/x86emu/x86emu_regs.h>

#include "x86bios.h"

//#define MAPPED_MEMORY_SIZE	0xc00000

//#define PAGE_RESERV		(4096*5)

//static
 unsigned char *pbiosMem = NULL;
static unsigned char *pbiosStack = NULL;

//static
 int busySegMap[5];

static struct x86emu xbios86emu;

static struct mtx x86bios_lock;

static uint8_t
vm86_emu_inb(struct x86emu *emu, uint16_t port)
{
	if (port == 0xb2) /* APM scratch register */
		return 0;
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return 0;
	return inb(port);
}

static uint16_t
vm86_emu_inw(struct x86emu *emu, uint16_t port)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return 0;
	return inw(port);
}

static uint32_t
vm86_emu_inl(struct x86emu *emu, uint16_t port)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return 0;
	return inl(port);
}

static void
vm86_emu_outb(struct x86emu *emu, uint16_t port, uint8_t val)
{
	if (port == 0xb2) /* APM scratch register */
		return;
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return;
	outb(port, val);
}

static void
vm86_emu_outw(struct x86emu *emu, uint16_t port, uint16_t val)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return;
	outw(port, val);
}

static void
vm86_emu_outl(struct x86emu *emu, uint16_t port, uint32_t val)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return;
	outl(port, val);
}

void
x86biosCall(struct x86regs *regs, int intno)
{
	
	if (intno < 0 || intno > 255)
		return;

	mtx_lock(&x86bios_lock);
	critical_enter();

	xbios86emu.x86.R_EAX = regs->R_EAX;
	xbios86emu.x86.R_EBX = regs->R_EBX;
	xbios86emu.x86.R_ECX = regs->R_ECX;
	xbios86emu.x86.R_EDX = regs->R_EDX;

	xbios86emu.x86.R_ESP = regs->R_ESP;
	xbios86emu.x86.R_EBP = regs->R_EBP;
	xbios86emu.x86.R_ESI = regs->R_ESI;
	xbios86emu.x86.R_EDI = regs->R_EDI;
	xbios86emu.x86.R_EIP = regs->R_EIP;
	xbios86emu.x86.R_EFLG = regs->R_EFLG;

	xbios86emu.x86.R_CS = regs->R_CS;
	xbios86emu.x86.R_DS = regs->R_DS;
	xbios86emu.x86.R_SS = regs->R_SS;
	xbios86emu.x86.R_ES = regs->R_ES;
	xbios86emu.x86.R_FS = regs->R_FS;
	xbios86emu.x86.R_GS = regs->R_GS;

	x86emu_exec_intr(&xbios86emu, intno);

	regs->R_EAX = xbios86emu.x86.R_EAX;
	regs->R_EBX = xbios86emu.x86.R_EBX;
	regs->R_ECX = xbios86emu.x86.R_ECX;
	regs->R_EDX = xbios86emu.x86.R_EDX;

	regs->R_ESP = xbios86emu.x86.R_ESP;
	regs->R_EBP = xbios86emu.x86.R_EBP;
	regs->R_ESI = xbios86emu.x86.R_ESI;
	regs->R_EDI = xbios86emu.x86.R_EDI;
	regs->R_EIP = xbios86emu.x86.R_EIP;
	regs->R_EFLG = xbios86emu.x86.R_EFLG;

	regs->R_CS = xbios86emu.x86.R_CS;
	regs->R_DS = xbios86emu.x86.R_DS;
	regs->R_SS = xbios86emu.x86.R_SS;
	regs->R_ES = xbios86emu.x86.R_ES;
	regs->R_FS = xbios86emu.x86.R_FS;
	regs->R_GS = xbios86emu.x86.R_GS;

	critical_exit();
	mtx_unlock(&x86bios_lock);
	
}
#if 0
/* simple algorithm was taken from xfree86 */
void *
x86biosAlloc(int count, int *segs)
{
	int i;
	int j;

	/* find the free segblock of page */
	for (i = 0; i < (PAGE_RESERV - count); i++)
	{
		if (busySegMap[i] == 0)
		{
			/* find the capacity of segblock */
			for (j = i; j < (i + count); j++)
			{
				if (busySegMap[j] == 1)
					break;
			}

			if (j == (i + count))
				break;
			i += count;
		}
	}

	if (i == (PAGE_RESERV - count))
		return NULL;

	/* make the segblock is used */
	for (j = i; j < (i + count); j++)
		busySegMap[i] = 1;

	*segs = i * 4096;

	return (pbiosMem + *segs);
}

/* simple algorithm was taken from xfree86 */
void
x86biosFree(void *pbuf, int count)
{
	int i;
	int busySeg;

	busySeg = ((unsigned char *)pbuf - (unsigned char *)pbiosMem)/4096;

	for (i = busySeg; i < (busySeg + count); i++)
		busySegMap[i] = 0;
}
#endif
void *
x86biosOffs(uint32_t offs)
{
	return (pbiosMem + offs);
}

static int
x86bios_modevent(module_t mod __unused, int type, void *data __unused)
{
	int err = 0;
	int offs;

	switch (type) {
	case MOD_LOAD:

		mtx_init(&x86bios_lock, "x86bios lock", NULL, MTX_DEF);

		/* Can `emumem' be NULL here? */
		pbiosMem = pmap_mapbios(0x0, MAPPED_MEMORY_SIZE);

		memset(&xbios86emu, 0, sizeof(xbios86emu));
		x86emu_init_default(&xbios86emu);

		xbios86emu.emu_inb = vm86_emu_inb;
		xbios86emu.emu_inw = vm86_emu_inw;
		xbios86emu.emu_inl = vm86_emu_inl;
		xbios86emu.emu_outb = vm86_emu_outb;
		xbios86emu.emu_outw = vm86_emu_outw;
		xbios86emu.emu_outl = vm86_emu_outl;

		xbios86emu.mem_base = (char *)pbiosMem;
		xbios86emu.mem_size = 1024 * 1024;

		memset(busySegMap, 0, sizeof(busySegMap));

		pbiosStack = x86biosAlloc(1, &offs);

		break;

	case MOD_UNLOAD:

		x86biosFree(pbiosStack, 1);

		if (pbiosMem)
			pmap_unmapdev((vm_offset_t)pbiosMem, MAPPED_MEMORY_SIZE);

		mtx_destroy(&x86bios_lock);

		break;

	default:
		err = ENOTSUP;
		break;

	}
	return (err);
}

static moduledata_t x86bios_mod = {
	"x86bios",
	x86bios_modevent,
	NULL,
};

DECLARE_MODULE(x86bios, x86bios_mod, SI_SUB_DRIVERS, SI_ORDER_FIRST);
MODULE_VERSION(x86bios, 1);
MODULE_DEPEND(x86bios, x86emu, 1, 1, 1);

