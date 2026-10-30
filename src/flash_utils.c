/*
 * flash_utils.c
 *
 * Author: amargan
 */

#include <stdio.h>

#include "flash_utils.h"
#include "device.h"
#include "driverlib.h"

void Disable_interrupt() { DINT; }
void Enable_interrupt()  { EINT; }

static bool flashApiConfigured = false;

static inline __attribute__((always_inline))
bool IsFlashRangeValid(uint32_t address, uint32_t size) {
    const uint32_t bank1EndAddressPlus1 = FlashBank1EndAddress + 1U;
    const uint32_t bank2EndAddressPlus1 = FlashBank2EndAddress + 1U;
    const bool inBank1 = (address >= FlashBank1StartAddress) &&
                         (address < bank1EndAddressPlus1) &&
                         (size <= (bank1EndAddressPlus1 - address));
    const bool inBank2 = (address >= FlashBank2StartAddress) &&
                         (address < bank2EndAddressPlus1) &&
                         (size <= (bank2EndAddressPlus1 - address));

    return (size != 0U) && (inBank1 || inBank2);
}

static inline __attribute__((always_inline))
void ClaimFlashSemaphore(void)
{
#ifdef CPU1
    IPC_claimFlashSemaphore(IPC_FLASHSEM_OWNER_CPU1);
#elif defined(CPU2)
    IPC_claimFlashSemaphore(IPC_FLASHSEM_OWNER_CPU2);
#endif
}

static inline __attribute__((always_inline)) void ReleaseFlashSemaphore(void)
{
#if defined(CPU1) || defined(CPU2)
    IPC_releaseFlashSemaphore();
#endif
}

uint32_t Calc_CRC(uint32_t addr, uint32_t length)
{
    uint32_t crc = 0U; // HAL_CRC_Calculate(&hcrc, (uint32_t *)addr, length);
    return crc;
}

static __attribute__((section(".TI.ramfunc")))
Fapi_StatusType ClearFSMStatus(void)
{
    Fapi_FlashStatusType oFlashStatus;
    Fapi_StatusType oReturnCheck;

    while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady) {}

    oFlashStatus = Fapi_getFsmStatus();
    if (oFlashStatus != 0U)
    {
        oReturnCheck = Fapi_issueAsyncCommand(Fapi_ClearStatus);
        while (Fapi_getFsmStatus() != 0U) {}

        if (oReturnCheck != Fapi_Status_Success)
        {
            return oReturnCheck;
        }
    }

    return Fapi_Status_Success;
}

__attribute__((section(".TI.ramfunc")))
Fapi_StatusType Configure_flashAPI(void)
{
    Fapi_StatusType oReturnCheck;
    Fapi_LibraryInfoType libInfo;

    flashApiConfigured = false;

    Flash_initModule(FLASH0CTRL_BASE, FLASH0ECC_BASE, DEVICE_FLASH_WAITSTATES);

    EALLOW;
    SysCtl_allocateFlashBank(SYSCTL_FLASH_BANK1, SYSCTL_CPUSEL_CPU1);
    SysCtl_allocateFlashBank(SYSCTL_FLASH_BANK2, SYSCTL_CPUSEL_CPU1);
    EDIS;

    ClaimFlashSemaphore();

    oReturnCheck = Fapi_initializeAPI(FlashTech_CPU0_BASE_ADDRESS,
                                      DEVICE_SYSCLK_FREQ / 1000000U);
    if (oReturnCheck != Fapi_Status_Success)
    {
        ReleaseFlashSemaphore();
        return oReturnCheck;
    }

    // TI's F28P65x Flash and EEPROM examples initialize FMC through Bank 0
    // even when the target storage area is located in another allocated bank.
    oReturnCheck = Fapi_setActiveFlashBank(Fapi_FlashBank0);
    if (oReturnCheck != Fapi_Status_Success)
    {
        ReleaseFlashSemaphore();
        return oReturnCheck;
    }

    ReleaseFlashSemaphore();
    flashApiConfigured = true;

    libInfo = Fapi_getLibraryInfo();
    printf("Flash lib info %d.%d %d\n",
           libInfo.u8ApiMajorVersion,
           libInfo.u8ApiMinorVersion,
           libInfo.u8ApiRevision);

    return Fapi_Status_Success;
}

