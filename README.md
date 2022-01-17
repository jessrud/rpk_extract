rpk_extract
====
simple tool for extracting records for Examina's .rpk archives

## compilation:
developed and tested using clang64 version 11 under msys2, should work fine with any c99 compiler.

`make rpk_extract.exe`

## usage:
`rpk_extract.exe <archive>`

will extract contents to current directory

## notes:
Haven't actually made sense of the structure of a lot of the data in these archives/databases, but some of them definitely do store direct draw surfaces (I imagine textures) and something with the extension `.rfi`  and some kind of mesh data.

If you happen to know anything specific about the structure of this data, feel free to open a discussion in the issue tracker about it.

----

## LICENSE
© 2022 Jesse Rudolph <firstname dot lastname @ gmail.com>

MIT

see https://opensource.org/licenses/MIT