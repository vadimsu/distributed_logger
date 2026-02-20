#!/usr/bin/python3
import json
import sys
import code_gen
import os


class ParseException(Exception):
    def __init__(self, name, msg):
        self.__prefix = name
        self.__msg = msg

    def __str__(self):
        return self.__prefix + ": " + self.__msg


class ReturnTypeMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__, msg)


class TypeNameMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__, msg)


class SegmentsMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__, msg)


class InvalidSegmentNumber(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__, msg)


class ParametersMissing(ParseException):
    def __init__(self, msg=""):
        super().__init__(__class__.__name__, msg)


def get_value_from_segment(segment):
    if 'name' not in segment:
        raise
    return segment['name']


def get_return_type(f):
    if 'return_type' not in f:
        raise ReturnTypeMissing(str(f))
    rt = f['return_type']
    if 'typename' not in rt:
        raise TypeNameMissing(str(rt))
    tn = rt['typename']
    if 'segments' not in tn:
        raise SegmentsMissing(str(tn))
    sgmnts = tn['segments']
    if len(sgmnts) != 1:
        raise InvalidSegmentNumber(str(len(sgmnts)))
    if 'name' not in sgmnts[0]:
        raise TypeNameMissing("in segment "+str(sgmnts[0]))
    return sgmnts[0]['name']


def get_name(f):
    if 'name' not in f:
        raise TypeNameMissing(str(f))
    n = f['name']
    if 'segments' not in n:
        raise SegmentsMissing(str(n))
    sgmnts = n['segments']
    if len(sgmnts) != 1:
        raise InvalidSegmentNumber(str(len(sgmnts)))
    return get_value_from_segment(sgmnts[0])


def get_parameter(p):
    if 'type' not in p:
        raise TypeNameMissing(str(p))
    t = p['type']
    if 'typename' not in t:
        raise TypeNameMissing(str(t))
    tn = t['typename']
    if 'segments' not in tn:
        raise SegmentsMissing(str(tn))
    sgmnts = tn['segments']
    if len(sgmnts) == 0:
        raise InvalidSegmentNumber(str(len(sgmnts)))
    return (p['name'], get_value_from_segment(sgmnts[0]))


def get_parameters(f):
    if 'parameters' not in f:
        raise ParametersMissing(str(f))
    ps = f['parameters']
    params = []
    for p in ps:
        params.append(get_parameter(p))
    return params


print("parsing ", sys.argv[1])
project_root = "../"
if len(sys.argv) > 2:
    project_root = sys.argv[2]
print("project root is at ",project_root)

with open(sys.argv[1]) as fp:
    code = json.load(fp)
    functions = code['namespace']['functions']
    functs = []
    for f in functions:
        try:
            functs.append({"return": get_return_type(f),
                          "name": get_name(f), "params": get_parameters(f)})
        except Exception as exc:
            print(exc)
    cppCodeGen = code_gen.CppCodeGen(functs)()
    goCodeGen = code_gen.GoCodeGen(functs)()
    print(cppCodeGen.get_storage_enum_code())
    print(cppCodeGen.get_declarations_code())
    print(goCodeGen.get_storage_enum_code())
    print(goCodeGen.get_storage_declarations_code())
    print(goCodeGen.get_storage_definitions_code())
#    print(goCodeGen.get_storage_structures_code())
    print(goCodeGen.get_decoder_code())
    os.makedirs(project_root + "generated", exist_ok=True)
    os.makedirs(project_root + "generated/server", exist_ok=True)
    os.makedirs(project_root + "generated/server/storage", exist_ok=True)
    os.makedirs(project_root + "generated/server/storage/mongo", exist_ok=True)
    os.makedirs(project_root + "generated/server/storage/clickhouse", exist_ok=True)
    os.makedirs(project_root + "generated/server/event_decoder", exist_ok=True)
    os.makedirs(project_root + "generated/client", exist_ok=True)
    with open(project_root + "generated/server/storage/storage.go", "w") as storage_fp:
        storage_fp.write(goCodeGen.get_storage_enum_code())
        storage_fp.write(goCodeGen.get_storage_structures_code())
        storage_fp.write(goCodeGen.get_storage_declarations_code())
    with open(project_root + "generated/server/storage/mongo/mongo.go", "w") as storage_fp:
        with open(project_root + "tools/codegen/templates/mongo_init.go") as mongo_init_fp:
            mongo_init = mongo_init_fp.read()
            storage_fp.write(mongo_init +
                             goCodeGen.get_storage_definitions_code())

    # Also generate ClickHouse storage file using a clickhouse init template
    with open(project_root + "generated/server/storage/clickhouse/clickhouse.go", "w") as storage_fp:
        with open(project_root + "tools/codegen/templates/clickhouse_init.go") as ch_init_fp:
            ch_init = ch_init_fp.read()
            storage_fp.write(ch_init + goCodeGen.get_clickhouse_definitions_code())
    with open(project_root + "generated/client/distributed_logger_api_int.hh", "w") \
            as storage_fp:
        storage_fp.write(cppCodeGen.get_storage_enum_code())
        storage_fp.write(cppCodeGen.get_declarations_code())
    with open(project_root + "generated/server/event_decoder/event_decoder.go", "w") as storage_fp:
        storage_fp.write(goCodeGen.get_decoder_code())
#    print(str(cppCodeGen()))
