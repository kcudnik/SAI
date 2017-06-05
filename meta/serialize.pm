#!/usr/bin/perl

package serialize;

use strict;
use warnings;
use diagnostics;
use Getopt::Std;
use Data::Dumper;
use utils;
use xmlutils;

require Exporter;

sub CreateSerializeForEnums
{
    WriteSectionComment "Enum serialize methods";

    for my $key (sort keys %main::SAI_ENUMS)
    {
        next if $key =~/_attr_t$/;

        if (not $key =~ /^sai_(\w+)_t$/)
        {
            LogWarning "wrong enum name '$key'";
            next;
        }

        my $inner = $1;

        WriteHeader "extern int sai_serialize_$inner(";
        WriteHeader "        _Out_ char *buffer,";
        WriteHeader "        _In_ $key $inner);";

        WriteSource "int sai_serialize_$inner(";
        WriteSource "        _Out_ char *buffer,";
        WriteSource "        _In_ $key $inner)";
        WriteSource "{";
        WriteSource "    return sai_serialize_enum(buffer, &sai_metadata_enum_$key, $inner);";
        WriteSource "}";
    }
}

# XXX this serialize is wrong, it should go like normal struct since
# its combination for easier readibility, we need 2 versions of this ?

sub CreateSerializeMetaKey
{
    WriteSectionComment "Serialize meta key";

    WriteHeader "extern int sai_serialize_object_meta_key(";
    WriteHeader "        _Out_ char *buffer,";
    WriteHeader "        _In_ const sai_object_meta_key_t *meta_key);";

    WriteSource "int sai_serialize_object_meta_key(";
    WriteSource "        _Out_ char *buffer,";
    WriteSource "        _In_ const sai_object_meta_key_t *meta_key)";
    WriteSource "{";

    WriteSource "    if (!sai_metadata_is_object_type_valid(meta_key->objecttype))";
    WriteSource "    {";
    WriteSource "        SAI_META_LOG_WARN(\"invalid object type (%d) in meta key\", meta_key->objecttype);";
    WriteSource "        return SAI_SERIALIZE_ERROR;";
    WriteSource "    }";

    WriteSource "    buffer += sai_serialize_object_type(buffer, meta_key->objecttype);";

    WriteSource "    *buffer++ = ':';";

    WriteSource "    switch (meta_key->objecttype)";
    WriteSource "    {";

    my @rawnames = GetNonObjectIdStructNames();

    for my $rawname (@rawnames)
    {
        my $OT = uc ("SAI_OBJECT_TYPE_$rawname");

        WriteSource "        case $OT:";
        WriteSource "            return sai_serialize_$rawname(buffer, &meta_key->objectkey.key.$rawname);";
    }

    WriteSource "        default:";
    WriteSource "            return sai_serialize_object_id(buffer, meta_key->objectkey.key.object_id);";

    WriteSource "    }";

    WriteSource "}";
}

