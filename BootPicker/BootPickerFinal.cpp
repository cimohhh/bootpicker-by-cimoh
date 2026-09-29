extern "C" {

#include <Uefi.h>
#include <Guid/FileInfo.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseLib.h>

// ============================================================
// BOOT TARGETS
// ============================================================

STATIC CONST CHAR16 *ARCH_BOOT_PATH =
    L"\\EFI\\Linux\\arch-linux.efi";

STATIC CONST CHAR16 *WINDOWS_BOOT_PATH =
    L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi";

// ============================================================
// BMP PATHS
// ============================================================

STATIC CONST CHAR16 *ARCH_IMAGE_PATHS[] =
{
    L"\\EFI\\BOOT\\Assets\\ArchSelected.bmp",
    L"\\EFI\\BootPicker\\Assets\\ArchSelected.bmp",
    L"\\Assets\\ArchSelected.bmp"
};

STATIC CONST CHAR16 *WINDOWS_IMAGE_PATHS[] =
{
    L"\\EFI\\BOOT\\Assets\\WindowsSelected.bmp",
    L"\\EFI\\BootPicker\\Assets\\WindowsSelected.bmp",
    L"\\Assets\\WindowsSelected.bmp"
};

// ============================================================
// LITTLE-ENDIAN BMP HELPERS
// ============================================================

STATIC
UINT16
Read16(
    CONST UINT8 *Data
)
{
    return
        (UINT16)Data[0] |
        ((UINT16)Data[1] << 8);
}

STATIC
UINT32
Read32(
    CONST UINT8 *Data
)
{
    return
        (UINT32)Data[0] |
        ((UINT32)Data[1] << 8) |
        ((UINT32)Data[2] << 16) |
        ((UINT32)Data[3] << 24);
}

// ============================================================
// READ A FILE FROM ANY EFI FILESYSTEM
// ============================================================

STATIC
EFI_STATUS
ReadFileFromAnyFileSystem(
    CONST CHAR16 *Path,
    VOID **FileBuffer,
    UINTN *FileSize
)
{
    EFI_STATUS Status;
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;

    if (
        Path == NULL ||
        FileBuffer == NULL ||
        FileSize == NULL
    )
    {
        return EFI_INVALID_PARAMETER;
    }

    *FileBuffer = NULL;
    *FileSize = 0;

    Status =
        gBS->LocateHandleBuffer(
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

    for (
        UINTN Index = 0;
        Index < HandleCount;
        Index++
    )
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
        EFI_FILE_PROTOCOL *Root = NULL;
        EFI_FILE_PROTOCOL *File = NULL;

        Status =
            gBS->HandleProtocol(
                Handles[Index],
                &gEfiSimpleFileSystemProtocolGuid,
                (VOID **)&FileSystem
            );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status =
            FileSystem->OpenVolume(
                FileSystem,
                &Root
            );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status =
            Root->Open(
                Root,
                &File,
                (CHAR16 *)Path,
                EFI_FILE_MODE_READ,
                0
            );

        if (EFI_ERROR(Status))
        {
            Root->Close(Root);
            continue;
        }

        UINTN InfoSize = 0;

        Status =
            File->GetInfo(
                File,
                &gEfiFileInfoGuid,
                &InfoSize,
                NULL
            );

        if (
            Status != EFI_BUFFER_TOO_SMALL ||
            InfoSize == 0
        )
        {
            File->Close(File);
            Root->Close(Root);
            continue;
        }

        EFI_FILE_INFO *FileInfo =
            (EFI_FILE_INFO *)AllocatePool(
                InfoSize
            );

        if (FileInfo == NULL)
        {
            File->Close(File);
            Root->Close(Root);
            FreePool(Handles);
            return EFI_OUT_OF_RESOURCES;
        }

        Status =
            File->GetInfo(
                File,
                &gEfiFileInfoGuid,
                &InfoSize,
                FileInfo
            );

        if (EFI_ERROR(Status))
        {
            FreePool(FileInfo);
            File->Close(File);
            Root->Close(Root);
            continue;
        }

        UINTN Size =
            (UINTN)FileInfo->FileSize;

        FreePool(FileInfo);

        VOID *Buffer =
            AllocatePool(
                Size
            );

        if (Buffer == NULL)
        {
            File->Close(File);
            Root->Close(Root);
            FreePool(Handles);
            return EFI_OUT_OF_RESOURCES;
        }

        UINTN ReadSize = Size;

        Status =
            File->Read(
                File,
                &ReadSize,
                Buffer
            );

        File->Close(File);
        Root->Close(Root);

        if (EFI_ERROR(Status))
        {
            FreePool(Buffer);
            continue;
        }

        *FileBuffer = Buffer;
        *FileSize = ReadSize;

        FreePool(Handles);
        return EFI_SUCCESS;
    }

    FreePool(Handles);
    return EFI_NOT_FOUND;
}

// ============================================================
// TRY MULTIPLE BMP LOCATIONS
// ============================================================

STATIC
EFI_STATUS
ReadImageFile(
    CONST CHAR16 **Paths,
    UINTN PathCount,
    VOID **FileBuffer,
    UINTN *FileSize
)
{
    EFI_STATUS Status =
        EFI_NOT_FOUND;

    for (
        UINTN Index = 0;
        Index < PathCount;
        Index++
    )
    {
        Status =
            ReadFileFromAnyFileSystem(
                Paths[Index],
                FileBuffer,
                FileSize
            );

        if (!EFI_ERROR(Status))
        {
            return EFI_SUCCESS;
        }
    }

    return Status;
}

// ============================================================
// DISPLAY 24-BIT BMP
// ============================================================

STATIC
EFI_STATUS
DisplayBmp(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop,
    CONST CHAR16 **Paths,
    UINTN PathCount
)
{
    EFI_STATUS Status;
    VOID *RawFile = NULL;
    UINTN RawFileSize = 0;

    Status =
        ReadImageFile(
            Paths,
            PathCount,
            &RawFile,
            &RawFileSize
        );

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    UINT8 *Data =
        (UINT8 *)RawFile;

    if (RawFileSize < 54)
    {
        FreePool(RawFile);
        return EFI_UNSUPPORTED;
    }

    if (
        Data[0] != 'B' ||
        Data[1] != 'M'
    )
    {
        FreePool(RawFile);
        return EFI_UNSUPPORTED;
    }

    UINT32 PixelOffset =
        Read32(
            Data + 10
        );

    INT32 Width =
        (INT32)Read32(
            Data + 18
        );

    INT32 Height =
        (INT32)Read32(
            Data + 22
        );

    UINT16 Planes =
        Read16(
            Data + 26
        );

    UINT16 BitsPerPixel =
        Read16(
            Data + 28
        );

    UINT32 Compression =
        Read32(
            Data + 30
        );

    if (
        Width <= 0 ||
        Height == 0 ||
        Planes != 1 ||
        BitsPerPixel != 24 ||
        Compression != 0
    )
    {
        FreePool(RawFile);
        return EFI_UNSUPPORTED;
    }

    BOOLEAN BottomUp =
        TRUE;

    UINTN SourceHeight;

    if (Height < 0)
    {
        BottomUp = FALSE;
        SourceHeight =
            (UINTN)(-Height);
    }
    else
    {
        SourceHeight =
            (UINTN)Height;
    }

    UINTN SourceWidth =
        (UINTN)Width;

    UINTN SourceStride =
        ((SourceWidth * 3 + 3) / 4) * 4;

    if (
        PixelOffset >= RawFileSize ||
        PixelOffset +
        SourceStride * SourceHeight >
        RawFileSize
    )
    {
        FreePool(RawFile);
        return EFI_UNSUPPORTED;
    }

    UINTN ScreenWidth =
        Gop->Mode->Info->HorizontalResolution;

    UINTN ScreenHeight =
        Gop->Mode->Info->VerticalResolution;

    UINTN PixelCount =
        ScreenWidth *
        ScreenHeight;

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Screen =
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)
        AllocatePool(
            PixelCount *
            sizeof(
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL
            )
        );

    if (Screen == NULL)
    {
        FreePool(RawFile);
        return EFI_OUT_OF_RESOURCES;
    }

    UINT8 *PixelData =
        Data +
        PixelOffset;

    for (
        UINTN Y = 0;
        Y < ScreenHeight;
        Y++
    )
    {
        UINTN SourceY =
            Y *
            SourceHeight /
            ScreenHeight;

        if (BottomUp)
        {
            SourceY =
                SourceHeight -
                1 -
                SourceY;
        }

        UINT8 *SourceRow =
            PixelData +
            SourceY *
            SourceStride;

        for (
            UINTN X = 0;
            X < ScreenWidth;
            X++
        )
        {
            UINTN SourceX =
                X *
                SourceWidth /
                ScreenWidth;

            UINT8 *SourcePixel =
                SourceRow +
                SourceX * 3;

            EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Destination =
                &Screen[
                    Y *
                    ScreenWidth +
                    X
                ];

            Destination->Blue =
                SourcePixel[0];

            Destination->Green =
                SourcePixel[1];

            Destination->Red =
                SourcePixel[2];

            Destination->Reserved =
                0;
        }
    }

    Status =
        Gop->Blt(
            Gop,
            Screen,
            EfiBltBufferToVideo,
            0,
            0,
            0,
            0,
            ScreenWidth,
            ScreenHeight,
            0
        );

    FreePool(Screen);
    FreePool(RawFile);

    return Status;
}