__attribute__((section(".TI.ramfunc")))
Fapi_StatusType Erase_dataFlashSector(uint32_t address, uint32_t size)
{
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusType oFlashStatus;
    Fapi_FlashStatusWordType oFlashStatusWord;
    uint32_t u32EndAddress;

    if (!flashApiConfigured ||
        !IsFlashRangeValid(address, size) ||
        ((address % DATA_FLASH_SECTOR_SIZE_WORDS) != 0U))
    {
        return Fapi_Error_InvalidAddress;
    }

    // Flash erase operates on complete 2 KB sectors. On C28, size and Flash
    // addresses use 16-bit words, so one sector occupies 0x400 address units.
    const uint32_t sectorCount =
        (size + DATA_FLASH_SECTOR_SIZE_WORDS - 1U) /
        DATA_FLASH_SECTOR_SIZE_WORDS;
    u32EndAddress = address + (sectorCount * DATA_FLASH_SECTOR_SIZE_WORDS);

    ClaimFlashSemaphore();

    for (uint32_t u32CurrentAddress = address;
         u32CurrentAddress < u32EndAddress;
         u32CurrentAddress += DATA_FLASH_SECTOR_SIZE_WORDS)
    {
        oReturnCheck = ClearFSMStatus();
        if (oReturnCheck != Fapi_Status_Success)
        {
            ReleaseFlashSemaphore();
            return oReturnCheck;
        }

        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE + FLASH_O_CMDWEPROTA,
                                   0x00000000U);
        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE + FLASH_O_CMDWEPROTB,
                                   0x00000000U);

        oReturnCheck = Fapi_issueAsyncCommandWithAddress(
            Fapi_EraseSector, (uint32_t *)u32CurrentAddress);

        while (Fapi_checkFsmForReady() != Fapi_Status_FsmReady) {}

        if (oReturnCheck != Fapi_Status_Success) {
            ReleaseFlashSemaphore();
            return oReturnCheck;
        }

        oFlashStatus = Fapi_getFsmStatus();
        if (oFlashStatus != 3U) {
            ReleaseFlashSemaphore();
            return Fapi_Error_Fail;
        }

        oReturnCheck = Fapi_doBlankCheck((uint32_t *)u32CurrentAddress,
                                         Sector2KB_u32length,
                                         &oFlashStatusWord);
        if (oReturnCheck != Fapi_Status_Success) {
            ReleaseFlashSemaphore();
            return oReturnCheck;
        }
    }

    ReleaseFlashSemaphore();
    return Fapi_Status_Success;
}

__attribute__((section(".TI.ramfunc")))
Fapi_StatusType Program_dataFlashSector(const uint16_t *src, uint32_t address, uint32_t size)
{
    Fapi_StatusType oReturnCheck;
    Fapi_FlashStatusType oFlashStatus;
    Fapi_FlashStatusWordType oFlashStatusWord;
    uint32_t programData32[4];
    uint16_t *programData = (uint16_t *)programData32;

    if (!flashApiConfigured ||
        (src == NULL) ||
        !IsFlashRangeValid(address, size) ||
        ((address % DATA_FLASH_ALIGNMENT_WORDS) != 0U))
    {
        return Fapi_Error_InvalidAddress;
    }

    ClaimFlashSemaphore();

    for (uint32_t offset = 0U; offset < size; offset += DATA_FLASH_PROGRAM_SIZE_WORDS)
    {
        uint32_t chunkSize = size - offset;
        uint32_t u32Index = address + offset;

        if (chunkSize > DATA_FLASH_PROGRAM_SIZE_WORDS) {
            chunkSize = DATA_FLASH_PROGRAM_SIZE_WORDS;
        }

        // AutoECC programs up to 128 bits (8 C28 words). Pad the final unit
        // with the erased value so the source is never read past its end.
        for (uint32_t i = 0U; i < DATA_FLASH_PROGRAM_SIZE_WORDS; i++) {
            programData[i] = (i < chunkSize) ? src[offset + i] : 0xFFFFU;
        }

        oReturnCheck = ClearFSMStatus();
        if (oReturnCheck != Fapi_Status_Success) {
            ReleaseFlashSemaphore();
            return oReturnCheck;
        }

        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE + FLASH_O_CMDWEPROTA,
                                   0x00000000U);
        Fapi_setupBankSectorEnable(FLASH_WRAPPER_PROGRAM_BASE + FLASH_O_CMDWEPROTB,
                                   0x00000000U);

        oReturnCheck = Fapi_issueProgrammingCommand((uint32_t *)u32Index,
                                                    programData,
                                                    DATA_FLASH_PROGRAM_SIZE_WORDS,
                                                    0, 0,
                                                    Fapi_AutoEccGeneration);

        while (Fapi_checkFsmForReady() == Fapi_Status_FsmBusy) {}

        if (oReturnCheck != Fapi_Status_Success) {
            ReleaseFlashSemaphore();
            return oReturnCheck;
        }

        oFlashStatus = Fapi_getFsmStatus();
        if (oFlashStatus != 3U) {
            ReleaseFlashSemaphore();
            return Fapi_Error_Fail;
        }

        oReturnCheck = Fapi_doVerify((uint32_t *)u32Index,
                                     4U,
                                     programData32,
                                     &oFlashStatusWord);
        if (oReturnCheck != Fapi_Status_Success) {
            ReleaseFlashSemaphore();
            return oReturnCheck;
        }
    }

    ReleaseFlashSemaphore();
    return Fapi_Status_Success;
}

/**
 * Flash Banksw are 256 KB and contains 128 sectors of 2 KB each.
 * On C28, the address range is 0x0C0000-0x0DFFFF in 16-bit words.
 */
__attribute__((section(".TI.ramfunc")))
bool Write_flash(uint32_t flash_addr, const void *src, uint32_t size)
{
    if (Erase_dataFlashSector(flash_addr, size) != Fapi_Status_Success) {
        return false;
    }

    if (Program_dataFlashSector((const uint16_t *)src, flash_addr, size) !=
        Fapi_Status_Success) {
        return false;
    }

    return true;
}
