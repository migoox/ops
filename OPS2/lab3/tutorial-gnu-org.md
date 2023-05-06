https://www.gnu.org/software/libc/manual/html_node/Socket-Concepts.html


# GNU ORG
## Socket Concepts:
Aby wybrać styl komunikacji należy wziąć pod uwagę:
- jakie mają być jednostki danych przy komunikacji
  - brak większej struktury - ciąg bajtów
  - grupowanie z użyciem pakietów
- czy dane mogą być tracone podczas komunikacji
  - np. streaming pozwala na utrate danych
  - inne style nie
- czy komunikacja przebiega tylko z jednym pertnerem
  - połączenie i następnie komunikacja z jednym gniazdem
  - każdy komunikat idzie do wybranego odbiorcy
- przestrzeń nazw (namespace)
  - nazwa gniazda np. adress moze oznaczac cos innego w zelzonsci od przestrzeni nazw
- protokół
  - protokół definiuje jaki nisko-poziomowy mechanizm ma zostać wykorzystany do wysyalania oraz odbierania danych
  - protokół jest ważny w obrębie konkretnej przestrzeni nazw, z tego powodu przestrzeń nazw nazywana jest czasem protocol family
  - ważne:
    - sockety komunikujące się ze sobą muszą mieć ten sam protokół
    - każdy protokół wymusza konkretny styl/przestrzeń nazw, np. TCP wymusza byte stream komunikacje oraz przestrzeń Internet
    - dla każdej kombinacji stylu i przestrzeni istnieje domyslny protokół oznaczany jako 0

## Style komunikacji
Ta sekcja przedstawia wspierane typy socketów przez GNU C Library:
- *Macro*: int SOCK_STREAM
  - styl SOCK_STREAM działa tak jak pipe -- operuje na stałym połączeniu z konkertnym zdalnym socketem i przekazuje informacje jako strumień bajtów 
- *Macro*: SOCK_DGRAM
  - ten styl jest używany do wysyłania zaadresowanych pakietów
  - za każdym razem kiedy robisz zapis jakichś danych, dane te stają się jednym pakietem
  - jako, że ten styl nie ma stałego połączenia, **każdy z pakietów musi zostać zaadresowany**
  - ten system stara sie zapewnić przesył każdego pakietu
  - kolejność wysyłanych odbieranych pakietów nie musi być taka sama jak ich kolejność wysyłania
  - typowo pakiet powinno wysylac sie ponownie, jeśli przez długi czas nie dostajemy potwierdzenia o jego odebraniu
- *Macro*: SOCK_RAW
  - nisko-poziomowe protokoły i interfejsy 

## Socket Addresses
- w przypadku socketów, mozemy mowic ze *name* i *address* to to samo
- nowo-utworzony socket nie ma adresu
  - inne procesy moga go znaleźć, tylko wtedy jesli nadamy adres
  - proces nadawania adresu socketowi nazywamy *bindingiem*
- powinniśmy bindować adres tylko w przypadku kiedy bedzie istnial jakis socket chcacy nawiazac z nim komunikacje, w przeciwnym przypadku (np. gdy nasz proces tylko nadaje i nidgy nie odbiera) nie ma to sensu 
  - pierwsze użycie socketu w celu **wysłania** danych, spowoduje automatyczne przypisanie adresu jezeli nie zostal 
- klient rzadko specyfikuje adres, czasmi jest to jednak konieczne, ponieważ są sytuacje, gdy serwer okresla jakis pattern adresuu
  - np. rsh i rlogin patrzą na socket klienta i omijają sprawdzanie hasła tylko wtedy gdy jest on mniejszy niż IPPORT_RESERVED
- niezależnie od przestrzeni nazw, używamy zarówno *bind* jak i *getsockname* w celu ustawianai adresów socketów
  - uzyte sa falszywe typy danych - `struct sockaddr *`
  - w praktyce adresy zyja sobie w strukturach innego typu, ale trzeba je zawsze castowac do `struct sockaddr*` kiedy przekazujemy je do *bind*

