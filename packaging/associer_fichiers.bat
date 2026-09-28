@echo off
rem Double-cliquez sur ce fichier pour associer le logo Easy Language
rem et l'editeur aux fichiers .elg (aucun droit administrateur requis).
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0associer_fichiers.ps1"
pause
