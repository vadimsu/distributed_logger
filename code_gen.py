#!/usr/bin/env python3
"""Small code generators for Go and C++ used by the parser demo.

This module contains two concrete CodeGen implementations that convert a
function-description dictionary into generated source code strings.
"""

from typing import Iterable, Dict, Any, List, Sequence

class CodeGen:
    def __init__(self, func_dict: Iterable[Dict[str, Any]]) -> None:
        self._func_dict = list(func_dict)

    def __call__(self) -> "CodeGen":
        self.generate_code()
        return self

    def generate_code(self) -> None:
        raise NotImplementedError


class GoCodeGen(CodeGen):
    def __init__(self, func_dict):
        super().__init__(func_dict)
        self._declarations_code = ""

    @staticmethod
    def get_event_name(param):
        """Return the event name from a parameter pair (name, type)."""
        return param[0]

    def generate_code(self):
        # Consider only functions that have at least two params (first is event id)
        valid_funcs = [f for f in self._func_dict if len(f.get('params', [])) >= 2]

        self._storage_enum_code = "package storage\n\nconst (\n"
        self._decoder_code = "package decoder\n\n"
        self._decoder_code += 'import (\n\t"distributedlogger.com/storage"\n\t"errors"\n)\n\n'
        self._storage_definitions_code = ""
        self._storage_structures_code = ""
        self._storage_declarations_code = "type StorageAPI interface{\n"
        for idx, f in enumerate(valid_funcs):
            if len(f['params']) < 2:
                continue
            event_name = GoCodeGen.get_event_name(f['params'][0])
            decoder_func = "func decode_" + event_name + \
                "(packet []byte, s storage.StorageAPI)(error) {\n"
            decoder_func = decoder_func + "\tvar decoded int = 0\n"
            decoder_func = decoder_func + "\tvar decodedThisTime int = 0\n"
            funct = '\t' + f['name'] + "_" + event_name + "("
            storage_struct = "type " + event_name[0].upper() + \
                event_name[1:] + "_struct struct {\n"
            self._storage_definitions_code = self._storage_definitions_code + \
                "func (s *MongoStorage) " + funct
            entry = '\t' + event_name[0].upper() + event_name[1:]
            if idx == 0:
                entry += " = iota"
            self._storage_enum_code = self._storage_enum_code + entry + '\n'
            if idx == len(self._func_dict) - 1:
                self._storage_enum_code = self._storage_enum_code + ")\n"
            params = f['params']
            param_idx = 0
            funct_body = "\tfields := []interface{}{\n" + '\t\t' + \
                "storage." + event_name[0].upper() + event_name[1:] + \
                "_struct{\n"
            params_array = []
            for p in params:
                param_type = p[1]
                if param_type == "uint64_t":
                    param_type = "uint64"
                funct = funct + p[0] + " " + param_type
                self._storage_definitions_code = \
                    self._storage_definitions_code + p[0] + " " + param_type
                storage_struct = storage_struct + '\t' + p[0][0].upper() + \
                    p[0][1:] + " " + param_type + '\n'
                funct_body = funct_body + '\t\t\t' + p[0][0].upper() + \
                    p[0][1:] + ":" + p[0]
                param_idx = param_idx + 1
                if param_idx < len(params):
                    funct = funct + ", "
                    self._storage_definitions_code = \
                        self._storage_definitions_code + ","
                funct_body = funct_body + ",\n"
                decoder_func = decoder_func + "\tvar " + p[0] + \
                    " " + param_type + '\n'
                if param_idx > 1:
                    if p[1] == "string":
                        decoder_func = decoder_func + \
                            '\t' + p[0] + \
                            ", decodedThisTime,_ "\
                            "= DecodeString(packet[decoded:],false)" + '\n'
                    else:
                        decoder_func = decoder_func + \
                            '\t' + p[0] + \
                            ", decodedThisTime,_ "\
                            "= DecodeUint64(packet[decoded:])" + '\n'
                    decoder_func = decoder_func + \
                        "\tdecoded = decoded + decodedThisTime\n"
                else:
                    decoder_func = decoder_func + \
                        "\t" + p[0] + \
                        " = storage." + p[0][0].upper() + p[0][1:] + "\n"

            decoder_func = decoder_func + \
                '\t' + "return s." + f['name'] + "_" + event_name + "("
            param_idx = 0
            for p in params:
                param_idx = param_idx + 1
                decoder_func = decoder_func + p[0]
                if param_idx < len(params):
                    decoder_func = decoder_func + ","
                else:
                    decoder_func = decoder_func + ")"
            decoder_func = decoder_func + "\n}"
            self._decoder_code = self._decoder_code + decoder_func + '\n'
            funct = funct + ") (error)"
            if idx < len(valid_funcs) - 1:
                funct = funct + '\n'
            funct_body = funct_body + "\t\t},\n\t}\n"
            funct_body = funct_body + '\t' + \
                "_, err := s.collection.InsertMany(context.TODO(), fields)\n"
            funct_body = funct_body + "\treturn err"
            self._storage_definitions_code = \
                self._storage_definitions_code + ") (error){\n"
            self._storage_definitions_code = \
                self._storage_definitions_code + funct_body
            self._storage_definitions_code = \
                self._storage_definitions_code + "\n}\n"
            storage_struct = storage_struct + "}\n"
            self._storage_structures_code = \
                self._storage_structures_code + storage_struct
            self._storage_declarations_code = \
                self._storage_declarations_code + funct
            if idx == len(valid_funcs) - 1:
                self._storage_declarations_code = \
                    self._storage_declarations_code + '\n'
        self._storage_declarations_code = self._storage_declarations_code + \
            "}\n"
        dispatch_funct = "func Event_dispatch(event uint64, "\
            "packet []byte, s storage.StorageAPI)(error){\n"
        dispatch_funct = dispatch_funct + "\tswitch event{\n"
        for f in valid_funcs:
            event_name = GoCodeGen.get_event_name(f['params'][0])
            dispatch_funct = dispatch_funct + \
                "\tcase storage." + event_name[0].upper() + \
                event_name[1:] + ":\n"
            dispatch_funct = dispatch_funct + \
                "\t\treturn decode_" + \
                GoCodeGen.get_event_name(f['params'][0]) + "(packet, s)\n"
        dispatch_funct = dispatch_funct + "\t}\n"
        dispatch_funct = dispatch_funct + \
            "\treturn errors.New(\"unknown event\")\n}"
        self._decoder_code = self._decoder_code + dispatch_funct

    def get_storage_declarations_code(self):
        return self._storage_declarations_code

    def get_storage_definitions_code(self):
        return self._storage_definitions_code

    def get_storage_structures_code(self):
        return self._storage_structures_code

    def get_storage_enum_code(self):
        return self._storage_enum_code

    def get_decoder_code(self):
        return self._decoder_code


