#!/usr/bin/awk -f
# Prints origin of memory region of linker script (MEMORY command).
# Usage: awk -v region=<name> -f ldregion.awk <linker script>
{
        sub(/\r$/, "")
}

$0 ~ "^[ \t]*" region "[ \t]*\\(" {
        if (match($0, /(org|ORIGIN|o)[ \t]*=[ \t]*0[xX][0-9a-fA-F]+/)) {
                s = substr($0, RSTART, RLENGTH)
                sub(/^[^=]*=[ \t]*/, "", s)
                print s
                exit
        }
}
