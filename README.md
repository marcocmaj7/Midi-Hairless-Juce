# Hairless MIDI Serial Bridge - JUCE Version

Una moderna reimplementazione del [Hairless MIDI Serial Bridge](http://projectgus.github.com/hairless-midiserial/) utilizzando il framework JUCE.

## Descrizione

Hairless MIDI Serial Bridge è il modo più semplice per connettere dispositivi seriali (come Arduino) per inviare e ricevere segnali MIDI. Questa versione utilizza JUCE invece di Qt, offrendo:

- **Cross-platform**: Supporto nativo per Windows, macOS e Linux
- **Moderna**: Costruito con JUCE 7.x e C++17
- **MIDI nativo**: Utilizza le API MIDI di JUCE per gestione ottimale
- **Comunicazione seriale**: Implementazione custom per porte seriali multi-piattaforma

## Caratteristiche

- ✅ Bridge bidirezionale MIDI ↔ Serial
- ✅ Supporto per tutti i messaggi MIDI standard
- ✅ Supporto SysEx
- ✅ Indicatori LED visivi per traffico MIDI/Serial
- ✅ Modalità debug con log dettagliati
- ✅ Selezione intuitiva delle porte
- ✅ Running Status MIDI
- ✅ Messaggi di debug custom dal device seriale

## Requisiti

### Per compilare
- **CMake** 3.15 o superiore
- **JUCE** 7.x (scaricato automaticamente da CMake)
- **Compilatore C++17** compatibile:
  - Windows: Visual Studio 2019+ o MinGW
  - macOS: Xcode 11+
  - Linux: GCC 7+ o Clang 6+

### Dipendenze di sistema

#### Windows
- Nessuna dipendenza aggiuntiva

#### macOS
- Framework IOKit (incluso in Xcode)

#### Linux
```bash
sudo apt-get install libasound2-dev libfreetype6-dev libx11-dev \
                     libxinerama-dev libxrandr-dev libxcursor-dev \
                     libgl1-mesa-dev libudev-dev
```

## Compilazione

### Windows

```powershell
# Crea directory di build
mkdir build
cd build

# Genera progetto con CMake
cmake .. -G "Visual Studio 17 2022"  # oppure "MinGW Makefiles"

# Compila
cmake --build . --config Release

# L'eseguibile sarà in: build\HairlessMidiSerial_artefacts\Release\HairlessMidiSerial.exe
```

### macOS

```bash
# Crea directory di build
mkdir build && cd build

# Genera progetto Xcode
cmake .. -G Xcode

# Compila
cmake --build . --config Release

# L'app sarà in: build/HairlessMidiSerial_artefacts/Release/HairlessMidiSerial.app
```

### Linux

```bash
# Crea directory di build
mkdir build && cd build

# Genera Makefile
cmake ..

# Compila
make -j$(nproc)

# L'eseguibile sarà in: build/HairlessMidiSerial_artefacts/HairlessMidiSerial
```

## Utilizzo

1. **Avvia l'applicazione**
2. **Seleziona la porta seriale** dal menu a tendina (es. COM3 su Windows, /dev/ttyUSB0 su Linux)
3. **Seleziona MIDI In** (opzionale) - per ricevere MIDI e inviarlo alla porta seriale
4. **Seleziona MIDI Out** (opzionale) - per ricevere dalla seriale e inviare a MIDI
5. **Attiva il Bridge** cliccando su "Bridge Active"
6. **Monitor LED** - Osserva gli indicatori LED per vedere il traffico in tempo reale
7. **Debug** - Abilita "Show Debug Messages" per vedere messaggi dettagliati

### Con Arduino

Usa la libreria [ardumidi](https://github.com/projectgus/hairless-midiserial/tree/master/ardumidi) per la comunicazione MIDI su Arduino.

Esempio base:
```cpp
#include <ardumidi.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    // Invia nota MIDI
    midi_note_on(0, 60, 127);  // Channel 1, Middle C, velocity 127
    delay(500);
    midi_note_off(0, 60, 0);
    delay(500);
}
```

## Struttura del Progetto

```
Midi-Hairless-JUCE/
├── CMakeLists.txt              # Configurazione CMake
├── README.md                   # Questo file
└── Source/
    ├── Main.cpp                # Entry point applicazione
    ├── MainComponent.h/cpp     # Interfaccia utente principale
    ├── MidiSerialBridge.h/cpp  # Logica bridge MIDI-Serial
    └── SerialPortManager.h/cpp # Gestione porte seriali
```

## Differenze dalla versione Qt

- **Framework**: JUCE invece di Qt
- **Build system**: CMake invece di qmake
- **MIDI**: API JUCE native invece di RtMidi
- **Serial**: Implementazione custom invece di qextserialport
- **UI**: Componenti JUCE invece di Qt Widgets
- **Moderna**: C++17, design pattern aggiornati

## Licenza

Questo progetto mantiene la stessa licenza dell'originale Hairless MIDI Serial Bridge.

## Crediti

- **Progetto originale**: [Hairless MIDI Serial Bridge](https://github.com/projectgus/hairless-midiserial) di Angus Gratton
- **JUCE Framework**: [JUCE](https://juce.com/)
- **Reimplementazione JUCE**: 2025

## Troubleshooting

### Windows: "Porta seriale non trovata"
- Verifica che i driver USB-Serial siano installati
- Controlla in Gestione Dispositivi che la porta COM sia riconosciuta

### Linux: "Permission denied" sulla porta seriale
```bash
# Aggiungi il tuo utente al gruppo dialout
sudo usermod -a -G dialout $USER
# Poi logout/login
```

### macOS: "L'applicazione non può essere aperta"
```bash
# Rimuovi quarantena
xattr -cr HairlessMidiSerial.app
```

## Sviluppo

Per contribuire o modificare il codice:

```bash
# Clone del repository
git clone https://github.com/tuousername/Midi-Hairless-JUCE.git
cd Midi-Hairless-JUCE

# Apri in IDE (opzionale)
# - Visual Studio: apri build/HairlessMidiSerial.sln
# - Xcode: apri build/HairlessMidiSerial.xcodeproj
# - CLion/VS Code: apri la cartella root
```

## Supporto

Per bug report e feature request, apri una issue su GitHub.
