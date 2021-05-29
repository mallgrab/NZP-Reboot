ftp -inv <<EOF
open 192.168.1.211 1337
binary
put EBOOT.PBP /ux0:/pspemu/PSP/GAME/nzportable/EBOOT.PBP
bye
EOF
