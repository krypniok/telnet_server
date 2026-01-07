#!/bin/bash
DB="db.sqlite"
CMD=$1
EMAIL=$2
AMOUNT=${3:-3600} # Default to 3600 seconds (1 hour) if not specified

# check ob sqlite3 überhaupt da ist
if ! command -v sqlite3 &> /dev/null; then
    echo "fehler: sqlite3 ist nicht installiert‽"
    exit 1
fi

if [ -z "$CMD" ] || [ -z "$EMAIL" ]; then
    echo "usage: $0 <validate|deny|delete> <email>"
    exit 1
fi

case $CMD in
    validate) SQL="UPDATE users SET valid_until = valid_until + $AMOUNT WHERE email = '$EMAIL';" ;;
    deny)     SQL="UPDATE users SET valid_until = 0 WHERE email = '$EMAIL';" ;;
    delete)   echo "$EMAIL" >> blacklist.txt; SQL="DELETE FROM users WHERE email = '$EMAIL';" ;;
    *) echo "unbekannt: $CMD"; exit 1 ;;
esac

# ausführen und erfolg prüfen
sqlite3 "$DB" "$SQL"
if [ $? -eq 0 ]; then
    echo "user $EMAIL: $CMD erfolgreich."
else
    echo "datenbank-fehler bei $EMAIL‽"
fi