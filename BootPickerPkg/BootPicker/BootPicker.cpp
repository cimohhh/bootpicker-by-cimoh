extern "C" {

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>


EFI_STATUS
BootEfiFile(
    EFI_HANDLE ImageHandle,
    CHAR16 *Path
)
{
    EFI_STATUS Status;
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;

    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
    );

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    for (UINTN i = 0; i < HandleCount; i++)
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
        EFI_FILE_PROTOCOL *Root;
        EFI_FILE_PROTOCOL *File;

        Status = gBS->HandleProtocol(
            Handles[i],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID **)&FileSystem
        );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status = FileSystem->OpenVolume(
            FileSystem,
            &Root
        );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        //
        // Check whether this filesystem contains the EFI file.
        //
        Status = Root->Open(
            Root,
            &File,
            Path,
            EFI_FILE_MODE_READ,
            0
        );

        if (EFI_ERROR(Status))
        {
            Root->Close(Root);
            continue;
        }

        File->Close(File);
        Root->Close(Root);

        //
        // We found it. Create a UEFI device path to it.
        //
        EFI_DEVICE_PATH_PROTOCOL *DevicePath =
            FileDevicePath(Handles[i], Path);

        if (DevicePath == NULL)
        {
            continue;
        }

        EFI_HANDLE BootImage = NULL;

        Status = gBS->LoadImage(
            FALSE,
            ImageHandle,
            DevicePath,
            NULL,
            0,
            &BootImage
        );

        FreePool(DevicePath);

        if (EFI_ERROR(Status))
        {
            FreePool(Handles);
            return Status;
        }

        //
        // Start the selected operating system's EFI loader.
        //
        Status = gBS->StartImage(
            BootImage,
            NULL,
            NULL
        );

        FreePool(Handles);
        return Status;
    }

    FreePool(Handles);

    return EFI_NOT_FOUND;
}


EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_INPUT_KEY Key;
    INTN Selected = 0;
    UINTN EventIndex;

    //
    // Prevent the Enter key from the motherboard boot menu
    // from immediately selecting something.
    //
    gBS->Stall(500000);

    while (
        SystemTable->ConIn->ReadKeyStroke(
            SystemTable->ConIn,
            &Key
        ) == EFI_SUCCESS
    )
    {
    }


    while (TRUE)
    {
        SystemTable->ConOut->ClearScreen(
            SystemTable->ConOut
        );

        Print(L"========================\r\n");
        Print(L"      BOOT PICKER\r\n");
        Print(L"========================\r\n\r\n");

        if (Selected == 0)
        {
            Print(L"> Arch Linux\r\n");
            Print(L"  Windows 11\r\n");
        }
        else
        {
            Print(L"  Arch Linux\r\n");
            Print(L"> Windows 11\r\n");
        }

        Print(L"\r\n");
        Print(L"Up/Down = Move\r\n");
        Print(L"Enter   = Boot\r\n");
        Print(L"Esc     = Exit\r\n");


        gBS->WaitForEvent(
            1,
            &SystemTable->ConIn->WaitForKey,
            &EventIndex
        );

        if (
            SystemTable->ConIn->ReadKeyStroke(
                SystemTable->ConIn,
                &Key
            ) != EFI_SUCCESS
        )
        {
            continue;
        }


        if (
            Key.ScanCode == SCAN_UP ||
            Key.ScanCode == SCAN_DOWN
        )
        {
            Selected = 1 - Selected;
        }

        else if (
            Key.UnicodeChar == CHAR_CARRIAGE_RETURN
        )
        {
            SystemTable->ConOut->ClearScreen(
                SystemTable->ConOut
            );

            EFI_STATUS Status;

            if (Selected == 0)
            {
                Print(L"Starting Arch Linux...\r\n");

                Status = BootEfiFile(
                    ImageHandle,
                    (CHAR16 *)L"\\EFI\\Linux\\arch-linux.efi"
                );
            }
            else
            {
                Print(L"Starting Windows 11...\r\n");

                Status = BootEfiFile(
                    ImageHandle,
                    (CHAR16 *)L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi"
                );
            }


            //
            // Normally a successful OS boot won't return here.
            // If it does, display the error.
            //
            Print(
                L"\r\nBoot loader returned: %r\r\n",
                Status
            );

            Print(
                L"Press any key to return to Boot Picker.\r\n"
            );

            gBS->WaitForEvent(
                1,
                &SystemTable->ConIn->WaitForKey,
                &EventIndex
            );

            SystemTable->ConIn->ReadKeyStroke(
                SystemTable->ConIn,
                &Key
            );
        }

        else if (Key.ScanCode == SCAN_ESC)
        {
            return EFI_SUCCESS;
        }
    }
}

}