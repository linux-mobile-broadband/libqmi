#!/usr/bin/env python
# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, version 2.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Copyright (C) 2011 Red Hat, Inc.
#

def constname(name):
    rlist = { '(': '',
              ')': '',
              ' ': '_',
              '/': '_',
              '.': '',
              ',': '' }
    # Capture stuff like "-9 dB" correctly
    if name.find("dB") == -1:
        rlist['-'] = '_'
    else:
        rlist['-'] = 'NEG_'

    sanitized = ""
    for c in name:
        try:
            sanitized += rlist[c]
        except KeyError:
            sanitized += c

    # Handle E_UTRA -> EUTRA and E_UTRAN -> EUTRAN
    foo = sanitized.upper().strip('_')
    foo.replace('E_UTRA', 'EUTRA')

    # And HSDPA+ -> HSDPA_PLUS then strip all plusses
    foo = foo.replace('HSDPA+', 'HSDPA_PLUS')
    foo = foo.replace('+', '')

    # 1xEV-DO -> 1x EVDO
    foo = foo.replace("1XEV_DO", "1X_EVDO")
    foo = foo.replace("EV_DO", "EVDO")

    # Wi-Fi -> WiFi
    foo = foo.replace("WI_FI", "WIFI")

    # modify certain words
    words = foo.split('_')
    klist = [ 'UNSUPPORTED' ]
    slist = [ 'STATES', 'REASONS', 'FORMATS', 'SELECTIONS', 'TYPES', 'TAGS',
              'PROTOCOLS', 'CLASSES', 'ACTIONS', 'VALUES', 'OPTIONS',
              'DOMAINS', 'DEVICES', 'MODES', 'MONTHS', 'PREFERENCES',
              'PREFS', 'DURATIONS', 'CAPABILITIES', 'INTERFACES',
              'TECHNOLOGIES', 'NETWORKS', 'DAYS', 'SYSTEMS', 'SCHEMES',
              'INDICATORS', 'ENCODINGS', 'INITIALS', 'BITS', 'MEDIUMS',
              'BASES', 'ERRORS', 'RESULTS', 'RATIOS', 'DELIVERIES',
              'FAMILIES', 'SETTINGS', 'SOURCES', 'ORDERS' ]
    blist = { 'CLASSES': 'CLASS' }
    final = ''
    for word in words:
        if word in klist or len(word) == 0:
            continue
        if len(final):
            final += '_'
        if word in blist:
            final += blist[word]
        elif word in slist:
            if word.endswith("IES"):
                final += word[:len(word) - 3] + "Y"
            elif word.endswith("S"):
                final += word[:len(word) - 1]
        else:
            final += word
    return final

def nicename(name):
    name = name.lower()
    name = name.replace("1xev-do", "1x evdo")
    name = name.replace("ev-do", "evdo")
    name = name.replace("wi-fi", "wifi")
    name = name.replace("%", "pct")
    name = name.replace(' ', '_').replace('/', '_').replace('-', '_').replace('.','').replace('(', '').replace(')', '')
    name = name.replace("___", "_").replace("__", "_")
    return name.strip('_')

def parse_entity_name(name):
        parts = name.split("/")
        return (parts[0].upper(), parts[1].upper().replace(" ", "_"), parts[2])

