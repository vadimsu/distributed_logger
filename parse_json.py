#!/usr/bin/python3
import json
import sys
import code_gen

class ParseException(Exception):
    def __init__(self, name, msg):
        self.__prefix = name
        self.__msg = msg
    def __str__(self):
        return self.__prefix + ": " + self.__msg

class ReturnTypeMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__,msg)

class TypeNameMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__,msg)

class SegmentsMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__,msg)

class InvalidSegmentNumber(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__,msg)

class ParametersMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__,msg)

def get_value_from_segment(segment):
    if not 'name' in segment:
        raise
    return segment['name']

def get_return_type(f):
    if not 'return_type' in f:
        raise ReturnTypeMissing(str(f))
    rt = f['return_type']
    if not 'typename' in rt:
        raise TypeNameMissing(str(rt))
    tn = rt['typename']
    if not 'segments' in tn:
        raise SegmentsMissing(str(tn))
    sgmnts = tn['segments']
    if len(sgmnts) != 1:
        raise InvalidSegmentNumber(str(len(sgmnts)))
    if not 'name' in sgmnts[0]:
        raise TypeNameMissing("in segment "+str(sgmnts[0]))
    return sgmnts[0]['name']

def get_name(f):
    if not 'name' in f:
        raise TypeNameMissing(str(f))
    n = f['name']
    if not 'segments' in n:
        raise SegmentsMissing(str(n))
    sgmnts = n['segments']
    if len(sgmnts) != 1:
        raise InvalidSegmentNumber(str(len(sgmnts)))
    return get_value_from_segment(sgmnts[0])

def get_parameter(p):
    if not 'type' in p:
        raise TypeNameMissing(str(p))
    t = p['type']
    if not 'typename' in t:
        raise TypeNameMissing(str(t))
    tn = t['typename']
    if not 'segments' in tn:
        raise SegmentsMissing(str(tn))
    sgmnts = tn['segments']
    if len(sgmnts) == 0:
        raise InvalidSegmentNumber(str(len(sgmnts)))
    return (p['name'], get_value_from_segment(sgmnts[0]))

def get_parameters(f):
    if not 'parameters' in f:
        raise ParametersMissing(str(f))
    ps = f['parameters']
    params = []
    for p in ps:
        params.append(get_parameter(p))
    return params

print("parsing ",sys.argv[1])
with open(sys.argv[1]) as fp:
    code = json.load(fp)
    functions = code['namespace']['functions']
    functs = []
    for f in functions:
        try:
            functs.append({"return":get_return_type(f), "name":get_name(f), "params":get_parameters(f)})
        except Exception as exc:
            print(exc)
    cppCodeGen = code_gen.CppCodeGen(functs)()
    goCodeGen = code_gen.GoCodeGen(functs)()
    with open("LogAPIs.hh","w") as hfp:
        hfp.write(cppCodeGen.get_declarations_code())
    print(cppCodeGen.get_storage_enum_code())
    print(cppCodeGen.get_declarations_code())
    print(goCodeGen.get_storage_enum_code())
    print(goCodeGen.get_storage_declarations_code())
    print(goCodeGen.get_storage_definitions_code())
    print(goCodeGen.get_storage_structures_code())
    print(goCodeGen.get_decoder_code())
#    print(str(cppCodeGen()))
