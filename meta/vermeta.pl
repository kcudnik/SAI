#!/usr/bin/perl

use strict;
use warnings;
use diagnostics;

# TODO can be combinded with ver.pl and use switch

print "#include <stdint.h>\n";
print "#include <stddef.h>\n";
print "#include <ver.h>\n";

for my $name (@ARGV)
{
    next if not $name =~ /^ver_meta_(\w+).o$/;

    my $commit = $1;

    print "extern const sai_meta_ver_enum_t sai_meta_ver_$commit\[\];\n";
}

print "const sai_meta_version_t sai_meta_version_enums\[\] = {\n";

for my $name (@ARGV)
{
    next if not $name =~ /^ver_meta_(\w+).o$/;

    my $commit = $1;
    
    print "    { .saiversion = 1111, .enums = sai_meta_ver_$commit },\n";

}

print "    { .saiversion = -1, .enums = NULL },\n";

print "};\n";

print "int main() { return 0; }\n";
