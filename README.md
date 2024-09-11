# LaskaKit Mapa
Moje pouziti LaskaKit Mapy spolu s Home Assistantem

Princip je takovy ze se stahnou udaje o desti a pokud neprsi na mape se zobrazuje teploata mest.

Pokud je dest ve vic jak 2 mestech tak se zobrazuji ve smycce posledni 3 snimky z radaru.

Mam volany webhook kazdych 10 minut, ale tohle si kazdy pripadne upravi po svem, pripadne to ohne cele.

K je mape pripojeny teplotni senzor a dalsi led pasek v tomto drzacku: https://www.thingiverse.com/thing:6037911

Pokud se to nekomu nehodi tak si to hold musi z kodu vyhazet.

Zdroje dat pro NodeRed:

https://lab.aperturelab.cz/mapa/srazky.php (Zdroj: RainViewer API)

https://lab.aperturelab.cz/mapa/teploty.php (Zdroj: OpenWeatherMap API)
