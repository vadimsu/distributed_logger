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
        self._storage_enum_code = "package storage\nconst (\n"
        self._decoder_code = "package decoder\n"
        self._decoder_code = self._decoder_code + "import (\n\t\"distributedlogger.com/storage\"\n\"errors\"\n)\n"
        self._storage_definitions_code = ""
#        self._storage_definitions_code = "package mongo\n"
#        self._storage_definitions_code = self._storage_definitions_code + "import (\n\"context\"\n"
##        self._storage_definitions_code = self._storage_definitions_code + "\"go.mongodb.org/mongo-driver/v2/bson\"\n"
#        self._storage_definitions_code = self._storage_definitions_code + "\"go.mongodb.org/mongo-driver/v2/mongo\"\n"
##        self._storage_definitions_code = self._storage_definitions_code + "\"go.mongodb.org/mongo-driver/v2/mongo/options\"\n"

#        self._storage_definitions_code = self._storage_definitions_code + "\"distributedlogger.com/storage\"\n" 
#        self._storage_definitions_code = self._storage_definitions_code + ")\n"


 #       self._storage_definitions_code = self._storage_definitions_code + "type MongoStorage struct{\ncollection *mongo.Collection\n}\n"
#       self._storage_structures_code = "package storage\n"
        self._storage_structures_code = ""
        self._storage_declarations_code = "type StorageAPI interface{\n"
        func_idx = 0
        for f in self._func_dict:
            if len(f['params']) < 2:
                continue
            event_name = GoCodeGen.get_event_name(f['params'][0])
            decoder_func = "func decode_" + event_name + "(packet []byte, s storage.StorageAPI)(error) {\n"
            decoder_func = decoder_func + "\tvar decoded int = 0\n"
            decoder_func = decoder_func + "\tvar decodedThisTime int = 0\n"
            funct = '\t' + f['name'] + "_" + event_name + "("
            storage_struct = "type " + event_name[0].upper() + event_name[1:] + "_struct struct {\n"
            self._storage_definitions_code = self._storage_definitions_code + "func (s *MongoStorage) " + funct
            self._storage_enum_code = self._storage_enum_code + '\t' + event_name[0].upper() + event_name[1:]
            if func_idx == 0:
                self._storage_enum_code = self._storage_enum_code + " = iota"
            self._storage_enum_code = self._storage_enum_code + '\n'
            func_idx = func_idx + 1
            if func_idx == len(self._func_dict):
                self._storage_enum_code = self._storage_enum_code + '\n' + ")\n"
            params = f['params']
            param_idx = 0
            funct_body = "\tfields := []interface{}{\n" + '\t\t' + "storage." + event_name[0].upper() + event_name[1:] +"_struct{\n"
            params_array = []
            for p in params:
                param_type = p[1]
                if param_type == "uint64_t":
                        param_type = "uint64"
                funct = funct + p[0] + " " + param_type
                self._storage_definitions_code = self._storage_definitions_code + p[0] + " " + param_type
                storage_struct = storage_struct + '\t' + p[0][0].upper() + p[0][1:] + " " + param_type + '\n'
                funct_body = funct_body + '\t\t\t' + p[0][0].upper() + p[0][1:] + ":" + p[0]
                param_idx = param_idx + 1
                if param_idx < len(params):
                    funct = funct + ", "
                    self._storage_definitions_code = self._storage_definitions_code + ","
                funct_body = funct_body + ",\n"
                decoder_func = decoder_func + "\tvar " + p[0] + " " + param_type + '\n'
                if param_idx > 1:
                    if p[1] == "string":
                        decoder_func = decoder_func + '\t' + p[0] + ", decodedThisTime,_ = DecodeString(packet[decoded:],false)" + '\n'
                    else:
                        decoder_func = decoder_func + '\t' + p[0] + ", decodedThisTime,_ = DecodeUint64(packet[decoded:])" + '\n'
                    decoder_func = decoder_func + "\tdecoded = decoded + decodedThisTime\n"
                else:
                        decoder_func = decoder_func + "\t" + p[0] + " = storage." + p[0][0].upper() + p[0][1:] + "\n"

            decoder_func = decoder_func + '\t' + "return s." + f['name'] + "_" + event_name + "("
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
        dispatch_funct  = "func Event_dispatch(event uint64, packet []byte, s storage.StorageAPI)(error){\n"
        dispatch_funct = dispatch_funct + "\tswitch event{\n"
        for f in self._func_dict:
            if len(f['params']) < 2:
                continue
            event_name = GoCodeGen.get_event_name(f['params'][0])
            dispatch_funct = dispatch_funct + "\tcase storage." + event_name[0].upper() + event_name[1:] + ":\n"
            dispatch_funct = dispatch_funct + "\t\treturn decode_" + GoCodeGen.get_event_name(f['params'][0]) + "(packet, s)\n"
        dispatch_funct = dispatch_funct + "\t}\n"
        dispatch_funct = dispatch_funct + "\treturn errors.New(\"unknown event\")\n}"
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
    def generate_code(self):
        self._code = ""
        self._storage_enum_code = "typedef enum {\n"
        func_idx = 0
        for f in self._func_dict:
            funct = 'inline\n' + f['return'] + ' ' + f['name'] + "("
            total_length_str = "\tint total_length = 4;" + '\n'
            params = f['params']
            self._storage_enum_code = self._storage_enum_code + '\t' + GoCodeGen.get_event_name(f['params'][0])
            func_idx = func_idx + 1
            if func_idx < len(self._func_dict):
                self._storage_enum_code = self._storage_enum_code + "," + '\n'
            else:
                self._storage_enum_code = self._storage_enum_code + '\n' + "} Events;\n"
            param_idx = 0
            for p in params:
                if p[1] == "string":
                    funct = funct + "std::"
                funct = funct + p[1] + " " + p[0]
                param_idx = param_idx + 1
                if p[1] == "string":
                    total_length_str = total_length_str + "\ttotal_length += " + p[0]+".size() + 4;\n"
                elif p[1] == "uint64_t":
                    total_length_str = total_length_str + "\ttotal_length += 8;\n"
                if param_idx < len(params):
                    funct = funct + ", "
            funct = funct + ") {\n"
            funct = funct + total_length_str + "\n"
            funct = funct + "\tstd::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(total_length);\n"
            funct = funct + "\tbuffer->setWriteOffset(4);\n"
            param_idx = 0
            for p in params:
                funct = funct + "\tencode(buffer," + p[0] + ");"
                param_idx = param_idx + 1
                if param_idx < len(params):
                    funct = funct + '\n'
            funct = funct + '\n'
            funct = funct + "\t_iio->Send(buffer);" + '\n'
            funct = funct + "}\n"
            self._code = self._code + funct
    def get_declarations_code(self):
        return self._code
    def get_storage_enum_code(self):
        return self._storage_enum_code
