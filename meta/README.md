[![Build Status](https://sonic-jenkins.westus.cloudapp.azure.com/buildStatus/icon?job=sai-meta-build)](https://sonic-jenkins.westus.cloudapp.azure.com/job/sai-meta-build)

Metadata for SAI
================

Metadata for SAI is a set of auto generated (based on SAI headers) data
and functions which allow SAI attributes validation and more.

Metadata is generated as ANSI C source and header.

Parser also forces headers to be well formated when adding new code.

To test your changes just type:

```sh
make
```

## SAI Objects Dependency Graph

<img widtd="100%" src="https://cdn.rawgit.com/kcudnik/SAI/svg/meta/saidepgraph.svg" alt="Dependency Graph" />

## Metadat restrictions

### Enum general restrictions

* all enum values must be increasing and non negative (except sai_status_t)
* First enum value must be zero except when enum is marked as flags (sai_enummetadta_t.containsflags)
* all enum values must be less than 0x10000
*

### Other restrictions

* SAI_STATUS_SUCCESS must be equal to zero
* object types must be between SAI_OBJECT_TYPE_NULL and SAI_OBJECT_TYPE_MAX

### Attribute restrictions

* possible attribute flags are:
  - SAI_ATTR_FLAGS_MANDATORY_ON_CREATE | SAI_ATTR_FLAGS_CREATE_ONLY | SAI_ATTR_FLAGS_KEY
  - SAI_ATTR_FLAGS_MANDATORY_ON_CREATE | SAI_ATTR_FLAGS_CREATE_ONLY
  - SAI_ATTR_FLAGS_MANDATORY_ON_CREATE | SAI_ATTR_FLAGS_CREATE_AND_SET
  - SAI_ATTR_FLAGS_CREATE_ONLY
  - SAI_ATTR_FLAGS_CREATE_AND_SET
  - SAI_ATTR_FLAGS_READ_ONLY
* validonly attribute can't be marked as SAI_ATTR_FLAGS_MANDATORY_ON_CREATE
* SAI_ATTR_FLAGS_MANDATORY_ON_CREATE can't have default value
* SAI_ATTR_FLAGS_CREATE_ONLY and SAI_ATTR_FLAGS_CREATE_AND_SET attributest must have defautl value specified (except acl entry/acl field, default value is considered "disabled")
* SAI_ATTR_FLAGS_READ_ONLY can't be conditional and can't be validonly
* SAI_ATTR_FLAGS_READ_ONLY can have default value but only marked as "internal" and only on OID and OID list
* on OID attribute when default falue is expected and it's const, it must be SAI_NULL_OBJECT_ID
* OID attribute must provide object list which are allowed to be used on that attribute
* switch object type can be only used in non obejct id structs
* fdb flush and hostif packet are not real objects, they are used only for serialization
