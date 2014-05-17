// Spataru Florina Gabriela 323 CA // florina.spataru@gmail.com
// Tema 4 PC // Aplicatie client pentru interogari DNS

1. Realizarea interogarii
   In functia make_query am "concatenat" in buffer header-ul, qname si question.
Pentru header si question m-am folosit de structurile din .h. Dns_header va
pointa catre inceputul lui buffer, deci daca il completez pe acesta, basically
introduc date si in buffer. In continuarea lui adaug qname si asa mai departe.
De aceea zic "concatenat", pentru ca de fac nu concatenez nimic, doar compeltez
campurile din structuri si ma joc cu pointeri.

2. Parsarea raspunsului
   Cel mai important element al parsarii va fi index. Acesta imi va pointa la
adresa unde ma aflu cu parsarea in raspuns. De fiecare data cand sar peste
ceva, il incrementez accordingly.
   Al doilea element important ar fi functia dns_to_host. Numele nu este prea
sugestiv, dar am explicat si in comentarii cat de mult pot, ce se intampla
acolo. Pe scurt, am ajuns la un camp de nume (index pointeaza acolo) si vreau
sa vad daca acolo chiar am un nume sau este un pointer catre o alta zona din
raspuns care contine acel nume. In ambele cazuri citesc numele si il adaug in
domain_name, diferenta ar fi ca in cazul in care am pointer, doar incrementez
index pentru nu am mers mai departe in zona de lucru curenta decat cu un byte.
Daca nu am pointer e clar ca efectiv merg mai departe cu index si trebuie sa
tin cont de acest aspect.

   Pentru inceput, am functia parse_answer. Aici aflu cat sunt ANCOUNT, NSCOUNT
si ARCOUNT adica answer, autorithy si additional. Vreau sa sar peste sectiunea
question asa ca citesc un nume care se va duce in domain_name, dar nu il
folosesc. Deci functia dns_to_host ma ajuta in cazul asta doar prin schimbarea
indexului, sa pot inainte cu parcurgerea. Nu s-ar fi putut realiza fara pentru
ca nu stiu dimensiunea lui qname. Apoi sar si peste qtype si qclass care au
impreuna 4 octeti. In punctul asta stiu ca index se afla la inceputul sectiunii
RR. In functie de ce am citit mai devreme (ce campuri am/cate - answer,
autorithy, additional), parsez urmatoarea parte de mesaj in functia
parse_record.
   Parse_record este functia cea mai consistenta din program. Citesc intai
numele si apoi ma folosesc de structura dns_rr_t ca sa accesez celelalte
campuri. Index creste cu 10, aceasta fiind dimensiunea structrii. Apoi, urmeaza
ca in functie de tipul RR-ului, sa parsez mai departe.
   - A - adresa de forma IP-ului. Citesc cate un atom si adaug punct dupa
fiecare mai putin ultimul
   - NS - parsez ca pe orice camp de nume
   - CNAME - la fel ca NS
   - MX - el contine un numar pe 16 biti si un camp de nume
   - SOA - contine 2 campuri de nume si 5 cu numere pe 16 biti care le citesc
basically ca la MX
   - TXT - contine un singur camp cu text. rdlength imi spune lungimea acestui
sir de text. Sirul contine caractere in felul urmator: primul este un numar
care imi spune cate caractere urmeaza, apoi iar un numar si un alt sir si asa
mai departe

Probleme:
   Apar uneori probleme cu pointerul din capurile de nume. Structura arata cam
asa:
            1 1 _ _ _ _ _ _   _ _ _ _ _ _ _ _
Octetul din stanga ne spune ca urmeaza un pointer si cel din dreapta contine
offsetul. Daca offsetul este mai mare de 255, atunci pentru stocare se folosesc
si biti din octetul din stanga, desi in mod normal, acei biti nu folosesc la
nimic. Nu am tratat acest caz, si sper sa nu apara prea des ^_^

Feedback:
   Tema mi s-a parut interesanta si, datorita ei, mi-am definitivat
cunostintele legate de pointeri. 