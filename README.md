# Migration of the beeloogger.de Arduino .ino Files to a VisualCode PlatformIO variant

beelogger.de is a german Platform for measering hive-scales with an arduino nano. This is a port of the original beelogger.ino files to a visual code - PlatformIO compatible variant. 

The reason for this is mainly 
- Code-Editor is much better in VC
- Library-Maintanence is project dependend 
- much clearer compiler warnings
- support for git 

## Notes
- LMIC Library is not up to date - but a newer Version will not fit on the Arduino Nano; 

# Lora-Version
Diese Version ist die Beelogger-Version für LORA-Wan;

@Todo (german):
- Abhängigkeiten der Bibliotheken aus dem lib Verzeichnis in die platform.ini schieben - sofern möglich. (erledigt)
- Änderungen an den Bibliotheken habe ich aktuell noch nicht untersucht. (geprüft - alles funktional - Update der Bibliotheken auf ggf. neuere Versionen muss nochmal geprüft werden)

- Die Test-Sketches als Plattform-io Tests einbauen. (offen)


