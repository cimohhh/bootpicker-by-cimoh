# bootpicker-cimoh

this is a custom UEFI boot picker written in C++ using TianoCore EDK II for dual booting Arch Linux and Windows 11. 
obviously its amazing because it was made by me, give it a try BUT before you do anything make sure to look at this README (VERY IMPORTANT!). 
this was made specifically for my dual boot system, your system may be different from mine and its very important that you configure this to work on YOUR system.

## features

- runs as a real UEFI application before the operating system starts
- lets you choose between Arch Linux and Windows 11
- uses the arrow keys to move between operating systems
- press Enter to boot the selected operating system
- press Esc to exit
- launches EFI bootloaders directly
- can be tested from a USB
- can be installed permanently as its own UEFI boot entry

## controls

- `Up Arrow` / `Down Arrow` — change selection
- `Enter` — boot selected operating system
- `Esc` — exit the boot picker

## default EFI paths

the current version is configured to use these paths:

### Arch Linux

```text
\EFI\Linux\arch-linux.efi
```

### Windows 11

```text
\EFI\Microsoft\Boot\bootmgfw.efi
```

your Linux EFI path may be different depending on how your system is configured.

if your Arch EFI file is stored somewhere else, edit the path inside `BootPicker.cpp` before building.

## project structure

```text
BootPickerPkg/
├── BootPickerPkg.dsc
└── BootPicker/
    ├── BootPicker.cpp
    └── BootPicker.inf
```

## what each file does

### BootPicker.cpp

this is the actual boot picker program.

it:

- draws the boot menu
- reads keyboard input
- moves the selection with the arrow keys
- launches the selected EFI bootloader
- boots either Arch Linux or Windows 11

### BootPicker.inf

this tells EDK II that the project is a UEFI application.

it defines:

- the source file
- required packages
- required UEFI libraries
- the application entry point

### BootPickerPkg.dsc

this is the main EDK II build configuration.

it defines:

- the platform
- target architecture
- libraries
- build options
- project components

## Requirements

to build the project, you will need:

- Windows
- TianoCore EDK II
- Visual Studio Build Tools
- NASM
- Python
- Git

## building

place `BootPickerPkg` inside your EDK II directory.

example:

```text
edk2/
└── BootPickerPkg/
```

open a visual studio developer command prompt.

change into your EDK II directory:

```cmd
cd /d C:\path\to\edk2
```

add NASM to your PATH if needed:

```cmd
set PATH=C:\path\to\NASM;%PATH%
```

set up EDK II:

```cmd
edksetup.bat
```

then build:

```cmd
build -a X64 -t VS2026 -b DEBUG -p BootPickerPkg\BootPickerPkg.dsc
```

if the build succeeds, you should see:

```text
- Done -
```

the compiled EFI application should appear at:

```text
Build\BootPicker\DEBUG_VS2026\X64\BootPicker.efi
```

## testing from a USB

format a USB drive as FAT32.

create this folder structure:

```text
EFI\BOOT
```

copy:

```text
BootPicker.efi
```

into:

```text
EFI\BOOT
```

then rename it to:

```text
BOOTX64.EFI
```

the final USB layout should look like this:

```text
USB
└── EFI
    └── BOOT
        └── BOOTX64.EFI
```

then reboot your PC and select the USB from your motherboard's UEFI boot menu.

## permanent installation

boot Picker can also be installed permanently on the EFI System Partition.

a safe layout is:

```text
\EFI\BootPicker\BootPicker.efi
```

do not replace the Windows or Linux EFI files.

### example on Linux

copy `BootPicker.efi` to your EFI system partition.

for example:

```text
/boot/EFI/BootPicker/BootPicker.efi
```

then create a new UEFI boot entry with `efibootmgr`.

example:

```bash
sudo efibootmgr -c -d /dev/nvme0n1 -p 1 -L "Boot Picker" -l '\EFI\BootPicker\BootPicker.efi'
```

check your current boot entries:

```bash
sudo efibootmgr
```

you should see an entry similar to:

```text
Boot0001* Boot Picker
```

if you want Boot Picker to launch automatically when your PC starts, place its boot entry first in the firmware boot order.

your disk and EFI partition may be different, so check your own system before running installation commands.

## important warning (i mean it, VERY IMPORTANT! OKAY?)

do not overwrite, rename, or delete your existing Windows or Linux EFI bootloaders.

files such as these should remain untouched:

```text
\EFI\Microsoft\Boot\bootmgfw.efi
\EFI\systemd\systemd-bootx64.efi
\EFI\Linux\arch-linux.efi
```

the safest method is to install boot picker as its own EFI application and create a separate UEFI firmware boot entry for it.

## secure boot

the current EFI binary is not signed by default.

because of this, systems with secure boot enabled may refuse to launch it.

if the application does not start, check whether secure boot is enabled.

be careful when changing secure boot settings, especially on systems using bitLocker (honestly disable that bullsht PLEASE) or device encryption.

## compatibility

this project was originally created and tested with:

- Arch Linux
- Windows 11
- systemd-boot
- UEFI firmware
- x86-64 hardware

other Linux distributions and bootloader configurations may require changing the EFI paths in `BootPicker.cpp`.

## my future ideas

- automatic EFI bootloader detection
- config file support
- support for more operating systems
- custom colors
- boot timeout
- default operating system selection
- better error messages
- graphical UEFI interface
- secure boot signing support

## license

this project is licensed under the MIT License.
