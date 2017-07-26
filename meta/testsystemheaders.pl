#!/usr/bin/perl

use strict;
use warnings;
use diagnostics;

use XML::Simple qw(:strict);
use Getopt::Std;
use Data::Dumper;
use utils;

our $INCLUDE_DIR = "../inc/";

my %options = ();
getopts("d", \%options);

our $optionPrintDebug = 1 if defined $options{d};

$SIG{__DIE__} = sub
{
    LogError "FATAL ERROR === MUST FIX === : @_";
    exit 1;
};

my $SYSTEM_INCLUDE_DIR = "/usr/include/sai";

if (not -d $SYSTEM_INCLUDE_DIR)
{
    LogInfo "looking for system SAI headers using pkg-config";

    my $pkg = `/usr/bin/pkg-config --cflags sai`;

    chomp $pkg;

    $SYSTEM_INCLUDE_DIR = $1 if $pkg =~ m!-I(\S+)!;
}

if (not -d $SYSTEM_INCLUDE_DIR)
{
    LogInfo "System SAI headers directory not found";
    exit;
}

LogInfo "Found system SAI headers directory at $SYSTEM_INCLUDE_DIR";

sub ExtractEnums
{
    my ($text, $dir) = @_;

    my @lines = split/\n/,$text;

    my $src = "#include <stdio.h>\n" .
              "#include \"sai.h\"\n" . 
              "#include \"saitam.h\"\n" . 
              "int main(){";

    for my $line (@lines)
    {
        next if not $line =~ /^\s*(SAI_\w+)/;

        $src .= "printf(\"$1 %d\\n\", $1);\n";
    }

    $src .= "}";

    WriteFile("/tmp/enums.c", $src);

    my $cmd = "/usr/bin/gcc -I$dir /tmp/enums.c -o /tmp/enums";

    LogDebug "executing cmd: $cmd";

    my $ret = system($cmd); # TODO we could speed this up using 1 compile for all headers

    my %Enums = ();

    if ($ret != 0)
    {
        LogError "failed to compile /tmp/enum.c";
        return %Enums; 
    }

    @lines = `/tmp/enums`;

    for my $line (@lines)
    {
        $Enums{$1} = $2 if $line =~ /(SAI_\w+) (-?\d+)/;
    }

    return %Enums; 
}

sub CompareHeaders
{
    my ($header, $local, $system) = @_;

    my %localEnums = ExtractEnums($local, $INCLUDE_DIR);
    my %systemEnums = ExtractEnums($system, $SYSTEM_INCLUDE_DIR);

    LogDebug "$header: " . scalar(keys %localEnums), " ", scalar(keys %systemEnums);

    # we can't use direct compare, since enum values could be shifted, we need
    # to compile values

    for my $key (keys %localEnums)
    {
        next if not defined $systemEnums{$key};

        next if $key =~ /_ATTR_END$/;
        next if $key =~ /_MAX$/;

        next if $systemEnums{$key} == $localEnums{$key};

        LogError "enum value is different $key: $systemEnums{$key} != $localEnums{$key}";
    }
}

my @headers = GetHeaderFiles();

for my $header (@headers)
{
    LogDebug "Checking $header";

    my $localHeader = "$INCLUDE_DIR/$header";
    my $systemHeader = "$SYSTEM_INCLUDE_DIR/$header";

    next if not -e $systemHeader;

    my $local = ReadHeaderFile($localHeader);
    my $system = ReadHeaderFile($systemHeader);

    next if $local eq $system;
    
    LogDebug "files $localHeader and $systemHeader are different";

    # chnges could be cosmetic, so just check SAI_* enums

    CompareHeaders($header, $local, $system);
}

exit 1 if ($warnings > 0 or $errors > 0);