## Address Formats
- typ `struct sockaddr*` jest typem generycznym uzyawanym przez *getsockname* i *bind*
  - nie mozna zastosować tego typu do interpretowania adresu albo jego tworzenia, w tym cleu nalezy skorzystać z prawdziwego typu danych dla odpowiedniej przestrzeni nazw
  - zazwyczaj najpierw tworzymy *adress* korzystajac z struktur dla danej przestrzeni i wtedy castujemy na `struct sockaddr*` przy wywołaniu *bind* albo *getsockname*
- `struct sockaddr` przechowuje między innymi `address format designator`, które mówi jaki typ danych powinien zostać użyty, żeby adress mógł być w pełni zinterpretowany/zrozumiany
- `struct sockaddr`:
  - `short int sa_family` - prawdziwy format
  - `char sa_data[14]` - dane adressu socketu, które są zależne od formatu, tak samo ich rozmiar jest zależny od typu, może być większy niż '14', '14' jest arbitralne
- lista nazw formatów adresów:
  - *AF_LOCAL* - lokalna przestrzeń nazw, używana do komunikacji międzyprocesowej
  - *AF_UNIX*, *AF_FILE* - synonimy *AF_LOCAL*
  - *AF_INET* - przestrzeń Internet Protocol version 4 (IPv4), używamy do komunikacji poprzez internet 
  - *AF_INET6* - przestrzeń Internet Protocol version 6 (IPv6)
  - *AF_UNSPEC* - rzadko używane, np. datagramy

## Setting Address
```c
 int bind (int socket, struct sockaddr *addr, socklen_t length)
```
- przypisuje adres do gniazda 'socket' (deskryptor gniazda), 
- pierwsza część adresu, to format designator, który określa przestrzeń nazw (np. *AF_INET*)
- zwraca 0 w przypadku sukcesu i -1 w przeciwnym przypadku

## Reading the Adresss of a Socket
```c
int getsockname (int socket, struct sockaddr *addr, socklen_t *length-ptr)
```
- zwraca inofrmacje na temat gniazda 'socket' (deskryptor gniazda) w 'addr' oraz 'length-ptr'
- 0 w przypadku sukcesu i -1 wpp
- zazwyczaj najpierw alokuje się miejsce dla wartości używanej przez poprawny tym danych dla przestrzeni nazw gniazda, a następnie castuje się do struct sockaddr *

## Interface Naming
- każdy interfrejs sieciowy ma swoja nazwe
  - np. lo (the loopback interface), eht0 (the first ethernet interface)
- aby odwoływać się do interfejsów za pomocą indeksów

```c
size_t IFNAMISIZ // stała definiująca maksymalny rozmiar bufora potrzebnogo do przechowywanai nazwy interfejsu

unsigned int if_nametoindex (const char *ifname) // zwraca indeks interfejsu przypisany do danej nazwy, jezeli nie ma takiego interfejsu zwraca 0 

char* ifindextoname (unsigned int ifindex, char *ifname) // dziala analogicznie tylko w druga strone, zwracany char* name musi miec bufor o rozmiarze co najmniej IFNAMSIZ, NULL zwrocone w przypadku invalid indeks

struct if_nameindex // przechowuje info na temat jednego interfejsu
    unsigned int if_index
    char *if_name

struct if_nameindex* if_nameindex (void) // zwraca tablicę 
if_nameindexów, wszystkie dostępne interfejsy są w niej obecne

void if_freenameindex(struct if_nameindex *ptr) // czysci to co zwraca poprzednia funkcja
```

## The Local Namespace
- adresy soketów to nazwy plików, zazwyczaj wrzucamy te pliki do /tmp
- nazwy używamy tylko wtedy kiedy chcemy otworzyć połączenie, 
  - po otwarciu połaczenia adres przestaje miec jakiekolwiek znaczenie
- nie mozna połączyć sie z takim socketem z innego urządzenia
  - nawet gdy obie maszyny dzielą ten sam system plików!
  - możliwe jest wtedy zobaczenie takiego pliku (wylistowanie go), ale połączenie już nie
- po zamknięciu socketa w lokalnej przestrzeni, powinno sie usunąć taki plik z systemu plików
- tylko jeden protokół jest wspierany, jego indeks to 0

