# ase-project

RFID card reader using a RC522 and an ESP32 microcontroller.  
Project for the subject Arquitetura para Sistemas Embutidos (Embedded Systems Architecture).

## Requirements

- ESP32 microcontroller
- RFID-RC522 reader
- Expressif IDF framework
- Espressif IDF VSCode extension (optional)
- ESP-IDF Power Shell (optional)

## Setup

The setup of the project can be make made using the Espressif IDF VSCode extension by clicking on ESP-IDF Build, Flash and Monitor buttons.

To setup using the terminal, either using ESP-IDF Power Shell or a terminal with the ESP-IDF environment variables set, run the following commands:

```bash
idf.py build                # Build the project
idf.py -p <port> flash      # Flash the project to the microcontroller
idf.py -p <port> monitor    # Monitor the serial output
```

On Windows, the port can be found on Device Manager, under Ports (COM & LPT).  
To exit the monitor, press ```Ctrl + ]``` or ```Ctrl + T Ctrl + X```.