sub CreateSerializeSingleNotification
{
    #
    # we are using sprintf here, but in all cases it's const string passed, so
    # compiler will optimize this to strcpy, and no snprintf function will be
    # actually called, actual functions called will be those written by user in
    # saiserialize.c and optimization should focus on those functions
    #
    # TODO we need version with like snprintf to write only N characters since
    # right how we don't know how long output will be, for long arrays it can
    # be even kB
    #

    my ($ntftype, $ntf) = @_;

    my $ntfname = $1 if $ntftype =~ /^sai_(\w+)_fn$/;

    my $idx = 0;
    my $buf = "buf"; # can be changed if there will be name conflict

    WriteSource "int sai_serialize_$ntfname(";
    WriteSource "        _Inout_ char *$buf,";

    WriteHeader "extern int sai_serialize_$ntfname(";
    WriteHeader "        _Inout_ char *$buf,";

    while (defined $ntf->{$idx})
    {
        my $end = (defined $ntf->{$idx + 1}) ? "," : ")";
        my $endheader = (defined $ntf->{$idx + 1}) ? "," : ");";

        my $type = $ntf->{$idx}{type};
        my $name = $ntf->{$idx}{name};

        WriteSource "        _In_  $type $name$end";
        WriteHeader "        _In_  $type $name$endheader";

        $idx++;
    }

    my $max = $idx - 1;

    WriteSource "{";
    WriteSource "    char *start_$buf = $buf;";

    # we will treat notification params as struct members
    # and they will be serialized as json object

    # all consts printfs could be exchanged to memcpy for optimize but we
    # assume number of notifications is small, and this is fast enough

    WriteSource "    $buf += sprintf($buf, \"{\\\"$ntfname\\\":\");";
    WriteSource "    $buf += sprintf($buf, \"{\");";

    # maybe drop quotes on param names?

    # TODO reuse this in struct serialize
    for $idx (0 .. $max)
    {
        my $type = $ntf->{$idx}{type};
        my $name = $ntf->{$idx}{name};
        my $count = $ntf->{$idx}{count};

        my $comma = ($idx != 0) ? "," : "";

        WriteSource "    $buf += sprintf($buf, \"$comma\\\"$name\\\":\");";

        if ($type =~ /^(?:const )?(sai_(\w+)_t|void)\s*\*$/)
        {
            $type = $1;

            my $shorttype = $2;

            my $numberList = 0;

            my $isstruct = (defined $main::ALL_STRUCTS{$type}) ? "&" : "";

            if ($type eq "void")
            {
                # treat void* as uint8_t*

                $numberList = 1;
                $shorttype = "u8";
                $name = "((const uint8_t*)$name)";
            }
            elsif (not defined $main::SAI_ENUMS{$type} and not defined $main::ALL_STRUCTS{$type})
            {
                LogError "type '$type' not enum and not struct, FIXME";
                next;
            }

            if (not defined $count)
            {
                LogError "count is not defined for '$name' on $ntfname";
                return;
            }

            if ($type eq "sai_attribute_t")
            {
                if (not defined $ntf->{$idx}{ot})
                {
                    LogError "param '$name' is $type and requires object type specified, but not provided";
                    next;
                }

                my $objectType = $ntf->{$idx}{ot};

                $isstruct = "$objectType, &";
            }

            WriteSource "    if ($name == NULL)";
            WriteSource "    {";
            WriteSource "        $buf += sprintf($buf, \"null\");";
            WriteSource "    }";
            WriteSource "    else";
            WriteSource "    {";
            WriteSource "        $buf += sprintf($buf, \"[\");"; # begin of array

            my $index     = $ntf->{$count};
            my $countName = $ntf->{$index}{name};
            my $countType = $ntf->{$index}{type};

            WriteSource "        $countType idx;";

            WriteSource "        for (idx = 0; idx < $countName; idx++)";
            WriteSource "        {";
            WriteSource "            $buf += sprintf($buf, \"\\\"\");" if $isstruct eq "" and not $numberList;
            WriteSource "            int ret = sai_serialize_$shorttype($buf, $isstruct$name\[idx\]);";
            WriteSource "            if (ret < 0)";
            WriteSource "            {";
            WriteSource "                SAI_META_LOG_WARN(\"failed to serialize '$name' at index %u\", (uint32_t)idx);";
            WriteSource "                return SAI_SERIALIZE_ERROR;";
            WriteSource "            }";
            WriteSource "            $buf += ret;";
            WriteSource "            $buf += sprintf($buf, \"\\\"\");" if $isstruct eq "" and not $numberList;
            WriteSource "            if (idx != $countName - 1)";
            WriteSource "               $buf += sprintf($buf, \",\");";
            WriteSource "        }";
            WriteSource "        $buf += sprintf($buf, \"]\");"; # end of array
            WriteSource "    }";
            next;
        }

        # primitives or types where serialization is defined explicitly
        # and it requires quotes (not numbers) and not structs/objects

        if ($type =~ m/^sai_(object_id|mac|ip_address|ip_prefix)_t$/ or defined $main::SAI_ENUMS{$type})
        {
            $type = $1 if $type =~ /^sai_(\w+)_t$/;

            WriteSource "    $buf += sprintf($buf, \"\\\"\");";
            WriteSource "    $buf += sai_serialize_$type($buf, $name);";
            WriteSource "    $buf += sprintf($buf, \"\\\"\");";
            next;
        }

        # primitives, numbers which don't require quotes

        if ($type =~ m/^(?:sai_)?(size|u?int\d+)_t$/)
        {
            $type = $1;
            $type =~ s/uint/u/;
            $type =~ s/int/s/;

            WriteSource "    $buf += sai_serialize_$type($buf, $name);";
            next;
        }

        LogError "not handled $type $name, FIXME";
        next;
    }

    WriteSource "    $buf += sprintf($buf, \"}\");"; # end of notification object with members
    WriteSource "    $buf += sprintf($buf, \"}\");"; # end of entire object
    WriteSource "    return (int)($buf - start_$buf);";
    WriteSource "}";
}

