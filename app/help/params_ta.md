# கட்டளை வரி அளவுருக்கள்

நிரல் (இன்னும்) நிறைய அளவுருக்களை ஆதரிக்கவில்லை. கட்டளையிலிருந்து அனைத்து மறுபெயர் அளவுருக்களையும் குறிப்பிடுவதன் மூலம் கோப்புகளை மறுபெயரிடுவது உண்மையில் சாத்தியமில்லை வரி, ஆனால் அவை சேமிக்கப்பட்ட [தொகுதி கோப்பு ](batch_ta.md) மூலம் கொடுக்கப்படலாம்.

ஆதரிக்கப்படும் அளவுருக்களின் பட்டியல் இங்கே:

- `-b கோப்பு.arb` : கோப்பு.arb எனப்படும் தொகுதி கோப்பை ஏற்றுகிறது
- `-g` : கோப்புகள் சேர்க்கப்பட்டு தொகுதி கோப்பு ஏற்றப்பட்ட பிறகு மறுபெயரிடும் செயல்முறையைத் தொடங்குகிறது
- `-x` : மறுபெயரிடல் செயல்முறைக்குப் பிறகு நிரலை விட்டு வெளியேறுகிறது
- `-q` : Silent mode, for scripts: no window and no dialog, the renaming starts and the program quits at the end (like `-g -x`, without the window). The exit code tells how it went: 0 everything done, 1 at least one file was not renamed because of an error, 2 nothing done (no file, no batch, batch file unreadable, or hidden files in the list). The log file of the options is still written
- `--shell Path` : (Explorer context menu) adds the path to the window that is already open, or starts the program if it is not running
- Only one window at a time: a second launch shows a message and quits. Paths given alone (a file dropped on the program or its shortcut, "Open with") are added to the window that is already open. With `-g` or `-x`, the message goes to the error output and the exit code is 3.
- `--install-shell` / `--remove-shell` : adds / removes the Explorer context menu entry, then quits
- `-a` : "-a" அளவுருவைப் பின்பற்றும் எழுத்துக்களின் அடிப்படையில் கோப்பு சேர் பயன்முறையை மாற்றவும்:
  - `f` : பின்வரும் கோப்பு பெயர்கள் / பாதைகளிலிருந்து கோப்புகளைச் சேர்க்கவும்
  - `F` : தொடர்ந்து வரும் அல்லது கோப்புறைகளில் உள்ள கோப்புறைகளைச் சேர்க்கவும், எடுத்துக்காட்டுகளைக் காண்க
  - `r` : கோப்புறைகளின் உள்ளடக்கங்களை மீண்டும் மீண்டும் சேர்த்தல்

கோப்பு பெயர்கள் மற்றும் பாதைகள் உறவினர் அல்லது முழுமையானதாக இருக்கலாம், மேலும் அவை வைல்டு கார்டுகளைக் கொண்டிருக்கலாம் (\* மற்றும் ?).

**Ant Renamer Qt**: the program is called `antrenamer-qt.exe` (Windows) or `antrenamer-qt` (Linux); in the examples below, replace `Renamer.exe` with this name.

இங்கே சில உதாரணங்கள்:

- `Renamer.exe -b ஈ:\சோதனை.arb -af இ:\தகவல்\*.* -g -x`  

    இது குறிப்பிடப்பட்ட தொகுதி கோப்பை ஏற்றும், இ:\\தகவல் கோப்புறையில் உள்ள
 அனைத்து கோப்புகளையும் சேர்த்து, மறுபெயரிடத் தொடங்கி பின்னர் வெளியேறும் 
- `Renamer.exe -af இ:\தகவல்`  

    இது இ:\\தகவல் எனப்படும் கோப்பைச் சேர்க்கும் 
- `Renamer.exe -afr இ:\தகவல்`  

    இது இ:\\தகவல் எனப்படும் கோப்பைச் சேர்க்கும். இது ஒரு கோப்புறையாக 
இருந்தால், அது கொண்டிருக்கும் கோப்புகளைச் சேர்க்கும் (மீண்டும் மீண்டும் 
தேடல்) 
- `Renamer.exe -aF இ:\தகவல்`  

    இது "தரவு" கோப்புறையைச் சேர்க்கும் 
- `Renamer.exe -aF இ:\தகவல்\*.*`  

    இது "தரவு" கோப்புறையில் உள்ள கோப்புறைகளைச் சேர்க்கும், ஆனால் கோப்புகள் அல்ல 
- `Renamer.exe -af இ:\தகவல்\*.* இ:\படங்\*.jpg இ:\சோதனை.txt -afr இ:\எம்பி3\*.*`  

    நீங்கள் பல பாதைகள்/கோப்பு பெயர்களை இணைக்கலாம், முகமூடிகளைப் 
பயன்படுத்தலாம் மற்றும் ஒரு கட்டளை வரியில் வெவ்வேறு "-a" அளவுருக்களைப் 
பயன்படுத்தலாம் 

எந்த "-a" அளவுருவும் இல்லாமல் ஒரு பெயர் கொடுக்கப்பட்டால், அது எறும்பு மறுபெயரிடு 2.07 (மற்றும் பழையது) இல் உள்ளதைப் போல பகுப்பாய்வு செய்யப்படும்: ".arb" ஆல் முடிவடையும் கோப்பு பெயர் கொடுக்கப்பட்டால், இது தொகுதி கோப்பாக ஏற்றப்படுகிறது; ஒரு கோப்புறை கொடுக்கப்பட்டால், அதன் உள்ளடக்கங்கள் மீண்டும் மீண்டும் சேர்க்கப்படும் (கோப்புகள் மட்டும், வைல்டு கார்டுகளை ஆதரிக்காது, முழு கோப்புறை பெயர் தேவை).

On Windows, it is possible to add Ant Renamer in the "Send to" list (in the menu that appears when you click with right mouse button on a folder in Windows Explorer). So this adds the possibility to send a folder to Ant Renamer from its contextual menu, to load its content in the files list. To do that, go to "Start" -\> "Run" and type `sendto`. A folder window should open; place there a shortcut to Ant Renamer.
