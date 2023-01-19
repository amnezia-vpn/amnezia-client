#! /usr/bin/perl
## vim:set ts=4 sw=4 et: -*- coding: utf-8 -*-
##
##---------------------------------------------------------------------------##
##
##  Author:
##      Markus F.X.J. Oberhumer         <markus@oberhumer.com>
##
##  Description:
##      Convert the output of the LZO lzotest program into a nice table.
##
##  Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
##
##---------------------------------------------------------------------------##

$PROG = $0;
require 'ctime.pl';

#
# get options
#

while ($_ = $ARGV[ $[ ], /^-/) {
    shift(@ARGV);
    /^--$/ && ($opt_last = 1, last);

    /^--sort=name/ && ($opt_sort_summary_by_name++, next);
    /^--sort=ratio/ && ($opt_sort_summary_by_ratio++, next);
    /^-s/ && ($opt_summary_only++, next);
    /^-t/ && ($opt_clear_time++, next);
}


$alg = '';
$sep = "+" . ("-" x 76) . "+\n";

$block_size = -1;

$n = 0;
@algs = ();
%average = ();
%total = ();

$lzo_version_string = '';
$lzo_version_date = '';


# /***********************************************************************
# //
# ************************************************************************/

while (<>) {

    if (/(^|\s)(\d+)\s+block\-size/i) {
        if ($block_size < 0) {
            $block_size = $2;
            &intro($block_size);
        } elsif ($block_size != $2) {
            die "$PROG: block-size: $block_size != $2\n";
        }
        next;
    }

    if (/^\s*LZO\s.*library\s+\(v\s*([\w\.\s]+)\s*\,\s*([^\)]+)\)/) {
        $lzo_version_string = $1;
        $lzo_version_date = $2;
        next;
    }

    if (/^\s*(\S+(\s+\[\S+\])?)\s*(\|.*\|)\s*$/i) {
        if ($1 ne $alg) {
            &footer($1);
            &header($1);
        }
        $line = $3;
        &stats(*line);
        print "$line\n" if (!$opt_summary_only);
    }
}
&footer($1);

&summary();

exit(0);


# /***********************************************************************
# //
# ************************************************************************/

sub stats {
    local (*l) = @_;
    local ($x1, $x2, $x3, $x4, $x5, $x6, $x7, $x8);

    if ($l !~ /^\|\s*(.+?)\s+(\d+)\s+(\d+)\s+(\d+)\s+([\d\.]+\s+)?([\d\.]+\s+)?([\d\.]+)\s+([\d\.]+)\s*\|/) {
        die $_;
    }

    $n++;

    $x1 = $1; $x2 = $2; $x3 = $3; $x4 = $4;
    $x5 = ($x2 > 0) ? $x4 * 100.0 / $x2 : 0.0;
    $x6 = ($x2 > 0) ? $x4 *   8.0 / $x2 : 0.0;
    $x7 = $7; $x8 = $8;

    # convert from kB/s to MB/s (for old versions of lzotest)
    if ($x7 =~ /\.\d\d$/) { $x7 = $x7 / 1000.0; }
    if ($x8 =~ /\.\d\d$/) { $x8 = $x8 / 1000.0; }

    if ($opt_clear_time) {
        $x7 = $x8 = 0.0;
    }

    $s[0] += $x2;
    $s[1] += $x3;
    $s[2] += $x4;
    $s[3] += $x5;
    $s[4] += $x6;
    if ($x7 > 0) {
        $s[5] += 1.0 / $x7; $sn[5] += 1;
    }
    if ($x8 > 0) {
        $s[6] += 1.0/ $x8; $sn[6] += 1;
    }

    $x1 =~ s/\s+$//;
    $l = sprintf("| %-14s %10d %5d %9d %6.1f %5.2f %9.3f %9.3f |",
                    $x1, $x2, $x3, $x4, $x5, $x6, $x7, $x8);
}


# /***********************************************************************
# //
# ************************************************************************/

sub header {
    local ($t) = @_;

    $alg = $t;

    # reset stats
    $n = 0;
    @s = (0, 0, 0, 0.0, 0.0, 0.0, 0.0);
    @sn = (0, 0, 0, 0, 0, 0, 0);

    return if $opt_summary_only;

    print "\n$alg\n\n";
    print $sep;
print <<EndOfString;
| File Name          Length   CxB    ComLen  Ratio% Bits  Com MB/s  Dec MB/s |
| ---------          ------   ---    ------  -----  ----  --------  -------- |
EndOfString
}


# /***********************************************************************
# //
# ************************************************************************/

sub footer {
    local ($t) = @_;
    local ($shm5, $shm6);

    return unless $alg;
    die if $n <= 0;
    die if $s[0] <= 0;

    # harmonic mean
    $shm5 = $s[5] > 0 ? $sn[5] / $s[5] : 0.0;
    $shm6 = $s[6] > 0 ? $sn[6] / $s[6] : 0.0;

    push(@algs,$alg);

    $average{$alg} =
        sprintf("| %-14s %10d %5d %9d %6.1f %5.2f %9.3f %9.3f |\n",
            "Average", $s[0]/$n, $s[1]/$n, $s[2]/$n,
            $s[3]/$n, $s[4]/$n,
            $shm5, $shm6);

    $total{$alg} =
        sprintf("| %-14s %10d %5d %9d %6.1f %5.2f %9.3f %9.3f |\n",
            "Total", $s[0], $s[1], $s[2],
            $s[2]/$s[0]*100, $s[2]/$s[0]*8,
            $shm5, $shm6);

    return if $opt_summary_only;

    print $sep;
    print $average{$alg};
    print $total{$alg};
    print $sep, "\n";
}