// ============================================================
// CHOOSE BEST GRAPHICS MODE
//
// First try 2560x1440.
// If unavailable, choose the mode with the most pixels.
// ============================================================

STATIC
EFI_STATUS
SetBestGraphicsMode(
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop
)
{
    EFI_STATUS Status;

    UINT32 BestMode =
        Gop->Mode->Mode;

    UINTN BestPixels = 0;

    for (
        UINT32 Mode = 0;
        Mode < Gop->Mode->MaxMode;
        Mode++
    )
    {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info = NULL;
        UINTN InfoSize = 0;

        Status =
            Gop->QueryMode(
                Gop,
                Mode,
                &InfoSize,
                &Info
            );

        if (
            EFI_ERROR(Status) ||
            Info == NULL
        )
        {
            continue;
        }

        // Prefer the native resolution of your 1440p monitor.
        if (
            Info->HorizontalResolution == 2560 &&
            Info->VerticalResolution == 1440
        )
        {
            BestMode = Mode;

            FreePool(Info);

            return
                Gop->SetMode(
                    Gop,
                    BestMode
                );
        }

        UINTN Pixels =
            (UINTN)Info->HorizontalResolution *
            (UINTN)Info->VerticalResolution;

        if (Pixels > BestPixels)
        {
            BestPixels = Pixels;
            BestMode = Mode;
        }

        FreePool(Info);
    }

    return
        Gop->SetMode(
            Gop,
            BestMode
        );
}

