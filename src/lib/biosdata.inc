;Biosdatenbereich
;Marius Greuel '94

@BIOS_DATA_SEG		equ 40h
@BIOS_DATA		segment para public ´DATA´
                        org 6ch
@BD_TIMER_CNTR          dd ?
@BD_TIMER_OVFLW_FLG     db ?
@BIOS_DATA		ends
