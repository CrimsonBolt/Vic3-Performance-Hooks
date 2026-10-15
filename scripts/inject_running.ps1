# vic3hook manual injector: injects vic3hook.dll into the ALREADY RUNNING
# victoria3.exe. You start the game first (Steam as usual), LOAD your save,
# PAUSE the game, then double-click "INJECT HOOKS.cmd". Prints the install
# verdict; never closes on its own — the .cmd wrapper keeps the window open.
# Personal use only.
$ErrorActionPreference = 'Stop'

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$DllFull = Join-Path $here 'vic3hook.dll'
if (-not (Test-Path $DllFull)) { throw "vic3hook.dll not found next to this script." }

# --- 1. the game must already be running ------------------------------------
$game = Get-Process -Name 'victoria3' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $game) {
    Write-Output '================================================================'
    Write-Output 'victoria3.exe is NOT running.'
    Write-Output 'Start the game first (Steam as usual), LOAD your save, PAUSE,'
    Write-Output 'then double-click INJECT HOOKS.cmd again.'
    Write-Output '================================================================'
    exit 1
}
Write-Output ("game : victoria3.exe (pid {0})" -f $game.Id)

# --- 2. inject (CreateRemoteThread + LoadLibraryW, the proven method) -------
Add-Type -TypeDefinition @"
using System; using System.Runtime.InteropServices;
public static class V3I {
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenProcess(UInt32 a, bool i, Int32 pid);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr VirtualAllocEx(IntPtr p, IntPtr a, IntPtr s, Int32 t, Int32 pr);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool WriteProcessMemory(IntPtr p, IntPtr b, byte[] buf, IntPtr s, out IntPtr w);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr CreateRemoteThread(IntPtr p, IntPtr a, IntPtr st, IntPtr f, IntPtr pa, UInt32 fl, out UInt32 tid);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern UInt32 WaitForSingleObject(IntPtr h, UInt32 ms);
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)] public static extern IntPtr GetModuleHandleW(string n);
    [DllImport("kernel32.dll", CharSet=CharSet.Ansi, SetLastError=true)] public static extern IntPtr GetProcAddress(IntPtr m, string n);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
}
"@
$access = 0x0002 -bor 0x0400 -bor 0x0008 -bor 0x0020 -bor 0x0010
$h = [V3I]::OpenProcess($access, $false, $game.Id)
if ($h -eq [IntPtr]::Zero) { throw ("OpenProcess failed (Windows error {0})" -f [Runtime.InteropServices.Marshal]::GetLastWin32Error()) }
$kernel = [V3I]::GetModuleHandleW('kernel32.dll')
$loadLibrary = [V3I]::GetProcAddress($kernel, 'LoadLibraryW')
$pathBytes = [System.Text.Encoding]::Unicode.GetBytes($DllFull + [char]0)
$remote = [V3I]::VirtualAllocEx($h, [IntPtr]::Zero, $pathBytes.Length, 0x3000, 0x04)
$written = [IntPtr]::Zero
if ($remote -eq [IntPtr]::Zero) { throw ("VirtualAllocEx failed (Windows error {0})" -f [Runtime.InteropServices.Marshal]::GetLastWin32Error()) }
if (-not [V3I]::WriteProcessMemory($h, $remote, $pathBytes, $pathBytes.Length, [ref]$written)) {
    throw ("WriteProcessMemory failed (Windows error {0})" -f [Runtime.InteropServices.Marshal]::GetLastWin32Error())
}
$tid = [uint32]0
$thread = [V3I]::CreateRemoteThread($h, [IntPtr]::Zero, [IntPtr]::Zero, $loadLibrary, $remote, 0, [ref]$tid)
if ($thread -eq [IntPtr]::Zero) { throw ("CreateRemoteThread failed (Windows error {0})" -f [Runtime.InteropServices.Marshal]::GetLastWin32Error()) }
[void][V3I]::WaitForSingleObject($thread, 5000)
[void][V3I]::CloseHandle($thread)
[void][V3I]::CloseHandle($h)
Write-Output 'injected vic3hook.dll - hooks install ~5 s later...'

# --- 3. verdict from the log (fresh per injection, CREATE_ALWAYS) ------------
Start-Sleep -Seconds 7
Write-Output '================================================================'

# game health first: patching suspends game threads; a deadlock shows here
$game2 = Get-Process -Id $game.Id -ErrorAction SilentlyContinue
$gameDead = $false; $gameStuck = $false
if (-not $game2) {
    $gameDead = $true
    Write-Output 'THE GAME EXITED during patching.'
    Write-Output 'The last "enabling..." line in vic3hook.log names the hook that'
    Write-Output 'did it. Nothing was saved/modified on disk - just restart.'
} elseif (-not $game2.Responding) {
    $gameStuck = $true
    Write-Output 'THE GAME IS NOT RESPONDING - patching deadlocked it.'
    Write-Output 'Close the game (Task Manager), restart, LOAD your save, PAUSE,'
    Write-Output 'and run this script again. Paused install is the proven path.'
}

$log = Join-Path $here 'vic3hook.log'
if (-not $gameDead -and -not (Test-Path $log)) {
    Write-Output 'vic3hook: NO LOG WAS CREATED - the DLL failed to load inside'
    Write-Output 'the game (architecture mismatch or antivirus block). Nothing'
    Write-Output 'was patched; the game is running vanilla.'
} elseif (Test-Path $log) {
    $lines = Get-Content $log
    if ($lines | Select-String 'prologue mismatch') {
        Write-Output 'vic3hook: WRONG GAME VERSION - hooks auto-disabled, the game'
        Write-Output 'is running pure vanilla. Nothing was patched.'
    } else {
        # the DLL prints "hooks installed: N/12" only when the install
        # finished; without that line the log is partial - do not trust it.
        $done = $lines | Select-String 'hooks installed: (\d+)/(\d+)'
        if ($done) {
            $n = [int]$done.Matches[0].Groups[1].Value
            $total = [int]$done.Matches[0].Groups[2].Value
            $dis = ($lines | Select-String 'disabled \(every_n=0\)').Count
            $fail = ($lines | Select-String 'FAILED').Count
            if ($n -eq $total -and -not $gameStuck -and -not $gameDead) {
                Write-Output ("vic3hook: {0}/{1} hooks installed. Have fun!" -f $n, $total)
            } elseif ($fail -gt 0) {
                Write-Output ("vic3hook: {0}/12 installed, {1} failed to patch" -f $n, $fail)
                Write-Output '(- see the log for which; game runs fine on the rest)'
            } elseif ($n -eq 0) {
                Write-Output 'vic3hook: all hooks disabled by config (every_n=0)'
                Write-Output 'in vic3hook.ini - running pure vanilla.'
            } elseif ($dis -gt 0) {
                Write-Output ("vic3hook: {0}/{1} hooks installed, {2} disabled by" -f $n, $total, $dis)
                Write-Output 'config (every_n=0 in vic3hook.ini). This is normal.'
            }
        } elseif (-not $gameDead -and -not $gameStuck) {
            $last = ($lines | Select-String 'hook \d+/12' | Select-Object -Last 1)
            Write-Output 'vic3hook: install did NOT finish (no completion line in'
            Write-Output 'the log). Last attempted step:'
            if ($last) { Write-Output ("    " + $last.Line.Trim()) }
            Write-Output 'The game may still run on the partial hooks - if anything'
            Write-Output 'behaves oddly, restart the game and inject while PAUSED.'
        }
    }
}
if (-not $gameStuck -and -not $gameDead) {
    Write-Output '(details: vic3hook.log next to this script)'
}
Write-Output '================================================================'
