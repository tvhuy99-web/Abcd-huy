Vietnamese catalog
==================

The Vietnamese catalog is stored as vi.po.gz to reduce repository size. The
Makefile, compile_mo.sh, and CMake localization target compile this compressed
catalog directly. To edit it, decompress vi.po.gz to vi.po, make the changes,
then recompress it with gzip -9 and remove the temporary plain-text copy.

Do not commit both vi.po and vi.po.gz; the plain-text catalog takes precedence
in build rules when both are present.
