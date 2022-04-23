
param(
  [Parameter(Position=0)]
  [string[]]
  $BinDir = "..\x64\Release",
  [string[]]
  $DistDir = "..\dist",
  [string[]]
  $SrcDir = "."
)

New-Item -ItemType Directory -Force -Path "$DistDir\include"
New-Item -ItemType Directory -Force -Path "$DistDir\lib"

# Creates a minimal header file by stripping out all the private bits and unnecessary includes/usings.
# It's pretty specific to how the current header file is structured and which includes and usings it strips out.
# Specifically, it strips out everything from the 2nd "public:" specifier onwards (the 2nd one is used to declare statics for callbacks).
$HdrFile = "WASMIF.h"
(Get-Content -Raw "$SrcDir\$HdrFile") `
    -replace [RegEx]'(?ims)^(?:#include|using namespace) ["<]?(?:windows|stdio|vector|SimConnect|Client\w+|CDA\w+)(?:\.h)?[">;]?(?:\n|\r){1,2}', '' `
    -replace [RegEx]'(?ims)(^\s*public:(?:\n|\r){1,2}.+)^\s*public:.+};', '$1};' |
  Out-File -Encoding utf8 -NoNewline -Width 2000 "$DistDir\include\$HdrFile"

copy -Force "$SrcDir\WASM.h" "$DistDir\include\"

if ($BinDir -like '*Debug*') {
  copy -Force "$BinDir\FSUIPC_WAPI.lib" "$DistDir\lib\FSUIPC_WAPI_debug.lib"
  copy -Force "$BinDir\FSUIPC_WAPI.pdb" "$DistDir\lib\FSUIPC_WAPI_debug.pdb"
}
else {
  copy -Force "$BinDir\FSUIPC_WAPI.lib" "$DistDir\lib\"
  copy -Force "$BinDir\FSUIPC_WAPI.pdb" "$DistDir\lib\"
}