# TODO we need unified hash for notifications/structs

sub CreateSerializeSingleStruct
{
    my $structName = shift;

    my %struct = ExtractStructInfo($structName, "struct_");

    my $structBase = $1 if $structName =~/^sai_(\w+)_t$/;

    my @keys = GetStructKeysInOrder(\%struct);

    # don't create serialize methods for metadata structs 
    return if ($struct{$keys[0]}->{file} =~ m!meta/sai\w+.h$!);

    LogInfo "Create serialzie for $structName";

    my $buf = "buf"; # can be changed if there will be name conflict

    WriteHeader "int sai_serialize_${structBase}(";
    WriteHeader "        _Out_ char *$buf,";
    WriteHeader "        _In_ const $structName *$structBase);";

    WriteSource "int sai_serialize_${structBase}(";
    WriteSource "        _Out_ char *$buf,";
    WriteSource "        _In_ const $structName *$structBase)";
    WriteSource "{";
    WriteSource "    char *start_$buf = $buf;";
    WriteSource "    $buf += sprintf($buf, \"{\");";

    my $quot = "$buf += sprintf($buf, \"\\\"\")";

    my $idx = 0;

    # TODO on s32/s32_list in struct we could declare enum type

    for my $name (@keys)
    {
        my $type = $struct{$name}{type};
        my $count = $struct{$name}{count};

        my $comma = ($idx != 0) ? "," : "";

        WriteSource "    $buf += sprintf($buf, \"$comma\\\"$name\\\":\");";

        my $suffix = $type;

        my $needQuote = 0;
        my $ispointer = 0;
        my $amp = "";

        if ($type =~ /\s*\*$/)
        {
            $type =~ s!\s*\*$!!;
            # we need count !
            $ispointer = 1;
        }

        if ($type eq "bool")
        {
            $needQuote = 1;
        }
        elsif ($type =~ m/^sai_(object_id|mac|ip_address|ip_prefix)_t$/)
        {
            $suffix = $1;
            $needQuote = 1;
        }
        elsif (defined $main::ALL_STRUCTS{$type} and $type =~ /^sai_(\w+)_t$/)
        {
            $suffix = $1;
            $amp = "&";
        }
        elsif (defined $main::SAI_ENUMS{$type} and $type =~ /^sai_(\w+)_t$/)
        {
            $suffix = $1;
            $needQuote = 1;
        }
        elsif ($type =~ /^sai_ip6_t$/)
        {
            $needQuote = 1;
            $suffix = "ipv6";
        }
        elsif ($type =~ /^sai_(vlan_id)_t$/)
        {
            $suffix = "u16";
        }
        elsif ($type =~ /^sai_(cos|queue_index)_t$/)
        {
            $suffix = "u8";
        }
        elsif ($type =~ /^(?:sai_)?(u?int\d+)_t$/)
        {
            $suffix = $1;
            $suffix =~ s/uint/u/;
            $suffix =~ s/int/s/;
        }
        else
        {
            LogError "not handled $type $name in $structName, FIXME";
            next;
        }

        $idx++;

        if (not $ispointer)
        {
            WriteSource "    $quot;" if $needQuote;
            WriteSource "    $buf += sai_serialize_$suffix($buf, $amp$structBase->$name);";
            WriteSource "    $quot;" if $needQuote;
            next;
        }

        # this is pointer

        # we don't have indexing in structs
        my $index     = $struct{ $keys[$count] }{count};
        my $countName = $struct{ $keys[$count] }{name};
        my $countType = $struct{ $keys[$count] }{type};

        WriteSource "    if ($structBase->$name == NULL)";
        WriteSource "    {";
        WriteSource "        $buf += sprintf($buf, \"null\");";
        WriteSource "    }";
        WriteSource "    else";
        WriteSource "    {";
        WriteSource "        $buf += sprintf($buf, \"[\");"; # begin of array
        WriteSource "        $countType idx;";
        WriteSource "        for (idx = 0; idx < $structBase->$countName; idx++)";
        WriteSource "        {";
        WriteSource "            $quot;" if $needQuote;
        WriteSource "            int ret = sai_serialize_$suffix($buf, $amp$structBase->$name\[idx\]);";
        WriteSource "            if (ret < 0)";
        WriteSource "            {";
        WriteSource "                SAI_META_LOG_WARN(\"failed to serialize '$name' at index %u\", (uint32_t)idx);";
        WriteSource "                return SAI_SERIALIZE_ERROR;";
        WriteSource "            }";
        WriteSource "            $buf += ret;";
        WriteSource "            $quot;" if $needQuote;
        WriteSource "            if (idx != $countName - 1)";
        WriteSource "               $buf += sprintf($buf, \",\");";
        WriteSource "        }";
        WriteSource "        $buf += sprintf($buf, \"]\");"; # end of array
        WriteSource "    }";

        next;

        #--------------------

        if (defined $main::ALL_STRUCTS{$type})
        {
            $type = $1 if $type =~ /^sai_(\w+)_t$/;

            WriteSource "    $buf += sai_serialize_$type($buf, $structBase->$name);";
            next;
        }

        # TODO need quote / need amp

        if ($type =~ m/^sai_(object_id|mac|ip_address|ip_prefix)_t$/ 
                or defined $main::SAI_ENUMS{$type}
                or $type eq "bool")
        {
            $type = $1 if $type =~ /^sai_(\w+)_t$/;

            WriteSource "    $quot;";
            WriteSource "    $buf += sai_serialize_$type($buf, $structBase->$name);";
            WriteSource "    $quot;";
            next;
        }

        # pointers types
        if ($type =~ /^(?:const )?(sai_(\w+)_t|void)\s*\*$/)
        {
            $type = $1;

            my $shorttype = $2;

            my $numberList = 0;

            my $isstruct = (defined $main::ALL_STRUCTS{$type}) ? "&" : "";

            if ($type eq "void")
            {
                # treat void* as uint8_t*

                $numberList = 1;
                $shorttype = "u8";
                $name = "((const uint8_t*)$name)";
            }
            elsif (not defined $main::SAI_ENUMS{$type} and not defined $main::ALL_STRUCTS{$type})
            {
                LogError "type '$type*' not enum and not struct, FIXME";
                next;
            }

            if (not defined $count)
            {
                LogError "count is not defined for '$name' on $structName";
                return;
            }

            if ($type eq "sai_attribute_t")
            {
                LogError "not supported $type in $structName";
                return
            }

            WriteSource "    if ($name == NULL)";
            WriteSource "    {";
            WriteSource "        $buf += sprintf($buf, \"null\");";
            WriteSource "    }";
            WriteSource "    else";
            WriteSource "    {";
            WriteSource "        $buf += sprintf($buf, \"[\");"; # begin of array

            # we don't have indexing in structs
            my $index     = $struct{ $keys[$count] }{count};
            my $countName = $struct{ $keys[$count] }{name};
            my $countType = $struct{ $keys[$count] }{type};

            WriteSource "        $countType idx;";

            WriteSource "        for (idx = 0; idx < $countName; idx++)";
            WriteSource "        {";
            WriteSource "            $buf += sprintf($buf, \"\\\"\");" if $isstruct eq "" and not $numberList;
            WriteSource "            int ret = sai_serialize_$shorttype($buf, $isstruct$name\[idx\]);";
            WriteSource "            if (ret < 0)";
            WriteSource "            {";
            WriteSource "                SAI_META_LOG_WARN(\"failed to serialize '$name' at index %u\", (uint32_t)idx);";
            WriteSource "                return SAI_SERIALIZE_ERROR;";
            WriteSource "            }";
            WriteSource "            $buf += ret;";
            WriteSource "            $buf += sprintf($buf, \"\\\"\");" if $isstruct eq "" and not $numberList;
            WriteSource "            if (idx != $countName - 1)";
            WriteSource "               $buf += sprintf($buf, \",\");";
            WriteSource "        }";
            WriteSource "        $buf += sprintf($buf, \"]\");"; # end of array
            WriteSource "    }";
            next;
        }

        # primitives or types where serialization is defined explicitly
        # and it requires quotes (not numbers) and not structs/objects

        # primitives, numbers which don't require quotes

        if ($type =~ m/^(?:sai_)?(size|u?int\d+)_t$/)
        {
            $type = $1;
            $type =~ s/uint/u/;
            $type =~ s/int/s/;

            WriteSource "    $buf += sai_serialize_$type($buf, $name);";
            next;
        }

        LogError "not handled $type $name in $structName, FIXME";
        next;
    }

    WriteSource "    $buf += sprintf($buf, \"}\");"; # end of struct
    WriteSource "    return (int)($buf - start_$buf);";
    WriteSource "}";
}

