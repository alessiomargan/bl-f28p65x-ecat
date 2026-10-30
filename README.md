# BL-F28P65X-ECAT

EtherCAT File-over-EtherCAT (FoE) recovery bootloader for the Texas
Instruments TMS320F28P65x CPU1. The bootloader occupies flash bank 0, receives
a packaged application through SOES/FoE, validates it, commits it atomically,
and starts the application from flash bank 1.

The companion application project is
[`f28p65x-soes`](https://github.com/alessiomargan/F28P65X-soes). It must be
linked for C28 word address `0x0A0000` and packaged with the image tool in this
repository before it is transferred.

## Features

- Dedicated recovery bootloader in flash bank 0.
- SOES-based EtherCAT slave with FoE write support.
- Exact FoE filename and password checks.
- Board-specific image target identifiers for controlCARD and LaunchPad.
- CRC-32 validation of the application payload and manifest.
- Commit marker written only after the complete image has been verified.
- Startup validation before any application jump.
- Recovery after an interrupted erase, write, or commit operation.
- Host tests for the production FoE callbacks and image validator.
- Pinned checksum verification for the reviewed SOES FoE implementation.

This is a single-slot recovery design, not an A/B updater. Starting a new
upload invalidates the installed application; an interrupted update leaves the
device in the bootloader so the master can retry.

## Supported targets

| Configuration | Hardware | Image target |
| --- | --- | --- |
| `CC_BLDR` | F28P65x controlCARD | `0xF28A6501` |
| `LP_BLDR` | F28P65x LaunchPad | `0xF28A6502` |

The default Makefile configuration is `CC_BLDR`.

## Flash layout

F28P65x addresses and sizes below are expressed in 16-bit C28 words unless
explicitly marked as octets.

| Region | Start | Size | Purpose |
| --- | ---: | ---: | --- |
| Bootloader | `0x080000` | `0x20000` words | Bank 0 executable and recovery path |
| Application slot | `0x0A0000` | `0x20000` words | Bank 1 FoE update slot |
| Application payload | `0x0A0000` | `0x1F800` words | Bank-1-linked application and `0xFFFF` padding |
| Manifest sector | `0x0BF800` | `0x400` words | 32-octet manifest at the start of the sector |
| Commit sector | `0x0BFC00` | `0x400` words | Commit marker, programmed last |
| Boot request | `0x0003FC` | `4` words | Shared NOINIT RAM handoff mailbox |

The application entry point is fixed at C28 word address `0x0A0000`.

## Repository layout

| Path | Description |
| --- | --- |
| `src/` | Bootloader, EtherCAT HAL, SOES hooks, FoE callbacks and flash helpers |
| `linker/` | Bootloader and generic C28 linker command files |
| `external/SOES/` | SOES Git submodule pinned to the reviewed FoE implementation |
| `tools/pack_foe_image.py` | Creates the validated fixed-size FoE image |
| `tools/check_foe_stack.py` | Verifies hashes of security-relevant SOES sources |
| `tools/gen_build_info.sh` | Generates `src/build_info.h` during the CCS pre-build step |
| `tests/` | Host-side update, interruption and corruption tests |
| `doc/` | Design notes and device/reference documentation |

## Prerequisites

- Code Composer Studio 21.0 or compatible CCS installation.
- TI C2000 compiler and C2000Ware versions configured by the CCS project.
- Python 3 for image packaging and host tests.
- A host C compiler (`cc`) for `make foe-test`.
- UniFlash/dslite when using the Makefile flash targets.

Local CCS and UniFlash defaults are stored in `Makefile.linux_local` and
`Makefile.darwin_local`. Every value can be overridden on the command line or
through the environment.

## Clone

Clone the repository together with SOES:

```sh
git clone --recurse-submodules git@github.com:alessiomargan/bl-f28p65x-ecat.git
cd bl-f28p65x-ecat
```

For an existing clone:

```sh
git submodule update --init --recursive
```

Do not update `external/SOES` or `tools/foe_stack.sha256.json` without
reviewing the FoE completion/error paths and rerunning the host tests.

## Build

The project can be imported and built in CCS using `CC_BLDR` or `LP_BLDR`.
The helper Makefile runs the CCS headless project builder:

```sh
# controlCARD (default)
make build CONFIG=CC_BLDR

# LaunchPad
make build CONFIG=LP_BLDR

# Clean full rebuild
make rebuild CONFIG=CC_BLDR
```

Useful overrides include:

```sh
make build CONFIG=CC_BLDR \
  CCS_INSTALL_DIR=/path/to/ccs2100 \
  CCS_WORKSPACE=/tmp/f28p65x-bootloader-workspace
```

The resulting executable is:

```text
<CONFIG>/bl-f28p65x-ecat.out
```

## Initial programming

The bootloader must first be programmed into bank 0 with CCS, UniFlash, or the
Makefile flash target:

```sh
make flash CONFIG=CC_BLDR
```

Verify the selected UniFlash erase policy before programming: installing the
bootloader must not unintentionally erase an application or persistent data in
other banks. `TARGET_CONFIG_FILE`, `DSLITE`, and `CORE` can be overridden when
needed.

With no valid committed application, the bootloader remains in EtherCAT
recovery mode.

## Create an FoE image

The input must be a flat binary produced from an application linked at C28
word address `0x0A0000`. It is not sufficient to rename a CCS `.out` file or
an unverified raw binary.

```sh
make foe-package \
  APP_BIN=/path/to/f28p65x-soes.bin \
  FOE_BOARD=cc \
  FOE_IMAGE=f28p65x.bin
```

Use `FOE_BOARD=lp` for the LaunchPad. The equivalent direct command is:

```sh
python3 tools/pack_foe_image.py \
  --board cc \
  /path/to/f28p65x-soes.bin \
  f28p65x.bin
```

The generated FoE file has these fixed properties:

| Property | Value |
| --- | --- |
| Filename | `f28p65x.bin` |
| Password | `0xF280` |
| Transfer size | `258080` octets |
| Payload byte order | Low octet, then high octet for each C28 word |
| Payload CRC | CRC-32/ISO-HDLC, compatible with `zlib.crc32()` |

The 32-octet manifest contains the magic, format version, board target,
payload length, entry point, payload CRC, reserved flags, and manifest CRC.
The commit marker is not part of the transferred file; the bootloader writes
it into the separate commit sector only after validation succeeds.

## FoE update flow

1. The application receives an EtherCAT transition to BOOT, writes the
   retained boot request and resets the CPU.
2. The bootloader consumes that request and remains in recovery instead of
   starting the currently installed application.
3. The master places the slave in Bootstrap and writes `f28p65x.bin` with
   password `0xF280`.
4. The bootloader erases the commit sector first, then erases and programs the
   application region sequentially.
5. On close, it validates the exact transfer length, manifest, board target,
   entry point, payload CRC and erased padding.
6. It writes and verifies the commit marker last. SOES sends the final ACK
   only when the complete update is valid.
7. Leaving BOOT for INIT resets the CPU. Startup validation runs again before
   branching to the application at `0x0A0000`.

The application and bootloader linker files must reserve the same
`.boot_request` NOINIT section at word address `0x0003FC`.

## Verification

Verify that the checked-out SOES sources match the reviewed version:

```sh
make foe-check
```

Run the host-side FoE and image-validation tests:

```sh
make foe-test
```

The tests use simulated mailbox and flash storage. They cover successful
updates, malformed requests, transfer aborts, payload/manifest corruption,
program and erase failures, reset during an upload, and torn commit markers.
They do not replace target testing of the TI compiler, Flash API, ECC,
retention RAM, reset causes, link map, power interruption, or the application
jump.

## Security and operational limits

- CRC detects accidental corruption but does not authenticate firmware.
- The FoE password is an access check, not a cryptographic signature.
- There is no downgrade prevention, secure boot integration, or encrypted
  transport.
- There is no A/B rollback or first-boot confirmation.
- A valid image that hangs after startup requires an external recovery policy.
- Flash and power-failure behavior must be qualified on the target hardware
  before production use.

See [`doc/FOE_BOOTLOADER.md`](doc/FOE_BOOTLOADER.md) for the design history,
image format rationale, test scope and hardware-validation checklist.

## License

Project code is provided under the MIT License; see [`LICENSE.md`](LICENSE.md).
Third-party components retain their respective licenses.
