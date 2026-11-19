## Introduction

In this tutorial, we will configure WinDbg for kernel debugging using KDNET. By the end of this guide, you will be able to:

- Configure Windows for debugging mode
- Install the SDK correctly
- Configure KDNET for remote debugging
- Test and validate the debugging connection

## 🗺️ Tutorial Roadmap

1. Configure Windows for Debugging Mode
2. Install the SDK
3. Configure KDNET
4. Test the Debugging Connection
5. Conclusion

---

## 1. Configuring Windows for Debugging Mode

Before starting kernel debugging, Windows must be configured to operate in debugging mode. This setting allows the operating system to be monitored and debugged correctly.

### Steps

1. Open **Command Prompt** as **Administrator**.
2. Run the following command to enable debugging mode:

   ```
   bcdedit /set debug on
   ```

3. Restart the computer for the changes to take effect.

If you need to disable debugging in the future, run:

```
bcdedit /set debug off
```

## 2. Installing the SDK (Software Development Kit)

The SDK is essential for kernel debugging because it provides the required debugging tools. It should be installed only on the **host machine**, where WinDbg will run.

### Steps

1. Download the SDK from the [official Microsoft website](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/).
2. Run the installer and, when selecting components, choose only **"Debugging Tools for Windows"**.
3. Complete the installation and verify that the debugger directory was created at:

   ```
   C:\Program Files (x86)\Windows Kits\10\Debuggers\x64
   ```

## 3. Configuring KDNET

KDNET enables remote debugging over a network. Follow the steps below to configure it.

### On the host machine

1. Navigate to:

   ```
   C:\Program Files (x86)\Windows Kits\10\Debuggers\x64
   ```

2. Copy the `Kdnet.exe` and `VerifiedNICList.xml` files to an accessible location.

### On the target machine

1. Create a directory named `KD` at the root of the C: drive (`C:\KD`).
2. Copy `Kdnet.exe` and `VerifiedNICList.xml` into `C:\KD`.
3. Open **Command Prompt** as **Administrator** and navigate to `C:\KD`:

   ```
   cd C:\KD
   ```

4. Run the following command to check for supported network interfaces:

   ```
   kdnet.exe
   ```

   - This displays the NICs supported by KDNET.

5. Start network debugging, replacing `[host_ip]` with the host machine's IP address and `[port]` with the desired port:

   ```
   kdnet.exe [host_ip] [port]
   ```

   **Example:**

   ```
   kdnet.exe 192.168.2.1 51111
   ```

6. If the command succeeds, a configuration key similar to the following will be displayed:

   ```
   Key=2steg4fzbj2sz.23418vzkd4ko3.1g34ou07z4pev.1sp3yo9yz874p
   ```

   This key will be used to configure the connection in WinDbg.

## 4. Testing the Debugging Connection

Now that the configuration key has been generated on the target machine, configure WinDbg on the host machine to establish a connection to the target machine's kernel.

### Steps

1. Open **WinDbg** on the host machine.
2. Press **CTRL + K** to open the Kernel Debugging configuration window.
3. On the **NET** tab, enter:

   - **Port Number**: the port specified in the `kdnet.exe` command.
   - **Key**: the configuration key generated on the target machine.

4. Click **OK** to confirm the settings.
5. Before restarting the target machine, test the connection by running the following command in the host machine's command prompt:

   ```
   windbg -k net:port=51111,key=2steg4fzbj2sz.23418vzkd4ko3.1g34ou07z4pev.1sp3yo9yz874p
   ```

6. If WinDbg connects successfully, restart the target machine to start the debugging session.

## 5. Conclusion

With WinDbg configured through KDNET, you now have a working remote kernel debugging environment. This setup is extremely useful for development, crash analysis, and studying the inner workings of the operating system.