sub CreateSerializeNotifications
{
    WriteSectionComment "Serialize notifications";

    for my $ntf (sort keys %main::NOTIFICATIONS)
    {
        CreateSerializeSingleNotification($ntf, $main::NOTIFICATIONS{$ntf});
    }
}

sub CreateSerializeStructs
{
    WriteSectionComment "Serialize structs";

    #
    # this method will auto generate serialization methods for all structs with
    # some exceptions like when struct contains unions and function pointers
    #

    for my $struct (sort keys %main::ALL_STRUCTS)
    {
        # TODO we could auto skip structs with unions and function pointers

        # user defined serialization
        next if $struct eq "sai_ip_address_t";
        next if $struct eq "sai_ip_prefix_t";
        next if $struct eq "sai_acl_action_data_t";
        next if $struct eq "sai_acl_field_data_t";
        next if $struct eq "sai_attribute_t";
        next if $struct eq "sai_tlv_t";
        next if $struct eq "sai_hmac_t";

        # TODO sai_acl_capability_t has enum list and it's only valid when 
        # similar for sai_tam_threshold_breach_event_t

        # non serializable
        next if $struct eq "sai_service_method_table_t";
        next if $struct eq "sai_object_key_t";
        next if $struct =~ /^sai_\w+_api_t$/;

        CreateSerializeSingleStruct($struct);
    }
}

