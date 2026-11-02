#include <stdint.h>
#include "ethercat_subdevice_cpu1_hal.h"
#include "flash_utils.h"
#include "soes/esc.h"
#include "boot_image.h"
#include "boot_f28p65x.h"

#define BOOT_REQUEST_MAGIC 0xB007B007UL

/* Both linker files map this four-word NOINIT section at CPU word address
 * 0x0003FC. The application commits the magic after writing its inverse. */
__attribute__((section(".boot_request")))
static volatile struct {
    uint32_t magic;
    uint32_t magic_inverse;
} boot_request;

static void reset_device(void)
{
    DINT;
    SysCtl_resetDevice();
    for (;;) {}
}

static bool force_recovery;
static uint16_t read_word(uint32_t address)
{
    return *(const volatile uint16_t *)address;
}
static bool program_words(const uint16_t *data, uint32_t address, uint32_t count)
{
    return Program_dataFlashSector(data, address, count) == Fapi_Status_Success;
}
static bool erase_words(uint32_t address, uint32_t count)
{
    return Erase_dataFlashSector(address, count) == Fapi_Status_Success;
}
static const boot_flash_io_t flash_io = {read_word, program_words, erase_words};

bool boot_capture_request(void)
{
    force_recovery = (boot_request.magic == BOOT_REQUEST_MAGIC) &&
                     (boot_request.magic_inverse == ~BOOT_REQUEST_MAGIC);
    boot_request.magic = 0UL;
    boot_request.magic_inverse = 0UL;
    return force_recovery;
}

uint32_t foe_update_prepare(void)
{
    return boot_image_prepare(&flash_io) ? 0UL : 1UL;
}

uint32_t foe_update_validate_and_stage(uint32_t address, uint32_t octets)
{
    return (address == BOOT_APP_WORD_ADDR &&
            boot_image_finish(&flash_io, octets)) ? 0UL : 1UL;
}

void boot_try_start_application(void)
{
    if (!force_recovery && boot_image_valid(&flash_io, true)) {
        /* No timer/ESC interrupts have been started yet. C28 codestart enters
         * _c_int00 and sets up the application's C runtime and stack anew. */
        DINT;
        IER = 0U;
        IFR = 0U;
        asm(" LB 0x0A0000");
        for (;;) {}
    }
}

void boot_post_state_change(uint8_t *as, uint8_t *an)
{
    /* Activation is explicit: the master leaves BOOT only after receiving
     * final ACK. Revalidation happens on reset, including interrupted writes. */
    if (*as == BOOT_TO_INIT && *an == ESCinit) { reset_device(); }
}
