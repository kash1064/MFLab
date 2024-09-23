/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    nullFilter.c

Abstract:

    This is the main module of the nullFilter mini filter driver.
    It is a simple minifilter that registers itself with the main filter
    for no callback operations.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//---------------------------------------------------------------------------
//      Global variables
//---------------------------------------------------------------------------


typedef struct _NULL_FILTER_DATA {

    //
    //  The filter handle that results from a call to
    //  FltRegisterFilter.
    //

    PFLT_FILTER FilterHandle;

} NULL_FILTER_DATA, *PNULL_FILTER_DATA;

/*************************************************************************
    Prototypes for the startup and unload routines used for
    this Filter.

    Implementation in nullFilter.c
*************************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
NullUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
NullQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

//
//  Structure that contains all the global data structures
//  used throughout NullFilter.
//

NULL_FILTER_DATA NullFilterData;

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NullUnload)
#pragma alloc_text(PAGE, NullQueryTeardown)
#endif


//---------------------------------------------------------------------------
//      Custom
//---------------------------------------------------------------------------

typedef struct _DATA_CONTEXT {

    UNICODE_STRING message;

} DATA_CONTEXT, * PDATA_CONTEXT;


const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_FILE_CONTEXT,
      0,
      NULL,
      sizeof(DATA_CONTEXT),
      'galF' },

    { FLT_CONTEXT_END }
};


FLT_PREOP_CALLBACK_STATUS
FuncPreCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
    UNREFERENCED_PARAMETER(Data);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PAGED_CODE();

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
FuncPostCreate(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION nameInfo;
    PDATA_CONTEXT dataContext;

    status = FltGetFileNameInformation(Data,
        FLT_FILE_NAME_NORMALIZED |
        FLT_FILE_NAME_QUERY_DEFAULT,
        &nameInfo);

    if (!NT_SUCCESS(status)) {

        return FLT_POSTOP_FINISHED_PROCESSING;

    }

    FltParseFileNameInformation(nameInfo);

    if (nameInfo != NULL) {
        status = FltGetFileContext(Data->Iopb->TargetInstance,
            Data->Iopb->TargetFileObject,
            &dataContext);

        if (!NT_SUCCESS(status)) {

            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: FltGetFileContext failed.\n");

            status = FltAllocateContext(NullFilterData.FilterHandle,
                FLT_FILE_CONTEXT,
                sizeof(DATA_CONTEXT),
                PagedPool,
                &dataContext);

            if (NT_SUCCESS(status)) {

                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: FltAllocateContext success %p.\n", dataContext);

                UNICODE_STRING contextString;
                UNICODE_STRING textString;
                RtlInitUnicodeString(&textString, L"test.txt");

                if (RtlCompareUnicodeString(&(nameInfo->FinalComponent), &textString, TRUE) == 0) {

                    RtlInitUnicodeString(&contextString, L"File is test.txt !!");

                    dataContext->message = contextString;

                    status = FltSetFileContext(Data->Iopb->TargetInstance,
                        Data->Iopb->TargetFileObject,
                        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                        dataContext,
                        NULL);

                    if (NT_SUCCESS(status)) {

                        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: File name is text.txt and FltSetFileContext success.\n");

                    }
                    else {

                        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: FltSetFileContext failed.\n");

                    }

                }
                else {

                    RtlInitUnicodeString(&contextString, L"Not valid file.");

                    dataContext->message = contextString;

                    status = FltSetFileContext(Data->Iopb->TargetInstance,
                        Data->Iopb->TargetFileObject,
                        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                        dataContext,
                        NULL);

                    if (NT_SUCCESS(status)) {

                        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: File name is not valid and FltSetFileContext success.\n");

                    }
                    else {

                        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: FltSetFileContext failed.\n");

                    }

                }

                FltReleaseContext(dataContext);

            }
            else {

                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: FltAllocateContext failed.\n");

            }

        }
        else {

            DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FuncPostCreate: FltGetFileContext success. Message is %wZ.\n", dataContext->message);

            FltReleaseContext(dataContext);

        }

        FltReleaseFileNameInformation(nameInfo);

    }
    
    return FLT_POSTOP_FINISHED_PROCESSING;
}

const FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      FuncPreCreate,
      FuncPostCreate},

    { IRP_MJ_OPERATION_END }
};


//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    ContextRegistration,                //  Context
    Callbacks,                          //  Operation callbacks

    NullUnload,                         //  FilterUnload

    NULL,                               //  InstanceSetup
    NullQueryTeardown,                  //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};


/*************************************************************************
    Filter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver. This
    registers the miniFilter with FltMgr and initializes all
    its global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.
    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    //
    //  Register with FltMgr
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &NullFilterData.FilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( NullFilterData.FilterHandle );

        if (!NT_SUCCESS( status )) {
            FltUnregisterFilter( NullFilterData.FilterHandle );
        }
    }
    return status;
}

NTSTATUS
NullUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unloaded indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Unload minifilter.\n");

    PAGED_CODE();

    FltUnregisterFilter( NullFilterData.FilterHandle );

    return STATUS_SUCCESS;
}

NTSTATUS
NullQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is the instance detach routine for this miniFilter driver.
    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Detach instance.\n");

    PAGED_CODE();

    return STATUS_SUCCESS;
}