#ProcessXmlFiles();
#
#    WriteSource "#include <sai.h>";
#    WriteSource "#include \"saimetadatatypes.h\"";
#    WriteSource "#include \"saimetadatautils.h\"";
#    WriteSource "#include \"saimetadatalogger.h\"";
#    WriteSource "#include \"saiserialize.h\"";
#    WriteSource "#include \"stdio.h\"";
#
#CreateSerializeNotifications();
#
#    WriteFile("saimetadata.c", $SOURCE_CONTENT);
#exit;
#my @structs = `ls xml/struct__sai_*|perl -npe 's/__/_/g;s/.+(sai_\\w+_t).xml/\\1/g'`;
#for my $s(@structs)
#{
#    chomp$s;
#    next if $s =~ /_api_t$/;
#
#    next if $s =~/saimetadatatypes/;
#
#    my %struct = ExtractStructInfo($s, "struct_");
#
#    next if $struct{ (keys %struct)[0] }->{file} =~ /saimetadatatypes/;
#
#
#    print "-- $s\n";
#    CreateSerializeMethodsForNonObjectId $s;
#}
#exit;
# for pointers we could add
#
# "@count" to description and point to struct member with that name and it will be treated as count for pointers
# when ther is no pointer, @count -- shoule be like hypens or -1
# for notifications, hmm how to detect count ? - in descripthom method we could add @count x y
# and 2 params names which to which should be
# and notifications we can treat like structs, we could actually declare struct for deserialize
# with those exact parameters as notification
#
# we could also generate deserialize_and call_notifation where notification strict would be passed
# and wight notification would be called and free itself but on exception there will be memory leak
#
# and force all structs with pointers should have @count attribute and count should be always uint32 bit
#
# count 1 if there is not param
#
# then we could generate serialize/deserialize and free methods for all
#
# TODO generate notifications metadata, if param is object type then objects must be specified
# for validation wheter notification returned valid object, and each struct that is using object_type
# should have @objects on object id members - a lot of work
#
# then we should generate all struct infos to get all functions for oid extraction etc

sub CreateSerializeMethods
{
    CreateSerializeForEnums();

    CreateSerializeMetaKey();

    CreateSerializeStructs();

    CreateSerializeNotifications();
}

BEGIN
{
    our @ISA    = qw(Exporter);
    our @EXPORT = qw/
    CreateSerializeMethods
    /;
}

1;
