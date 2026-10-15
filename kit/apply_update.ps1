# vic3hook staged-update applier: called by INJECT HOOKS.cmd AFTER the
# injector script has exited (no file locks). Copies update\staged over the
# kit, preserving the user's vic3hook.ini customizations (new hook keys from
# the shipped ini are merged in, existing values are never overwritten).
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$staged = Join-Path $here 'update\staged'
if (-not (Test-Path $staged)) { Write-Output 'No staged update found.'; exit 1 }

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

Get-ChildItem $staged | ForEach-Object {
    if ($_.Name -ne 'vic3hook.ini') {
        Copy-Item $_.FullName -Destination $here -Recurse -Force
    }
}
Remove-Item (Join-Path $here 'update') -Recurse -Force -ErrorAction SilentlyContinue
$v = ''
$vf = Join-Path $here 'version.txt'
if (Test-Path $vf) { $v = (Get-Content $vf -First 1).Trim() }
Write-Output ("kit updated to version {0}. Run INJECT HOOKS.cmd again to use it." -f $v)
