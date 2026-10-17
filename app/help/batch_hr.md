# Predefinirana lista (serija)

Ant Renamer može pohraniti i koristiti predefinirane popise akcija, ovdje pod nazivom "serija". Njima se može upravljati sa stranice [Akcije](actions_hr.md).

Kao dodatak alatnoj traci Datoteka koja je uvijek vidljiva, alatna traka za upravljanje tim naredbenim datotekama prikazuje se:  

![Alatna traka serija](img/batchtoolbar_en.png)

Ovdje se naredbe:

- ![Dodaj u seriju](img/batchadd.png) Tipka "Dodaj akciju u seriju" će dodati odabranu akciju i njene parametare na popis.
- ![Ukloni iz serije](img/batchdel.png) Tipka "Ukloni izabrane akcije" će ukloniti s popisa stavke koje su odabrane.
- ![Očisti](img/clear.png) Tipka "Očisti seriju" će ukloniti s popisa sve predefinirane akcije.
- ![Otvori](img/open.png) Tipka "Otvori" će prikazati standardni prozor za odabir datoteke, tako da možete odabrati datoteku za skupne obradbe za učitavanje u popis.
- ![Spremi](img/save.png) Tipka "Spremi" će prikazati standardni prozor za odabir datoteke, tako da možete odabrati gdje spremiti sadržaj popisa za buduću uporabu.

Ove datotekame za skupne obrade se mogu koristiti za kombiniranje više radnji u jednom procesu preimenovanja. One se mogu spremiti za buduću uporabu, ili ih koristiti iz [naredbenog retka](params_hr.md).

Pohranjene su u XML datotekama, ali imaju "??.arb" datotečni nastavak (sa značenjem "Ant Renamer Batch").

Popis trenutnih planiranih akcija prikazuje se u donjem dijelu, pod nazivom "Sadržaj serije". Ako nije vidljivo, može se prikazati klikom na kućicu na lijevoj strani njenog naziva.

Možete premjestiti stavke gore i dolje na popisu povlačenjem & ispuštanjem ili pomoću četiri dodatne ikone koje se pojavljuju na alatnoj traci kada je zaslon serije vidljiv.

Nakon što je dodan, predmet se ne može mijenjati. Morate ga ukloniti, redefinirati parametre i dodati ga opet.

Ant Renamer Qt, modern layout: an item of the list can be modified. Double-click it to load its parameters in the editor, then click "Validate the modification" or "Modify", see [editing an action of the batch](modernui_hr.md#edit).