# /***********************************************************************
# //
# ************************************************************************/

$sort_mode = 0;

sub cmp_by_ratio {
    local ($aa, $bb);

    if ($sort_mode == 0) {
        $aa = $average{$a};
        $bb = $average{$b};
    } elsif ($sort_mode == 1) {
        $aa = $total{$a};
        $bb = $total{$b};
    } else {
        die;
    }

    ($aa =~ m%^\s*\|\s+\S+\s+\d+\s+\d+\s+\d+\s+(\S+)%) || die;
    $aa = $1;
    ($bb =~ m%^\s*\|\s+\S+\s+\d+\s+\d+\s+\d+\s+(\S+)%) || die;
    $bb = $1;

    # $aa < $bb;
    $aa cmp $bb;
}


# /***********************************************************************
# //
# ************************************************************************/

sub summary {
    local ($l);
    local (@k);

    $sort_mode = 0;
    if ($opt_sort_summary_by_name) {
        @k = sort(@algs);
    } elsif ($opt_sort_summary_by_ratio) {
        @k = sort(cmp_by_ratio @algs);
    } else {
        @k = @algs;
    }

    print "\n\n";
    print "Summary of average values\n\n";
    print $sep;
print <<EndOfString;
| Algorithm          Length   CxB    ComLen  Ratio% Bits  Com MB/s  Dec MB/s |
| ---------          ------   ---    ------  -----  ----  --------  -------- |
EndOfString

    for (@k) {
        $l = $average{$_};
        $l =~ s/Average[\s]{7}/sprintf("%-14s",$_)/e;
        print $l;
    }
    print $sep;



    $sort_mode = 1;
    if ($opt_sort_summary_by_name) {
        @k = sort(@algs);
    } elsif ($opt_sort_summary_by_ratio) {
        @k = sort(cmp_by_ratio @algs);
    } else {
        @k = @algs;
    }

    print "\n\n";
    print "Summary of total values\n\n";
    print $sep;
print <<EndOfString;
| Algorithm          Length   CxB    ComLen  Ratio% Bits  Com MB/s  Dec MB/s |
| ---------          ------   ---    ------  -----  ----  --------  -------- |
EndOfString

    for (@k) {
        $l = $total{$_};
        $l =~ s/Total[\s]{9}/sprintf("%-14s",$_)/e;
        print $l;
    }
    print $sep;
}


# /***********************************************************************
# //
# ************************************************************************/

sub intro {
    local ($bs) = @_;
    local ($v, $t, $x);
    local ($u, $uname_m, $uname_s, $uname_r);

    $t = &ctime(time); chop($t);
    $t = sprintf("%-55s |", $t);

    $v='';
    if ($lzo_version_string) {
        $v = $lzo_version_string;
        $v .= ', ' . $lzo_version_date if $lzo_version_date;
        $v = sprintf("%-55s |", $v);
        $v = sprintf("| LZO version      : %s\n", $v);
    }

    if ($bs % 1024 == 0) {
        $x = sprintf("%d (= %d kB)", $bs, $bs / 1024);
    } else {
        $x = sprintf("%d (= %.3f kB)", $bs, $bs / 1024.0);
    }
    $x = sprintf("%-55s |", $x);

    $u='';
    if (1 == 1) {
        $uname_s = `uname -s`; $uname_s =~ s/^\s+//; $uname_s =~ s/\s+$//;
        $uname_r = `uname -r`; $uname_r =~ s/^\s+//; $uname_r =~ s/\s+$//;
        $uname_m = `uname -m`; $uname_m =~ s/^\s+//; $uname_m =~ s/\s+$//;
        if ($uname_s && $uname_m) {
            $u = $uname_s;
            $u .= ' ' . $uname_r if $uname_r;
            $u .= ' ' . $uname_m;
            $u = sprintf("%-55s |", $u);
            $u = sprintf("| Operating system : %s\n", $u);
        }
    }
    print <<EndOfString;

+----------------------------------------------------------------------------+
| DATA COMPRESSION TEST                                                      |
| =====================                                                      |
| Time of run      : $t
$v$u| Context length   : $x
+----------------------------------------------------------------------------+


Notes:
- CxB is the number of independent blocks a file was splitted
- MB/s is the speed measured in 1,000,000 uncompressed bytes per second
- all averages are calculated from the un-rounded values
- the average ratio & bits are calculated by the arithmetic mean
- the average speed is calculated by the harmonic mean


EndOfString
}

__END__


### insert something like this after 'Time of run':

| Hardware         : Intel Pentium 133, 64 MB RAM, 256 kB Cache          |
| Operating system : MS-DOS 7.10, HIMEM.SYS 3.95, DOS/4GW 1.97           |
| Compiler         : Watcom C32 10.5                                     |
| Compiler flags   : -mf -5r -oneatx                                     |
| Test suite       : Calgary Corpus Suite                                |
| Files in suite   : 14                                                  |
| Timing accuracy  : One part in 100                                     |


