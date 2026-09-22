# Associe l'extension .elg au logo Easy Language et a l'editeur graphique,
# pour l'utilisateur courant uniquement (aucun droit administrateur requis).
#
# A executer depuis le dossier contenant easy_editeur.exe et icone.ico
# (le dossier extrait du zip de distribution).

$ErrorActionPreference = "Stop"
$dossier = Split-Path -Parent $MyInvocation.MyCommand.Path
$exeEditeur = Join-Path $dossier "easy_editeur.exe"
$icone = Join-Path $dossier "icone.ico"

if (-not (Test-Path $exeEditeur)) {
    Write-Error "easy_editeur.exe introuvable a cote de ce script."
    exit 1
}
if (-not (Test-Path $icone)) {
    Write-Error "icone.ico introuvable a cote de ce script."
    exit 1
}

New-Item -Path "HKCU:\Software\Classes\.elg" -Force | Out-Null
Set-ItemProperty -Path "HKCU:\Software\Classes\.elg" -Name "(default)" -Value "EasyLanguage.Script"

New-Item -Path "HKCU:\Software\Classes\EasyLanguage.Script" -Force | Out-Null
Set-ItemProperty -Path "HKCU:\Software\Classes\EasyLanguage.Script" -Name "(default)" -Value "Script Easy Language"

New-Item -Path "HKCU:\Software\Classes\EasyLanguage.Script\DefaultIcon" -Force | Out-Null
Set-ItemProperty -Path "HKCU:\Software\Classes\EasyLanguage.Script\DefaultIcon" -Name "(default)" -Value "`"$icone`""

New-Item -Path "HKCU:\Software\Classes\EasyLanguage.Script\shell\open\command" -Force | Out-Null
Set-ItemProperty -Path "HKCU:\Software\Classes\EasyLanguage.Script\shell\open\command" -Name "(default)" -Value "`"$exeEditeur`" `"%1`""

# Force l'Explorateur a rafraichir le cache des icones tout de suite.
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Process explorer.exe

Write-Host "Termine : les fichiers .elg ont maintenant le logo Easy Language et s'ouvrent dans l'editeur au double-clic."