#### Details of Local Namespace
- int PF_LOCAL jest równoważne PF_UNIX, PF_FILE
  - PF_UNIX i PF_FILE są tylko do wstecznej kompatybilności
- `struct sockaddr_un` 
  - zdefiniowane w `sys/un.h`
  - używane do określenia local namespace socket adresy
  - `short int sun_family` - identyfikuje adress family - storujemy AF_LOCAL
  - `sun_path[108]` - filename który będziemy używali
- powinno się obliczać parametr *length* dla adresów socketa w lokalnej przestrzeni jako sumę rozmiaru sun_family oraz string length (nie rozmiar alokacji) 
- `int SUN_LEN (struct sokaddr_un * ptr)`
  - makro ktore oblicza length

```c
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

int
make_named_socket (const char *filename)
{
  struct sockaddr_un name;
  int sock;
  size_t size;

  /* Create the local socket, packets are used */
  sock = socket (PF_LOCAL, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  /* Bind a name to the socket. */
  name.sun_family = AF_LOCAL;
  strncpy (name.sun_path, filename, sizeof (name.sun_path));
  name.sun_path[sizeof (name.sun_path) - 1] = '\0';

  /* The size of the address is
     the offset of the start of the filename,
     plus its length (not including the terminating null byte).
     Alternatively you can just do:
     size = SUN_LEN (&name);
 */
  size = (offsetof (struct sockaddr_un, sun_path)
          + strlen (name.sun_path));

  if (bind (sock, (struct sockaddr *) &name, size) < 0)
    {
      perror ("bind");
      exit (EXIT_FAILURE);
    }

  return sock;
}
```

## Internet Namespace
#### Internet Address Formats
- AF_INET - IPv4, AF_INET6 - IPv6
- wybrany protokół, służy jako część adresu
- `netinet/in.h`
- `struct sockaddr_in` - data type który uzywamy do reprezentowanie socket adresu Internet namespace'u
  - `sa_family_t sin_family` - storujemy tutaj AF_INET
  - `struct in_addr sin_addr` - IPv4 address (używamy network byte order! bo niektore systemy maja little endian a inne big endian)
  - `unsigned short int sin_port` - Internet Port
- kiedy wyołujemy *bind* lub *getsockname*, musimy okreslic `sizeof(struct sockaddr_in)` jako *length*
- `struct sockaddr_in6` - używane do reprezentacji socketów Internet namespac'u z IPv6
  - `sa_family_t sin6_family` - storujemy tutaj AF_INET6
  - `struct in6_addr sin6_addr` - adres IPv6 dla host machine
  - `uint32_t sin6_flowinfo` - typowo to pole ustawia sie na 0, przechowywane w network byte-order 
  - `uint32_t sin6_scope_id` - identyfikuje interfejs dla ktoreego adres jest valid, host byte-order
  - `uint32_t sin6_port` - number portu 

#### Host Adresses
###### Abstract Host Adresses
- IPv4 to liczba składająca się z 4 bajtów danych 
  - podzielone na dwie częsci - *network number* i *local network address number*
  - na początku korzystano z tak zwanych klas, które określają ile bitów przeznaczonych jest na pierwszą a ile na drugą część
  - dzisiaj określamy to za pomocą masek 
  - IPv6 nie posiada żadnych klas
- Wyrózniamy klasy
  - A - jeden bajt reprezentujący *network number* (przedział 0-127)
    - bardzo mało sieci, ale za to duża liczba hostów w sieci
    - sieć 0 jest zarezerwowana dla *broadcast* do wszystkich sieci
    - host 0 zarezerwowane dla *broadcast* do wszystich hostów danej sieci
    - sieć 127 zarezerwowana do *loopback* - 127.0.0.1 zawsze referuje do hosta na którym jesteśmy
  - B - dwa bajty reprezentujące *network number*
  - C - trzy bajty reprezentujące *network number*
- jedno urządzenie może być częścią wielu sieci oraz mieć wiele adresów hostów
  - pomimo tego nigdy nie mogą istnieć dwa urządzenia z takim samym adresem hosta
