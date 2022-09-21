This requires jq and a copy of Win3.1 in ./WINDOWS

I used the following two scripts to find the list of DLLs whose entries get used.

imported.sh:
```sh
#!/bin/sh
find WINDOWS -type f -exec file {} + |
	grep 'NE for MS Windows' |
	cut -d: -f1 |
	xargs rpcli -j 2>/dev/null |
	jq -r '.[] | .fields[] | select(.desc.name == "Imports") | .data[] | .[1]' |
	sort -u
```

modulenames.sh:
```sh
#!/bin/sh
find WINDOWS -type f -exec file {} + |
	grep 'NE for MS Windows' |
	cut -d: -f1 |
	sort |
	tee /dev/stderr 2>temp1.txt |
	xargs rpcli -j 2>/dev/null |
	jq -r '.[] | .fields[] | select(.desc.name == "Module Name") | .data' >temp2.txt
paste temp2.txt temp1.txt
```

Use them like so:
```sh
modulenames.sh > modnames.txt
./imported.sh | xargs -I{} grep '^{}	' modnames.txt
```

Based on that list I made the following script which actually generates the header
genexports.sh:
```sh
#!/bin/sh
echo "// see EXENEEntries_info.md for more info on how this file was generated."
gen1() {
	echo "static const OrdinalName $1_entries[] = {"
	rpcli -j $2 2>/dev/null |
		jq -r '.[0].fields[] | select(.desc.name == "Entries") | .data[] | select(.[1] != "(No name)") | "	{ \(.[0]), \"\(.[1])\" },"'
	echo '};'
}
postgen1() {
	echo "static const OrdinalNameTable entries[] = {"
}
gen2() {
	echo "	{ \"$1\", $1_entries, ARRAY_SIZE($1_entries) },"
}
postgen2() {
	echo '};'
}

for i in 1 2; do
gen$i COMM     WINDOWS/SYSTEM/COMM.DRV
gen$i COMMDLG  WINDOWS/SYSTEM/COMMDLG.DLL
gen$i DISPLAY  WINDOWS/SYSTEM/VGA.DRV
gen$i GDI      WINDOWS/SYSTEM/GDI.EXE
gen$i GRABBER  WINDOWS/SYSTEM/VGA.3GR
gen$i KERNEL   WINDOWS/SYSTEM/KRNL386.EXE
gen$i KEYBOARD WINDOWS/SYSTEM/KEYBOARD.DRV
gen$i LZEXPAND WINDOWS/SYSTEM/LZEXPAND.DLL
gen$i MMSYSTEM WINDOWS/SYSTEM/MMSYSTEM.DLL
gen$i MOUSE    WINDOWS/SYSTEM/MOUSE.DRV
gen$i OLECLI   WINDOWS/SYSTEM/OLECLI.DLL
gen$i OLESVR   WINDOWS/SYSTEM/OLESVR.DLL
gen$i SHELL    WINDOWS/SYSTEM/SHELL.DLL
gen$i SOUND    WINDOWS/SYSTEM/SOUND.DRV
gen$i SYSTEM   WINDOWS/SYSTEM/SYSTEM.DRV
gen$i TOOLHELP WINDOWS/SYSTEM/TOOLHELP.DLL
gen$i USER     WINDOWS/SYSTEM/USER.EXE
gen$i VER      WINDOWS/SYSTEM/VER.DLL
gen$i WIN87EM  WINDOWS/SYSTEM/WIN87EM.DLL
postgen$i
done
```
