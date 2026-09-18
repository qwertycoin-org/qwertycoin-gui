[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$PackageDir,

    [Parameter(Mandatory = $true)]
    [string]$ArtifactName,

    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    [Parameter(Mandatory = $true)]
    [string]$IsccPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Assert-SafeCompilerValue {
    param([string]$Name, [string]$Value)
    if ([string]::IsNullOrWhiteSpace($Value) -or $Value.IndexOfAny([char[]]"`r`n`"") -ge 0) {
        throw "$Name contains unsupported characters"
    }
}

if ($ArtifactName -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$') {
    throw 'ArtifactName contains unsupported characters'
}
if ($Version -notmatch '^[0-9]+\.[0-9]+\.[0-9]+$') {
    throw 'Version must contain exactly three numeric components'
}
$artifactPattern = '^qwertycoin-gui-v(?<display>[0-9]+\.[0-9]+\.[0-9]+(?:-rc[1-9][0-9]*)?)(?:-review-(?<review>[0-9a-f]{7,40}))?-windows-x86_64$'
if ($ArtifactName -notmatch $artifactPattern) {
    throw 'ArtifactName must follow qwertycoin-gui-v<VERSION>[-rcN][-review-<COMMIT>]-windows-x86_64'
}
$displayVersion = $Matches.display
$reviewRevision = if ($Matches.ContainsKey('review')) { $Matches.review } else { '' }
if (($displayVersion -ne $Version) -and (-not $displayVersion.StartsWith("$Version-rc"))) {
    throw "ArtifactName version $displayVersion does not match source version $Version"
}

$scriptDirectory = Split-Path -Parent $PSCommandPath
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptDirectory '..\..\..'))
$packagePath = [System.IO.Path]::GetFullPath($PackageDir)
$outputPath = [System.IO.Path]::GetFullPath($OutputDir)
$compilerPath = [System.IO.Path]::GetFullPath($IsccPath)
$setupPath = Join-Path $outputPath "$ArtifactName-setup.exe"
$checksumPath = "$setupPath.sha256"

if (-not (Test-Path -LiteralPath $packagePath -PathType Container)) {
    throw "Package directory does not exist: $packagePath"
}
if ((Split-Path -Leaf $packagePath) -ne $ArtifactName) {
    throw 'Package directory name must exactly match ArtifactName'
}
if (-not (Test-Path -LiteralPath $compilerPath -PathType Leaf)) {
    throw "ISCC.exe does not exist: $compilerPath"
}
if (Test-Path -LiteralPath $setupPath -PathType Any) {
    throw "Refusing to overwrite existing setup: $setupPath"
}
if (Test-Path -LiteralPath $checksumPath -PathType Any) {
    throw "Refusing to overwrite existing setup checksum: $checksumPath"
}

$requiredFiles = @(
    'qwertycoin-gui.exe',
    'qwertycoind.exe',
    'qwertycoin-wallet-cli.exe',
    'qwertycoin-wallet-rpc.exe',
    'BUILD-INFO.txt',
    'LICENSE',
    'README.md',
    'platforms\qwindows.dll'
)
foreach ($relativePath in $requiredFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $packagePath $relativePath) -PathType Leaf)) {
        throw "Verified package is missing required file: $relativePath"
    }
}

$buildInfo = @{}
foreach ($line in Get-Content -LiteralPath (Join-Path $packagePath 'BUILD-INFO.txt')) {
    if ($line -notmatch '^([a-z_]+)=([^\r\n]+)$') {
        throw "Malformed BUILD-INFO.txt line: $line"
    }
    if ($buildInfo.ContainsKey($Matches[1])) {
        throw "Duplicate BUILD-INFO.txt key: $($Matches[1])"
    }
    $buildInfo[$Matches[1]] = $Matches[2]
}
foreach ($key in @('source_revision', 'core_revision', 'app_version', 'runner_os', 'runner_arch', 'qt_version')) {
    if (-not $buildInfo.ContainsKey($key)) {
        throw "BUILD-INFO.txt is missing $key"
    }
}
if ($buildInfo.source_revision -notmatch '^[0-9a-f]{40}$' -or
    $buildInfo.core_revision -notmatch '^[0-9a-f]{40}$') {
    throw 'BUILD-INFO.txt contains invalid source revisions'
}
if ($reviewRevision -ne '' -and -not $buildInfo.source_revision.StartsWith($reviewRevision)) {
    throw "Review artifact revision $reviewRevision does not match package source revision $($buildInfo.source_revision)"
}
if ($buildInfo.runner_os -ne 'Windows' -or $buildInfo.runner_arch -ne 'X64') {
    throw 'Package is not the expected Windows X64 build'
}
if ($buildInfo.app_version -ne $Version) {
    throw "Package application version $($buildInfo.app_version) does not match requested version $Version"
}

foreach ($value in @($packagePath, $outputPath, $repositoryRoot, $buildInfo.source_revision, $buildInfo.core_revision)) {
    Assert-SafeCompilerValue -Name 'compiler define' -Value $value
}

New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("qwertycoin-installer-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null

try {
    $manifestPath = Join-Path $temporaryRoot 'program-files-v1.sha256'
    $fileListPath = Join-Path $temporaryRoot 'program-files.generated.iss'
    $manifestTool = Join-Path $scriptDirectory 'package_manifest.py'
    & python $manifestTool build `
        --package-dir $packagePath `
        --manifest $manifestPath `
        --inno-file-list $fileListPath
    if ($LASTEXITCODE -ne 0) {
        throw 'Program package manifest generation failed'
    }
    & python $manifestTool verify --package-dir $packagePath --manifest $manifestPath
    if ($LASTEXITCODE -ne 0) {
        throw 'Program package manifest verification failed'
    }

    $manifestHash = (Get-FileHash -LiteralPath $manifestPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $innoScript = Join-Path $scriptDirectory 'Qwertycoin.iss'
    $compilerArguments = @(
        '/Qp',
        "/DAppVersion=$Version",
        "/DAppDisplayVersion=$displayVersion",
        "/DArtifactName=$ArtifactName",
        "/DPackageDir=$packagePath",
        "/DOutputDir=$outputPath",
        "/DProgramManifest=$manifestPath",
        "/DProgramManifestSHA256=$manifestHash",
        "/DProgramFileList=$fileListPath",
        "/DSourceRevision=$($buildInfo.source_revision)",
        "/DCoreRevision=$($buildInfo.core_revision)",
        "/DRepositoryRoot=$repositoryRoot",
        $innoScript
    )
    & $compilerPath @compilerArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Inno Setup compiler failed with exit code $LASTEXITCODE"
    }
    if (-not (Test-Path -LiteralPath $setupPath -PathType Leaf)) {
        throw "Inno Setup did not create the expected output: $setupPath"
    }

    $signature = Get-AuthenticodeSignature -LiteralPath $setupPath
    if ($signature.Status -notin @('NotSigned', 'Valid')) {
        throw "Unexpected setup Authenticode status: $($signature.Status)"
    }
    $setupHash = (Get-FileHash -LiteralPath $setupPath -Algorithm SHA256).Hash.ToLowerInvariant()
    [System.IO.File]::WriteAllText(
        $checksumPath,
        "$setupHash  $ArtifactName-setup.exe`n",
        [System.Text.UTF8Encoding]::new($false)
    )
    Write-Host "Created $setupPath"
    Write-Host "SHA-256 $setupHash"
    Write-Host "Authenticode $($signature.Status)"
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot -PathType Container) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
