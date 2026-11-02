# Parametri naredbenog retka

Program ne podržava (još uvijek) puno parametara. Zapravo i nije moguće preimenovati datoteke određivanjem svih potrebnih parametara za preimenovanje iz naredbenog retka, ali oni se mogu dati kroz [spremanje datoteke za skupne obradbe](batch_hr.md).

Ovdje je popis podržanih parametara:

- `-b FileName.arb` : Učitava datoteku za skupne obradbe FileName.arb
- `-g` : Pokreće postupak preimenovanja nakon što su datoteke dodane i datoteka za skupne obradbe učitana
- `-x` : Zatvara program nakon preimenovanja
- `-q` : Silent mode, for scripts: no window and no dialog, the renaming starts and the program quits at the end (like `-g -x`, without the window). The exit code tells how it went: 0 everything done, 1 at least one file was not renamed because of an error, 2 nothing done (no file, no batch, batch file unreadable, or hidden files in the list). The log file of the options is still written
- `--shell Path` : (Explorer context menu) adds the path to the window that is already open, or starts the program if it is not running
- `--install-shell` / `--remove-shell` : adds / removes the Explorer context menu entry, then quits
- `-a` : Mijenja način dodavanja datoteke, na temelju slova nakon "-a" parametra:
  - `f` : Dodaj datoteke iz naziva datoteka / putanja koje slijede
  - `F` : Dodaje mape koje slijede, ili koje su sadržane u mapama, vidjeti primjere
  - `r` : Rekurzivno dodavanje sadržaja mape

Nazivi datoteka i putanje mogu biti relativne ili apsolutne i mogu sadržavati zamjenske znakove (\* i ?).

**Ant Renamer Qt**: the program is called `antrenamer-qt.exe` (Windows) or `antrenamer-qt` (Linux); in the examples below, replace `Renamer.exe` with this name.

Evo nekoliko primjera:

- `Renamer.exe -b d:\test.arb -af c:\data\*.* -g -x`  

    To će učitati navedenu datoteku za skupne obradbe, dodati sve datoteke sadržane u C:\\data mapi, pokrenuti preimenovanje i onda izaći
- `Renamer.exe -af c:\data`  

    To će dodati datoteku naziva c:\\data
- `Renamer.exe -afr c:\data`  

    To će dodati datoteku naziva c:\\data. Ako je to mapa, dodati će datoteke koje sadrži (rekurzivna pretraga)
- `Renamer.exe -aF c:\data`  

    To će dodati "data" mapu
- `Renamer.exe -aF c:\data\*.*`  

    To će dodati mape sadržane u mapi "data", ali ne i datoteke
- `Renamer.exe -af c:\data\*.* c:\images\*.jpg c:\test.txt -afr c:\mp3\*.*`  

    Možete kombinirati nekoliko putanja/naziva datoteka koristeći maske i koristeći različite "-a" parametre u jednom naredbenom retku

Ako je naziv dan bez ikakvog "-a" parametra, to će se analizirati kao u Ant Renamer 2.07 (i starijim) : ako naziv datoteke završava s ".arb", biti će učitana kao datoteka za skupne obradbe; ako je dana mapa, njen se sadržaj dodaje rekurzivno (samo datoteke, nisu podržani zamjenski znakovi, potreban je potpuni naziv mape).

On Windows, it is possible to add Ant Renamer in the "Send to" list (in the menu that appears when you click with right mouse button on a folder in Windows Explorer). So this adds the possibility to send a folder to Ant Renamer from its contextual menu, to load its content in the files list. To do that, go to "Start" -\> "Run" and type `sendto`. A folder window should open; place there a shortcut to Ant Renamer.
