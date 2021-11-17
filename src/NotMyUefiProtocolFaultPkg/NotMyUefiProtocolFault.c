#include <NotMyUefiProtocolFault.h>

#ifdef _MSC_VER
#pragma optimize("", off)
#endif

EFI_GUID DummyGuid =   {0x8BE4DF61, 0x93CA, 0x11d2, {0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0xAA, 0xBB }};

// A hand-rolled implementation for memset()
VOID MySetMem(IN VOID *Buffer, IN UINTN Size, IN UINT8 Value)
{
    UINTN i;
    UINT8 * OutputBuffer = (UINT8 *)Buffer;
    for (i = 0; i < Size; i++) {
        OutputBuffer[i] = Value;
    }
}

EFI_STATUS UninitializedStackMemoryLeak(IN EFI_SYSTEM_TABLE  *SystemTable, UINT32 FaultType)
{
    UINT8 VariableData[32]; // Uninitialized
    UINTN VariableDataSize = sizeof(VariableData);
    UINT32 Attributes = 0;
    EFI_STATUS status = EFI_SUCCESS;
    
    status = SystemTable->RuntimeServices->SetVariable(L"FaultType",
                                                       &DummyGuid,
                                                       Attributes,
                                                       sizeof(FaultType),
                                                       &FaultType);
    if (EFI_ERROR(status)) {
        goto Exit;
    }
    // Read the variable to stack memory.
    // We'll re-use 'FaultType' for this purpose.
    status = SystemTable->RuntimeServices->GetVariable(L"FaultType",
                                                       &DummyGuid,
                                                       &Attributes,
                                                       &VariableDataSize,
                                                       VariableData);
    if (EFI_ERROR(status)) {
        goto Exit;
    }
    
    // Write it back to NVRAM.
    // The bug is that we're using the original maximum size and not the actual size.
    status = SystemTable->RuntimeServices->SetVariable(L"FaultType",
                                                       &DummyGuid,
                                                       Attributes,
                                                       sizeof(VariableData),
                                                       VariableData);
    if (EFI_ERROR(status)) {
        goto Exit;
    }
    
Exit:
    return status;
}


EFI_STATUS
EFIAPI
GenerateBug ( 
    IN EFI_SYSTEM_TABLE *SystemTable, 
    IN UINT32 FaultType
    )
{
    UINT8 * Buffer = NULL;
    UINTN BufferSize = 8;
    VOID * MaybeNull = NULL;
    EFI_STATUS status = EFI_SUCCESS;
    
    // Allocate the vulnerable pool buffer.
    status = SystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                     BufferSize,
                                                     (VOID **)&Buffer);
    if (EFI_ERROR(status)) {
        goto Exit;
    }
  
    if (EFI_ERROR(status)) {
        goto Exit;
    }

    // Carry-out the selected fault.
    switch (FaultType)
    {
    case POOL_OVERFLOW_COPY_MEM:
        SystemTable->BootServices->CopyMem(Buffer, &DummyGuid, BufferSize + 1);
        break;
        
    case POOL_UNDERFLOW_COPY_MEM:
        SystemTable->BootServices->CopyMem(Buffer - 1, &DummyGuid, BufferSize);
        break;
        
    case POOL_OVERFLOW_SET_MEM:
        SystemTable->BootServices->SetMem(Buffer, BufferSize + 1, 0xAA);
        break;

    case POOL_UNDERFLOW_SET_MEM:
        SystemTable->BootServices->SetMem(Buffer - 1, BufferSize, 0xAA);
        break;

    case POOL_OVERFLOW_USER_CODE:
        MySetMem(Buffer, BufferSize + 1, 0xAA);
        break;

    case POOL_UNDERFLOW_USER_CODE:
        MySetMem(Buffer - 1, BufferSize, 0xAA);
        break;

    case POOL_OOB_READ_AHEAD:
        status = *(Buffer + BufferSize);
        break;

    case POOL_OOB_READ_BEHIND:
        status = *(Buffer - 1);
        break;

    case POOL_DOUBLE_FREE:
        SystemTable->BootServices->FreePool(Buffer);
        SystemTable->BootServices->FreePool(Buffer);
        break;

    case POOL_INVALID_FREE:
        SystemTable->BootServices->FreePool(Buffer + 1);
        break;
    
    case POOL_UAF_READ:
        SystemTable->BootServices->FreePool(Buffer);
        status = Buffer[2];
        break;
        
    case POOL_UAF_WRITE:
        SystemTable->BootServices->FreePool(Buffer);
        Buffer[2] = 0xAA;
        break;
        
    case NULL_DEREFERENCE_DETERMINISTIC:
        *(UINT8 *)NULL = 0xAA;
        break;

    case NULL_DEREFERENCE_NON_DETERMINISTIC:
        SystemTable->BootServices->AllocatePool(EfiLoaderData,
                                                BufferSize,
                                                &MaybeNull);
        // We're not checking for the return value from AllocatePool()
        *(UINT8 *)MaybeNull = 0xAA;
        SystemTable->BootServices->FreePool(MaybeNull);
        break;
        
    case STACK_BUFFER_OVERFLOW:
        SystemTable->BootServices->SetMem(&Buffer, 0x100, 0xAA);
        break;
        
    case STACK_UNINITIALIZED_MEMORY_LEAK:
        status = UninitializedStackMemoryLeak(SystemTable, FaultType);
        break;

    default:
        break;
  }

Exit:
  return status;

}


void EFIAPI InitNotMyUefiProtocol() {
    gEfiNotMyUefiProtocolFaultProtocol.Revision = 1;
    gEfiNotMyUefiProtocolFaultProtocol.GenerateBug = GenerateBug;
}


EFI_STATUS
EFIAPI
NotMyUefiProtocolFaultEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS status = EFI_SUCCESS;
    InitNotMyUefiProtocol();
    status = gBS->InstallProtocolInterface(
        &ImageHandle,
        &gEfiNotMyUefiProtocolFaultGUID,
        EFI_NATIVE_INTERFACE,
        &gEfiNotMyUefiProtocolFaultProtocol
    );
    return status;
}