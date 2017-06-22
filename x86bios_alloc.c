
#include <sys/param.h>
#include "x86bios.h"

extern unsigned char *pbiosMem;
extern int busySegMap[5];

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
