#!/usr/bin/perl

use strict;
use warnings;
use diagnostics;

print "#include <sai.h>\n";
print "#include <saiextensions.h>\n";
print "#include <stddef.h>\n";
print "#include <ver.h>\n";

my $tag = $ARGV[0];
my $commit = $ARGV[1];

print "/* $tag ($commit) */\n";

print "const sai_meta_ver_enum_t sai_meta_ver_$commit\[\] = {\n";

while (<STDIN>)
{
    chomp;
    print "    { .value = $_, .name = \"$_\" },\n";

}
    print "    { .value = -1, .name = NULL },\n";

print "};\n";