// ============================================================
// BOOT EFI FILE
// ============================================================

STATIC
EFI_STATUS
BootEfiFile(
    EFI_HANDLE ImageHandle,
    CONST CHAR16 *Path
)
{
    EFI_STATUS Status;
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;

    Status =
        gBS->LocateHandleBuffer(
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

    for (
        UINTN Index = 0;
        Index < HandleCount;
        Index++
    )
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
        EFI_FILE_PROTOCOL *Root = NULL;
        EFI_FILE_PROTOCOL *File = NULL;

        Status =
            gBS->HandleProtocol(
                Handles[Index],
                &gEfiSimpleFileSystemProtocolGuid,
                (VOID **)&FileSystem
            );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status =
            FileSystem->OpenVolume(
                FileSystem,
                &Root
            );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status =
            Root->Open(
                Root,
                &File,
                (CHAR16 *)Path,
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

        EFI_DEVICE_PATH_PROTOCOL *DevicePath =
            FileDevicePath(
                Handles[Index],
                (CHAR16 *)Path
            );

        if (DevicePath == NULL)
        {
            continue;
        }

        EFI_HANDLE NewImageHandle = NULL;

        Status =
            gBS->LoadImage(
                FALSE,
                ImageHandle,
                DevicePath,
                NULL,
                0,
                &NewImageHandle
            );

        FreePool(DevicePath);

        if (EFI_ERROR(Status))
        {
            continue;
        }

        FreePool(Handles);

        return
            gBS->StartImage(
                NewImageHandle,
                NULL,
                NULL
            );
    }

    FreePool(Handles);
    return EFI_NOT_FOUND;
}

// ============================================================
// MAIN
// ============================================================

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
    EFI_INPUT_KEY Key;
    UINTN EventIndex;
    INTN Selected = 0;

    Status =
        gBS->LocateProtocol(
            &gEfiGraphicsOutputProtocolGuid,
            NULL,
            (VOID **)&Gop
        );

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    // Try 2560x1440 first, otherwise highest available GOP mode.
    SetBestGraphicsMode(Gop);

    if (SystemTable->ConOut != NULL)
    {
        SystemTable->ConOut->EnableCursor(
            SystemTable->ConOut,
            FALSE
        );
    }

    // Prevent the Enter key from the motherboard boot menu
    // from immediately selecting Arch.
    gBS->Stall(
        500000
    );

    while (
        SystemTable->ConIn->ReadKeyStroke(
            SystemTable->ConIn,
            &Key
        ) == EFI_SUCCESS
    )
    {
    }

    // Initial screen = Arch selected.
    DisplayBmp(
        Gop,
        ARCH_IMAGE_PATHS,
        sizeof(ARCH_IMAGE_PATHS) /
        sizeof(ARCH_IMAGE_PATHS[0])
    );

    while (TRUE)
    {
        Status =
            gBS->WaitForEvent(
                1,
                &SystemTable->ConIn->WaitForKey,
                &EventIndex
            );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status =
            SystemTable->ConIn->ReadKeyStroke(
                SystemTable->ConIn,
                &Key
            );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        if (Key.ScanCode == SCAN_UP)
        {
            Selected = 0;

            DisplayBmp(
                Gop,
                ARCH_IMAGE_PATHS,
                sizeof(ARCH_IMAGE_PATHS) /
                sizeof(ARCH_IMAGE_PATHS[0])
            );
        }
        else if (
            Key.ScanCode ==
            SCAN_DOWN
        )
        {
            Selected = 1;

            DisplayBmp(
                Gop,
                WINDOWS_IMAGE_PATHS,
                sizeof(WINDOWS_IMAGE_PATHS) /
                sizeof(WINDOWS_IMAGE_PATHS[0])
            );
        }
        else if (
            Key.UnicodeChar ==
            CHAR_CARRIAGE_RETURN
        )
        {
            if (Selected == 0)
            {
                BootEfiFile(
                    ImageHandle,
                    ARCH_BOOT_PATH
                );
            }
            else
            {
                BootEfiFile(
                    ImageHandle,
                    WINDOWS_BOOT_PATH
                );
            }
        }
        else if (
            Key.ScanCode ==
            SCAN_ESC
        )
        {
            return EFI_SUCCESS;
        }
    }
}

}