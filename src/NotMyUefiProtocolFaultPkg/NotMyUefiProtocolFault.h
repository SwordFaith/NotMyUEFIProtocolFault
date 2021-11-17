#ifndef NOTMYUEFIPROTOCOLFAULT_H
#define NOTMYUEFIPROTOCOLFAULT_H

typedef enum _FAULT_TYPE_T
{
    // Triggers an overflow from BS->CopyMem()
    POOL_OVERFLOW_COPY_MEM = 1,
    // Triggers an underflow from BS->CopyMem()
    POOL_UNDERFLOW_COPY_MEM = 2,
    // Triggers an overflow from BS->SetMem()
    POOL_OVERFLOW_SET_MEM = 3,
    // Triggers an underflow from BS->SetMem()
    POOL_UNDERFLOW_SET_MEM = 4,
    // Triggers an overflow from user code
    POOL_OVERFLOW_USER_CODE = 5,
    // Triggers an underflow from user code
    POOL_UNDERFLOW_USER_CODE = 6,
    // Triggers an out-of-bounds read ahead of the buffer
    POOL_OOB_READ_AHEAD = 7,
    // Triggers an out-of-bounds read behind the buffer
    POOL_OOB_READ_BEHIND = 8,
    // Frees the same pool block twice in a row
    POOL_DOUBLE_FREE = 9,
    // Frees a pointer which wasn't allocated by BS->AllocatePool()
    POOL_INVALID_FREE = 10,
    // Reads from the buffer after it was freed
    POOL_UAF_READ = 11,
    // Writes to the buffer after it was freed
    POOL_UAF_WRITE = 12,
    // Writes to the NULL page
    NULL_DEREFERENCE_DETERMINISTIC = 13,
    // Allocates a buffer with BS->AllocatePool(), then uses it without checking for NULL first
    NULL_DEREFERENCE_NON_DETERMINISTIC = 14,
    // Stack-based buffer overflow
    STACK_BUFFER_OVERFLOW = 15,
    // Leak uninitialized stack memory
    STACK_UNINITIALIZED_MEMORY_LEAK = 16
} FAULT_TYPE_T;


extern EFI_BOOT_SERVICES *gBS;
extern EFI_SYSTEM_TABLE *gST;
extern EFI_RUNTIME_SERVICES *gRT;

#define EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL_GUID \
{ 0xdb43bd29, 0xfb31, 0x49bd, { 0xb1, 0x2f, 0x4c, 0x8d, 0xbb, 0x7c, 0xd1, 0x0a } }
#define NOTMYUEFIPROTOCOLFAULT_PROTOCOL_GUID EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL_GUID  

EFI_GUID gEfiNotMyUefiProtocolFaultGUID = EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL_GUID;

/**
 * Make protocol function generate buggy behavior
 * @param SystemTable
 * @param FaultType
 * @retval EFI_SUCCESS
**/
typedef EFI_STATUS (EFIAPI* EFI_BUGGY_PROTOCOL) (
    IN EFI_SYSTEM_TABLE *SystemTable, 
    IN UINT32 FaultType
);

struct _EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL {
    UINT64 Revision;
    EFI_BUGGY_PROTOCOL GenerateBug;
};
typedef struct _EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL;
typedef struct EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL EFI_NOTMYUEFIPROTOCOLFAULT;

EFI_NOTMYUEFIPROTOCOLFAULT_PROTOCOL gEfiNotMyUefiProtocolFaultProtocol;

#endif