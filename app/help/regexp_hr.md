# Regular expressions

Ant Renamer Qt uses the regular expressions of Qt (QRegularExpression, which is the PCRE2 library). You can find a [syntax reference on the Qt site](https://doc.qt.io/qt-6/qregularexpression.html), or the [PCRE2 syntax summary](https://www.pcre.org/current/doc/html/pcre2syntax.html). These expressions are quite complex but also quite "standard", so I won't make here a detailed guide explaining how they work. (The original Ant Renamer used the TRegExpr library, which is very close for everyday use: expressions written for it keep working.)

For the "Regular expression" action, you have to provide two values:

- **Expression**, containing a valid regular expression;
- **New name**, containing the new name of the file, with $*n* backreferences (*n* being the number of a group, in parentheses, of the expression) to indicate where found strings have to be inserted.

## How it works

- The expression is searched in the whole file name, extension included (not in the folder path).
- The search is **not case-sensitive**. To make it case-sensitive, start the expression with `(?-i)`.
- Every part of the name that matches the expression is replaced by the new name; what does not match is kept as it is. So `[0-9]+` with a new name of `X` turns "a12b345.txt" into "aXbX.txt". Use `^` to indicate the beginning of the name and `$` for the end of the name to match the whole name only.
- In the new name, `$1`, `$2`... `$9` insert the text found by the corresponding group, `$&` (or `$0`) inserts everything that was found, and `${10}` is the way to reach groups beyond 9 (`$10` means group 1 followed by a "0"). A group that found nothing gives an empty text.
- Lots of non-alphanumeric characters have a special meaning in regular expressions. To use them, the escape character is the backslash ("\\"). This also applies to the "$" sign that has a special meaning in the "new name" field: write `\$` to get a "$". If you need to use the backslash as a character (e.g. as path delimiter to send the file to another folder) you have to double it ("\\\\"), see the ["Force directories" option](options_hr.md#processing).
- If the expression is not valid, the preview shows the name `<RegExp Error>` followed by the message of the library, and a real run does not rename anything: each file gets a blue line in the log saying why. An empty expression changes nothing.

## Examples

Swapping artist and title from mp3 file names:

- Expression = `(.*) - (.*)\.mp3`
- New name = `$2 - $1.mp3`

Extract episode number and title from series video files with episode number as S*nn*E*mm* followed by title:

- Expression = `Code\.Quantum\.S([0-9]{2})E([0-9]{2})\.(.*)\.FRENCH.XViD\.avi`
- New name = `Code Quantum - $1$2 - $3.avi`

Remove the track number that starts a name ("01 - Song.mp3" becomes "Song.mp3"):

- Expression = `^[0-9]+ - (.*)`
- New name = `$1`

Put a date written day-month-year in front of the name in year-month-day order ("Trip 25-12-2024.jpg" becomes "2024-12-25 Trip .jpg"):

- Expression = `(.*)([0-9]{2})-([0-9]{2})-([0-9]{4})`
- New name = `$4-$3-$2 $1`

If a simple replacement is enough, the [String replacement](actions_hr.md#stringrepl) action (with its wildcards option) is easier to use.
