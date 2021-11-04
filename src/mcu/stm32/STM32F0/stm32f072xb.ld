/* Memory Spaces Definitions */

ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 52K
  STORE (rx) : ORIGIN = 0x0800D000, LENGTH = 76K
  RAM  (xrw) : ORIGIN = 0x20000000, LENGTH = 16k
}

/* Highest address of the user mode stack */
_estack = 0x20004000;

INCLUDE "../sections_flash.ld"