class CppCodeGen(CodeGen):
    def __init__(self, func_dict):
        super().__init__(func_dict)
        self._code = ""

    def generate_code(self) -> None:
        lines: List[str] = []
        enum_lines: List[str] = ["typedef enum {"]

        valid_funcs = [f for f in self._func_dict if len(f.get("params", [])) >= 1]

        for idx, f in enumerate(valid_funcs):
            event_name = GoCodeGen.get_event_name(f["params"][0])
            if idx != len(valid_funcs) - 1:
                enum_lines.append(f"\t{event_name},")
            else:
                enum_lines.append(f"\t{event_name}")

            # Function signature
            ret = f.get("return", "void")
            name = f['name'][0].lower() + f['name'][1:]
            params_parts: List[str] = []
            total_length = ["\tint total_length = 4;"]

            for p in f.get("params", []):
                p_name, p_type = p[0], p[1]
                prefix = "std::" if p_type == "string" else ""
                params_parts.append(f"{prefix}{p_type} {p_name}")
                if p_type == "string":
                    total_length.append(f"\ttotal_length += {p_name}.size() + 4;")
                elif p_type == "uint64_t":
                    total_length.append("\ttotal_length += 8;")

            lines.append(f"inline\n{ret} {name}({', '.join(params_parts)}) {{")
            lines.extend(total_length)
            lines.append("\tstd::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(total_length);")
            lines.append("\tbuffer->setWriteOffset(4);")
            for p in f.get("params", []):
                lines.append(f"\tencode(buffer,{p[0]});")
            lines.append("\t_iio->send(buffer);")
            lines.append("}")

        enum_lines.append("} Events;")
        self._storage_enum_code = "\n".join(enum_lines) + "\n"
        self._code = "\n".join(lines) + "\n"

    def get_declarations_code(self):
        return self._code

    def get_storage_enum_code(self):
        return self._storage_enum_code