- Sieć bez klas
  - używamy maski 32-bitowej, która składa się z bitów '1' określających część *network number* oraz z bitów '0' określających część hosta
  - bity maski powinny być zapalone od lewej strony i być ciągłe!
  - przykładowo maska klasy A to 255.0.0.0
  - w bezklasowej adresy zapisujemy np. tak 10.0.0.0/8 co oznacza adres 10.0.0.0 z maską 255.0.0.0
- adresy IPv6 składają się z 128 bitów, zapisujemy często jako 8 hexadecimal liczb
  - przykładowo IPv6 loopback ‘0:0:0:0:0:0:0:1’ może zostać zapisany jako  ‘::1’
###### Abstract Host Adress Data Type
- Ipv4 Internet adresy hostó są reprezentowane jako inty typu `uint32_t`
  - `struct in_addr` może również zostać użyty do przechowania adresu
  - konwersja w obie strony nie jest problematyczna (struktura ta definiuje tylko jedno pole `s_addr`, które jest typu `uint32_t`)
  - lepiej nie stosować `unsigned long int`, bo moze byc inaczej interpretowane w roznych miejscach
- IPv6 Internet adresy hostów sa pakowane do struktury `struct in6_addr`
  - `netinet/in.h`
- `uint32_t INADDR_LOOPBACK` - makro które zwraca loopback, wiec mozemy to traktowac jako adres aktualnej maszyny, nie musimy wiec znajdowac rzeczywistego adresu
- `uint32_t INADDR_ANY` - "any incoming address"
- `uint32_t INADDR_BROADCAST` 
- `uint32_t INADDR_NONE` - zwracane przez niektóre funkcje, wskazuje ze wystąpił error
- `struct in6_addr` - przechowuje adres IPv6, czyli 128 bitów danych, do których możemy dostać się z użyciem union
- stała `struct in6_addr in6addr_loopback` - przechowuje '::1', IN6ADDR_LOOPBACK_INIT może zostać użyte aby umożliwić inicjalizacje 
- stała `struct in6_addr in6addr_any` - przechowuje '::', IN6ADDR_ANY_INIT umożliwia inicjalizację

###### Host Address Functions
- te funkcje umożliwiają manipulowanie adresami w sieci Internet, zadeklarowane są w headerze `arpa/inet.h`
  - umożliwiają między innymi byte-conversion, na internet byte-order

```c
int inet_aton (const char *name, struct in_addr *addr)
// konwertuje adres IPv4 Internet hosta "name"
// w notacji a.b.c.d do struktury adersu ip
// zwraca 0 jesli sie udalo i nie zero wpp

uint32_t inet_addr (const char *name)
// to osamo co wyzej, ale wynik zwracany jest jako uint32_t
// jezeli input nie jest valid to zwracany jest INADDR_NONE 
// (odpowiada 255.255.255.255), inet_aton jest czystszy jesli chodzi
// o wykrywanie bledow

uint32_t inet_network (const char *name)
// "name" w notacji a.b.c.d, wyluskuje network number w zaleznosci 
// od przyjetej klasy (nie dziala dla adreesow bezklasowych)
// zwraca -1 w przyapdku niepowodzenia i 0 w przypadku powodzenia
// NIE UZYWAMY JUZ

uint32_t inet_lnaof (struct in_addr addr)
// wyluskuje local number, tylko dla klasowych sieci, NIE UZYWAMY JUZ

char * inet_ntoa (struct in_addr addr)
// konwersja w drugą stronę

struct in_addr inet_makeaddr (uint32_t net, uint32_t local)
// tworzy adres kiedy okreslimy część network i część local

uint32_t inet_netof(struct in_addr addr)
// zwraca numer network

int inet_pton (int af, const char *cp, void *buf)
// konwertuje IPv4 lub IPv6 z tekstowego do binary
// af = AF_INET lub af = AF_INET6
// cp to pointer na input string

const char * inet_ntop (int af, const void *cp, char *buf, socklen_t len)
// to samo co wyzej tylko w druga strone
```

