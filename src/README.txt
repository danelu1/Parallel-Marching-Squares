Pentru implementarea variantei paralele a temei am considerat in prima faza o structura numita "thread_characteristics" care sa descrie componentele fiecarui thread.
Structura contine campurile:
    -> "id", pentru a stii ce thread executa operatiile necesare;
    -> "step_x", "step_y", pentru a calcula dimensiunile "p" si "q" ale grid-ului binar;
    -> "sigma", pentru a retine valoarea de izolare din enunt;
    -> "P", pentru retinerea numarului de thread-uri;
    -> "image", reprezinta imaginea ce trebuie procesata;
    -> "new_image", reprezinta imaginea rezultata in urma rularii algoritmului;
    -> "map", reprezinta lista in care sunt retinute toate contururile din directorul "contours";
    -> "grid", reprezinta grid-ul binar din enunt;
    -> "filename", reprezinta fisierul de iesire in care va fi salvat rezultatul;
    -> "barrier", reprezinta bariera pe care o voi folosii in implementarea paralela.

In functia "main" imi initializez toate caracteristicile legate de thread-uri. Mai intai atribui unor variabile valorile introduse in linia de comanda
(fisierul in care se afla imaginea, cel in care vreau sa se salveze rezultatul si numarul de thread-uri care sa execute functia "thread_function"), dupa
care consider un vector cu elemente de tip "pthread_t" , o bariera pe care o initializez si vectorul ce contine caracteristicile fiecarui thread in parte.
Imaginea initiala o citesc cu ajutorul functiei "read_ppm" oferita in rezolvarea secventiala a temei, iar lista de contururi o completez cu ajutorul functiei
"init_contour_map", la fel oferita in scheletul temei. In continuare, aloc memorie pentru grid si pentru imaginea ce va reprezenta rezultatul algoritmului,
dupa care initializez caracteristicile pentru fiecare thread ce va rula functia "thread_function"(cele din structura). De la inceput consider ca rezultatul
va avea dimensiunea de 2048x2048, dupa cum este specificat si in enuntul temei. Pornesc toate thread-urile intr-un singur "for" cu ajutorul functiei
"pthread_create", dupa care folosesc "pthread_join" in alt "for". La final, dupa ce campul "new_image" din structura a fost completat pixel cu pixel, scriu
output-ul in fisierul dat ca argument in linia de comanda, dupa care distrug bariera si eliberez toata memoria alocata.

In continuare, folosesc structura respectiva pentru a imi declara caracteristicile thread-ului curent cu care voi lucra. Incep cu primul pas din algoritm, in care
verific daca dimensiunile imaginii sunt mai mici sau egale cu "2048", caz in care imaginea "new_image"(ce reprezinta rezultatul) va fii chiar imaginea initiala, iar
in caz contrar incep sa paralelizez functia "rescale_image" din varianta secventiala a temei. Consider 2 indecsi("start" si "end") pe care ii calculez cu ajutorul
formulelor din laboratorul 1, dupa care paralelizez bucla exterioara corespunzatoare functiei de redimensionare. Folosesc o bariera la final, deoarece avem nevoie
de imaginea redimensionata pentru a putea continua algoritmul(vrem ca toate thread-urile sa completeze fiecare pixel din imaginea pe care o vom prelucra).
La pasul urmator, dorim sa paralelizam operatiile responsabile cu completarea grid-ului binar descris in enuntul problemei. Se calculeaza mai intai dimensiunile
"p" si "q" ale grid-ului, dupa care se incepe completarea in interior, respectiv ultima linie si coloana, prin compararea culorii pixel-ului curent cu cea data
de valoarea de izolare. Pentru acest pas am paralelizat bucla exterioara a celor 2 "for"-uri, ce reprezinta completarea interna a grid-ului, dupa care am reusit
sa paralelizez si cele 2 "for"-uri separate, responsabile cu completarea liniei si coloanei terminale.
In final, paralelizez si ultimul pas al algoritmului prin bucla exterioara data de functia "march", in care calculez valoarea ce trebuie asociata fiecarui patrat
al imaginii si mai apoi actualizata aceasta cu conturul corespunzator din directorul "contours".