class Entity:
    def __init__(self, line):
        # Each entity defines a TLV item in a QMI request or response.  The
        # entity's 'struct' field maps to the ID of a struct in Struct.txt,
        # which describes the fields of that struct.  For example:
        #
        # 33^"32,16"^"WDS/Start Network Interface Request/Primary DNS"^50021^-1
        #
        # The first field (33) indicates that this entity is valid for the QMI
        # message eDB2_ET_QMI_WDS_REQ. The "32,16" is a tuple
        # (eQMI_WDS_START_NET, 16), the first item of which (32) indicates the
        # general QMI service the entity is associated with (WDS, CTL, NAS, etc)
        # and the second item indicates the specific entity number (ie, the 'T'
        # in TLV).
        #
        # The 'struct' field (50021) points to:
        #
        # Struct.txt:50021^0^0^50118^""^-1^1^"4"
        #
        # which indicates that the only member of this struct is field #50118:
        #
        # Field.txt:50118^"IP V4 Address"^8^0^2^0
        #
        # which should be pretty self-explanatory.  Note that different entities
        # can point to the same struct.


        parts = line.split('^')
        if len(parts) < 4:
            raise Exception("Invalid entity line '%s'" % line)
        self.uniqueid = parts[0] + '.' + parts[1].replace('"', '').replace(',', '.')
        self.type = int(parts[0])  # eDB2EntityType

        self.key = parts[1].replace('"','')  # tuple of (eQMIMessageXXX, entity number)
        self.cmdno = int(self.key.split(",")[0])
        self.tlvno = int(self.key.split(",")[1])
        self.name = parts[2].replace('"', '')
        self.struct = int(parts[3])
        self.format = None
        self.internal = True
        self.extformat = None
        if len(parts) > 4:
            self.format = int(parts[4])
        if len(parts) > 5:
            self.internal = int(parts[5]) != 0
        if len(parts) > 6:
            self.extformat = int(parts[6])

    def validate(self, structs):
        if not structs.has_child(self.struct):
            raise Exception("Entity missing struct: %d" % self.struct)

    def emit(self, fields, structs, enums):
        if self.tlvno == 2 and self.name.find("/Result Code") > 0 and self.struct == 50000:
            # ignore this entity if it's a standard QMI result code struct
            return self.struct

        # Tell the struct this value is for to emit itself
        s = structs.get_child(self.struct)
        s.emit(self.name, self.cmdno, self.tlvno, fields, structs, enums)
        return self.struct

class Entities:
    def __init__(self, path):
        self.byid = {}
        f = file(path + "Entity.txt")
        for line in f:
            ent = Entity(line)
            self.byid[ent.uniqueid] = ent

    def validate(self, structs):
        for e in self.byid.values():
            e.validate(structs)

    def emit(self, fields, structs, enums):
        # emit the standard status TLV struct
        print "struct qmi_result_code { /* QMI Result Code TLV (0x0002) */"
        print "\tgobi_qmi_results qmi_result; /* QMI Result */"
        print "\tgobi_qmi_errors qmi_error; /* QMI Error */"
        print "};"
        print ""

        structs_used = []
        for e in self.byid.values():
            sused = e.emit(fields, structs, enums)
            structs_used.append(sused)
        return structs_used


class Field:

    # eDB2FieldType
    FIELD_TYPE_STD           = 0 # Field is a standard type (see below)
    FIELD_TYPE_ENUM_UNSIGNED = 1 # Field is an unsigned enumerated type
    FIELD_TYPE_ENUM_SIGNED   = 2 # Field is a signed enumerated type

    # eDB2StdFieldType
    FIELD_STD_BOOL = 0         # boolean (0/1, false/true)
    FIELD_STD_INT8 = 1         # 8-bit signed integer
    FIELD_STD_UINT8 = 2        # 8-bit unsigned integer
    FIELD_STD_INT16 = 3        # 16-bit signed integer
    FIELD_STD_UINT16 = 4       # 16-bit unsigned integer
    FIELD_STD_INT32 = 5        # 32-bit signed integer
    FIELD_STD_UINT32 = 6       # 32-bit unsigned integer
    FIELD_STD_INT64 = 7        # 64-bit signed integer
    FIELD_STD_UINT64 = 8       # 64-bit unsigned integer
    FIELD_STD_STRING_A = 9     # ANSI (ASCII?) fixed length string
    FIELD_STD_STRING_U = 10    # UNICODE fixed length string
    FIELD_STD_STRING_ANT = 11  # ANSI (ASCII?) NULL terminated string
    FIELD_STD_STRING_UNT = 12  # UNICODE NULL terminated string
    FIELD_STD_FLOAT32 = 13     # 32-bit floating point value
    FIELD_STD_FLOAT64 = 14     # 64-bit floating point value
    FIELD_STD_STRING_U8 = 15   # UTF-8 encoded fixed length string
    FIELD_STD_STRING_U8NT = 16 # UTF-8 encoded NULL terminated string

    stdtypes = {
        # Maps field type to [ <C type>, <is array> ]
        FIELD_STD_BOOL: [ 'bool', False ],
        FIELD_STD_INT8: [ 'int8', False ],
        FIELD_STD_UINT8: [ 'uint8', False ],
        FIELD_STD_INT16: [ 'int16', False ],
        FIELD_STD_UINT16: [ 'uint16', False ],
        FIELD_STD_INT32: [ 'int32', False ],
        FIELD_STD_UINT32: [ 'uint32', False ],
        FIELD_STD_INT64: [ 'int64', False ],
        FIELD_STD_UINT64: [ 'uint64', False ],
        FIELD_STD_STRING_A: [ 'char', True ],
        FIELD_STD_STRING_U: [ 'uint16', True ],
        FIELD_STD_STRING_ANT: [ 'char *', False ],
        FIELD_STD_STRING_UNT: [ 'char *', False ],
        FIELD_STD_FLOAT32: [ 'float32', False ],
        FIELD_STD_FLOAT64: [ 'float64', False ],
    }

    def __init__(self, line):
        parts = line.split('^')
        if len(parts) < 6:
            raise Exception("Invalid field line '%s'" % line)
        self.id = int(parts[0])
        self.name = parts[1].replace('"', '')
        self.size = int(parts[2])
        self.type = int(parts[3]) # eDB2FieldType
        self.typeval = int(parts[4])  # eDB2StdFieldType if 'type' == 0
        self.hex = int(parts[5]) != 0
        self.descid = None
        self.internal = False
        if len(parts) > 6:
            self.descid = int(parts[6])
        if len(parts) > 7:
            self.internal = int(parts[7])

        # Field.txt:50118^"IP V4 Address"^8^0^2^0

    def emit(self, enums, newsize, comment, isarray):
        realtype = ''
        arraybits = ''

        if self.type == Field.FIELD_TYPE_STD:  # eDB2_FIELD_STD
            tinfo = Field.stdtypes[self.typeval]
            realtype = tinfo[0]

            arraysize = 0
            if tinfo[1] or newsize > 0:
                if newsize > 0:
                    arraysize = newsize
                else:
                    arraysize = self.size

                # Unicode strings are two-byte characters
                if self.typeval == Field.FIELD_STD_STRING_U:
                    arraysize = arraysize * 2

                # Convert back to bytes
                arraybits = "[%d]" % (arraysize / 8)
        elif self.type == Field.FIELD_TYPE_ENUM_UNSIGNED or self.type == Field.FIELD_TYPE_ENUM_SIGNED:
            # It's a enum; find the enum
            e = enums.get_child(self.typeval)
            realtype = "gobi_%s" % nicename(e.name)
            if isarray:
                if newsize != 0:
                    raise Exception("Unhandled ENUM field type with size %d" % newsize)
                arraybits = "[0]";
        else:
            raise ValueError("Unknown Field type")

        if comment:
            comment = " (%s)" % comment
        print "\t%s %s%s; /* %s%s */" % (realtype, nicename(self.name), arraybits, self.name, comment)