###### Host Names
- maszyna z adresem '158.121.106.19' jest również znana jako 'alpha.gnu.org', tak samo inne maszyny w domenie 'gnu.org' mogą odwoływać się do tej maszyny jako 'alpha'
- /etc/hosts lub server przechowuje baze danych ktora pozwala na takie aliasy 
- *struct hostent*
  - używane do reprezentowania rekordu w bazie danych
  - `char *h_name` - oficjalna nazwa hosta
  - `char **h_aliases` - wszystkie alternatywne nazwy hosta, jako null terminated stringi
  - `int h_addrtype` - AF_INET lub AF_INET6
  - `int h_length` - długość w bitach, każdego adresu
  - `char **h_addr_list` - wektor adresow hosta 
  - `char *h_addr` - synonim dla `h_addr_list[0]` - pierwszy host adres
- mozna uzywac *gethostbyname*, *gethostbyname2* lub *gethostbyaddr* aby
przeszukać hosts database, informacja jest zwracana jako statycznie zaalokowana struktura
```c
struct hostent * gethostbyname (const char *name)
// zwraca informacje o hoscie nazwanego name, null pointer w przypadku errora

struct hostent * gethostbyname2 (const char *name, int af)
// robi to samo ale pozwala na srpecyzowanie hostname family
// AF_INET lub AF_INET6

struct hostent * gethostbyaddr (const void *addr, socklen_t length, int format)
// zwraca informacje o hoscie z jego INternet adresem w zmiennej "addr"
// addr* nie musi byc pointerem do chara, może wskazywać do IPv4 lub Ip6 adresu
// "lenght" to rozmiar *addr, 
// "format" to albo AF_INET labo AF_INET6
// w przypadku faila zwracany jest null pointer

// we wszystkich przypadkach errorow ustawiane jest errno
// - HOST_NOT_FOUND
// - TRY_AGAIN
// - NO_RECOVERY 
// - NO_ADDRESS

int gethostbyname_r (const char *restrict name, struct hostent *restrict result_buf, char *restrict buf, size_t buflen, struct hostent **restrict result, int *restrict h_errnop)
// zwraca informacje o hoscie nazwanego name

int gethostbyname2_r (const char *restrict name, struct hostent *restrict result_buf, char *restrict buf, size_t buflen, struct hostent **restrict result, int *restrict h_errnop)

int gethostbyaddr_r (const char *restrict name, struct hostent *restrict result_buf, char *restrict buf, size_t buflen, struct hostent **restrict result, int *restrict h_errnop)

void sethostent (int stayopen)
// otwiera baze danych hostent
// jesli stayopen jest niezerowe to funkcje takie jak
// gethostbyname czy tez gethostbyaddr nie spowoduja zamknięcia bazy danych
// (normalnie by zamknęły)

struct hostent * gethostent (void)
// zwraca następny record w bazie danych

void endhostent (void)
// zamyka baze danych
```
- zasadnicza róznica pomiedzy funkcjami zwyklymi a z dopiskiem 'r' jest taka, że te bez 'r' nie są multithread-safe
  - dzieje sie tak przez uzycie statycznego bufora
  
###### Internet Ports
- socket address w przestrzeni nazw Internet, składa się z Internet adress oraz z *numeru portu* (0 - 65 535)
- numery portów mniejszen iż *IPPORT_RESERVED* są zarezerwowane dla standardowych serwerów takich jak *finger* lub *telnet*
  - mozna uzyc *getservbyname* aby zmapować service name do numeru portu
- jeżeli npaiszesz server, ktory nie jest standardowo zdefiniowany w bazie danych, musisz wybrać mu numer portu
  - zawsze używaj liczb większych niż IPPORT_USERRESERVED
- Jeżeli użyjesz gniazda bez ustawienia adresu, system zrobi to za ciebie, taka liczba znajdzie się między IPPORT_RESERVED oraz IPPORT_UNRRESERVED
- Internet pozwala na istnienie dwóch gniazd o tym samym numerze portu, pod warunkiem, że nigdy nie próbują komunikować się jednocześnie z tym samym gniazdem
- Nie powinno powtarzać numerów portów, poza specjalnymi przypadkami, kiedy protokół wyższego poziomu tego wymaga, zazwyczaj system nie pozwoli ci na to, bo *bind* automatycznie wymuesza różne numery portów
  - aby użyc tego samego portu, trzeba ustawić opcje gniazda na *SO_REUSEADDR*

