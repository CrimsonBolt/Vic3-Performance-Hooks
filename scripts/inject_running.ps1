# vic3hook manual injector: injects vic3hook.dll into the ALREADY RUNNING
# victoria3.exe. You start the game first (Steam as usual), LOAD your save,
# PAUSE the game, then double-click "INJECT HOOKS.cmd". Prints the install
# verdict; never closes on its own — the .cmd wrapper keeps the window open.
# On launch (unless -SkipUpdate) it checks the GitHub release repo for a
# newer kit version and PROMPTS: update now (downloaded and applied in this
# window, before injection — nothing is ever downloaded or applied without
# an explicit yes), open the releases page for a manual download, or keep
# the current version. vic3hook.ini customizations are always preserved.
param([switch]$SkipUpdate)
$ErrorActionPreference = 'Stop'

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$DllFull = Join-Path $here 'vic3hook.dll'
if (-not (Test-Path $DllFull)) { throw "vic3hook.dll not found next to this script." }

# --- 0. update check against the GitHub release repo -------------------------
$script:ReleasePage = 'https://github.com/CrimsonBolt/Vic3-Performance-Hooks/releases/latest'
function Invoke-UpdateCheck {
    $local = '0'
    $vf = Join-Path $here 'version.txt'
    if (Test-Path $vf) { $local = (Get-Content $vf -First 1).Trim() }
    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        $rel = Invoke-RestMethod -Uri 'https://api.github.com/repos/CrimsonBolt/Vic3-Performance-Hooks/releases/latest' `
            -Headers @{ 'User-Agent' = 'vic3hook-update-check' } -TimeoutSec 5
    } catch { return }  # offline or blocked: run what we have, silently
    # remote version: prefer the release tag (tags carry the kit version,
    # e.g. 1.14.5.6); fall back to a version.txt asset for older releases
    # that were tagged without it. A release with neither is not updatable.
    $remote = ''
    $tag = ''
    if ($rel.tag_name) { $tag = $rel.tag_name.Trim() }
    if ($tag -match '^v?(\d+(\.\d+){1,3})$') { $remote = $Matches[1] }
    if (-not $remote) {
        $verAsset = $rel.assets | Where-Object { $_.name -eq 'version.txt' } | Select-Object -First 1
        if ($verAsset) {
            try {
                $c = (Invoke-WebRequest -Uri $verAsset.browser_download_url -UseBasicParsing -TimeoutSec 10).Content
                if ($c -is [byte[]]) { $c = [Text.Encoding]::UTF8.GetString($c) }  # PS 5.1 returns bytes for text assets
                $remote = ($c | Out-String).Trim()
            } catch { return }
        }
    }
    if (-not $remote) { return }
    try { $lvo = [version]$local } catch { $lvo = [version]'0' }
    try { $rvo = [version]$remote } catch { return }
    if ($rvo -le $lvo) { return }

    # never download or change anything without an explicit choice
    Write-Output '================================================================'
    Write-Output ("update available: kit {0}   (installed: {1})" -f $remote, $local)
    if ($rel.name) { Write-Output ("release: {0}" -f $rel.name) }
    Write-Output ("page  : {0}" -f $script:ReleasePage)
    Write-Output ''
    Write-Output '[U] Update now (downloads from GitHub and applies it here)'
    Write-Output '[M] Open the releases page for a manual download'
    Write-Output '[Enter] Not now - continue with the installed version'
    $choice = (Read-Host 'your choice').Trim()
    if ($choice -ieq 'm') {
        Start-Process $script:ReleasePage
        Write-Output 'opened the releases page in your browser - continuing'
        Write-Output 'with the installed version.'
        return
    }
    if ($choice -ine 'u') { Write-Output 'staying on the installed version.'; return }

    # consent given - but the running game may hold the DLL open (injected
    # earlier this session); an in-place update needs the file unlocked.
    try {
        $lockTest = [IO.File]::Open($DllFull, 'Open', 'ReadWrite', 'None')
        $lockTest.Close()
    } catch {
        Write-Output 'cannot update: the running game still has the current'
        Write-Output 'vic3hook.dll loaded (you injected earlier this session).'
        Write-Output 'Close the game and run INJECT HOOKS.cmd again to update.'
        return
    }
    $zipAsset = $rel.assets | Where-Object { $_.name -like '*.zip' } | Select-Object -First 1
    if (-not $zipAsset) {
        Write-Output 'no zip asset on the release - download it manually from the page above.'
        return
    }
    $sizeMb = [math]::Round($zipAsset.size / 1MB, 2)
    Write-Output ("downloading {0} ({1} MB) from GitHub..." -f $zipAsset.name, $sizeMb)
    $tmp = Join-Path ([IO.Path]::GetTempPath()) ("vic3hook-update-" + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $tmp -Force | Out-Null
    try {
        $zipPath = Join-Path $tmp 'kit.zip'
        Invoke-WebRequest -Uri $zipAsset.browser_download_url -OutFile $zipPath -UseBasicParsing -TimeoutSec 120
        if ((Get-Item $zipPath).Length -lt 10000) { throw 'downloaded file too small' }
        $staged = Join-Path $tmp 'staged'
        Expand-Archive -Path $zipPath -DestinationPath $staged -Force
        # sanity: a real kit zip contains these
        if (-not (Test-Path (Join-Path $staged 'vic3hook.dll')) -or -not (Test-Path (Join-Path $staged 'version.txt'))) {
            throw 'zip does not look like a vic3hook kit'
        }
        # preserve the user's ini: never overwrite values, only merge in new keys
        $userIni = Join-Path $here 'vic3hook.ini'
        $newIni = Join-Path $staged 'vic3hook.ini'
        if ((Test-Path $userIni) -and (Test-Path $newIni)) {
            $user = Get-Content $userIni
            $new = Get-Content $newIni
            $userKeys = @($user | Where-Object { $_ -match '^\s*\w+_every_n=' } | ForEach-Object { ($_.Split('='))[0].Trim() })
            $missing = @($new | Where-Object { $_ -match '^\s*\w+_every_n=' -and $userKeys -notcontains ($_.Split('='))[0].Trim() })
            if ($missing.Count -gt 0) {
                Add-Content -Path $userIni -Value ''
                Add-Content -Path $userIni -Value '; merged in by update (new keys at their defaults):'
                $missing | ForEach-Object { Add-Content -Path $userIni -Value $_ }
            }
        }
        Get-ChildItem $staged | Where-Object { $_.Name -ne 'vic3hook.ini' } | ForEach-Object {
            Copy-Item $_.FullName -Destination $here -Recurse -Force
        }
        $v = (Get-Content (Join-Path $here 'version.txt') -First 1).Trim()
        Write-Output ("kit updated to {0} - your vic3hook.ini settings were kept." -f $v)
    } catch {
        Write-Output ("update failed ({0}) - continuing with the installed kit." -f $_.Exception.Message)
    } finally {
        Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    }
}
if (-not $SkipUpdate) { Invoke-UpdateCheck }

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
    $mismatch = ($lines | Select-String 'prologue mismatch|outside the game image').Count
    $installedCount = ($lines | Select-String 'INSTALLED at').Count
    if ($mismatch -gt 0 -and $installedCount -eq 0) {
        Write-Output 'vic3hook: WRONG GAME VERSION - every hook failed its byte'
        Write-Output 'check and auto-disabled. Nothing was patched; the game is'
        Write-Output 'running pure vanilla.'
    } elseif ($mismatch -gt 0) {
        Write-Output ("vic3hook: PARTIAL VERSION MISMATCH - {0} hook(s) installed and" -f $installedCount)
        Write-Output ("ACTIVE, {0} failed the byte check and are disabled." -f $mismatch)
        Write-Output 'The game is NOT vanilla: some throttling is active.'
        Write-Output 'Restart without injecting for pure vanilla, or update the'
        Write-Output 'hook table (see README: Building from source).'
    } else {
        # the DLL prints "hooks installed: N/M" only when the install
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
                Write-Output ("vic3hook: {0}/{1} installed, {2} failed to patch" -f $n, $total, $fail)
                Write-Output '(- see the log for which; game runs fine on the rest)'
            } elseif ($n -eq 0) {
                Write-Output 'vic3hook: all hooks disabled by config (every_n=0)'
                Write-Output 'in vic3hook.ini - running pure vanilla.'
            } elseif ($dis -gt 0) {
                Write-Output ("vic3hook: {0}/{1} hooks installed, {2} disabled by" -f $n, $total, $dis)
                Write-Output 'config (every_n=0 in vic3hook.ini). This is normal.'
            }
        } elseif (-not $gameDead -and -not $gameStuck) {
            $last = ($lines | Select-String 'hook \d+/\d+' | Select-Object -Last 1)
            Write-Output 'vic3hook: install did NOT finish (no completion line in'
            Write-Output 'the log). Last attempted step:'
            if ($last) { Write-Output ("    " + $last.Line.Trim()) } else { Write-Output '    (no hook step was reached - read the log)' }
            Write-Output 'The game may still run on the partial hooks - if anything'
            Write-Output 'behaves oddly, restart the game and inject while PAUSED.'
        }
    }
}
if (-not $gameStuck -and -not $gameDead) {
    Write-Output '(details: vic3hook.log next to this script)'
}
Write-Output '================================================================'
