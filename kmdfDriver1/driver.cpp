#include "includes.h"
#include <ntifs.h>
#include <ntddk.h>


void sampleUnload(_In_ PDRIVER_OBJECT driver_object) {
	UNREFERENCED_PARAMETER(driver_object);

	UNICODE_STRING sym_link;
	RtlInitUnicodeString(&sym_link, L"\\??\\PriorityBooster");
	IoDeleteSymbolicLink(&sym_link);

	IoDeleteDevice(driver_object->DeviceObject);

	DbgPrintEx(0, 0, "Driver1 unloaded\n");
}

NTSTATUS PriorityBoosterDeviceControl(_In_ PDEVICE_OBJECT device_object, _In_ PIRP irp)
{
	UNREFERENCED_PARAMETER(device_object);

	auto stack = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS status = STATUS_SUCCESS;
	PTHREAD_DATA pData = {};

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_BOOSTER_SET_PRIORITY:

		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(THREAD_DATA)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		pData = (PTHREAD_DATA)(stack->Parameters.DeviceIoControl.Type3InputBuffer);

		if (!pData) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		if (pData->priority < 1 || pData->priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		PETHREAD pThread;
		status = PsLookupThreadByThreadId(ULongToHandle(pData->thread_id), &pThread);
		if (!NT_SUCCESS(status)) {
			break;
		}

		KeSetPriorityThread((PKTHREAD)pThread, pData->priority);
		ObDereferenceObject(pThread);
		break;
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}
NTSTATUS PriorityBoosterCreateClose(_In_ PDEVICE_OBJECT device_object,_In_ PIRP irp) 
{
	UNREFERENCED_PARAMETER(device_object);
	
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING reg_path)
{
	UNREFERENCED_PARAMETER(reg_path);

	DbgPrintEx(0, 0, "Driver1 initialized\n");

	driver_object->DriverUnload = sampleUnload;

	driver_object->MajorFunction[IRP_MJ_CREATE]			= PriorityBoosterCreateClose;
	driver_object->MajorFunction[IRP_MJ_CLOSE]			= PriorityBoosterCreateClose;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;

	UNICODE_STRING device_name = RTL_CONSTANT_STRING(L"\\Device\\PriorityBooster");
	/*
	
	PFILE_OBJECT file;
	NTSTATUS status = IoGetDeviceObjectPointer(&device_name, FILE_ALL_ACCESS, &file, &device_object);
	if (device_object != nullptr || file != nullptr) {
		DbgPrintEx(0, 0, "There is already a driver with that name\n");
		return status;
	}
	*/
	PDEVICE_OBJECT device_object;
	NTSTATUS status = IoCreateDevice(
		driver_object,
		0,
		&device_name,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&device_object);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(0, 0, "Failed to create the device\n");
		return status;
	}

	UNICODE_STRING sym_link;
	//https://stackoverflow.com/questions/21703592/open-device-name-using-createfile//
	RtlInitUnicodeString(&sym_link, L"\\??\\PriorityBooster");
	status = IoCreateSymbolicLink(&sym_link, &device_name);
	if (!NT_SUCCESS(status)) {
		DbgPrintEx(0, 0, "Failed to create the sym link\n");
		IoDeleteDevice(driver_object->DeviceObject);
		return status;
	}



	return STATUS_SUCCESS;
}
