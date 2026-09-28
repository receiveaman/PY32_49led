# 49LED RS485 联调回归脚本（移植后必跑）
# 用法: powershell -NoProfile -ExecutionPolicy Bypass -File tools\rs485_regression.ps1 [-Port COM13]
# 7 条指令逐条发送并核对应答，全绿输出 "7/7 PASS"
param(
    [string]$Port = "COM13",
    [int]$Baud = 115200
)

function ToHex([byte[]]$b) { ($b | ForEach-Object { '{0:X2}' -f $_ }) -join ' ' }

$sp = New-Object System.IO.Ports.SerialPort $Port, $Baud, none, 8, one
$sp.ReadTimeout = 500
try { $sp.Open() } catch { Write-Output "OPEN FAIL ($Port): $($_.Exception.Message)"; exit 1 }

$pass = 0
$fail = 0

function Step($tag, [byte[]]$tx, [string]$wantPrefix) {
    $script:sp.DiscardInBuffer()
    $script:sp.Write($tx, 0, $tx.Length)
    Start-Sleep -Milliseconds 60
    $buf = New-Object byte[] 64
    $n = 0
    try { $n = $script:sp.Read($buf, 0, 64) } catch { }
    $rx = if ($n -gt 0) { ToHex $buf[0..($n - 1)] } else { "<EMPTY>" }
    $ok = $rx.StartsWith($wantPrefix)
    if ($ok) { $script:pass++ } else { $script:fail++ }
    "{0,-26} TX {1}`n{0,-26} RX {2}   {3}" -f $tag, (ToHex $tx), $rx, $(if ($ok) { "PASS" } else { "FAIL want=$wantPrefix" })
    Start-Sleep -Milliseconds 60
}

try {
    # 1 查资源列表（任意模式都应答）
    Step "1 resource-list" ([byte[]](0x01,0x01,0x00,0x00,0x18,0x50)) "01 02 00 07 01 00 00 00 31 00 00"
    # 2 切受控模式
    Step "2 mode=remote"    ([byte[]](0x01,0x06,0x00,0x03,0x00,0x00,0x01,0x22,0xCA)) "01 06 00 03 00 00 01"
    # 3 关通信看门狗
    Step "3 watchdog=off"   ([byte[]](0x01,0x06,0x00,0x03,0x02,0x00,0x00,0x22,0xAA)) "01 06 00 03 02 00 00"
    # 4 亮度 100
    Step "4 brightness=100" ([byte[]](0x01,0x06,0x00,0x03,0x01,0x00,0x64,0xC9,0x5B)) "01 06 00 03 01 00 64"
    # 5 写全亮（应答位图 D6=0x80 只有灯48）
    Step "5 write all-on"   ([byte[]](0x01,0x03,0x00,0x08,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x09,0x3E)) "01 04 00 08 01 FF FF FF FF FF FF 80"
    # 6 回读显存
    Step "6 read back"      ([byte[]](0x01,0x03,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x45,0xF1)) "01 04 00 08 00 FF FF FF FF FF FF 80"
    # 7 写全灭
    Step "7 write all-off"  ([byte[]](0x01,0x03,0x00,0x08,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x89,0x30)) "01 04 00 08 01 00 00 00 00 00 00 00"
} finally {
    $sp.Close()
}

Write-Output "---- $pass/$($pass+$fail) PASS ----"
if ($fail -ne 0) { exit 1 }
