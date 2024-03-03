# RFID Driver
## Project Overview
The goal of this project is to create a driver/library for the MFRC522 on Zephyr OS.
### Components
- ESP32-WROOM-32E microcontroller running Zephyr OS.
- MFRC522 RFID reader.
- MIFARE 1k card/fob.
### Features
- Connects to tag using the ISO/IEC 14443-A protocol.
- Authenticates, reads, and writes data using the MIFARE proprietary protocol.
## RC522 Driver
A driver for the MFRC522 already exists for the Arduino ecosystem. However, this project is built on Zephyr OS, so the driver needs to be ported to interop with the Zephyr SPI interface. The main difference comes in the functions to read from and write to the registers on the MFRC522, as these functions directly interact with the SPI bus. Many of the other functions look much more similar to the functions in the Arduino library, as they don’t need to interact with the SPI bus directly. The remaining differences are mostly either differences in the timing API, or small changes to the interface exposed to the application code. Not all features of the Arduino library are implemented, as they are not all necessary for this application (such as the anti-collision procedure). The final API includes functions to connect to the tag, to authenticate using the MIFARE protocol, and to read and write data to the blocks.
## References
### Datasheets
- [MIFARE RC522](https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf)
- [RFID-RC522 Module](https://www.handsontec.com/dataspecs/RC522.pdf)
- [MIFARE Classic EV1 1k](https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf)
- [FM11RF08](https://www.myplastikkarten.de/FILE/Tipografie/62/Contenuti/Download/sheet-rfid-karten-fudan-f08-1k-data-sheet-grafik-info.pdf)
### Videos
- [ISO/IEC 14443A Activation and AntiCollision](https://youtu.be/QDJMJ_INX3w?si=cH2V6cHwknUlnkjb)
### Documentation
- [Zephyr OS](https://docs.zephyrproject.org/latest/index.html)
- [ESP32-DevKitC V4](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/hw-reference/esp32/get-started-devkitc.html)
### Source Code
- [Arduino RC522 Library](https://github.com/miguelbalboa/rfid)
- [Zephyr OS](https://github.com/zephyrproject-rtos/zephyr)
- [Raspberry PI RC522 Library](https://github.com/nikkoluo/RC522-RFID-Reader)
### Textbooks
- RFID Handbook: Fundamentals and Applications in Contactless Smart Cards, Radio Frequency Identification and Near-Field Communication
### Articles
- [Miller Code](https://epxx.co/artigos/baseband_miller.html)
### Forums
- [ATQA Format](https://stackoverflow.com/questions/49257070/atqa-in-mifare-and-rfu-configurations)
### Guides
- [MIFARE Protocol Guide](https://rfidprodukter.com/images/produktblad/METRATEC_GmbH/MIFARE/metraTec_MiFare_Protocol-Guide_2-0.pdf)
- [MIFARE Type Identification Procedure](https://www.nxp.com/docs/en/application-note/AN10833.pdf)
- [Hacking MIFARE Classic Tags](https://github.com/XaviTorello/mifare-classic-toolkit/tree/master)
