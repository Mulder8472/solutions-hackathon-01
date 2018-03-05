# jenkins status light

habe noch den Webserver eingebaut und diesen zum Konfigurieren des Hosts und Jobnamens verwendet. Allerdings aktuell ganz simpel per URL Parameter (kein Formular…).

Den Host und Jobname wie folgt per http GET konfigurieren (die IP natürlich ersetzen):  http://192.168.0.23/configure?host=jenkins.mono-project.com&jobname=test-linker-mainline

Hartcodiert sind aber schon ein Host und ein Jobname, damit direkt nach dem Starten der Jenkins abgefragt wird.
Nach Aufruf der obigen URL werden die Variablen host und jobname überschrieben und beim nächsten Durchlauf verwendet.

Das Ergebnis als byte ist dann im Terminal zu sehen.

ssid und password muss noch im code gesetzt werden fürs WLAN
