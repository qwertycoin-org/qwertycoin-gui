[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PackageDir,

    [Parameter(Mandatory = $true)]
    [string]$ArtifactName,

    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$SetupPath,

    [Parameter(Mandatory = $true)]
    [string]$IsccPath,

    [Parameter(Mandatory = $true)]
    [string]$ReportPath,

    [switch]$SkipExecutableSmoke
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$scriptDirectory = Split-Path -Parent $PSCommandPath
$createInstaller = Join-Path $scriptDirectory 'create_windows_installer.ps1'
$packagePath = [System.IO.Path]::GetFullPath($PackageDir)
$currentSetupPath = [System.IO.Path]::GetFullPath($SetupPath)
$reportFullPath = [System.IO.Path]::GetFullPath($ReportPath)
$testRoot = Join-Path $env:RUNNER_TEMP ("qwertycoin-installer-test-" + [guid]::NewGuid().ToString('N'))
$installPath = Join-Path $testRoot 'Programme mit Leerzeichen und Umlaut ü\Qwertycoin'
$results = [System.Collections.Generic.List[string]]::new()
$desktopLink = Join-Path ([Environment]::GetFolderPath('CommonDesktopDirectory')) 'Qwertycoin.lnk'
$startMenuLink = Join-Path ([Environment]::GetFolderPath('CommonPrograms')) 'Qwertycoin\Qwertycoin.lnk'

function Add-Result {
    param([string]$Text)
    $results.Add("- PASS: $Text")
}

function Wait-TestProcess {
    param(
        [System.Diagnostics.Process]$Process,
        [int]$TimeoutMilliseconds,
        [string]$Description
    )
    if (-not $Process.WaitForExit($TimeoutMilliseconds)) {
        try {
            Stop-Process -Id $Process.Id -Force -ErrorAction Stop
            $Process.WaitForExit()
        }
        catch {
            Write-Warning "Could not stop timed-out process $($Process.Id): $($_.Exception.Message)"
        }
        throw "$Description timed out after $TimeoutMilliseconds ms"
    }
    # Ensure redirected/native process state, including ExitCode, is complete.
    $Process.WaitForExit()
}

function Invoke-Setup {
    param(
        [string]$Executable,
        [string]$Destination,
        [switch]$ExpectFailure
    )
    $logPath = Join-Path $testRoot ("setup-" + [guid]::NewGuid().ToString('N') + '.log')
    $arguments = @(
        '/VERYSILENT',
        '/SUPPRESSMSGBOXES',
        '/NORESTART',
        '/CLOSEAPPLICATIONS',
        "/DIR=`"$Destination`"",
        "/LOG=`"$logPath`""
    )
    Write-Host "Running setup: $([System.IO.Path]::GetFileName($Executable)); expected failure: $ExpectFailure"
    $process = Start-Process -FilePath $Executable -ArgumentList $arguments -PassThru
    Wait-TestProcess -Process $process -TimeoutMilliseconds 300000 -Description 'Setup'
    if ($ExpectFailure) {
        if ($process.ExitCode -eq 0) {
            throw "Setup unexpectedly succeeded; log: $logPath"
        }
    }
    elseif ($process.ExitCode -ne 0) {
        throw "Setup failed with exit code $($process.ExitCode); log: $logPath"
    }
    return $process.ExitCode
}

function Assert-PackageInstalled {
    param([string]$ExpectedPackage, [string]$InstalledDirectory)
    foreach ($source in Get-ChildItem -LiteralPath $ExpectedPackage -Recurse -File) {
        $relative = [System.IO.Path]::GetRelativePath($ExpectedPackage, $source.FullName)
        $target = Join-Path $InstalledDirectory $relative
        if (-not (Test-Path -LiteralPath $target -PathType Leaf)) {
            throw "Installed package is missing $relative"
        }
        $sourceHash = (Get-FileHash -LiteralPath $source.FullName -Algorithm SHA256).Hash
        $targetHash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
        if ($sourceHash -ne $targetHash) {
            throw "Installed package digest differs for $relative"
        }
    }
}

function Get-UserDataDigests {
    param([string[]]$Paths)
    $digests = @{}
    foreach ($path in $Paths) {
        $digests[$path] = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    }
    return $digests
}

function Assert-UserDataDigests {
    param([hashtable]$Expected)
    foreach ($path in $Expected.Keys) {
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "User data was removed: $path"
        }
        $actual = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ($actual -ne $Expected[$path]) {
            throw "User data was modified: $path"
        }
    }
}

