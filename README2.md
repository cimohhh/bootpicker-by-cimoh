# BootPicker

A custom graphical **UEFI boot picker** for dual-booting **Arch Linux and Windows 11**, built in C++ with EDK II.

BootPicker runs before either operating system starts and provides a simple keyboard-controlled graphical menu for choosing which OS to boot.

![Arch Linux selected](BootPicker/Assets/ArchSelectedFinal.png)

![Windows 11 selected](BootPicker/Assets/WindowsSelectedFinal.png)

## Features

- Native UEFI application
- Graphical interface using UEFI GOP
- 2560×1440 boot screen artwork
- Arch Linux and Windows 11 support
- Keyboard navigation
- Automatically prefers a 2560×1440 GOP mode
- Falls back to the highest available GOP resolution
- Directly chainloads the selected EFI bootloader
- No GRUB required
- Can be safely tested from a USB drive before installation

## Controls

| Key | Action |
| --- | --- |
| `↑` | Select Arch Linux |
| `↓` | Select Windows 11 |
| `Enter` | Boot selected operating system |
| `Esc` | Exit BootPicker |

## Default boot paths

BootPicker currently expects:

### Arch Linux

```text
\EFI\Linux\arch-linux.efi
```

### Windows 11

```text
\EFI\Microsoft\Boot\bootmgfw.efi
```

These paths can be changed near the top of:

```text
BootPicker/BootPickerFinal.cpp
```

## Project structure

```text
bootpicker-cimoh/
│
├── BootPickerFinalPkg.dsc
│
├── BootPicker/
│   ├── BootPickerFinal.cpp
│   ├── BootPickerFinal.inf
│   │
│   └── Assets/
│       ├── ArchSelected.bmp
│       ├── WindowsSelected.bmp
│       ├── ArchSelectedFinal.png
│       └── WindowsSelectedFinal.png
│
├── README.md
└── LICENSE
```

The PNG files are the source/display versions of the artwork.

The BMP files are the versions loaded directly by BootPicker while running in UEFI.

## Requirements

To build BootPicker on Windows:

- Windows 11
- EDK II
- Visual Studio Build Tools
- NASM
- Python
- Git

The current project was built using the `VS2026` EDK II toolchain.

## Building

Clone EDK II first:

```cmd
git clone https://github.com/tianocore/edk2.git
cd edk2
```

Initialize the EDK II submodules if required:

```cmd
git submodule update --init
```

Clone BootPicker directly into the EDK II workspace as `BootPickerPkg`:

```cmd
git clone https://github.com/cimohhh/bootpicker-cimoh.git BootPickerPkg
```

Open a Visual Studio Developer Command Prompt and go to the EDK II directory:

```cmd
cd /d C:\path\to\edk2
```

Make sure NASM is available in `PATH`, then initialize EDK II:

```cmd
edksetup.bat
```

Build BootPicker:

```cmd
build -a X64 -t VS2026 -b DEBUG -p BootPickerPkg\BootPickerFinalPkg.dsc
```

After a successful build, the EFI executable should be located at:

```text
Build\BootPickerFinal\DEBUG_VS2026\X64\BootPicker.efi
```

## Assets

BootPicker uses two 24-bit BMP files:

```text
ArchSelected.bmp
WindowsSelected.bmp
```

The final artwork is designed for:

```text
2560 × 1440
RGB
24-bit BMP
```

BootPicker searches several locations for its images, including:

```text
\EFI\BootPicker\Assets\
\EFI\BOOT\Assets\
\Assets\
```

For a normal permanent installation, the recommended layout is:

```text
EFI\
└── BootPicker\
    ├── BootPicker.efi
    └── Assets\
        ├── ArchSelected.bmp
        └── WindowsSelected.bmp
```

## Testing from USB

Testing from USB is recommended before installing BootPicker to your internal EFI System Partition.

Format a USB drive as a UEFI-readable filesystem such as FAT32.

Create:

```text
EFI\
├── BOOT\
│   └── BOOTX64.EFI
│
└── BootPicker\
    └── Assets\
        ├── ArchSelected.bmp
        └── WindowsSelected.bmp
```

Copy the compiled BootPicker EFI executable to:

```text
EFI\BOOT\BOOTX64.EFI
```

Then copy both BMP files to:

```text
EFI\BootPicker\Assets\
```

Reboot the computer and select the USB's **UEFI** entry from the motherboard boot menu.

This allows BootPicker to be tested without replacing the existing internal boot configuration.

## Permanent installation

> **Warning**
>
> Be careful when modifying the EFI System Partition. Do not delete or overwrite your Windows or Linux bootloader files.

A typical internal EFI layout may look like:

```text
EFI\
├── BootPicker\
│   ├── BootPicker.efi
│   └── Assets\
│       ├── ArchSelected.bmp
│       └── WindowsSelected.bmp
│
├── Linux\
│   └── arch-linux.efi
│
├── Microsoft\
│   └── Boot\
│       └── bootmgfw.efi
│
└── systemd\
```

BootPicker should be installed separately from the operating-system bootloaders.

Do **not** overwrite:

```text
EFI\Microsoft\
EFI\Linux\
EFI\systemd\
```

Once installed, create a UEFI firmware boot entry pointing to:

```text
\EFI\BootPicker\BootPicker.efi
```

## How it works

BootPicker uses the UEFI Graphics Output Protocol to display the graphical menu.

At startup it:

1. Locates the UEFI Graphics Output Protocol.
2. Searches for a 2560×1440 graphics mode.
3. Falls back to the available mode with the highest pixel count if 2560×1440 is unavailable.
4. Loads the selected-state BMP from an EFI filesystem.
5. Displays the artwork directly through GOP.
6. Waits for keyboard input.
7. Loads and starts the selected operating system's EFI executable.

The graphical interface itself is stored as pre-rendered images, allowing smoother text, logos, borders, and effects than firmware fonts would normally provide.

## Secure Boot

BootPicker is currently an unsigned custom EFI application.

Systems with Secure Boot enabled may refuse to launch it unless the EFI binary is signed with a trusted key.

If BootPicker does not launch, check the system's Secure Boot configuration.

## Current targets

The default configuration is designed for:

- Arch Linux
- Windows 11
- x86-64 UEFI systems

Other Linux distributions can be used by changing the Linux EFI path in the source code.

## Disclaimer

Modifying EFI partitions and firmware boot entries can make an operating system temporarily unbootable if done incorrectly.

Always keep a known-good boot method available and test BootPicker from USB before making it the default firmware boot entry.

## License

See [LICENSE](LICENSE).
