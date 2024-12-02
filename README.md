## Autor

Christian Saloň

## Popis programu

Program imapcl, ktorý umožnuje čítanie elektronickej pošty pomocou protokolu IMAP. Program po spustení stiahne správy uložené na serveri a uloží ich do zadaného adresára. Na štandardný výstup vypíše počet stiahnutých správ. Pomocou dodatočných parametrov je možné funkcionalitu meniť. V interaktívnom režime sa program pripojí k schránke a bude bežať až do ukončenia príkazom QUIT. Používateľ bude mať možnosť sťahovať správy príkazom DOWNLOAD(NEW|ALL) [MAILDIR], čítať nové správy príkazom READNEW [MAILDIR].

### Rozšírenia

Implementovaný interaktívny režim s podporou STARTTLS. Do interaktívneho režimu bol pridaný príkaz STARTTLS a príkaz LOGIN, ktorý autentizuje užívateľa s údajmi poskytnutých v autentizačnom súbore.

## Príklad spustenia

make

./imapcl server [-p port] [-T [-c certfile] [-C certaddr]] [-n] [-h] -a auth_file [-b MAILBOX] -o out_dir [-i]