function Assert-Shortcut {
    param([string]$ShortcutPath, [string]$ExpectedTarget, [string]$ExpectedWorkingDirectory)
    if (-not (Test-Path -LiteralPath $ShortcutPath -PathType Leaf)) {
        throw "Shortcut is missing: $ShortcutPath"
    }
    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($ShortcutPath)
    if ([System.IO.Path]::GetFullPath($shortcut.TargetPath) -ne [System.IO.Path]::GetFullPath($ExpectedTarget)) {
        throw "Shortcut target is incorrect: $ShortcutPath"
    }
    if ([System.IO.Path]::GetFullPath($shortcut.WorkingDirectory) -ne [System.IO.Path]::GetFullPath($ExpectedWorkingDirectory)) {
        throw "Shortcut working directory is incorrect: $ShortcutPath"
    }
}

function Find-UninstallEntries {
    # Inno Setup derives the uninstall key from the stable AppId and appends
    # _is1.  Query that exact key instead of guessing from optional ARP values
    # or scanning unrelated software entries.
    $uninstallSubkey = '{BEBB425B-5F3A-4F6C-AC09-DE09BE430880}_is1'
    $roots = @(
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall',
        'HKLM:\Software\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall'
    )
    $entries = @()
    foreach ($root in $roots) {
        $entryPath = Join-Path $root $uninstallSubkey
        if (Test-Path -LiteralPath $entryPath) {
            $entry = Get-ItemProperty -LiteralPath $entryPath
            $displayName = $entry.PSObject.Properties['DisplayName']
            $appPath = $entry.PSObject.Properties['Inno Setup: App Path']
            if ($null -eq $displayName -or $displayName.Value -ne 'Qwertycoin') {
                throw "Installed-app entry has an unexpected DisplayName: $entryPath"
            }
            if ($null -eq $appPath -or -not $appPath.Value) {
                throw "Installed-app entry is missing Inno Setup: App Path: $entryPath"
            }
            if ([System.IO.Path]::GetFullPath($appPath.Value) -ne [System.IO.Path]::GetFullPath($installPath)) {
                throw "Installed-app entry points to an unexpected directory: $entryPath"
            }
            $entries += $entry
        }
    }
    return @($entries)
}

function Remove-TestInstallation {
    $uninstaller = Join-Path $installPath '.qwertycoin-installer\uninstall\unins000.exe'
    if (Test-Path -LiteralPath $uninstaller -PathType Leaf) {
        Write-Host 'Running test uninstaller'
        $process = Start-Process -FilePath $uninstaller -ArgumentList @(
            '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
        ) -PassThru
        Wait-TestProcess -Process $process -TimeoutMilliseconds 180000 -Description 'Uninstaller'
        if ($process.ExitCode -ne 0) {
            throw "Uninstaller failed with exit code $($process.ExitCode)"
        }
    }
}

New-Item -ItemType Directory -Path $testRoot | Out-Null
$userDigests = $null

