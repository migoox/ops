## Zapis do łącza
### Zapis bez flagi O_NONBLOCK
1. Jeśli *ndata <= PIPE_BUF*
    - *npipe_free >= ndata*, to atomicznie wpisz do pipe'a
    - *npipe_free < ndata*, to zablokuj proces wpisujący, proces bedzie czekał, aż w pipe będzie *ndata* wolnego miejsca
2. Jeśli *ndata > PIPE_BUF*
    - wpisuj tyle ile się da, jeśli nie ma miejsca to blokuj proces, tak aby poczekać na wolne miejsce

### Zapis z flagą O_NONBLOCK
1. Jeśli *ndata <= PIPE_BUF*
    - *npipe_free >= ndata*, to atomicznie wpisz do pipe'a
    - *npipe_free < ndata*, to zwróć *-1* i ustaw *errno=EAGAIN* 
2. Jeśli *ndata > PIPE_BUF*
    - jeżeli *npipe_free == 0* to zwróć *-1* oraz ustaw *errno=EAGAIN* 
    - jeżeli *npipe_free > 0* to zwróć *-1* lub liczbę zapisanych bajtów
    (w zależności od tego czy zapis sie powiódł) oraz ustaw *errno=EAGAIN* 

### Zerwanie łącza
Jeśli łącze zostanie zerwane, a proces spróbuje coś do niego zapisać, to 
wygeneruje to sygnał *SIGPIPE*, który go zabije.

Należy dodać obsługę zerwania łącza w przypadku zapisu, trzeba ignorować
sygnał *SIGPIPE* i sprawdzać write pod kątem błędu *EPIPE*.
## Odczyt z łącza
Odczytywanie odbywa się za pomocą *read()* co oznacza, że nie ma automatycznych
mechanizmów, które sprawdzą czy odczytana porcja bajtów stanowi kompletną daną.

Można to rozwiązać tworząc headery, lub zawsze wysyłając dane o rozmiarze *=PIPE_BUF*, 
dopełniając zerami.

Czytanie z pustego łącza powoduje zablokowanie procesu czytającego do momentu
kiedy dane są dostępne.

### Zerwanie łącza
Odczyt zera bajtów z łacza oznacza EOF czyli zerwanie połączenia. *SIGPIPE* oraz
*EPIPE* nie dotyczą odczytu z łącza.




# Co student musi wiedzieć

## FIFO
**manpages:** man 7 fifo, man 3p mkfifo

Informacje podstawowe:
- kiedy procesy wymieniają dane, nie są one fizycznie przechowywane na pliku FIFO, plik
ten służy wyłącznie jako punkt referencyjny umożliwiający komunikację obsłużoną przez kernel
- FIFO musi być uruchomione na obu końcach (read and write) zanim dane będą mogły być przekazane
- po otworzeniu FIFO z jednej strony przez proces, jest on blokowany do momentu, kiedy FIFO zostanie otworzone z drugiej strony
- proces może otworzyć FIFO w trybie nieblokującym, wtedy możliwe read-only, kiedy na drugim koncu nikt nie pisze, ale nie mozliwe jest write-only (skutkuje *ENXIO*)
- nie można używać *lseek* (sprzeczność z kolejką)

Obsługa:
- tworzymy za pomocą *mkfifo()*
- koniec czytający otwieramy za pomocą *open()* z flagą *O_RDONLY*
- koniec zapisujący otwieramy za pomocą *open()* z flaga *O_WRONLY*

```cpp
#include <sys/stat.h>

int mkfifo(const char *path, mode_t mode);
```

Podajemy ścieżkę relatywną lub globalną z pomoca *path* do
pliku.

Podajemy permisje pliku FIFO za pomocą *mode*.

Jeśli zwrócone zostanie *-1* to wystąpił błąd, jeśli *0* to nie.

## pipe
**manpages:** man 7 pipe
- jedyna różnica pomiędzy FIFO i pipe jest sposób utworzenia

## alphanumeric
man 3 isalpha

int isalpha(int c):
checks for an alphabetic character; in the standard "C" locale, it is equivalent to (isupper(c) || islower(c)).   In  some
locales, there may be additional characters for which isalpha() is true—letters which are neither uppercase nor lowercase.

0 - okej
wpp - nie jest alfanumeryczny

## limits.h

The <limits.h> header shall define macros and symbolic constants
       for various limits.  Different categories of limits are described
       below, representing various limits on resources that the
       implementation imposes on applications.  All macros and symbolic
       constants defined in this header shall be suitable for use in #if
       preprocessing directives.

pozwala na sprawdzenie PIPE_BUF


