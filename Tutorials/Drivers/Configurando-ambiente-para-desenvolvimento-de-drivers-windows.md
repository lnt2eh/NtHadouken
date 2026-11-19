# Introduction

In this tutorial, we will prepare a development environment for anyone who wants to get started with driver development but does not know where to begin. The goal is to provide beginners with the information they need to set up their environments and start learning Windows NT driver development. Please note that this tutorial does not cover creating a virtual machine or related topics.

## 🗺️ Tutorial Roadmap

- Install Visual Studio 2022 and the WDK
- Install the SDK
- Install OSR Loader
- Configure Windows debugging mode and enable testsigning to disable DSE enforcement for test development
- Download DebugView
- Develop a "Hello World" driver and view the kernel buffer in DebugView

## Installing Visual Studio 2022

The Visual Studio 2022 installation page covers steps 1, 2, and 3. You can download the Visual Studio 2022 installer, SDK, and WDK using the following link: [Download WDK](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk). This page already contains the steps you need to follow.

### Installing Visual Studio 2022

1. After launching the Visual Studio 2022 installer, select **"Desktop development with C++"** on the initial page.
2. After selecting it, open the **"Individual components"** tab, search for **"Spectre"**, and select **"MSVC v143 - VS 2022 C++ x64/x86 Spectre-mitigated libraries"**.
3. Search for **"WDK"** and select **"Windows Driver Kit (WDK)"**.

## Installing the SDK

Installing the SDK is straightforward; simply follow the **"Next", "Next"** prompts.

## Installing OSR Loader

You can download OSR Loader here: [Download OSR Loader](https://www.osronline.com/OsrDown.cfm/osrloaderv30.zip%5Ename=osrloaderv30.zip&id=157).

1. After extracting the folder, navigate to **"EXTRACTED-FOLDER\Projects\OsrLoader\kit\WLH\AMD64\FRE"**.
2. Run **OSRLOADER.exe**.

## Configuring Windows Debugging Mode and Test Signing

After installing all the components, we need to configure Windows so that we can build and load our drivers into the operating system. This is necessary because of the **DSE (Driver Signature Enforcement)** policy, which prevents digitally unsigned drivers from being installed.

1. Configure Windows for debugging mode:

   ```bash
   bcdedit /set debug on
   ```

2. Enable **testsigning**:

   ```bash
   bcdedit /set testsigning on
   ```

3. Restart the machine to apply the settings.

## Downloading DebugView

You can get DebugView here: [Download DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview).

It will be used to display the kernel message buffer.

## Developing a "Hello World" Driver and Viewing the Kernel Buffer in DebugView

Now that the environment is ready, let's verify that everything is working correctly.

### Creating a Visual Studio Project

1. Open Visual Studio and select **"Create a new project"**.
2. In the template search box, look for **"Empty WDM Driver"** and select it.
3. Give the project a name (**for example: MyDriver1**) and make sure to select **"Place solution and project in the same directory"**.
4. Click **"Create"**.

### Writing the Driver Code

1. In Solution Explorer, under **"Driver Files"**, delete the **.INF** file.
2. Right-click **"Driver Files"** > **Add -> New Item**.
3. Name the file with the **.c** extension (**for example: Hello.c**) and click **Add**.

Now add the following code to the file:

```c
#include <ntddk.h>

VOID kernelUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    KdPrint(("Unloading Driver\n"));
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    KdPrint(("Hello, World\n"));
    DriverObject->DriverUnload = kernelUnload;
    return STATUS_SUCCESS;
}
```

### Building the Driver

To build the driver, press **Ctrl + Shift + B** or go to **Build** > **Build Solution**.

### Loading the Driver with OSR Loader

1. In OSR Loader, click **"Browse"** and select the **.sys** file generated in the **x64\Debug** folder.
2. Click **"Register Service"**.
3. Open **DebugView** as administrator and press **Ctrl + K** to enable kernel event capture.
4. In OSR Loader, click **"Start Service"**.
5. If everything works correctly, you will see the message **"Hello, World"** in DebugView.
6. Click **"Stop Service"** to unload the driver.

## Conclusion

We have configured our development environment, built a driver, and loaded a simple driver into Windows. You are now ready to explore Windows NT driver development further. Good luck with your studies!