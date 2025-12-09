#!/usr/bin/python3
#import CodeGen

class CodeGen:
    def __init__(self, func_dict):
         self._func_dict = func_dict
    def __call__(self):
        self.generate_code()
        return self


class GoCodeGen(CodeGen):
    def __init__(self, func_dict):
        super().__init__(func_dict)
        self._declarations_code = ""
    @classmethod
    def get_event_name(cls, params):
        return params[0]
    def generate_code(self):
        self._storage_enum_code = "const (\n"
        self._event_map = {}
        self._parser_definition_code = ""
        self._storage_definitions_code = ""
        self._storage_structures_code = ""
        self._storage_declarations_code = "type StorageAPI interface{\n"
        func_idx = 0
        for f in self._func_dict:
            if len(f['params']) < 2:
                continue
            funct = '\t' + f['name'] + "_" + GoCodeGen.get_event_name(f['params'][1]) + "("
            storage_struct = "type " + f['name'] + "_" + GoCodeGen.get_event_name(f['params'][1]) + " struct {\n"
            self._storage_definitions_code = "func (s *MongoStorage) " + funct
            self._storage_enum_code = self._storage_enum_code + '\t' + GoCodeGen.get_event_name(f['params'][1])
            if func_idx == 0:
                self._storage_enum_code + " = iota"
            func_idx = func_idx + 1
            if func_idx == len(self._func_dict):
                self._storage_enum_code = self._storage_enum_code + '\n' + ")"
            params = f['params']
            param_idx = 0
            funct_body = "\tfields := []interface{}{\n" + '\t\t' + GoCodeGen.get_event_name(f['params'][1]) +"_struct{\n"
            for p in params:
                funct = funct + p[0] + " " + p[1]
                self._storage_definitions_code = self._storage_definitions_code + p[0] + " " + p[1]
                storage_struct = storage_struct + '\t' + p[0] + " " + p[1] + '\n'
                funct_body = funct_body + '\t\t\t' + p[0] + ":" + p[0]
                param_idx = param_idx + 1
                if param_idx < len(params):
                    funct = funct + ", "
                    funct_body = funct_body + ","
                    self._storage_definitions_code = self._storage_definitions_code + ","
                funct_body = funct_body +'\n'
            funct = funct + ") (error)"
            if func_idx < len(self._func_dict):
                funct = funct + '\n'
            funct_body = funct_body + "\t\t},\n\t}\n"
            funct_body = funct_body + '\t' + "_, err := s.collection.InsertMany(context.TODO(), fields)\n"
            funct_body = funct_body + "\treturn err"
            self._storage_definitions_code = self._storage_definitions_code + ") (error){\n"
            self._storage_definitions_code = self._storage_definitions_code + funct_body
            self._storage_definitions_code = self._storage_definitions_code + "\n}\n"
            storage_struct = storage_struct + "}\n"
            self._storage_structures_code = self._storage_structures_code + storage_struct
            self._storage_declarations_code = self._storage_declarations_code + funct
            if func_idx == len(self._func_dict):
                self._storage_declarations_code = self._storage_declarations_code + '\n'
        self._storage_declarations_code = self._storage_declarations_code + "}\n"
    def get_storage_declarations_code(self):
        return self._storage_declarations_code
    def get_storage_definitions_code(self):
        return self._storage_definitions_code
    def get_storage_structures_code(self):
        return self._storage_structures_code
    def get_storage_enum_code(self):
        return self._storage_enum_code

class CppCodeGen(CodeGen):
    def __init__(self, func_dict):
        super().__init__(func_dict)
        self._code = ""
    def generate_code(self):
        self._code = ""
        self._storage_enum_code = "typedef enum {\n"
        func_idx = 0
        for f in self._func_dict:
            funct = f['return'] + ' ' + f['name'] + "("
            total_length_str = "\tint total_length = 0;" + '\n'
            params = f['params']
            self._storage_enum_code = self._storage_enum_code + '\t' + GoCodeGen.get_event_name(f['params'][1])
            func_idx = func_idx + 1
            if func_idx < len(self._func_dict):
                self._storage_enum_code = self._storage_enum_code + "," + '\n'
            else:
                self._storage_enum_code = self._storage_enum_code + '\n' + "} Events;"
            param_idx = 0
            for p in params:
                if p[1] == "string":
                    funct = funct + "std::"
                funct = funct + p[1] + " " + p[0]
                param_idx = param_idx + 1
                if p[1] == "string":
                    total_length_str = total_length_str + "\ttotal_length + " + p[0]+".size() + 4;\n"
                elif p[1] == "uint64_t":
                    total_length_str = total_length_str + "\ttotal_length + 12;\n"
                if param_idx < len(params):
                    funct = funct + ", "
            funct = funct + ") {\n"
            funct = funct + total_length_str + "\n"
            funct = funct + "\tstd::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(total_length);\n"
            param_idx = 0
            for p in params:
                funct = funct + "\tencode(buffer," + p[0] + ");"
                param_idx = param_idx + 1
                if param_idx < len(params):
                    funct = funct + '\n'
            funct = funct + '\n'
            funct = funct + "\t_iio->Send(buffer);" + '\n'
            funct = funct + "}"
            self._code = self._code + funct
    def get_declarations_code(self):
        return self._code
    def get_storage_enum_code(self):
        return self._storage_enum_code
