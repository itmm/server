# Das Interaktive Server Projekt

In diesem Projekt stelle ich einen Server vor, dessen Dokumentenstruktur
in einem Repository abgelegt wird. Diese Dokumente generieren zum einen aus
Source-Datein fertige HTML-, CSS- und JS-Dateien, die direkt ausgeliefert
werden. Dadurch bestimmen sie die Schnittstelle des Servers.

Zum anderen können die Source-Dateien untereinander referenziert werden und
so unterschiedliche Detail-Grade anzeigen. Das Ziel in weiter Ferne ist es,
mit einer Server-Instanz Projekte zu verwalten.

Dabei sollen alle in einem Projekt benötigten Informationen in diesem Server
abgelegt und versioniert werden. Das Source-Format soll textbasiert sein,
damit auch ohne den Server alle Informationen verfügbar sind und über eine
besondere Server-Schnittstelle auch direkt verarbeitet werden können.

Als große Vision ist es die Zusammenführung vom Apapche Web-Server mit
Mediawiki oder WordPress. Dadurch, dass alles in einem Prozess läuft, fällt
ein großer Teil der Interprozess-Kommunikation weg und das Projekt wird
einfacher.

Geschrieben ist es in C++ mit Hilfe meines Dokumentations-Werkzeugs
[MD-Patcher](https://github.com/itmm/md-patcher).

Im ersten Schritt wird in [1_simple-server.md](./1_simple-server.md) ein
rudimentärer Server vorgestellt, der Schritt für Schritt erweitert wird.