class Fields:
    def __init__(self, path):
        self.byid = {}

        f = file(path + "Field.txt")
        for line in f:
            field = Field(line.strip())
            self.byid[field.id] = field

    def has_child(self, fid):
        return self.byid.has_key(fid)

    def get_child(self, fid):
        return self.byid[fid]


class StructFragment:
    # eDB2FragmentType
    TYPE_FIELD              = 0 # Simple field fragment
    TYPE_STRUCT             = 1 # Structure fragment
    TYPE_CONSTANT_PAD       = 2 # Pad fragment, fixed length (bits)
    TYPE_VARIABLE_PAD_BITS  = 3 # Pad fragment, variable (bits)
    TYPE_VARIABLE_PAD_BYTES = 4 # Pad fragment, variable (bytes)
    TYPE_FULL_BYTE_PAD      = 5 # Pad fragment, pad to a full byte
    TYPE_MSB_2_LSB          = 6 # Switch to MSB -> LSB order
    TYPE_LSB_2_MSB          = 7 # Switch to LSB -> MSB order

    # eDB2ModifierType
    MOD_NONE             = 0  # Modifier is not used
    MOD_CONSTANT_ARRAY   = 1  # Constant (elements) array
    MOD_VARIABLE_ARRAY   = 2  # Variable (elements) array (modval gives #elements field)
    MOD_OBSOLETE_3       = 3  # Constant (bits) array [OBS]
    MOD_OBSOLETE_4       = 4  # Variable (bits) array [OBS]
    MOD_OPTIONAL         = 5  # Fragment is optional
    MOD_VARIABLE_ARRAY2  = 6  # Variable (elements) array, start/stop given
    MOD_VARIABLE_ARRAY3  = 7  # Variable (elements) array, simple expression
    MOD_VARIABLE_STRING1 = 8  # Variable length string (bit length)
    MOD_VARIABLE_STRING2 = 9  # Variable length string (byte length)
    MOD_VARIABLE_STRING3 = 10 # Variable length string (character length)

    # Struct fragments (ie, each line in Struct.txt describe each member of
    # a struct.  The format is as follows:
    #
    # id^order^type^fieldid^name^offset^modtype^modval
    #
    # 50409^1^0^54024^""^-1^2^"54023"
    #
    # This describes the second element (ie, element #1) of struct 50409,
    # which is of type FIELD (0) and the value is described by Field #54024.
    # Additionally, it's "modtype" is VARIABLE_ARRAY which is described by
    # Field #54023.  Basically, this fragment is a variable array of
    # "Medium Preference" values, each element of which is one of the
    # "QMI PDS Mediums" (Enum.txt, #50407 given by Field.txt #54024).  The
    # number of elements in the array is given by the "modtype" and "modval"
    # pointers, which say that Field #54023 (which is also the first member
    # of this struct) specifies the number of elements in this variable array.

    def __init__(self, line):
        parts = line.split('^')
        if len(parts) < 8:
            raise Exception("Invalid struct fragment '%s'" % line)
        self.id = int(parts[0])
        self.order = int(parts[1])
        self.type = int(parts[2])   # eDB2FragmentType
        self.value = int(parts[3])  # id of field in Fields.txt
        self.name = parts[4].replace('"', '')
        self.offset = int(parts[5])
        self.modtype = int(parts[6])  # eDB2ModifierType
        self.modval = parts[7].replace('"', '')

        self.structsize = 0
        if self.type == StructFragment.TYPE_CONSTANT_PAD:
            self.structsize = self.value

    def validate(self, fields, structs):
        if self.type == StructFragment.TYPE_FIELD:
            if not fields.has_child(self.value):
                raise Exception("No field %d" % self.value)
        elif self.type == StructFragment.TYPE_STRUCT:
            if not structs.has_child(self.value):
                raise Exception("No struct %d" % self.value)
        elif self.type == StructFragment.TYPE_CONSTANT_PAD:
            # specifies the total size of the struct; if there's not
            # enough data for the given size the struct should be padded
            # out to that size
            if self.value < 0 or self.value > 1000:
                raise Exception("Invalid constant pad size %d" % self.value)
        elif self.type == StructFragment.TYPE_MSB_2_LSB:
            pass
        else:
            raise Exception("Surprising struct %d:%d field: %d" % (self.id, self.order, self.type))

    def emit(self, name, fields, structs, enums):
        if self.type == StructFragment.TYPE_FIELD:
            f = fields.get_child(self.value)

            # modify the field like cProtocolEntityNav::ProcessFragment(); if
            # the struct fragment is a "string" type and has a modval that
            # points to another Field.txt entry, that entry defines the string
            # length
            newsize = 0
            comment = ""
            isarray = False
            if self.modtype == StructFragment.MOD_VARIABLE_STRING1 or \
                    self.modtype == StructFragment.MOD_VARIABLE_STRING2 or \
                    self.modtype == StructFragment.MOD_VARIABLE_STRING3:
                flen = fields.get_child(int(self.modval))
                newsize = flen.size
                if self.modtype == StructFragment.MOD_VARIABLE_STRING2 or \
                        self.modtype == StructFragment.MOD_VARIABLE_STRING3:
                    newsize = newsize * 8  # Convert to size-in-bits
            elif self.modtype == StructFragment.MOD_CONSTANT_ARRAY:
                # size in bytes is modval
                newsize = int(self.modval) * 8
            elif self.modtype == StructFragment.MOD_VARIABLE_ARRAY:
                # The "modval" is the field id that gives the # of elements
                # of this variable array.  That field is usually the immediately
                # previous fragment of this struct.
                fdesc = fields.get_child(int(self.modval))
                comment = "size given by %s" % nicename(fdesc.name)
                isarray = True

            f.emit(enums, newsize, comment, isarray)
        elif self.type == StructFragment.TYPE_STRUCT:
            s = structs.get_child(self.value)
            print "\t%s %s;\t" % (nicename(name), nicename(self.name))

    def struct_size(self):
        return self.structsize


