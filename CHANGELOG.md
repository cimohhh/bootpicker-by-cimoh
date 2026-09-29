# Changelog

all notable changes to this project will be documented here.





## v1.0.1 - 9/28/26

### added
- full graphical UEFI boot picker interface.
- Arch Linux and Windows 11 boot options.
- keyboard navigation using `↑`, `↓`, `Enter`, and `Esc`.
- UEFI graphics output protocol (GOP) support.
- automatic preference for 2560×1440 graphics mode.
- fallback to the highest available GOP resolution.
- custom 2560×1440 Arch selected and Windows selected artwork.
- direct EFI chainloading for Arch Linux and Windows 11.
- USB testing support before permanent installation.

### changed
- replaced the previous text based boot picker interface with a graphical UI.
- added full screen pre rendered artwork for smoother text, logos, borders, and glow effects.
- improved display quality on 1440p monitors by using native resolution assets.
- added separate graphical assets for the selected Arch Linux and Windows 11 states.
- organized graphical assets under `BootPicker/Assets/`.

### boot paths
- Arch Linux: `\EFI\Linux\arch-linux.efi`
- Windows 11: `\EFI\Microsoft\Boot\bootmgfw.efi`

### notes
- bootpicker is an unsigned EFI application, so secure boot may need to be disabled unless the binary is signed.
- the graphical version was tested successfully from USB before being installed as the permanent UEFI boot picker.
  




## v1.0.0

- initial public release
- added UEFI boot picker
- added Arch Linux support (cause i use arch btw)
- added Windows 11 support
- added arrow key navigation
- added direct EFI bootloader launching
- added USB testing support
- added permanent UEFI installation instructions
