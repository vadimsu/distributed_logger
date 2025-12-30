import pytest

from code_gen import GoCodeGen, CppCodeGen


def sample_funcs():
    return [
        {
            'name': 'Store',
            'return': 'error',
            'params': [('eventid', 'uint64_t'), ('id', 'uint64_t'), ('name', 'string')],
        }
    ]


def test_go_codegen_generates_snippets():
    funcs = sample_funcs()
    g = GoCodeGen(funcs)
    g.generate_code()

    enum_code = g.get_storage_enum_code()
    decoder_code = g.get_decoder_code()
    decls = g.get_storage_declarations_code()

    assert 'package storage' in enum_code
    assert 'eventid' in enum_code or 'Eventid' in enum_code
    assert 'func decode_eventid' in decoder_code
    assert 'type StorageAPI' in decls


def test_cpp_codegen_generates_snippets():
    funcs = sample_funcs()
    c = CppCodeGen(funcs)
    c.generate_code()

    decls = c.get_declarations_code()
    enum_code = c.get_storage_enum_code()

    assert 'inline' in decls
    assert 'Events' in enum_code


# ---------------------- Additional tests ----------------------

def test_go_codegen_skips_short_params():
    # functions with fewer than two params should be ignored for Go codegen
    funcs = [
        {'name': 'Short', 'return': 'error', 'params': [('only', 'uint64_t')]}
    ]
    g = GoCodeGen(funcs)
    g.generate_code()

    assert 'func decode_only' not in g.get_decoder_code()
    assert 'Only' not in g.get_storage_enum_code()


def test_go_codegen_multiple_events():
    funcs = [
        {'name': 'Store', 'return': 'error', 'params': [('evt1', 'uint64_t'), ('id', 'uint64_t'), ('data', 'string')]},
        {'name': 'Save', 'return': 'error', 'params': [('evt2', 'uint64_t'), ('x', 'uint64_t'), ('y', 'uint64_t')]},
    ]
    g = GoCodeGen(funcs)
    g.generate_code()

    enum_code = g.get_storage_enum_code()
    decoder_code = g.get_decoder_code()

    assert '\tEvt1' in enum_code
    assert '\tEvt2' in enum_code
    assert 'decode_evt1' in decoder_code
    assert 'decode_evt2' in decoder_code
    assert 'case storage.Evt1' in decoder_code
    assert 'case storage.Evt2' in decoder_code


def test_go_codegen_param_type_conversion_and_decode_calls():
    g = GoCodeGen(sample_funcs())
    g.generate_code()

    # uint64_t should be converted to uint64 in storage structures
    structs = g.get_storage_structures_code()
    assert 'Id uint64' in structs or 'id uint64' in structs

    # Decoder should call DecodeUint64 for numeric fields and DecodeString for strings
    decoder = g.get_decoder_code()
    assert 'DecodeUint64' in decoder
    assert 'DecodeString' in decoder


def test_go_codegen_empty_funcs_returns_defaults():
    g = GoCodeGen([])
    g.generate_code()

    # Should still produce valid skeletons but no cases
    assert 'package storage' in g.get_storage_enum_code()
    assert 'case storage.' not in g.get_decoder_code()
    assert 'unknown event' in g.get_decoder_code()


def test_cpp_codegen_string_and_uint64_behavior():
    funcs = [
        {'name': 'Emit', 'return': 'void', 'params': [('evt', 'uint64_t'), ('name', 'string')]}
    ]
    c = CppCodeGen(funcs)
    c.generate_code()

    decls = c.get_declarations_code()
    enum_code = c.get_storage_enum_code()

    # Check that string params are prefixed with std:: and contribute to length calculation
    assert 'std::string name' in decls or 'std::string name' in decls
    assert '.size()' in decls
    assert 'evt' in enum_code


def test_codegen_call_returns_self_and_runs_generate():
    funcs = sample_funcs()
    g = GoCodeGen(funcs)
    returned = g()
    assert returned is g
    # After calling, code should be generated
    assert g.get_decoder_code()


def test_nonstandard_param_types_passthrough():
    funcs = [
        {'name': 'X', 'return': 'void', 'params': [('evt', 'uint64_t'), ('flag', 'bool'), ('count', 'int')]} 
    ]
    g = GoCodeGen(funcs)
    g.generate_code()

    # bool and int should be present as-is in declarations
    decls = g.get_storage_declarations_code()
    assert 'bool' in decls
    assert 'int' in decls


def test_enum_iota_behavior_with_skips():
    # If the first valid function is not the first in original list, the iota should be on the first emitted
    funcs = [
        {'name': 'Nope', 'return': 'void', 'params': [('only', 'uint64_t')]},
        {'name': 'FirstValid', 'return': 'void', 'params': [('evt', 'uint64_t'), ('x', 'uint64_t')]},
        {'name': 'SecondValid', 'return': 'void', 'params': [('evt2', 'uint64_t'), ('y', 'uint64_t')]},
    ]
    g = GoCodeGen(funcs)
    g.generate_code()

    enum_code = g.get_storage_enum_code()
    # the first listed valid event should have = iota
    assert '= iota' in enum_code
    assert 'Evt' in enum_code
    assert 'Evt2' in enum_code