try {
    if (-not (Test-Path -LiteralPath $currentSetupPath -PathType Leaf)) {
        throw "Current setup does not exist: $currentSetupPath"
    }

    $occupiedPath = Join-Path $testRoot 'occupied-first-install'
    New-Item -ItemType Directory -Path $occupiedPath | Out-Null
    $occupiedWallet = Join-Path $occupiedPath 'wallet without extension'
    [System.IO.File]::WriteAllText($occupiedWallet, 'never overwrite this wallet')
    $occupiedHash = (Get-FileHash -LiteralPath $occupiedWallet -Algorithm SHA256).Hash
    Invoke-Setup -Executable $currentSetupPath -Destination $occupiedPath -ExpectFailure | Out-Null
    if ((Get-FileHash -LiteralPath $occupiedWallet -Algorithm SHA256).Hash -ne $occupiedHash) {
        throw 'Rejected first installation changed the existing file'
    }
    if (Test-Path -LiteralPath (Join-Path $occupiedPath 'qwertycoin-gui.exe')) {
        throw 'Rejected first installation wrote program files'
    }
    Add-Result 'a non-empty unknown first-install directory is rejected before modification'

    $versionParts = $Version.Split('.') | ForEach-Object { [int]$_ }
    if ($versionParts[2] -gt 0) {
        $previousVersion = "$($versionParts[0]).$($versionParts[1]).$($versionParts[2] - 1)"
    }
    elseif ($versionParts[1] -gt 0) {
        $previousVersion = "$($versionParts[0]).$($versionParts[1] - 1).0"
    }
    elseif ($versionParts[0] -gt 0) {
        $previousVersion = "$($versionParts[0] - 1).0.0"
    }
    else {
        throw 'Cannot derive a lower semantic version for the upgrade fixture'
    }
    $previousArtifact = "qwertycoin-gui-v$previousVersion-windows-x86_64"
    $previousPackage = Join-Path $testRoot $previousArtifact
    Copy-Item -LiteralPath $packagePath -Destination $previousPackage -Recurse
    $previousBuildInfo = Join-Path $previousPackage 'BUILD-INFO.txt'
    $updatedBuildInfo = (Get-Content -LiteralPath $previousBuildInfo) | ForEach-Object {
        if ($_ -match '^app_version=') { "app_version=$previousVersion" } else { $_ }
    }
    [System.IO.File]::WriteAllLines(
        $previousBuildInfo,
        $updatedBuildInfo,
        [System.Text.UTF8Encoding]::new($false)
    )
    [System.IO.File]::WriteAllText(
        (Join-Path $previousPackage 'obsolete-runtime.dll'),
        'old managed runtime file'
    )
    $previousOutput = Join-Path $testRoot 'previous-output'
    & $createInstaller `
        -PackageDir $previousPackage `
        -ArtifactName $previousArtifact `
        -Version $previousVersion `
        -OutputDir $previousOutput `
        -IsccPath $IsccPath
    $previousSetup = Join-Path $previousOutput "$previousArtifact-setup.exe"

    Invoke-Setup -Executable $previousSetup -Destination $installPath | Out-Null
    Assert-PackageInstalled -ExpectedPackage $previousPackage -InstalledDirectory $installPath
    Assert-Shortcut -ShortcutPath $desktopLink `
        -ExpectedTarget (Join-Path $installPath 'qwertycoin-gui.exe') `
        -ExpectedWorkingDirectory $installPath
    Assert-Shortcut -ShortcutPath $startMenuLink `
        -ExpectedTarget (Join-Path $installPath 'qwertycoin-gui.exe') `
        -ExpectedWorkingDirectory $installPath
    if (@(Find-UninstallEntries).Count -ne 1) {
        throw 'Fresh installation did not create exactly one installed-app entry'
    }
    Add-Result 'fresh all-users installation, desktop/start-menu shortcuts and installed-app entry'

    $userFiles = @(
        (Join-Path $installPath 'qwertycoin-storage\settings.ini'),
        (Join-Path $installPath 'user-state\wallet without extension'),
        (Join-Path $installPath 'user-state\wallet.keys'),
        (Join-Path $installPath 'user-state\wallet-cache'),
        (Join-Path $installPath 'epose-test\service-keystore-v2'),
        (Join-Path $installPath 'blockchain-test\lmdb\data.mdb'),
        (Join-Path $installPath 'notes\manuell übrig.txt')
    )
    $counter = 0
    foreach ($path in $userFiles) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $path) -Force | Out-Null
        [System.IO.File]::WriteAllText($path, "isolated user data $counter")
        $counter++
    }
    $userDigests = Get-UserDataDigests -Paths $userFiles

    [System.IO.File]::WriteAllText((Join-Path $installPath 'Qt5Core.dll'), 'damaged old DLL')
    Remove-Item -LiteralPath (Join-Path $installPath 'Qt5Svg.dll')
    Invoke-Setup -Executable $currentSetupPath -Destination $installPath | Out-Null
    Assert-PackageInstalled -ExpectedPackage $packagePath -InstalledDirectory $installPath
    Assert-UserDataDigests -Expected $userDigests
    if (Test-Path -LiteralPath (Join-Path $installPath 'obsolete-runtime.dll')) {
        throw 'Upgrade retained a package-owned obsolete file'
    }
    if (@(Find-UninstallEntries).Count -ne 1) {
        throw 'Upgrade did not retain exactly one installed-app entry'
    }
    Assert-Shortcut -ShortcutPath $desktopLink `
        -ExpectedTarget (Join-Path $installPath 'qwertycoin-gui.exe') `
        -ExpectedWorkingDirectory $installPath
    Add-Result 'upgrade replaces all current program files, removes only obsolete managed files and preserves user data'

    [System.IO.File]::WriteAllText((Join-Path $installPath 'Qt5Core.dll'), 'damaged current DLL')
    Remove-Item -LiteralPath (Join-Path $installPath 'Qt5Svg.dll')
    Invoke-Setup -Executable $currentSetupPath -Destination $installPath | Out-Null
    Assert-PackageInstalled -ExpectedPackage $packagePath -InstalledDirectory $installPath
    Assert-UserDataDigests -Expected $userDigests
    Add-Result 'same-version repair restores a modified DLL and a missing package file'

    $guiHashBeforeDowngrade = (Get-FileHash -LiteralPath (Join-Path $installPath 'qwertycoin-gui.exe') -Algorithm SHA256).Hash
    Invoke-Setup -Executable $previousSetup -Destination $installPath -ExpectFailure | Out-Null
    if ((Get-FileHash -LiteralPath (Join-Path $installPath 'qwertycoin-gui.exe') -Algorithm SHA256).Hash -ne $guiHashBeforeDowngrade) {
        throw 'Rejected downgrade changed the installed GUI'
    }
    Assert-UserDataDigests -Expected $userDigests
    Add-Result 'downgrade is rejected before changing program or user files'

    $outsidePath = Join-Path $testRoot 'outside-junction-target'
    New-Item -ItemType Directory -Path $outsidePath | Out-Null
    $platformPath = Join-Path $installPath 'platforms'
    $platformBackup = Join-Path $testRoot 'platforms-original'
    Move-Item -LiteralPath $platformPath -Destination $platformBackup
    New-Item -ItemType Junction -Path $platformPath -Target $outsidePath | Out-Null
    Invoke-Setup -Executable $currentSetupPath -Destination $installPath -ExpectFailure | Out-Null
    if (Get-ChildItem -LiteralPath $outsidePath -Force) {
        throw 'Rejected reparse-point installation wrote outside the install directory'
    }
    Remove-Item -LiteralPath $platformPath -Force
    Move-Item -LiteralPath $platformBackup -Destination $platformPath
    Assert-UserDataDigests -Expected $userDigests
    Add-Result 'reparse-point traversal is rejected without writes outside the install directory'

    $oldQtHash = (Get-FileHash -LiteralPath (Join-Path $installPath 'Qt5Core.dll') -Algorithm SHA256).Hash
    $lockScript = @"
`$stream = [System.IO.File]::Open('$((Join-Path $installPath 'Qt5Core.dll').Replace("'", "''"))', 'Open', 'Read', 'None')
[System.IO.File]::WriteAllText('$((Join-Path $testRoot 'lock-ready').Replace("'", "''"))', 'ready')
Start-Sleep -Seconds 90
`$stream.Dispose()
"@
    $encodedLockScript = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($lockScript))
    $lockProcess = Start-Process -FilePath 'powershell.exe' -ArgumentList @(
        '-NoProfile', '-NonInteractive', '-EncodedCommand', $encodedLockScript
    ) -PassThru -WindowStyle Hidden
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds(15)
        while (-not (Test-Path -LiteralPath (Join-Path $testRoot 'lock-ready'))) {
            if ([DateTime]::UtcNow -gt $deadline) {
                throw 'Timed out waiting for the locked-file fixture'
            }
            Start-Sleep -Milliseconds 100
        }
        Invoke-Setup -Executable $currentSetupPath -Destination $installPath -ExpectFailure | Out-Null
    }
    finally {
        if (-not $lockProcess.HasExited) {
            Stop-Process -Id $lockProcess.Id -Force
        }
        $lockProcess.WaitForExit()
    }
    if ((Get-FileHash -LiteralPath (Join-Path $installPath 'Qt5Core.dll') -Algorithm SHA256).Hash -ne $oldQtHash) {
        throw 'Locked-file failure changed the locked program file'
    }
    Assert-UserDataDigests -Expected $userDigests
    Add-Result 'a still-locked program file aborts without a mixed install or user-data changes'

    if (-not $SkipExecutableSmoke) {
        $env:QT_QPA_PLATFORM = 'offscreen'
        foreach ($command in @(
            @{ Path = (Join-Path $installPath 'qwertycoin-gui.exe'); Arguments = @('--test-qml') },
            @{ Path = (Join-Path $installPath 'qwertycoind.exe'); Arguments = @('--version') },
            @{ Path = (Join-Path $installPath 'qwertycoin-wallet-cli.exe'); Arguments = @('--version') },
            @{ Path = (Join-Path $installPath 'qwertycoin-wallet-rpc.exe'); Arguments = @('--version') }
        )) {
            Write-Host "Smoke testing $([System.IO.Path]::GetFileName($command.Path))"
            $process = Start-Process -FilePath $command.Path -ArgumentList $command.Arguments `
                -WorkingDirectory $installPath -PassThru
            Wait-TestProcess -Process $process -TimeoutMilliseconds 30000 `
                -Description "Executable smoke test for $($command.Path)"
            if ($process.ExitCode -ne 0) {
                throw "Installed executable smoke test failed: $($command.Path)"
            }
        }
        Add-Result 'installed GUI/Core entry points start from the intended working directory without external runtime downloads'
    }

    Remove-TestInstallation
    Assert-UserDataDigests -Expected $userDigests
    foreach ($source in Get-ChildItem -LiteralPath $packagePath -Recurse -File) {
        $relative = [System.IO.Path]::GetRelativePath($packagePath, $source.FullName)
        if (Test-Path -LiteralPath (Join-Path $installPath $relative)) {
            throw "Uninstall retained an installer-owned program file: $relative"
        }
    }
    if (@(Find-UninstallEntries).Count -ne 0) {
        throw 'Uninstall retained the installed-app entry'
    }
    if ((Test-Path -LiteralPath $desktopLink) -or (Test-Path -LiteralPath $startMenuLink)) {
        throw 'Uninstall retained an installer-created shortcut'
    }
    Add-Result 'uninstall removes managed programs and links while preserving every isolated user-data fixture byte-for-byte'

    New-Item -ItemType Directory -Path (Split-Path -Parent $reportFullPath) -Force | Out-Null
    $report = @(
        '# Windows installer native verification',
        '',
        "- Package: ``$ArtifactName``",
        "- Version: ``$Version``",
        '- Runner: GitHub-hosted Windows x86_64',
        '- User data: isolated fixtures only; no real wallet or funds',
        '- Installer signing: unsigned unless a later release-signing stage is added',
        '',
        '## Executed checks',
        ''
    ) + $results
    [System.IO.File]::WriteAllLines($reportFullPath, $report, [System.Text.UTF8Encoding]::new($false))
    Write-Host "Windows installer verification completed: $reportFullPath"
}
finally {
    try {
        Remove-TestInstallation
    }
    catch {
        Write-Warning "Cleanup uninstaller failed: $($_.Exception.Message)"
    }
    if (Test-Path -LiteralPath $desktopLink) {
        Remove-Item -LiteralPath $desktopLink -Force
    }
    if (Test-Path -LiteralPath $startMenuLink) {
        Remove-Item -LiteralPath $startMenuLink -Force
    }
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
