#!/usr/bin/perl

use strict;
use warnings;
use diagnostics;

undef $/;

my $content = <STDIN>;

exit if not $content =~ /_attr_t/s;

my @lines = split/\n/,$content;

my $mark = "";

for my $line (@lines)
{
    next if $line =~ m!^(/| |$|}|{)!;
    next if $line =~ m!__SAI\w*_H_!;
    next if $line =~ m!^#(if|endif|else|define _)!;
    next if $line =~ m!^typedef +u?int!i;

    if ($line =~ m!^#include!)
    {
        $mark .= "i";

        #print "i\n";
        next;
    }

    if ($line =~ m!^typedef enum _(sai_\w+_attr_t)!)
    {
        $mark .= "E";
        #print "E $1\n";
        next;
    }

    if ($line =~ m!^typedef enum _(sai_\w+_t)!)
    {
        $mark .= "e";
        #print "e $1\n";
        next;
    }

    if ($line =~ m!^typedef .+\*(sai_\w+_fn)!)
    {
        my $fun = $1;

        my $m = "U";

        $m = "c" if ($fun =~ /^sai_create_/);
        $m = "r" if ($fun =~ /^sai_remove_(?!all)/);
        $m = "s" if ($fun =~ /^sai_set_\w+_attribute_fn/);
        $m = "g" if ($fun =~ /^sai_get_\w+_attribute_fn/);
        $m = "G" if ($fun =~ /^sai_get_\w+_stats_fn/);
        $m = "C" if ($fun =~ /^sai_clear_\w+_stats_fn/);
        $m = "n" if ($fun =~ /^sai_\w+_notification_fn/);

        $mark .= $m;

        if ($m eq "U")
        {
            #  printf "Unknown: $fun\n";
        }

        #print "f $fun\n";
        next;
    }

    if ($line =~ m!^typedef struct _(sai_\w+_api_t)!)
    {
        $mark .= "A";
        #print "a $1\n";
        next;
    }

    if ($line =~ m!^typedef struct _(sai_\w+_t)!)
    {
        $mark .= "S";
        #print "s $1\n";
        next;
    }

    if ($line =~ m!^#define (SAI_\w+)!)
    {
        $mark .= "d";
        #print "d $1\n";
        next;
    }

    print "ERROR: XXX $line\n";
}

# i - include
# d - define
# e - enum
# E - enum attribute
# c - fn create
# r - fn remove
# s - fn set attr
# g - fn get attr
# G - fn get stats
# C - fn clear stats
# n - fn notification
# U - fn unknown
# S - struct
# A - struct api
# Q - quad (may include get/clear stats)

$mark =~ s/crsg(GC+)?/Q/g;

$mark =~ s/e+/e/g; # compres enums
$mark =~ s/d+/d/g; # compres defines
$mark =~ s/i+/i/g; # compres includes
$mark =~ s/U+/U/g; # compres unknowns
$mark =~ s/n+/n/g; # compres notifications

#e?S?E?n?U?
if (not $mark =~ m!^id?(e?S?EQ(e?S?E?(Un?|U?n))?)+A$!)
{
    print "$mark\n";
}
else
{
    #print "$mark\n";
}