class Struct:
    def __init__(self, sid):
        self.id = sid
        self.fragments = []
        self.name = "struct_%d" % sid
        self.structsize = 0

    def add_fragment(self, fragment):
        for f in self.fragments:
            if fragment.order == f.order:
                raise Exception("Struct %d already has fragment order %d" % (self.id, fragment.order))
        self.fragments.append(fragment)
        self.fragments.sort(lambda x, y: cmp(x.order, y.order))

        s = fragment.struct_size()
#        print "id %d, s %d, size %d" % (self.id, s, self.structsize)
#        if s > 0:
#            if self.structsize != 0:
#                raise Exception("Already found a CONSTANT_PAD fragment")
#            self.structsize = s

    def validate(self, fields, structs):
        for f in self.fragments:
            f.validate(fields, structs)

    def emit(self, name, cmdno, tlvno, fields, structs, enums):
        if not name:
            name = self.name
            svcname = "Unknown"
            cmdname = "Unknown"
            tlvname = "Unknown"
        else:
            (svcname, cmdname, tlvname) = parse_entity_name(name)
        print '/**'
        print ' * SVC: %s' % svcname
        print ' * CMD: 0x%04x (%s)' % (cmdno, cmdname)
        print ' * TLV: 0x%02x   (%s)' % (tlvno, tlvname)
        print ' */'
        print 'struct %s {' % nicename(name)
        for f in self.fragments:
            f.emit(name, fields, structs, enums)
        print "};\n"

