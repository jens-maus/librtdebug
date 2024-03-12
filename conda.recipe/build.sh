#!/bin/bash

make distclean
make PREFIX=${PREFIX}
make install
