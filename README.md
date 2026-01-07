# Chrono Genesis - Telnet Server Framework

Ein hochperformantes, in C geschriebenes Framework für "Newschool Oldschool" Text-Adventures, MUDs oder das ultimative System-Administration-Tool Supreme.

Dieses Projekt startete als PHP-Prototyp und wurde zu einem vollwertigen, multithreaded C-Server portiert, der moderne Features mit dem Charme alter BBS-Systeme verbindet.

## 🚀 Features

*   **Multithreading:** Jeder Client läuft in einem eigenen Thread. Skaliert auf tausende gleichzeitige Verbindungen.
*   **SQLite Backend:** Persistente Speicherung von Benutzern, Passwörtern und Statistiken (Traffic).
*   **Advanced TUI Framework:**
    *   Fenster, Boxen, Karten und Listen.
    *   UTF-8 Support & ASCII Art Fonts.
    *   Dynamisches Resizing (NAWS - Negotiate About Window Size).
    *   Echtzeit-Rendering mit dynamischer FPS-Begrenzung.
*   **3D Engine:** Ein rotierender 3D-Würfel, gerendert in ASCII/ANSI mit "Half-Block" Zeichen.
*   **XCMD Support:** Experimenteller Support für Client-seitige Befehlsausführung (z.B. TTS/Sprachausgabe) via Telnet Subnegotiation (`IAC SB 200`).

## 🛠️ Build & Run

### Voraussetzungen
*   Linux
*   GCC & Make
*   `libsqlite3-dev`

### Kompilieren
```bash
make
```

### Starten
```bash
./server
```

### Verbinden
```bash
telnet localhost 12345
```

## 🎮 Steuerung

*   **Pfeiltasten:** Navigation
*   **Enter:** Bestätigen
*   **F1:** Intro Screen
*   **F2:** 3D Cube Demo
*   **ESC:** Logout / Disconnect

## ⚙️ Konfiguration

Timings, Timeouts und FPS-Raten können direkt im Header von `server.c` (Abschnitt `Configuration & Timings`) angepasst werden.

## 📝 Lizenz

Free for all. Hack the Planet.