class Structs:
    def __init__(self, path):
        self.structs = {}
        f = file(path + "Struct.txt")
        for line in f:
            if len(line.strip()) == 0:
                continue
            frag = StructFragment(line.strip())
            if not self.structs.has_key(frag.id):
                struct = Struct(frag.id)
                self.structs[struct.id] = struct
            struct.add_fragment(frag)
        f.close()

    def validate(self, fields):
        for s in self.structs.values():
            s.validate(fields, self)

    def has_child(self, sid):
        return self.structs.has_key(sid)

    def get_child(self, sid):
        return self.structs[sid]

    def emit_unused(self, used, fields, enums):
        print '/**** UNKNOWN TLVs ****/\n'
        for s in self.structs.values():
            if not s.id in used:
                s.emit(None, 0, 0, fields, self, enums)

class EnumEntry:
    def __init__(self, line):
        parts = line.split('^')
        if len(parts) < 3:
            raise Exception("Invalid enum entry line '%s'" % line)
        self.id = int(parts[0])
        self.value = int(parts[1], 0)
        self.name = parts[2].replace('"', '')
        self.descid = None
        if len(parts) > 3:
            self.descid = int(parts[3])

    def emit(self, enum_name):
        print "\tGOBI_%s_%s\t\t= 0x%08x,     /* %s */" % (
                    constname(enum_name),
                    constname(self.name),
                    self.value, self.name)

class Enum:
    def __init__(self, line):
        parts = line.split('^')
        if len(parts) < 4:
            raise Exception("Invalid enum line '%s'" % line)
        self.id = int(parts[0])
        self.name = parts[1].replace('"', '')
        self.descid = int(parts[2])
        self.internal = int(parts[3]) != 0
        self.values = []   # list of EnumEntry objects

    def add_entry(self, entry):
        for v in self.values:
            if entry.value == v.value:
                raise Exception("Enum %d already has value %d" % (self.id, v.value))
        self.values.append(entry)
        self.values.sort(lambda x, y: cmp(x.value, y.value))

    def emit(self):
        print 'typedef enum { /* %s */ ' % self.name
        for en in self.values:
            en.emit(self.name)
        print "} gobi_%s;\n" % nicename(self.name)
            

class Enums:
    def __init__(self, path):
        self.enums = {}

        # parse the enums
        f = file(path + "Enum.txt")
        for line in f:
            try:
                enum = Enum(line.strip())
                self.enums[enum.id] = enum
            except Exception(e):
                pass
        f.close()

        # and now the enum entries
        f = file(path + "EnumEntry.txt")
        for line in f:
            try:
                entry = EnumEntry(line.strip())
                self.enums[entry.id].add_entry(entry)
            except Exception(e):
                pass
        f.close()

    def emit(self):
        for e in self.enums:
            self.enums[e].emit()

    def get_child(self, eid):
        return self.enums[eid]


import sys

if len(sys.argv) > 2:
    print "Usage: qmidb.py <path to Entity.txt>"
    sys.exit(1)
path = ""
if len(sys.argv) == 2:
    path = sys.argv[1] + "/"

enums = Enums(path)
entities = Entities(path)
fields = Fields(path)
structs = Structs(path)

structs.validate(fields)
entities.validate(structs)

print '/* GENERATED CODE. DO NOT EDIT. */'
print '\ntypedef uint8 bool;\n'
enums.emit()

print '\n\n'

structs_used = entities.emit(fields, structs, enums)

# emit structs that weren't associated with an entity
structs.emit_unused(structs_used, fields, enums)
