# AsterOS 0.1 — Bootable OS for web VMs

AsterOS is a small **real x86 32-bit bare-metal kernel**, not an app, Linux distribution, or simulated website. GRUB loads the kernel from a bootable ISO. The OS uses VGA text output and PS/2 keyboard input to provide a simple interactive terminal.

## Build the ISO entirely through GitHub (no downloads/installations needed to compile)

1. Create a new GitHub repository (e.g. `AsterOS`).
2. **Upload the contents of this folder to the root of the repository**, retaining the hidden `.github/workflows/build-iso.yml` path. Don't upload only the outer ZIP without extracting it.
3. Commit the files to the `main` branch.
4. On GitHub, open **Actions** > **Build AsterOS ISO**. Click **Run workflow** and run on `main` (or use the automatically started build from the push).
5. Open the successful run. Under **Artifacts**, click **AsterOS-bootable-ISO**. GitHub downloads a ZIP containing `asteros.iso`.
6. Extract that downloaded artifact ZIP. **Upload `asteros.iso`** (not the ZIP) as an ISO/CD-ROM image in your browser x86 virtual machine.

**IMPORTANT:** Some web emulators are set up only for standard Linux images or lack legacy BIOS/Multiboot support. AsterOS requires a 32-bit x86 emulator with legacy BIOS/GRUB boot, VGA text mode, and an emulated PS/2 keyboard. Browser emulation performance and boot compatibility vary. Use 128 MB RAM if configurable and choose CD-ROM/ISO as boot media. Disable UEFI if offered.

## Build locally (optional)

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y gcc binutils make grub-pc-bin grub-common xorriso mtools
make iso
```

Output: `build/asteros.iso`. To test with QEMU (optional): `sudo apt install qemu-system-x86 && make run`.

## Commands

- `help` — command list
- `clear` — clear terminal
- `about` — OS details
- `mem` — show RAM info
- `echo hello` — display text
- `color 10` — change terminal foreground color
- `reboot` — reboot virtual machine
- `halt` — halt CPU

## Directory layout

```
.github/workflows/build-iso.yml  GitHub Actions build and artifact upload
src/boot.S                      Multiboot header and x86 entry point
src/kernel.c                    VGA console, keyboard, shell
iso/boot/grub/grub.cfg          GRUB boot entry
linker.ld                       Kernel memory layout
Makefile                        Build, ISO, check, run commands
.gitignore                      Exclude generated files
```

This is an educational starter operating system. There is no filesystem, GUI, USB input, networking, process scheduler, or disk installation support yet.
