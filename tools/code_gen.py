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
        self._clickhouse_definitions_code = ""

    @staticmethod
    def get_event_name(param):
        """Return the event name from a parameter pair (name, type)."""
        return param[0]

    def generate_code(self):
        # Consider only functions that have at least one param
        valid_funcs = [f for f in self._func_dict if len(f.get('params', [])) >= 1]

        # Storage enum generation
        self._storage_enum_code = "package storage\n\nconst (\n"
        for idx, f in enumerate(valid_funcs):
            event_name = GoCodeGen.get_event_name(f['params'][0])
            entry = '\t' + event_name[0].upper() + event_name[1:]
            if idx == 0:
                entry += " = iota"
            self._storage_enum_code = self._storage_enum_code + entry + '\n'
        entry = '\tEventMax'
        self._storage_enum_code = self._storage_enum_code + entry + '\n'
        self._storage_enum_code = self._storage_enum_code + ")\n"

        # Storage structures generation
        self._storage_structures_code = ""
        for f in valid_funcs:
            event_name = GoCodeGen.get_event_name(f['params'][0])
            struct_name = event_name[0].upper() + event_name[1:] + "_struct"
            self._storage_structures_code += f"type {struct_name} struct {{\n"
            
            for p in f['params']:
                param_name = p[0]
                param_type = p[1]
                if param_type == "uint64_t":
                    param_type = "uint64"
                
                field_name = param_name[0].upper() + param_name[1:]
                self._storage_structures_code += f"\t{field_name} {param_type}\n"
            
            self._storage_structures_code += "}\n"

        # Storage declarations (interface)
        self._storage_declarations_code = "type StorageAPI interface{\n\tFlush([][]byte)(error)\n}\n"

        # Decoder generation
        self._decoder_code = "package event_decoder\n\n"
        self._decoder_code += "import (\n"
        self._decoder_code += '\t"distributedlogger.com/storage"\n'
        self._decoder_code += ")\n\n"
        self._decoder_code += "import (\n"
        self._decoder_code += '\t"distributedlogger.com/decoder"\n'
        self._decoder_code += ")\n\n"

        for fidx, f in enumerate(valid_funcs):
            event_name = GoCodeGen.get_event_name(f['params'][0])
            struct_name = event_name[0].upper() + event_name[1:] + "_struct"

            # Decoder function signature (returns struct and error)
            # Match reference formatting: first function has space before '{', subsequent functions do not
            if fidx == 0:
                decoder_func = f"func Decode_{event_name}(packet []byte)(storage.{struct_name}, error) {{\n"
            else:
                decoder_func = f"func Decode_{event_name}(packet []byte)(storage.{struct_name}, error){{\n"
            decoder_func += "\tvar decoded int = 0\n"
            decoder_func += "\tvar decodedThisTime int = 0\n"

            # Decode each parameter
            for idx, p in enumerate(f['params']):
                param_name = p[0]
                param_type = p[1]

                if param_type == "uint64_t":
                    param_type = "uint64"

                field_name = param_name[0].upper() + param_name[1:]

                decoder_func += f"\tvar {param_name} {param_type}\n"

                if idx == 0:
                    # First parameter is the event ID
                    decoder_func += f"\t{param_name} = storage.{field_name}\n"
                else:
                    # Subsequent parameters are decoded from packet and errors checked
                    if p[1] == "string":
                        if idx == 1:
                            decoder_func += f"\t{param_name}, decodedThisTime, err := decoder.DecodeString(packet[decoded:])\n"
                        else:
                            decoder_func += f"\t{param_name}, decodedThisTime, err = decoder.DecodeString(packet[decoded:])\n"
                    else:
                        if idx == 1:
                            decoder_func += f"\t{param_name}, decodedThisTime, err := decoder.DecodeUint64(packet[decoded:])\n"
                        else:
                            decoder_func += f"\t{param_name}, decodedThisTime, err = decoder.DecodeUint64(packet[decoded:])\n"
                    decoder_func += "\tif err != nil {\n"
                    decoder_func += f"\t\treturn storage.{struct_name}{{}}, err\n"
                    decoder_func += "\t}\n"
                    decoder_func += "\tdecoded = decoded + decodedThisTime\n"

            # Return statement with nil error
            decoder_func += f"\treturn storage.{struct_name}{{\n"
            for p in f['params']:
                param_name = p[0]
                field_name = param_name[0].upper() + param_name[1:]
                decoder_func += f"\t\t\t{field_name}:{param_name},\n"
            decoder_func += "\t\t}, nil\n"
            decoder_func += "}\n"

            self._decoder_code += decoder_func

        # ClickHouse Flush method generation
        self._clickhouse_definitions_code = "\n"
        
        # Generate Flush method body with switch cases
        flush_method = "func (s *ClickHouseStorage) Flush(batch [][]byte)(error){\n"
        flush_method += "\tvar err error = nil\n"
        flush_method += "\tb, err := s.conn.PrepareBatch(context.Background(),\n"
        flush_method += '\t\tfmt.Sprintf("INSERT INTO %s (event, payload)", s.table))\n'
        flush_method += "\tif err != nil {\n"
        flush_method += "\t\tfmt.Println(err)\n"
        flush_method += "\t\treturn err\n"
        flush_method += "\t}\n"
        flush_method += "\tfor i := range batch {\n"
        flush_method += "\t\tevent, decodedThisTime, err := decoder.DecodeUint64(batch[i])\n"
        flush_method += "\t\tif err != nil {\n"
        flush_method += "\t\t\tfmt.Println(err)\n"
        flush_method += "\t\t\tcontinue\n"
        flush_method += "\t\t}\n"
        flush_method += "\t\tswitch event {\n"
        
        for f in valid_funcs:
            event_name = GoCodeGen.get_event_name(f['params'][0])
            struct_name = event_name[0].upper() + event_name[1:] + "_struct"
            decoder_func_name = f"Decode_{event_name}"
            
            flush_method += f"\t\t\tcase storage.{struct_name[:-7]}:\n"  # Remove _struct suffix
            flush_method += f"\t\t\t\t{event_name}, err := event_decoder.{decoder_func_name}(batch[i][decodedThisTime:])\n"
            flush_method += "\t\t\t\tif err != nil {\n"
            flush_method += "\t\t\t\t\treturn err\n"
            flush_method += "\t\t\t\t}\n"
            flush_method += "\t\t\t\tpayload := map[string]interface{}{\n"
            
            # Add payload fields (skip first parameter which is event ID)
            for p in f['params'][1:]:
                param_name = p[0]
                field_name = param_name[0].upper() + param_name[1:]
                flush_method += f"\t\t\t\t\t\"{field_name}\": {event_name}.{field_name},\n"
            
            flush_method += "\t\t\t\t}\n"
            flush_method += "\t\t\t\tjs, _ := json.Marshal(payload)\n"
            flush_method += f"\t\t\t\terr = b.Append(uint64(storage.{struct_name[:-7]}), string(js))\n"
            flush_method += "\t\t\t\tif err != nil {\n"
            flush_method += "\t\t\t\t\tfmt.Println(err)\n"
            flush_method += "\t\t\t\t}\n"
        
        flush_method += "\t\t}\n"
        flush_method += "\t}\n"
        flush_method += "\terr = b.Send()\n"
        flush_method += "\tif err != nil {\n"
        flush_method += "\t\tfmt.Println(err)\n"
        flush_method += "\t}\n"
        flush_method += "\treturn err\n"
        flush_method += "}\n\n"
        
        self._clickhouse_definitions_code += flush_method

        # Generate GetMigrations method
        migrations_lines: List[str] = ["func (s *ClickHouseStorage) GetMigrations() []string {"]
        migrations_lines.append("\treturn []string{")
        
        for f in valid_funcs:
            event_name = GoCodeGen.get_event_name(f['params'][0])
            struct_name = event_name[0].upper() + event_name[1:] + "_struct"
            
            # Collect columns (skip first parameter which is event ID)
            cols: List[str] = []
            json_extracts: List[str] = []
            
            for p in f['params'][1:]:
                param_name = p[0]
                param_type = p[1]
                field_name = param_name[0].upper() + param_name[1:]
                
                if param_type == "uint64_t":
                    ch_type = "UInt64"
                    json_extracts.append(f"JSONExtractUInt(payload, '{field_name}') AS {field_name}")
                elif param_type == "string":
                    ch_type = "String"
                    json_extracts.append(f"JSONExtractString(payload, '{field_name}') AS {field_name}")
                elif param_type == "bool":
                    ch_type = "Bool"
                    json_extracts.append(f"JSONExtractBool(payload, '{field_name}') AS {field_name}")
                else:
                    ch_type = "String"
                    json_extracts.append(f"JSONExtractString(payload, '{field_name}') AS {field_name}")
                
                cols.append(f"{field_name} {ch_type}")
            
            # DDL for typed table
            cols_str = ", ".join(cols)
            ddl = f'fmt.Sprintf("CREATE TABLE IF NOT EXISTS %s_{event_name} ({cols_str}) ENGINE = MergeTree() ORDER BY tuple()", s.table),'
            migrations_lines.append(f"\t\t{ddl}")
            
            # Materialized view
            json_extracts_str = ", ".join(json_extracts)
            mv = f'fmt.Sprintf("CREATE MATERIALIZED VIEW IF NOT EXISTS mv_%s_{event_name} TO %s_{event_name} AS SELECT {json_extracts_str} FROM %s WHERE event = %d", s.table, s.table, s.table, storage.{struct_name[:-7]})'
            migrations_lines.append(f"\t\t{mv},")
        
        migrations_lines.append("\t}")
        migrations_lines.append("}")
        
        self._clickhouse_definitions_code += "\n".join(migrations_lines) + "\n"

    def get_storage_declarations_code(self):
        return self._storage_declarations_code

    def get_storage_definitions_code(self):
        # Generate Flush method for Mongo storage
        valid_funcs = [f for f in self._func_dict if len(f.get('params', [])) >= 1]
        
        mongo_flush = "func (s *MongoStorage) Flush(batch [][]byte)(error){\n"
        mongo_flush += "\tvar fields []interface{}\n"
        mongo_flush += "\tfor _, packet := range batch {\n"
        mongo_flush += "\t\tevent, decodedThisTime, err := decoder.DecodeUint64(packet)\n"
        mongo_flush += "\t\tif err != nil {\n"
        mongo_flush += "\t\t\treturn err\n"
        mongo_flush += "\t\t}\n"
        mongo_flush += "\t\tswitch event {\n"
        
        for fidx, f in enumerate(valid_funcs):
            event_name = GoCodeGen.get_event_name(f['params'][0])
            struct_name = event_name[0].upper() + event_name[1:] + "_struct"
            decoder_func_name = f"Decode_{event_name}"
            
            mongo_flush += f"\t\tcase storage.{struct_name[:-7]}:\n"  # Remove _struct suffix
            # Reference has inconsistent spacing: first event uses 'decoded,err', second uses 'decoded, err'
            if fidx == 0:
                mongo_flush += f"\t\t\tdecoded,err := event_decoder.{decoder_func_name}(packet[decodedThisTime:])\n"
            else:
                mongo_flush += f"\t\t\tdecoded, err := event_decoder.{decoder_func_name}(packet[decodedThisTime:])\n"
            mongo_flush += "\t\t\tif err != nil {\n"
            mongo_flush += "\t\t\t\treturn err\n"
            mongo_flush += "\t\t\t}\n"
            mongo_flush += f"\t\t\tfields = append(fields, decoded)\n"
        
        mongo_flush += "\t\t}\n"
        mongo_flush += "\t}\n"
        mongo_flush += "\tif len(fields) > 0 {\n"
        mongo_flush += "\t\t_, err := s.collection.InsertMany(context.TODO(), fields)\n"
        mongo_flush += "\t\treturn err\n"
        mongo_flush += "\t}\n"
        mongo_flush += "\treturn nil\n"
        mongo_flush += "}\n"
        
        return mongo_flush

    def get_storage_structures_code(self):
        return self._storage_structures_code

    def get_storage_enum_code(self):
        return self._storage_enum_code

    def get_decoder_code(self):
        return self._decoder_code

    def get_clickhouse_definitions_code(self) -> str:
        return self._clickhouse_definitions_code


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
            name = f['name']
            params_parts: List[str] = []
            total_length = ["\tint total_length = 4;"]
            name_suffix = ""
            for p in f.get("params", []):
                p_name, p_type = p[0], p[1]
                if name_suffix == "":
                    name_suffix = p_name
                    total_length.append("\ttotal_length += 8;")
                    continue
                prefix = "std::" if p_type == "string" else ""
                params_parts.append(f"{prefix}{p_type} {p_name}")
                if p_type == "string":
                    total_length.append(f"\ttotal_length += {p_name}.size() + 2;")
                elif p_type == "uint64_t":
                    total_length.append("\ttotal_length += 8;")

            lines.append(f"inline")
            lines.append(f"{ret} {name}_{name_suffix}({', '.join(params_parts)}) {{")
            lines.extend(total_length)
            lines.append("\tstd::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(total_length);")
            lines.append("\tbuffer->setWriteOffset(4);")
            for idx, p in enumerate(f.get("params", [])):
                if idx == 0:
                    lines.append(f"\tencode(buffer,Events::{p[0]});")
                else:
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
