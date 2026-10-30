#ifndef LOGO_ASCII_H_
#define LOGO_ASCII_H_

#include <stdio.h>

const unsigned char logo_ascii[45][81] = {
"+---------------------------------------------------------------------------+",
"|                                                                           |",
"|                              BL-F28P65X-ECAT                              |",
"|                     ETHERCAT FoE RECOVERY BOOTLOADER                      |",
"|                                                                           |",
"|         o====o====o====o====o====o====o====o====o====o====o====o          |",
"|                                                                           |",
"|                         ######    ####    ######                          |",
"|                         ##       ##  ##   ##                              |",
"|                         #####    ##  ##   #####                           |",
"|                         ##       ##  ##   ##                              |",
"|                         ##        ####    ######                          |",
"|                                                                           |",
"|       +---------------------- F28P65X CPU1 ----------------------+        |",
"|       |                                                          |        |",
"|       |  C2000 CORE  <---->  ESC SUBSYSTEM  <---->  FLASH API    |        |",
"|       |                                                          |        |",
"|       +----------------------------------------------------------+        |",
"|                                                                           |",
"|      ECAT PORT 0  <========  ETHERCAT FRAME  ========>  ECAT PORT 1       |",
"|                                                                           |",
"|            [ ESC CORE ]  <==>  [ SOES ]  <==>  [ FoE SERVER ]             |",
"|                                                                           |",
"|                           FIRMWARE UPDATE PATH                            |",
"|                                                                           |",
"|     [ MASTER ] -- FoE --> [ RX BUFFER ] -- VERIFY --> [ BANK 1 APP ]      |",
"|                                                                           |",
"|                               BOOT DECISION                               |",
"|                                                                           |",
"|          RESET --> VALID IMAGE ? --> RUN APPLICATION @ 0x0A0000           |",
"|                               +--> RECOVERY / ETHERCAT BOOT MODE          |",
"|                                                                           |",
"|    -------------------------------------------------------------------    |",
"|   POWER-FAIL SAFE     |     VALIDATE BEFORE EXECUTE     |     CRC CHECK   |",
"|    COMMIT MARKER LAST  |     RECOVERY ALWAYS AVAILABLE  |     ACK LAST    |",
"|    -------------------------------------------------------------------    |",
"|                                                                           |",
"|                      TARGET   : TMS320F28P65X / CPU1                      |",
"|                          BOOT SLOT: FLASH BANK 0                          |",
"|                          APP SLOT : FLASH BANK 1                          |",
"|               PROTOCOL : ETHERCAT FILE-OVER-ETHERCAT (FoE)                |",
"|                                                                           |",
"|        [ LINK ]   [ ESC READY ]   [ FoE READY ]   [ FLASH READY ]         |",
"|    ======================= WAITING FOR MASTER =======================     |",
"+---------------------------------------------------------------------------+"
};

static inline void print_ascii_banner(void)
{
    for (int i = 0; i < 45; ++i) {
        printf("%s\r\n", logo_ascii[i]);
    }
}

#endif /* LOGO_ASCII_H_ */
