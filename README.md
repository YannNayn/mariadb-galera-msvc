mariadb-galera-msvc
===================
compiling:
have zlib.lib or zdll.lib in the link libpath as z.lib
have libeay32.lib in the link libpath as crypto.lib
have cmake installed (HEAD does it 2.18 is checked for this purpose)
have wix installed
launch a VC++ command prompt
in the source directory type :
>set WIX_WCAUTIL_LIBRARY=%WIX%\SDK\VS2008\lib\x86\wcautil.lib 
>set WIX_DUTIL_LIBRARY=%WIX%\SDK\VS2008\lib\x86\dutil.lib
where WIX is the  Wix installer installation path
type :
>set DEFAULT_TMPDIR="c:\\temp"
>cmake -G "NMake Makefiles"  -Wno-dev -DWITH_WSREP:BOOL=ON -DTMPDIR=%DEFAULT_TMPDIR% -DWIX_WCAUTIL_LIBRARY:FILE=%WIX_WCAUTIL_LIBRARY:\=/% -DWIX_DUTIL_LIBRARY:FILE=%WIX_DUTIL_LIBRARY:\=/%  -DWITH_EMBEDDED_SERVER=1 -DWITH_OQGRAPH:BOOL=FALSE -DWITH_ZLIB:STRING=system -DWITH_SSL:STRING=system -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo
>nmake install

not tested yet it just compiles please check the mariadb license
now for galera under windows ...

