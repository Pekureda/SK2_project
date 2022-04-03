# SK2_project

Poniższy opis jest fragmentem sprawozdania z projektu zaliczeniowego laboratoriów Sieci Komputerowych 2.

Tematem projektu było przygotowanie serwera oraz aplikacji imitującej popularny serwis do przeprowadzania quizów "Kahoot".

Projekty były realizowane w grupach dwuosobowych.
Repozytorium zawiera jedynie serwer, tj. część wyłączną autora repozytorium.

# 1. Opis protokołu komunikacyjnego

Każdy “komunikat” jest terminowany ‘`\n`’.

Pierwsze słowo w komunikacie jest tagiem - wyjątkiem jest przekazanie hostowi rankingu,
gdzie wysyłana jest lista `nick_gracza liczba_pkt.`, który to `ranking` jest terminowany `gniknar`.

Wybór rodzaju użytkownika:

`host` - serwer odpowiada `gid nr_gry`, gdzie `nr_gry` jest w zakresie 0 do UINT_MAX.

`player nick_gracza` - serwer oczekuje na podanie id gry. Po dopisaniu `gid nr_gry`, gracz może otrzymać:
1.  `gc nosuchroom` - gdy pokój gry nie istnieje
2.  `gc roomclosed` - gdy pokój istnieje ale nie został otwarty
3. `expected gid` - gdy gracz po wpisaniu `player nick_gracza` nie poda gid tylko coś innego
4. `error gid ciąg_znaków` - gdy numer powiązany z gid nie jest konwertowalny do unsigned long przez std::stoul (zamiast liczby został podany znak, ciąg znaków niebędący wyłącznie cyframi.

W każdym innym przypadku: `error tag rodzaj_użytkownika`, oraz następuje rozłączenie z serwerem.

Początkowym stanem gry jest acceptingQuestions, gdy to host przesyła do serwera serię pytań oraz odpowiedzi:
1. `q treść_pytania` - wysyła do serwera treść pytania. Można użyć, gdy jeżeli było
ostatnio wysłane pytanie, to zostały zdefiniowane 4 możliwe odpowiedzi oraz została
zdefiniowana poprawna odpowiedź.
2. `a możliwa_odpowiedź` - wysyła do serwera treść możliwej odpowiedzi. Można użyć,
gdy do ostatnio wysłanego pytania nie zdefiniowano 4 odpowiedzi.
3. `ca indeks` - wysyła do serwera indeks poprawnej odpowiedzi. Można wysłać, gdy na ostatnie pytanie zdefiniowano 4 możliwe odpowiedzi. Zakres liczb od 1 do 4 (p. ko.)
4. W każdym innym przypadku wysyłany jest error.

Po zdefiniowaniu co najmniej jednego pytania można pokój otworzyć komunikatem open (stan gry przechodzi do roomOpen). Po otwarciu pokoju traci się możliwość dodawania pytań.

Gdy gracz dołączy do pokoju host otrzyma komunikat `playerconn nick_gracza`, z kolei gdy gracz się rozłączy, host otrzyma komunikat `playerdisconn nick_gracza`.
Do czasu wysłania przez hosta `begin` gracze mogą dołączać do gry. Po komunikacie begin stan gry zmienia się na acceptingAnswers oraz rozsyłane są pytania.
Przed pierwszym pytaniem do graczy jest wysyłany tag “`gs liczba_pytań`”.
Każde pytanie jest jest rozsyłane w następującym schemacie:

```
q treść_pytania
a odp1
a odp2
a odp3
a odp4
```
Po tym oczekuje się od gracza odpowiedzi w czasie 15 sekund (do ewentualnej modyfikacji).

Uwzględnia się jedynie pierwszą odpowiedź gracza. Każda kolejna jest ignorowana. Odpowiedź udziela się `ca indeks` gdzie indeks jest w zakresie od 1 do 4 (p. ko.)
Po rozesłaniu pytań host otrzymuje `sent liczba_połączonych_graczy`. Od tego momentu serwer zapisuje czas do weryfikacji, czy gracz udzielił odpowiedzi w zadanym okresie oraz czy host nie chce zbyt szybko wysłać kolejnych pytań.

Do kolejnego pytania host przechodzi wysyłając do serwera `next`. Jeżeli zrobi to przed upływem czasu przeznaczonego na pytanie; host otrzyma od serwera `error`.
Wysłanie komunikatu `next` po ostatnim pytaniu powoduje rozesłanie wyników. Gracze otrzymują `points liczba_zdobytych_pkt`. Host otrzymuje listę rankingową o następującym schemacie:
```
ranking
nick_gracza1 liczba_pkt1
nick_gracza2 liczba_pkt2
…
nick_graczaN liczba_pktN
gniknar
```
Po całym tym procesie wszystkie połączenia z tą grą są terminowane oraz sama gra jest anihilowana.

# 2. Opis implementacji
Serwer został napisany w języku C++. Oprócz wykorzystania STL i innych
standardowych bibliotek, wykorzystuje się biblioteki niezbędne do implementacji
funkcji sieciowych przy wykorzystaniu Berkeley sockets (BSD sockets), oraz
wątków w standardach POSIX (pthread). Projekt został zorganizowany w
klasach: communication (ogólny sposób wysyłania oraz odbioru danych,“spakowany” w przestrzeń “cmn”), Connection (obsługa połączeń z serwerem
oraz zadań klientów), Game (klasa gry oraz jej obsługa), Question (klasa
przechowująca i obsługująca pytania)

# 3. Opis sposobu kompilacji i uruchomienia projektu
Serwer jest dostarczony już skompilowany. W razie potrzeby samodzielnej
kompilacji - projekt został przygotowany do kompilacji w IDE MS Visual Studio
Code - należy uruchomić task “C/C++: g++ build active file”, ewentualnie
poleceniem (testowane na g++ Ubuntu 9.3.0-17ubuntu1~20.04. Kompilowany
kod powinien być zgodny z C++14. Kod jest “kompilowalny” ze standardem
C++2a (dodatkowa flaga -std=c++2a. Program nie był testowany dla C++2a):
`g++ -pthread -Wall -g communication.cpp Connection.cpp
Game.cpp Server.cpp main.cpp -o main`
Serwer uruchamia się wykonując produkt kompilacji (w powyższym przypadku
`./main` zakładając, że wykonujący powyższe polecenia znajduje się w katalogu
z kodem serwera).
