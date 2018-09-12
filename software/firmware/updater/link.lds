ENTRY(main)

SECTIONS
{
  . = 0x20000100;
  .text :
  {
   main = .;
   *(.text*)
   *(.text)
   *(.rdata)
   *(.rodata)
   *(.rodata*)
   . = ALIGN(4);
  }

  .data :
  {
   *(.fsdata)
   *(.data)
   *(.data*)
   *(.idata)
   . = ALIGN(4);
  }

  bss = .;
  .bss (NOLOAD) :
  {
   *(.fsbss)
   *(.bss)
   *(.bss*)
  }
  end = . ;
  /DISCARD/ :
  {
  *(.note)
  *(.note*)
  *(.comment)
  }
}

