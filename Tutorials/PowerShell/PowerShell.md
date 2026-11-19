# PowerShell Mini-Course - NtHadouken

## Introduction to PowerShell

**PowerShell** is a powerful automation and command-line language developed by Microsoft and based on the .NET framework. It provides advanced features for system administration, file manipulation, and network management, and is widely used by IT professionals, system administrators, and automation enthusiasts.

Unlike Command Prompt (cmd), PowerShell works with **objects** rather than plain text, enabling more complex and efficient operations. It also supports CMD commands, making the transition between the tools easier.

### **Note: This is not a complete course. It is a mini-course covering introductory PowerShell concepts, from basic commands to intermediate topics that can be applied to development and automation. If you are looking for a deeper understanding, keep exploring and practicing the concepts presented here.**

---

## What Are Cmdlets?

**Cmdlets** are built-in PowerShell commands designed to perform specific operations. They follow the **Verb-Noun** format, which makes commands easier to read and understand.

Example:

```powershell
Get-Command -CommandType Cmdlet
```

Some common cmdlets include:

- **Get-Help** – Gets help about a command.
- **Get-Command** – Lists available commands.
- **Get-Service** – Displays system services.
- **Stop-Service** – Stops a running service.

### Getting Help

Before using a cmdlet, it is recommended that you check its documentation:

```powershell
Get-Help CmdletName
```

To update the help database:

```powershell
Update-Help
```

---

## Cmdlets, Functions, and Aliases

1. **Cmdlets:** Commands built into PowerShell.

   ```powershell
   Get-Command -CommandType Cmdlet
   ```

2. **Functions:** Reusable blocks of code.

   ```powershell
   Get-Command -CommandType Function
   ```

3. **Aliases:** Short names for longer commands.

   ```powershell
   Get-Command -CommandType Alias
   ```

   Creating a custom alias:

   ```powershell
   Set-Alias clear-screen Clear-Host
   ```

---

## Output Control

### Redirection Operators

- **`|`** – Passes the output of one command to another.
- **`>`** – Redirects output to a file (overwriting its contents).
- **`>>`** – Appends output to an existing file.
- **`2>`** – Redirects errors to a file.
- **`2>>`** – Appends errors to a file.
- **`2>&1`** – Redirects errors to standard output.

Example:

```powershell
Get-Process | Out-File "processes.txt"
```

---

## Filtering Results with Where-Object

To filter output, use **Where-Object**:

```powershell
Get-ChildItem | Where-Object {$_.Mode -like "*a*"}
```

### Comparison Operators

- `-lt` (Less than)
- `-le` (Less than or equal to)
- `-gt` (Greater than)
- `-ge` (Greater than or equal to)
- `-eq` (Equal to)
- `-ne` (Not equal to)
- `-like` (Supports wildcards)

---

## PowerShell Modules

Modules add functionality to PowerShell.

- List available modules:

  ```powershell
  Get-Module -ListAvailable
  ```

- Install a module:

  ```powershell
  Install-Module -Name ModuleName
  ```

- Import a module:

  ```powershell
  Import-Module ModuleName
  ```

---

## Variables

Variables store temporary data:

```powershell
$name = "NtHadouken"
Write-Host "Welcome, $name"
```

---

## Arrays

Arrays store indexed collections of data.

```powershell
$array = @("Peter", "Mary", "John")
```

Accessing elements:

```powershell
$array[0]  # Returns "Peter"
```

---

## Hash Tables

Hash tables store key-value pairs.

```powershell
$devices = @{Server1 = "192.168.2.1"; Router = "192.168.2.254"}
```

Adding and removing values:

```powershell
$devices["Server2"] = "192.168.1.10"
$devices.Remove("Server1")
```

Example script using a hash table:

```powershell
Clear-Host
$devices = [ordered] @{Server1="192.168.2.1"; Router="192.168.2.254"}
$devices["Loopback"]="127.0.0.1"
Test-Connection $devices.Router -Count 1
$devices.Values
```

---

## Select-String

**Select-String** searches for patterns within files:

```powershell
Get-Content "log.txt" | Select-String "Error"
```

Or:

```powershell
Select-String -Path "log.txt" -Pattern "Error"
```

---

## IF-ELSE

Use conditional statements to control script execution.

Example:

```powershell
Clear-Host
$service = Get-Service -Name spooler

if ($service.Status -eq "Running")
{
    Write-Host "Service is running"
}
else
{
    Write-Host "Service is stopped"
}
```

## Loops

- **For**

  ```powershell
  for(initialization; condition; next value)
  {
      Repeated code
  }
  ```

- **FOREACH**

  ```powershell
  ForEach($variable in collection items)
  {
      Repeated code
  }
  ```

- **While**

  ```powershell
  While(condition)
  {
      Repeated code
  }
  ```

### Example Script

```powershell
# FOR loop example

Clear-Host
$conn = 0
$failures = 0
$success = 0

for($counter=1; $counter -le 1; $counter++)
{
    Write-Host "Pinging 192.168.2.$counter"
    $conn = Test-Connection 192.168.1.$counter -Quiet
    if($conn -eq "true")
    {
        Write-Host "The connection to 192.168.2.$counter succeeded" -ForegroundColor Green
        $success++
    }
    else
    {
        Write-Host "The connection to 192.168.2.$counter failed" -ForegroundColor Yellow
        $failures++
    }
}
Write-Host "Total successful connections: $success" -ForegroundColor Green
Write-Host "Total failures: $failures" -ForegroundColor Yellow

# FOREACH example
Write-Host
foreach($files in Get-Process)
{
    if($files.ProcessName -like "Notepad*")
    {
        Write-Host Process $files.ProcessName.ToUpper() "is open" -ForegroundColor Yellow
    }
}

# WHILE example

Write-Host
$i = 0
while($i -le 5)
{
    Write-Host $i
    $i++
}
```

## Functions

Functions are commands in a script that remain available only for the duration of the session in which the script is running.

- Long commands.
- Repeated tasks.
- Use of parameters.

## Example

```powershell
Function Add-Numbers
{
    param($a, $b)
    $result = $a + $b
    Write-Host $result
}
```

## Workflows

Workflows allow you to create and manage long-running scripts.

They can be used to:

- Stop
- Suspend
- Restart
- Repeat
- Execute tasks in parallel

When working with remote machines, it is important to use `Write-Output` because the `Write-Host` cmdlet prints directly to the interactive host rather than to a remote host.

## New-Object with WScript.Shell

### WScript.Shell

You can use New-Object to work with COM (Component Object Model) components. These components range from the various libraries included in WSH (Windows Script Host) to ActiveX applications such as Internet Explorer.

- New-Object -ComObject WScript.Shell
- New-Object -ComObject WScript.Network
- New-Object -ComObject Scripting.Dictionary
- New-Object -ComObject Scripting.FileSystemObject

## Example

```powershell
$wshell = New-Object -ComObject WScript.Shell

$wshell | Get-Member

$wshell.Popup("NtHadouken", 0, "PowerShell", 0x1)

$wshell.Run("cmd")
$wshell.AppActivate("Notepad")
Start-Sleep -Seconds 2
$wshell.SendKeys("echo NtHadouken ~")
```

This mini-course introduces the fundamental concepts of PowerShell. It serves as a starting point for exploring the potential of this powerful tool. Keep practicing and experimenting with different cmdlets to deepen your knowledge!