```c
// header netinet/in.h
int IPPORT_RESREVED
// porty mniejsze niz IPPORT_RESERVED są zarezerwowane dla superuser
int IPPORT USERRESERVED
// porty >= IPPORT_USERRESERVED to porty ktore mozna uzyc explicite  
``` 

###### Services Database
- baza danych, która monituruje dobrze znane usługi albo plikiem /etc/services albo serverem o tej samej nazwie
- poniżej znajdująsię narzędzia, zadeklarowane w `netdb.h`, które można wykorzystać w celu uzyskania dostęu do bazy danych

```c
// przechowuje informacje o rekordach w service database
struct servent{
  char* s_name; // oficjalna nazwa usługi
  char **s_aliases; // alternatywne nazwy usługi
  int s_port; // numer portu usługi, network byte-order
  int *s_proto; // nazwa protokołu, którego używamy z tą usługą
}
```

- Aby pozyskać informacje na temat konkretnej usługi używamy *getservbyname* albo *getservbyport*, zwracana jest statycznie zaalokowana struktura (not thread safe)
  - należy skopiować informacje zeby je zachowac

```c
struct servent * getservbyname (const char *name, const char *proto)
// The getservbyname function returns information about the 
// service named name using protocol proto. 
// If it can’t find such a service, it returns a null pointer. 

struct servent * getservbyport (int port, const char *proto)
// The getservbyport function returns information about 
// the service at port port using protocol proto.

void setservent (int stayopen)
// This function opens the services database to begin scanning it.
// If the stayopen argument is nonzero, this sets a flag 
// so that subsequent calls to getservbyname or getservbyport
// will not close the database (as they usually would). 

struct servent * getservent (void)
// This function returns the next entry in the services database.

void endservent (void)
// This function closes the services database. 
```

###### Byte Order
- Rózne typy komputerów używaja albo big endian albo little endian
  - z tego poowdu utworzono network byte order
- kiedy ustanawiamy połączenie Internet za pomocą socketa, musimy zapewnić, że dane w *sin_port* oraz *sin_addr* w strukturze *sockaddr_in* są reprezentowane w network byte order
- w przypadku kodowoania danych liczbowych w wiadomościach wysyłanych przez gniazdo również musimy dokonać konwersji
- getservbyname, gethostbyname, inet_addr zwracają dane w network byte order, więc można je przekopiować bezpośrednio do sockaddr_in
  - w przeciwnych przypadkach potrzebna jest konwersja explicite

```c
uint16_t htons (uint16_t hostshort)
// z host byte order do network byte order

uint16_t ntohs (uint16_t netshort)
// z newtork byte order do host byte order

uint32_t htonl (uint32_t hostlong)
uint32_t ntohl (uint32_t netlong)
// analogicznie tylko dla long intow

```

###### Protocols Database
- protokół implementuje takie rzeczy jak checksumy, instrukcje routingu itd.
- w Internet namespace domyślna komunikacja zależy od stylu komunikacji
  - TCP (transmission control protocol) - byte stream communication  
  - UDP (user datagram protocol) - datagram communication
- domyślnie używamy UDP,
- Protokoły Internet są ogólnie oznaczane przez nazwę a nie numer
- protokoły o których host wie, są przechowywane w bazie danych
  - /etc/protocols
  - lub serwer
  - getprotobyname

```c
// ten typ danych jest używany do
// zareprezentowania rekordów w protokołach baz danych
struct protoent {
  char *p_name; // oficjalna nazwa protokołu
  char **p_aliases; // alternatywne nazwy protokołu
  int p_proto; // numer protokołu (host byte order), uzywamy jako protocol argument w socket
}

struct protoent * getprotobyname (const char *name)
struct protoent * getprotobynumber (int protocol)
void setprotoent (int stayopen)
struct protoent * getprotoent (void)
void endprotoent (void)
```