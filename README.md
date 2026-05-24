# Proiect_PM_2026
Inima Interactiva

# Inima Interactiva
https://ocw.cs.pub.ro/courses/pm/prj2026/alexandru.jipa2803/andreea.voinea1305

## Descriere

Proiectul consta intr-o inima interactiva realizata din 11 LED-uri RGB montate pe breadboard. Sistemul foloseste un senzor de puls HW-827, un display OLED SSD1306, un buzzer pasiv si un buton de control.

Utilizatorul apasa butonul pentru a porni sistemul. Dupa o etapa scurta de calibrare, sistemul asteapta detectarea degetului pe senzor. Cand degetul este detectat, incepe o sesiune de 60 de secunde. In timpul sesiunii, LED-urile rosii pulseaza sincron cu buzzerul, iar display-ul afiseaza timpul ramas si valoarea BPM.

La final:

- LED-ul verde indica un BPM normal.
- LED-ul rosu indica un BPM in afara intervalului normal.

Intervalul considerat normal este **60-100 BPM**.

> In implementarea finala, senzorul este folosit pentru detectarea prezentei degetului, iar ritmul cardiac este generat software pentru o demonstratie stabila.

---

## Componente

| Componenta | Cantitate |
|---|---:|
| ATmega328P Xplained Mini | 1 |
| Senzor puls HW-827 | 1 |
| LED-uri RGB anod comun | 11 |
| Display OLED SSD1306 I2C | 1 |
| Buzzer pasiv 5V | 1 |
| Buton tactil | 1 |
| Tranzistoare BD139 | 2 |
| Rezistente 1K ohm | multiple |
| Rezistente 220 ohm | multiple |
| Breadboard | 2 |
| Fire jumper | multiple |

---

## Conexiuni principale

| Modul | Pin microcontroller |
|---|---|
| Canal rosu LED-uri | PD5 |
| Canal verde LED-uri | PD6 |
| Buzzer | PD1 |
| Buton | PD2 |
| Senzor puls S / OUT | PC0 / ADC0 |
| OLED SDA | PC4 |
| OLED SCL | PC5 |

Senzorul este alimentat la **5V**, iar GND-ul senzorului este comun cu GND-ul placii.

LED-urile RGB sunt de tip **anod comun**. Pinul comun este legat la `+5V`, iar canalele de culoare sunt comandate prin tranzistoare BD139 pe partea de `GND`.

---

## Software

Codul este scris in C pentru ATmega328P, fara framework Arduino. Sunt folosite direct registrele microcontrollerului pentru:

- GPIO;
- ADC;
- I2C / TWI;
- Timer0;
- intreruperi;
- control OLED SSD1306.

Programul este organizat ca masina de stari:

| Stare | Descriere |
|---|---|
| `STATE_OFF` | sistem oprit |
| `STATE_CALIB` | calibrare scurta |
| `STATE_WAIT_FINGER` | asteapta degetul pe senzor |
| `STATE_MEASURING` | sesiune activa de 60 secunde |

Fluxul principal:

1. Sistemul porneste in starea `OFF`.
2. Butonul porneste secventa.
3. Sistemul intra in `CALIB`.
4. Apoi afiseaza `FINGER` si asteapta degetul.
5. Dupa detectarea degetului, incepe sesiunea de 60 secunde.
6. LED-urile rosii si buzzerul pulseaza sincronizat.
7. Display-ul afiseaza timpul ramas si BPM-ul.
8. La final se afiseaza rezultatul si se aprinde rosu sau verde.

VIDEO DEMONSTRATIV: https://youtube.com/shorts/KqERROrqigY?feature=share
