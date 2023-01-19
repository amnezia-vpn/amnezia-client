#! /usr/bin/perl
## vim:set ts=4 sw=4 et: -*- coding: utf-8 -*-
##
##---------------------------------------------------------------------------##
##
##  Author:
##      Markus F.X.J. Oberhumer         <markus@oberhumer.com>
##
##  Description:
##      Create incompressible files for compression test
##
##  Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
##
##---------------------------------------------------------------------------##

$x = ' ' x 65536;
$i = 0;
while ($i < 65536) {
    substr($x,$i,1) = pack('C',rand(256));
    $i++;
}

for $i (1,2,4,8,16,32,64) {
    $name = sprintf("u%04d.dat",$i);
    open(OUTFILE,">$name") || die "$0: cannot open '$name': $!";
    binmode(OUTFILE);
    print OUTFILE substr($x,0,$i*1024);
    close(OUTFILE);
}

exit(0);
