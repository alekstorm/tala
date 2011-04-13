#! /bin/csh
#
# Simple script to do a multi-mv command based on file extensions.
#
# John Yancey (jyancey@acm.org)
# Wed Mar 31 16:58:27 CST 1999

echo -n "Enter in the source file(s) extension(s) w/o the period. -> "
set EXT = $<

echo -n "Enter the destination file(s) extension(s) w/o the period. -> "
set NEXT = $<

foreach FILE ( `ls *.$EXT | awk -F"." '{print $1}'` )
        mv $FILE.$EXT $FILE.$NEXT
end

