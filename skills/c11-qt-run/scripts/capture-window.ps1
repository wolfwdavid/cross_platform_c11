# Capture the c11-qt app window to a PNG.
# Usage: powershell.exe -NoProfile -ExecutionPolicy Bypass -File capture-window.ps1 <out.png>
# Finds the window by c11.exe's PID, brings it to the foreground, and saves a screenshot.
# Caveat: CopyFromScreen grabs screen pixels, so if another window covers c11 you'll
# capture that. SetForegroundWindow is best-effort (Windows may refuse it from a
# background process) — if the wrong app appears, click the c11 window and retry.
param([Parameter(Mandatory=$true)][string]$OutPath)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class C11Cap {
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc cb, IntPtr l);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  public delegate bool EnumWindowsProc(IntPtr h, IntPtr l);
  public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$proc = Get-Process c11 -ErrorAction SilentlyContinue | Sort-Object StartTime | Select-Object -First 1
if (-not $proc) { Write-Error "c11.exe is not running"; exit 1 }
$target = [uint32]$proc.Id

$best = [IntPtr]::Zero; $bestArea = 0
$cb = [C11Cap+EnumWindowsProc]{ param($h,$l)
  if ([C11Cap]::IsWindowVisible($h)) {
    [uint32]$wpid = 0; [C11Cap]::GetWindowThreadProcessId($h, [ref]$wpid) | Out-Null
    if ($wpid -eq $target) {
      $r = New-Object C11Cap+RECT; [C11Cap]::GetWindowRect($h, [ref]$r) | Out-Null
      $area = ($r.Right-$r.Left)*($r.Bottom-$r.Top)
      if ($area -gt $bestArea) { $script:best = $h; $script:bestArea = $area }
    }
  }; return $true
}
[C11Cap]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
if ($script:best -eq [IntPtr]::Zero) { Write-Error "no visible c11 window"; exit 1 }

[C11Cap]::ShowWindow($script:best, 9) | Out-Null   # SW_RESTORE
[C11Cap]::BringWindowToTop($script:best) | Out-Null
[C11Cap]::SetForegroundWindow($script:best) | Out-Null
Start-Sleep -Milliseconds 700

$r = New-Object C11Cap+RECT; [C11Cap]::GetWindowRect($script:best, [ref]$r) | Out-Null
$w = $r.Right-$r.Left; $h = $r.Bottom-$r.Top
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, (New-Object System.Drawing.Size $w, $h))
$bmp.Save($OutPath)
Write-Output "captured ${w}x${h} -> $OutPath"
