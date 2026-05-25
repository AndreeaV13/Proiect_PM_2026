# Proiect_PM_2026
# Inima Interactiva
https://ocw.cs.pub.ro/courses/pm/prj2026/alexandru.jipa2803/andreea.voinea1305
## Descriere
Proiectul consta intr-o inima interactiva realizata din 11 LED-uri RGB montate pe breadboard. Sistemul foloseste un senzor de puls HW-827, un display OLED SSD1306, un buzzer pasiv si un buton de control.

Utilizatorul apasa butonul pentru a porni sistemul. Dupa o etapa scurta de calibrare, sistemul asteapta detectarea degetului pe senzor. Cand degetul este detectat, incepe o sesiune de 60 de secunde in care un BPM aleator este generat automat. In timpul sesiunii, LED-urile verzi pulseaza sincron cu buzzerul care emite un efect "lub-dub", iar display-ul afiseaza timpul ramas si numarul de batai.

La final:
- Daca numarul de batai este intre 60 si 100: LED-ul verde se aprinde fix si se aude o melodie vesela (note ascendente).
- Daca numarul de batai este in afara intervalului: LED-urile raman stinse si se aude o melodie trista (note descendente).

Daca utilizatorul ridica degetul in timpul sesiunii, sistemul revine automat la starea OFF dupa cateva secunde.

> In implementarea finala, senzorul HW-827 este folosit pentru detectarea prezentei degetului (nu pentru masurarea pulsului biologic). Ritmul cardiac este generat aleator de software la fiecare sesiune, pentru o demonstratie stabila si predictibila.
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
| Canal verde LED-uri | PD5 |
| Buzzer | PD1 |
| Buton | PD2 |
| Senzor puls S / OUT | PC0 / ADC0 |
| OLED SDA | PC4 |
| OLED SCL | PC5 |

Senzorul este alimentat la **5V**, iar GND-ul senzorului este comun cu GND-ul placii.

LED-urile RGB sunt de tip **anod comun**. Pinul comun este legat la `+5V`, iar canalul verde este comandat printr-un tranzistor BD139 pe partea de `GND`.
---
## Software
Codul este scris in C pentru ATmega328P, fara framework Arduino. Sunt folosite direct registrele microcontrollerului pentru:
- GPIO (control LED-uri, buton);
- ADC (citire senzor HW-827);
- I2C / TWI (comunicare cu OLED SSD1306);
- Timer0 (baza de timp la 1 ms prin intrerupere);
- intreruperi (ISR pentru cronometru).

Programul este organizat ca masina de stari:

| Stare | Descriere |
|---|---|
| `STATE_OFF` | sistem oprit, display "OFF" |
| `STATE_CALIB` | calibrare 5 secunde, display "CALIB" |
| `STATE_WAIT_FINGER` | asteapta degetul pe senzor, display "FINGER" |
| `STATE_MEASURING` | sesiune activa de 60 secunde |

Fluxul principal:
1. Sistemul porneste in starea `OFF`.
2. Butonul porneste secventa.
3. Sistemul intra in `CALIB` (5 secunde, fara deget pe senzor).
4. Apoi afiseaza `FINGER` si asteapta degetul.
5. La detectarea degetului, se genereaza un BPM aleator (50-120) si incepe sesiunea de 60 secunde.
6. LED-urile verzi si buzzerul pulseaza sincronizat la fiecare bataie.
7. Display-ul afiseaza timpul ramas si numarul de batai inregistrate.
8. La final se afiseaza BPM-ul total si verdictul (OK sau ANORMAL), insotit de o melodie corespunzatoare.

### Functionalitati notabile
- **Citire filtrata a senzorului**: 16 esantioane ADC, cu eliminarea automata a valorilor aberante (glitch-uri sub 200 sau peste 950).
- **Histerezis la detectia degetului**: praguri diferite pentru punere (620) si ridicare (540), pentru a evita oscilatiile.
- **Stabilizare la pornire**: primele 5 secunde dupa detectarea degetului nu se verifica pierderea acestuia.
- **Generator de numere aleatoare**: bazat pe momentul exact al atingerii senzorului, fiecare sesiune produce un BPM diferit.
- **Melodii la final**: note ascendente (Do-Mi-Sol-Do) pentru OK, note descendente (La-Sol-Fa-Mi) pentru ANORMAL, generate prin buzzer pasiv la frecvente diferite.
- **Font custom in PROGMEM**: litere si cifre stocate in memoria Flash, afisate pe OLED SSD1306 prin I2C.

VIDEO DEMONSTRATIV: https://youtube.com/shorts/KqERROrqigY?feature=share
